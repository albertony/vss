# ShadowRun

ShadowRun is a utility based on Microsoft's feature packed open source utility Vshadow, but focused on a
single very specific feature: The creation of temporary read-only shadow copy and running of a specified
utility that can work on this, before everything is automatically cleaned up.

See root of repository [vss](..) for background information about Volume Shadow Copy Service (also known
as Volume Snapshot Service, VSS for short).

Following is some write-up on motivation, and the process of trying to get VShadow to work satisfactorily.

**If are familiar with, or don't care about, the background story, jump right to the description of features: [Introducing ShadowRun](https://github.com/albertony/vss/tree/master/shadowrun#introducing-shadowrun).**

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
to [achive this](#using-vshadow-with-rclone). Further down this site I [introduce](#introducing-shadowrun)
my own ShadowRun application where I have added improvements that makes the [solution](#using-shadowrun-with-rclone)
easier, more flexible, and more robust.

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
the last challenge is how to actually access the shadow device. Most regular file commands and programs
cannot access it by the device name directly! Some do actually: COPY can be used to copy
a file directly using `copy %VSHADOW_DEVICE_1%\somefile.txt C:\somefile_bak.txt`, but DIR
does not work etc so you will probably not get to do what you want. Luckily the built-in
`MKLINK` utility are able to create a directory symbolic link with the snapshot device as target,
and then you can "mount" it to a regular directory path on your C: drive.

One tip to test out your command step by step is to add `PAUSE` statements in the batch
script specified to the `-exec` option. Then you vshadow will suspend during execution
until you press a key, and you can e.g. use another Command Prompt instance to interact with
temporary snapshot.

Another trick to test this out interactively is to specify a "blocking" command in the `-exec`
argument. For example, if you specify `notepad.exe` then after vshadow has created the
snapshot it start the regular Notepad application and then wait for it to exit before
cleaning up the snapshot! So now you can just leave the notepad window open, start another
Command Prompt and access the snapshot. When you are done you just close the Notepad Window,
and the snapshot is gone.

```
vshadow.exe -nw -script=setvars.cmd -exec=C:\Windows\System32\notepad.exe C:
```

Instead of notepad, you can also start a new instance of the "Command Interpreter".
Then vshadow will start a new command prompt inside the one you started vshadow
from, so it appears as if vshadow has just suspended and you can interact with
the command prompt. Now to resume vshadow, to let it clean up (remove the snapshot)
you type `exit` to get out of the "inner" command prompt.

```
vshadow.exe -nw -script=setvars.cmd -exec=C:\Windows\System32\cmd.exe C:
```

Another alternative is to make the snapshot "persistent", by adding the `-p` option. Then
it will not be removed automatically when vshadow command has completed, it will
even persists across restarts, so you can play around with it as long as you want.

Create it:

```
vshadow.exe -p -nw -script=setvars.cmd C:
```

Load the generated script containing environment variables:

```
setvars.cmd
```

Mount it:

```
vshadow -el=%SHADOW_ID_1%,T:
```

Remove it:

```
vshadow -ds=%SHADOW_ID_1%
```

### Using VShadow with Rclone

Let's say that we want to sync a directory `C:\Data\` to the cloud. Using rclone you would normally
execute `rclone sync C:\Data\ remote:Data`. To make this command read source files from a VSS snapshot,
you can create a batch script with the following content:

```
rem Load the variables from a temporary script generated by vshadow.exe
call "%~dp0setvars.cmd" || exit /b 1

rem Create the symbolic link to the snapshot (the backslash after SHADOW_DEVICE_1 is important!)
mklink /d C:\Snapshot\ %SHADOW_DEVICE_1%\ || exit /b 1

rem Execute rclone with the source C:\Snapshot\Data containing a snapshot of C:\Data
rclone sync C:\Snapshot\Data\ remote:Data

rem Delete the symbolic link
rmdir C:\Snapshot\

rem Delete the temporary file created by vshadow.exe
del "%~dp0setvars.cmd"
```

If you save this script as `exec.cmd` you could now execute the following command from the same
directory:

```
vshadow.exe -nw -script=setvars.cmd -exec=exec.cmd C:
```

Or, if you want a single-click solution; create a second batch script in the same directory,
which executes this command - but with full paths of both file references to avoid issues with
changing working directory:

```
vshadow.exe -nw -script="%~dp0setvars.cmd" -exec="%~dp0exec.cmd" C:
```

When the vshadow.exe command is executed, possibly from your single-click wrapper script, it will:
1. Create a read-only snapshot of your `C:` drive.
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
- The path `C:\Snapshot\` is a temporary symbolic link only accessible while exec.cmd is running, you will see
it in Windows Explorer until the sync is completed. But the directory is read-only, because the
`vshadow.exe` command included the command line argument `-nw` ("no writers").
- The contents of `C:\Snapshot\` that rclone sees, is a mirror image of `C:\`, meaning there will be a
directory `C:\Snapshot\Program Files` mirroring `C:\Program Files` etc. If this is confusing, read
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
- The mklink command returns error when the link path (`C:\Snapshot\`) already exists, and this is important to
handle. If we let the script just continue, rclone will try to sync a subfolder named data (`C:\Snapshot\Data`).
If this does not exist then rclone will just abort with error, so that is ok. Worse if this path does exists,
then rclone will actually sync it. What is normally expected in `C:\Snapshot\Data` is a snapshot of `C:\Data`.
In worst case this has previously been successfully synced to `remote:Data`, and after the last sync this new
folder `C:\Snapshot\Data` has been created with content that is not at all similar to what is in `C:\Data`.
The result is that the current sync will end up deleting everything from `remote:Data` and upload whatever is in
`C:\Snapshot\Data`. A lot of "ifs" here, perhaps a bit paranoid to expect it, but unless the link path is
very carefully chosen it could happen.
- The mklink command does not verify the link target, so even if the variable `%SHADOW_DEVICE_1%` contains an invalid
value or for some reason is empty, the mklink command will succeed. The following rclone sync command using the
link as source will then simply fail (`Failed to sync: directory not found`), so this so ok.

### Improvements

It may be confusing that `C:\Snapshot\` is a mirror image of `C:\`, and when referring `C:\Snapshot\Data` it is
a mirror of `C:\Data`. When writing more complex scripts this confusion may easily lead to bugs. You may find
it less confusing if you create an alias `T:` that you can use when referrring to the snapshot. This can easily
be done using the built-in `SUBST`command: Add `subst T: C:\` (error handling discussed later) before the
`rclone sync` command and `subst T: /d` after, and change the sync command to use `T:` instead
of `C:`: `rclone sync T:\Snapshot\Data\ remote:Data`.

Instead of just making `T:` an alias to the entire `C:` drive, you can make it an alias directly into the
snapshot image at `C:\Snapshot\`. Then the paths on `C:` you would normally use with rclone directly without vss,
now becomes identical when used on the snapshot `T:`. This may be even less confusing: `T:` is now the image of
`C:`.  This has the additional benefit of not increasing the path lengths of the source paths that rclone gets
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

##### The resulting version of `exec.cmd` can be something like this:

```
setlocal enabledelayedexpansion
call "%~dp0setvars.cmd" || set exit_code=!errorlevel!&&goto end
mklink /d C:\Snapshot\ %SHADOW_DEVICE_1%\ || set exit_code=!errorlevel!&&goto end
subst T:\ C:\Snapshot\ || set exit_code=!errorlevel!&&goto remove_link
rclone sync T:\Data\ remote:Data
set exit_code=%errorlevel%
subst T: /d
:remove_link
rmdir C:\Snapshot\
:end
del "%~dp0setvars.cmd"
exit /b %exit_code%
```

If everything goes fine until the rclone command, then the exit code from rclone is what will be returned.
If the removal of virtual drive (`subst T: /d`) and directory symbolic link (`rmdir C:\Snapshot\`) fails it is
just ignored. This will lead to the drive/directory being left accessible after the script has completed, and
if you run the script again it will abort with error because the path/drive is already in use. You could add
a check of the result from these two commands too. For example just write an additional warning
(append `||echo WARNING: Manual cleanup required`), or also set the exit code if any of these fails
(append `||set exit_code=!errorlevel!&&echo WARNING: Manual cleanup required`), depending if you see this
as something that should be reported as an error or since the rclone command passed you consider it more of
a success.

### Alternative variants

An alternative to using the SUBST command to create an alias, is to create a network share that you access
via localhost. This can be done by replacing the `rclone sync C:\Snapshot\Data\ remote:Data` line with something
like this (could be extended with similar error handling as above):

```
net share Snapshot=C:\Snapshot
rclone sync \\localhost\Snapshot\Data\ remote:Data
net share Snapshot /delete
```

A rather different approach could be to create the snapshot as a persistent one, using the `-p` option, as
described in [Using VShadow](#using-vshadow). Then our `-exec` script could actually execute vshadow again
to let it perform the mounting, using the `-el` or `-er` option. You would have to explicitely delete
the snapshot again by executing vshadow again, using the `-ds` option. Perhaps it would be possible to
run the command to remove snapshot from the `-exec` script? I haven't tried this.

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
This makes it easier to execute other applications directly using the `-exec` option,
instead of always having to go via a batch script (or parse the file content) to be
able to find the snapshot information.

Added option: `-env`

#### Automatically mounting

Support for automatically mounting the created shadow copies so that they are accessible
from a drive letter. The drive letters used are set in new environment variables SHADOW_DRIVE_n.
You can also specify which drive letters you want to use, default is to use whichever is available.

The snapshots are mounted as named device object, so-called
[MS-DOS device](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-ms-dos-device-names),
which essentially is a user mode symbolic link in Windows' Object Manager component,
to the device object representing the snapshot. What it means, is that most applications
will get to the snapshot device through a regular drive letter, and be able to access it
like a regular hard drive. The built-in command line utility `subst` does the same thing.
These symbolic links (drive letters) will not be visible to other users. They are only valid
for the current session, so will be gone after a restart if not explicitely deleted before that.
This is a good match for the use in this application, where the device we are mounting is
only existing while the application is running, and only intended to be used by this application.

Added option: `-mount`

#### Additional environment variables

In addition to SHADOW_DRIVE_n mentioned above, there is also a new variable SHADOW_SET_COUNT that
can be used to easily loop on the other SHADOW variables, while avoiding reading any variables
that for some reason was inherited from the parent environment, for index numbers not part of
the current run.

#### Arguments to the executed command

Command line arguments to the exec command can be specified. The main executable must be specified
with the `-exec` option as before, but any command arguments can now be specified with `arg=value`,
which can be repated. If number of arguments is large then this quickly makes the command line quite
daunting, so an alternative method is to specify all arguments to shadowrun, then add a special `--`
limiter, and then specify arguments that will be passed directly to the command as you would write them
when executing it directly.

Added options: `-arg` and `--`

##### Quoting

The default behavior is to ensure all arguments are surrounded by double quotes when the command
is executed. This means that in most cases you don't have to do anything special, just specify
the arguments as you normally would: Surround with double quotes if the value contain spaces,
else you can skip the quotes.

So you could run some sweet little single-liner like the following. No need to author a
batch script for `-exec` in advance. No files will be generated, like with `-script`.
Less "moving parts", usually means less risk of unexpected behaviour!

```
shadowrun.exe -env -mount -wait -exec=C:\Tools\Rclone\rclone.exe C: -- sync %SHADOW_DRIVE_1%\Files remote: --dry-run --filter-from %SHADOW_DRIVE_1%\Files\.rclonefilter --exclude-if-present .rcloneignore --checkers 6 --transfers 6 --buffer-size 64M --jottacloud-md5-memory-limit 512M --include /Files/Pictures/**
```

Not all arguments can be quoted though. For example if you were to run the following
command it would fail for same reason as when you try to execute "dir" including the
quotes in a command prompt ('"dir"' is not recognized as an internal or external command)

```
shadowrun.exe -env -mount -wait -exec=C:\Windows\System32\cmd.exe C: -- /C dir %SHADOW_DRIVE_1%
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
-arg="""C:\Program Files"""
```

Note that the `-nq` option will only have effect for following arguments. So you can
specify some `-arg` entries first, then `-nq`, and then some more `-arg` which will
have the "no quoting" behaviour. Since you cannot specify any program options after
the `--`, all arguments given here will be affected by a `nq` option.

#### Pass-through exit code

Vshadow returns the following [exit codes](https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/VShadowVolumeShadowCopy/cpp/shadow.cpp#L28-L34):

* 0 - Success
* 1 - Object not found
* 2 - Runtime Error 
* 3 - Memory allocation error

It does check the exit code of the executed command (`-exec`), but if it is nonzero
then vshadow will always just treat it as an error and return its own exit code (value 2).
This means you will not get the exit code of the command back (without parsing the message output),
and if the executed command uses non-zero exit codes for non-error situations this will not
be considered.

ShadowRun will return the exit code from the executed command (`-exec`), so that
you can handle the exit code from shadowrun as you would when running the command directly.
If shadowrun itself fails for some reason, then it will still set its own exit code, so
if there is overlap with the exit codes used by your command you will not be able to tell
from the exit code which of them it came from. To solve this there is a new option,
`-errorcode`, which can be set to an integer value which will be used as an offset for
exit code values that shadowrun should return. So if you set `-errorcode=1000` then
you will get exit code 1000 instead of 1, 1001 instead of 2 and 1002 instead of 3.
Then you can assume that, if exit code is non-zero then below 1000 it comes from the
executed command, else it comes from vshadow.

Added option: `-errorcode`

#### Improved wait option

Wait option from the original vshadow is kept in shadowrun, but with different functionality:
Here it waits after the snapshot(s) have been created, and after any exec commands have
been executed, but before starting to cleanup. This means you are able to interact with
the temporary shadow copies on the side (similar to the trick I described earlier by
letting vshadow execute Notepad).

Added option: `-wait`

### Other changes

#### Backwards compatibility with vshadow syntax

The `-nw` ("no-writer") option from the original vshadow source is implicit, but kept
for compatibility so that you will be able to run shadowrun with the same arguments
as vshadow for the specific functionality that shadowrun has focused on.

#### Improved output

The output messages printed from vshadow is a bit misleading when used for the purpose
described here, since it is not the snapshot creation itself that is our main focus of
attention but the executed command and the snapshot is more or less something that
should just be done silently in the background. For this reason I have started
changing the message strings printed so that it does not talk so much about "backup" etc.

#### Build configuration

On 64-bit versions of Windows (x64), the VSS framework COM libraries are 64-bit, so the application must
also be. The original vshadow repository contains Visual Studio project, but only with 32-bit (Win32)
build configuration. Attempting to run a 32-bit build will fail mysteriously, but with hints in
Windows Event Log with error messages from VSS regarding COM component not being registered or similar.

### Source code

The source code is still based on the original source code from vshadow, it is not a complete
rewrite, although smaller parts of it have been refactored, and of course lots of it removed,
and some code added for the new features. This is something I might reconsider, but
for now the risk of introducing more errors then I fix is too big. After all, vshadow has
been in use for a long time, and if the source code from the open source repository is more or
less the same as is used to build the exe included in Windows SDK then it has probably been
trough some tough tests over the years.

I have have not been very conscious about compiler version, language version etc.
Using C++ standard features only, avoiding Microsoft specific features, are not considered
important since the VSS integration is Windows specific anyway.

I removed build dependency to C++ ATL (Active Template Library), which is the
original (vshadow) source code. It were used solely for the CComPtr smart pointer class,
so I rewrote this to use the Microsoft specific _com_ptr_t class instead, which is very similar.

## Using ShadowRun with Rclone

Picking up on the case of [using VShadow with Rclone](#using-vshadow-with-rclone), where we wanted
to sync a directory `C:\Data\` to the cloud, and by use of rclone one would normally execute

```
rclone sync C:\Data\ remote:Data
```

Now instead of all the steps and batch scripting needed to make this work with VSS using the
original `vshadow` utility, we can be able to do this very easy with `shadowrun`:

```
shadowrun.exe -env -mount -exec=C:\Tools\Rclone\rclone.exe C: -- sync %SHADOW_DRIVE_1%\Data\ remote:Data
```

We are using the added features for [setting process environment variables](#setting-process-environment-variables),
[automatically mounting](automatically-mounting) and [arguments to the executed command](#arguments-to-the-executed-command).
We can also check the exit code, which will be the one from rclone, due to the added feature [pass-through exit code](#pass-through-exit-code).

See description of the [quoting feature](#quoting) for description of possible quoting issues, and a more complex example.
