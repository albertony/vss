# ShadowRun

ShadowRun is a utility based on Microsoft's feature packed open source utility Vshadow, but focused on a
single very specific feature: The creation of temporary read-only shadow copy and running of a specified
utility that can work on this, before everything is automatically cleaned up.

See root of repository [vss](..) for background information about Volume Shadow Copy Service (also known
as Volume Snapshot Service, VSS for short).

***
## Motivation

I use [Rclone](https://github.com/rclone/rclone) ("rsync for cloud storage") to encrypt and upload
files to my cloud storage provider. Rclone is a cross-platform command line program to sync files and
directories to and from different cloud storage providers. Very flexible, robust and easy to use,
highly recomended!

When uploading a very large file produced by a different application running in the background, it may
happen that before the upload completes the original file is modified, which could break the upload,
or the program wanting to write to the original file fails because the file is locked.

A similar case can occur when uploading a set of smaller files, for example if you have a directory
with different files related to each other larger. If the directory are written to while the upload
is executing, perhaps the upload of each individual file completes successfully, but the "version"
of the files may be different as some where updated during the upload while others were not.
If you have control over when the files are going to be modified, and when the upload is going to
read them, you can avoid this. But if the upload takes a long time, and you have to suspend
other activities to avoid modifying the files, then this may become a problem.

If we could make use of VSS in this scenario, we could create a temporary read-only snapshot
of the hard drive (a logical drive, often referred to as a volume), known as a shadow copy
(I will throughout this readme probably use different terms for the same thing; snapshot, image,
shadow copy, etc), and then run the upload from that snapshot instead of the "live" system.
Because then:
- The files (in the snapshot) will never be modified during the upload.
- The files (in the snapshot) are never locked by other programs.

Following [below](#using-vshadow) is a description of how you can use the original vshadow utility
to achive this. Further down this site I [introduce](#introducing-shadowrun) my own ShadowRun
application where I have added improvements that makes the solution easier, more flexible, and
and more robust.


***
## Using VShadow

The vshadow utility from Microsoft can be used to integrate external utilities like rclone
with VSS. It is not an entirely straight forward proces, nor a very elegant solution, but
it will most likely get you to a solution that you can live with. The following section
describes how you can do this, and some of the issues and downsides that lead me to create
my own ShadowRun run utility with additional features.

See main [readme](..) at the root of this repository, and the [readme](../vshadow) from the
original source code for an description of vshadow.

Vshadow can be used with command-line argument `-nw` ("no writers") to create a temporary read-only
snapshot that will be destroyed immediately when the program completes. Sounds kind of useless?
Well, it also has the command-line argument `-exec` where you can specify an executable,
which will often be a batch script, that it should run before returning. This executable will then
be able to access the temporary snapshot. The snapshot are identified with a GUID and lives inside
Windows as a kind of hidden hard disk volume device. So how can the executable find it? Well there
is a third command-line argument third argument `-script` where you can name a batch script that
vshadow will write a set of environment variables identifying the snapshot. The generated
script is a valid batch script containing `SET` statements, so if you in the `-exec` argument
specifies a batch script you it can include a `CALL` statement referencing the generated file
named according to `-script`. Now evaluating environment variable `%SHADOW_DEVICE_1%` in your
`-exec` script will return the device name of the temporary snapshot! So then we are close, but
the last challenge is how to actually access the shadow device. Regular file commands and programs
cannot access it by the device name directly! Some do actually: COPY can be used to copy
a file directly using `copy %VSHADOW_DEVICE_1%\somefile.txt c:\somefile_bak.txt`, but DIR
does not work etc so you will probably not get to do what you want. Luckily the built-in `MKLINK`
utility are able to create a directory symbolic link with the snapshot device as target, and then
you can "mount" it to a regular directory path on your C: drive.

One trick to test this out interactively is to specify a "blocking" command in the `-exec`
argument. For example, if you specify `-exec=C:\Windows\System32\notepad.exe` then after
vshadow has created the snapshot it start the regular Notepad application and then wait
for it to exit before cleaning up the snapshot! So now you can just leave the notepad
window open, start another Command Prompt and access the snapshot! When you are done you
just close the Notepad Window.

### How to

Let's say that we want to sync a directory `c:\data` to the cloud. Using rclone you would normally
execute `rclone sync c:\data remote:data`. To make this command read source files from a VSS snapshot,
you can create a batch script with the following content:

```
rem Load the variables from a temporary script generated by vshadow.exe
call "%~dp0setvars.cmd" || exit /b 1

rem Create the symbolic link to the snapshot (the backslash after shadow_device_1 is important!)
mklink /d c:\snapshot\ %shadow_device_1%\ || exit /b 1

rem Execute rclone with the source c:\snapshot\data containing a snapshot of c:\data
rclone sync c:\snapshot\data\ remote:data

rem Delete the symbolic link
rmdir c:\snapshot\

rem Delete the temporary file created by vshadow.exe
del "%~dp0setvars.cmd"
```

If you save this script as `exec.cmd` you could now execute the following command from the same
directory:

```
vshadow.exe -nw -script=setvars.cmd -exec=exec.cmd c:
```

Or, if you want a single-click solution; create a second batch script in the same directory,
which executes this command - but with full paths of both file references to avoid issues with
changing working directory:

```
vshadow.exe -nw -script="%~dp0setvars.cmd" -exec="%~dp0exec.cmd" c:
```

When the vshadow.exe command is executed, possibly from your single-click wrapper script, it will:
1. Create a read-only snapshot of your c: drive.
2. Generate a file named `setvars.cmd` containing variables identifying the snapshot created.
3. Execute `exec.cmd`, which will:
    1. Load variables from `setvars.cmd`.
    2. Mount (symbolic link) the snapshot identified by the loaded variables.
    3. Execute the rclone command to sync from the snapshot.
    4. Unmount the snapshot (delete the symbolic link).
    5. Delete the generated `setvars.cmd` file.
4. Delete the snapshot.

#### Additional comments

Clarifications:
- Vshadow must be run with administrator privileges, so with UAC enabled you must start `vsshadow.exe` or the wrapper script using the "Run as administrator" option.
- The path `c:\snapshot\` is a temporary symbolic link only accessible while exec.cmd is running, you will see
it in Windows Explorer until the sync is completed. But the directory is read-only, because the
`vshadow.exe` command included the command line argument `-nw` ("no writers").
- The contents of `c:\snapshot\` that rclone sees, is a mirror image of `c:\`, meaning there will be a
directory `c:\snapshot\Program Files` mirroring `c:\Program Files` etc. If this is confusing, read
about improvements below.
- Upon successful return vshadow will print message "Snapshot creation done.". This just means everything
went well: It created the snapshot AND executed our script AND the snapshot was automatically deleted.
Upon failure it will print message "Aborting the backup..." but this does not mean anything, because there
is no backup to abort (in read-only/no writers mode) so it is not skipping anything that it would else do
on a successful run
(see [source1](https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/VShadowVolumeShadowCopy/cpp/util.h#L692-L697),
[source2](https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/VShadowVolumeShadowCopy/cpp/shadow.cpp#L881-L910)
and [source3](https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/VShadowVolumeShadowCopy/cpp/create.cpp#L154-L157) for proof).

About error handling and robustness:
- Wherever the batch scripts are referenced, we prefix their names with `%~dp0` to make absolute paths
assuming they are in the same directory as the script they are referenced from, and surrounding with double
quotes in case there are spaces in the path. This will prevent any surprises when running from different
working directories, e.g. "Run as administrator" is notorious for always setting working directory to `C:\Windows\System32`.
- The mklink command returns error when the link path (`c:\snapshot\`) already exists, and this is important to
handle. If we let the script just continue, rclone will try to sync a subfolder named data (`c:\snapshot\data`).
If this does not exist then rclone will just abort with error, so that is ok. Worse if this path does exists,
then rclone will actually sync it. What is normally expected in `c:\snapshot\data` is a snapshot of `c:\data`.
In worst case this has previously been successfully synced to `dest:data`, and after the last sync this new
folder `c:\snapshot\data` has been created with content that is not at all similar to what is in `c:\data`.
The result is that the current sync will end up deleting everything from `dest:data` and upload whatever is in
`c:\snapshot\data`. A lot of "ifs" here, perhaps a bit paranoid to expect it, but unless the link path is
very carefully chosen it could happen.
- The mklink command does not verify the link target, so even if the variable %shadow_device_1% contains an invalid
value or for some reason is empty, the mklink command will succeed. The following rclone sync command using the
link as source will then simply fail (`Failed to sync: directory not found`), so this so ok.

### Improvements

It may be confusing that `c:\snapshot\` is a mirror image of `c:\`, and when referring `c:\snapshot\data` it is
a mirror of `c:\data`. When writing more complex scripts this confusion may easily lead to bugs. You may find
it less confusing if you create an alias `t:` that you can use when referrring to the snapshot. This can easily
be done using the built-in `SUBST`command: Add `subst t: c:\` (error handling discussed later) before the
`rclone sync` command and `subst t: /d` after, and change the sync command to use `t:` instead
of `c:`: `rclone sync t:\snapshot\data\ dest:data`.

Instead of just making `t:` an alias to the entire `c:` drive, you can make it an alias directly into the
snapshot image at `c:\snapshot\`. Then the paths on `c:` you would normally use with rclone directly without vss,
now becomes identical when used on the snapshot `t:`. This may be even less confusing: `t:` is now the image of
`c:`.  This has the additional benefit of not increasing the path lengths of the source paths that rclone gets
to work with, in case that could hit some limit.

Proper error handling is still recommended, early abort when any of the commands fail. In addition to the original
example we now have an additional `subst` command that we must consider. It is the same situation with `subst` as
with the `mklink` command, as discussed above: If we let the script just continue if the drive already is in use,
rclone will sync whatever is on it with potential destructive effect on existing data at the destination. We now
perform the same mklink command as before and then subst, but if subst fail we should not just abort but first
delete the link created by mklink. This makes the script code a bit more complex.

Another improvements is to return a proper exit code (`ERRORLEVEL`). The `vsshadow.exe` utility will check the
exit code from the `-exec` script it executes, and if the script returns nonzero then `vsshadow.exe` will also
return a non-zero exit code (but not the one returned by the script, see below), whch you then can check from
the top level script (if it is a bit more complex than our `vs.cmd`).

The resulting version of `exec.cmd` can be something like this:

```
setlocal enabledelayedexpansion
call "%~dp0setvars.cmd" || set exit_code=!errorlevel!&&goto end
mklink /d c:\snapshot\ %shadow_device_1%\ || set exit_code=!errorlevel!&&goto end
subst t:\ c:\snapshot\ || set exit_code=!errorlevel!&&goto remove_link
rclone sync t:\data\ dest:data
set exit_code=%errorlevel%
subst t: /d
:remove_link
rmdir c:\snapshot\
:end
del "%~dp0setvars.cmd"
exit /b %exit_code%
```

If everything goes fine until the rclone command, then the exit code from rclone is what will be returned.
If the removal of virtual drive (`subst t: /d`) and directory symbolic link (`rmdir c:\snapshot\`) fails it is
just ignored. This will lead to the drive/directory being left accessible after the script has completed, and
if you run the script again it will abort with error because the path/drive is already in use. You could add
a check of the result from these two commands too. For example just write an additional warning
(append `||ECHO WARNING: Manual cleanup required`), or also set the exit code if any of these fails
(append ` || set exit_code=!errorlevel!&&ECHO WARNING: Manual cleanup required`), depending if you see this
as something that should be reported as an error or since the rclone command passed you consider it more of
a success.

### Alternative variants

An alternative to using the SUBST command to create an alias, is to create a network share that you access via localhost. This can be done by replacing the `rclone sync c:\snapshot\data\ dest:data` line with something like this (could be extended with similar error handling as above):

```
net share snapshot=c:\snapshot
rclone sync \\localhost\snapshot\data\ dest:data
net share snapshot /delete
```

Another alternative is to crate a dedicated volume/partition/drive. This is kind of an "intrusive" approach, but it is probably the most reliable and least code needed. With a dedicated drive just for mounting, there is for example little risk the mount path is in use by somethin else. You can create a dedicated partition on your hard drive to use only for mounting. It can be of a small symbolic size of just 1MB. If there is no room for a new partition on your disk (e.g. c: drive), then shrink it by just the 1MB. Create a new partition, and assign it a permanent drive letter, `b:` for example. Now change the content of `exec.cmd` to:

```
call "%~dp0setvars.cmd" || exit /b 1
mklink /d b:\snapshot\ %shadow_device_1%\ || exit /b 1
rclone sync b:\snapshot\data\ dest:data
rmdir b:\snapshot\
```

### Exit codes

Exit codes from vshadow.exe:
* 0 - Success
* 1 - Object not found
* 2 - Runtime Error 
* 3 - Memory allocation error

(https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/VShadowVolumeShadowCopy/cpp/shadow.cpp#L28-L34)

Exit codes from rclone.exe:
* 0 - success
* 1 - Syntax or usage error
* 2 - Error not otherwise categorised
* 3 - Directory not found
* 4 - File not found
* 5 - Temporary error (one that more retries might fix) (Retry errors)
* 6 - Less serious errors (like 461 errors from dropbox) (NoRetry errors)
* 7 - Fatal error (one that more retries won’t fix, like account suspended) (Fatal errors)
* 8 - Transfer exceeded - limit set by --max-transfer reached

(https://rclone.org/docs/#list-of-exit-codes)


***
## Introducing ShadowRun

To repeat myself from the introduction: ShadowRun is a utility based on Microsoft's feature packed
open source utility Vshadow, but focused on a single very specific feature: The creation of
temporary read-only shadow copy and running of a specified utility that can work on this,
before everything is automatically cleaned up.

### Added features


#### Setting process environment variables

Support for setting environment variables directly into the process instead of having
to go via generated batch file containing `SET` statements, given by the `-script` option.

#### Automatically mounting

Support for automatically mounting the created shadow copies so that they are accessible
from a drive letter. The drive letters used are set in new environment variables SHADOW_DRIVE_n.
You can also specify which drive letters you want to use, default is to use whichever is available.

#### Additional environment variables

In addition to SHADOW_DRIVE_n, there is also a new variable SHADOW_SET_COUNT that can be used 
to avoid reading SHADOW_..._n variables that for some reason exists with numbers not valid for the current
run.

#### Arguments to the executed command

Command line arguments to the exec command can be specified. The main executable must be specified
with the `-exec` option as before, but any command arguments can now be specified with `arg=value`,
which can be repated. If number of arguments is large then this quickly makes the command line quite
daunting, so an alternative method is to specify all arguments to shadowrun, then add a special `--`
limiter, and then specify arguments that will be passed directly to the command as you would write them
when executing it directly.

##### Quoting

The default behavior is to ensure all arguments are surrounded by double quotes when the command
is executed. This means that in most cases you don't have to do anything special, just specify
the arguments as you normally would: Surround with double quotes if the value contain spaces,
else you can skip the quotes.

Not all arguments can be quoted though. For example if you were to run the following
command it would fail for same reason as when you try to execute "dir" including the
quotes in a command prompt ('"dir"' is not recognized as an internal or external command)

```
shadowrun.exe -env -mount -wait -exec=C:\Windows\System32\cmd.exe C: -- /C DIR %SHADOW_DRIVE_1%
```

To solve this there is an option `-nq` ("no quoting") which disables the forced quoting.
This means, though, that it is a bit more cumbersome to specify values with space,
which actually needs quoting: You have to surround them with triple quotes! The reason
is that you need one pair of quotes for the value to appear as one entry from your
shell and into the ShadowRun application. But when the application reads the value it
will "consume" the quotes, so when sending the value along to the exec command then it
would no be quoted. Therefore you need an additional pair of quotes for the exec command.
But now you get the problem of quotes inside quotes, so you need some kind of escaping.
Adding an extra quote is the best way to do this, so this means you need triple quoting
```
-arg="""c:\program files"""
```

Note that the `-nq` option will only have effect for following arguments. So you can
specify some `-arg` entries first, then `-nq`, and then some more `-arg` which will
have the "no quoting" behaviour. Since you cannot specify any program options after
the `--`, all arguments given here will be affected by a `nq` option.

#### Pass-through exit code

You will now get the exit code from the executed command (`-exec`), so that
you can handle that as you would when running it directly. If shadowrun fails then it will set
exit codes also, but to be able to distinct between them you can set option `-errorcode` to an integer
offset for exit codes that shadowrun should return.

#### Improved wait option

Wait option from the original vshadow is kept, but with different functionality: Here it waits after
the snapshot(s) have been created, and after any exec commands have been executed, but before starting
to cleanup. This means you are able to interact with the temporary shadow copies on the side (similar to
the trick I described earlier by letting vshadow execute Notepad). The snapshots are "mounted" as
named device object, so-called MS-DOS device, similar to what the you can do with the command line
utility `subst`. They will not be visible to other users, and not every application will see them either,
but from `Command Prompt` you should be able to `cd` into it, show it with `subst` etc.

### Other changes

#### Backwards compatibility with vshadow syntax

The `-nw` ("no-writer") option from the original vshadow source is implicit, but included for compatibility.

#### Improved output

The output messages printed from vshadow is a bit misleading when used in this situation, since it is not
the snapshot creation itself that is our main focus of attention but the executed command and the snapshot
is more or less something that should just be done silently in the background. For this reason I have started
changing the message strings printed so that it does not talk so much about "backup" etc.

#### Build configuration

On 64-bit versions of Windows (x64), the VSS framework COM libraries are 64-bit, so the application must
also be. The original vshadow repository contains Visual Studio project, but only with 32-bit (Win32)
build configuration. Attempting to run a 32-bit build will fail mysteriously, but with hints in
Windows Event Log with error messages from VSS regarding COM component not being registered or similar.

### Source code

The source code is still based on the original source code from vshadow, it is not a complete
rewrite, although parts of it have been refactored, and obiously code has been added.

I am have not been very conscious about compiler versions, language features etc. Using C++ standard features only
avoiding Microsoft specific features are not important as the VSS integration is Windows specific anyway.

I removed build dependency to C++ ATL (Active Template Library), which is the original (vshadow) source code.
It were used solely for the CComPtr smart pointer class, so I rewrote this to use the Microsoft specific
_com_ptr_t class instead, which is very similar.
