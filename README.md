# VSS

VShadow is a command-line tool that you can use to create and manage volume shadow copies.
It is actually intended as a sample for demonstrating the use of the
[Volume Shadow Copy Service](https://docs.microsoft.com/en-gb/windows/win32/vss/volume-shadow-copy-service-portal)
(VSS) COM API. It even bears the name "Volume Shadow Copy **sample client**", still.

For more information about the VShadow tool and its command-line options,
see [VShadow Tool and Sample](https://docs.microsoft.com/en-gb/windows/win32/vss/vshadow-tool-and-sample)
and [VShadow Tool Examples](https://docs.microsoft.com/en-gb/windows/win32/vss/vshadow-tool-examples).

The source code is published in [Windows classic samples](https://github.com/microsoft/Windows-classic-samples) repository,
with Visual Studio project file ready for building! License is MIT, with basically the only demand
that a copyright notice are included in "all copies or substantial portions of the Software".

## VSS administration utilities

The "Volume Shadow Copy sample client" (vshadow.exe) utility can be used for some basic
administration of volume shadow copies, like listing the existing volume shadow copies
on the system, delete specific ones etc. Windows 10 includes a utility called
"Volume Shadow Copy Service administrative command-line tool" (vssadmin.exe),
which can do some of the same most basic this (list and delete). It does not support
the creation of new ones though, there are a lot more functionality in vshadow.

## Projects in this repository

### VShadow

Contains the original source code of the `Volume Shadow Copy sample client` from Microsoft's
[Windows classic samples](https://github.com/microsoft/Windows-classic-samples) repository - the
Windows 8 version: https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/VShadowVolumeShadowCopy.

This source requires ATL.


### ShadowRun

A small utility based on the VShadow source code, but focused on a very specific feature:
The creation of temporary read-only shadow copy and running of a specified utility that
can work on this, before it is cleaned up.

The changes include:
* Support for setting environment variables directly into the process instead of having
to go via generated batch file, given by the `-script` option, containing `SET` statements.
* Support for automatically mounting the created shadow copies so that they are accessible
from a drive letter. The drive letters used are set in new environment variables SHADOW_DRIVE_n.
You can also specify which drive letters you want to use, default is to use whichever is available.
* In addition to SHADOW_DRIVE_n, there is also a new variable SHADOW_SET_COUNT that can be used 
to avoid reading SHADOW_..._n variables that for some reason exists with numbers not valid for the current
run.
* Exec command can now be specified with command line arguments. The main executable must be specified
with the `-exec` option as before, but command arguments can now be specified with `arg=value`,
which can be repated. If number of arguments is large then this quickly makes the command line quite
daunting, so an alternative method is to specify all arguments to shadowrun, then add a special `--`
limiter, and then specify arguments that will be passed directly to the command as you would write them
when executing it directly.
* Pass-through exit code: You will now get the exit code from the executed command (`-exec`), so that
you can handle that as you would when running it directly. If shadowrun fails then it will set
exit codes also, but to be able to distinct between them you can set option `-errorcode` to an integer
offset for exit codes that shadowrun should return.
* Wait option from the original vshadow is kept, but with different functionality: Here it waits after
the snapshot(s) have been created, and after any exec commands have been executed, but before starting
to cleanup. This means you are able to interact with the temporary shadow copies on the side!
The snapshots are "mounted" as named device object, so-called MS-DOS device, similar to what the you can
do with the command line utility `subst`. They will not be visible to other users, and not every application
will see them either, but from `Command Prompt` you should be able to `cd` into it, show it with `subst` etc.
* The `-nw` ("no-writer") option from the original vshadow source is implicit, but included for compatibility.
* Removed build dependency to ATL.
* The source code is still based on the original source code from vshadow, it is not a complete rewrite,
although parts of it have been refactored, and obiously code has been added.


## Notes

Microsofts [Windows classic samples](https://github.com/microsoft/Windows-classic-samples) repository contains
two slightly different copies of the source code:
* https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/Win7Samples/winbase/vss/vshadow
* https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/VShadowVolumeShadowCopy
The source code is very similar, but with a few differences:
* The Win7Samples/winbase/vss/vshadow (I will just be calling it the Win7 version from now on)
contains project file for Visual Studio 2005, while Samples/VShadowVolumeShadowCopy (which I will call Win8 version from now on)
contains upgraded project file for Visual Studio 2013.
* Win7 version defines the preprocessor directive VSS_SERVER in its project file, the Win8 version does not.
* The COM security initialization call to CoInitializeSecurity in VssClient::Initialize uses some different options:
   * Win7 version sets dwCapabilities to EOAC_NONE ("No special options"), Win8 to EOAC_DYNAMIC_CLOAKING ("Cloaking").
   * Win7 version sets dwImpLevel to RPC_C_IMP_LEVEL_IDENTIFY, Win8 to RPC_C_IMP_LEVEL_IMPERSONATE
* The Win8 version checks that the Windows version is at lest Windows 8 (hence my nickname for it).
* The Win8 version adds support for UNC paths as arguments.

The executable included in Windows 10 SDK, which identifies itself as version 3.0:
* Does support UNC paths (like the Win8 version)
    * You can check by executing a command like the following (the file exec.cmd must exist):
	 `vshadow -nw -script=temp.cmd -exec=exec.cmd C: somethingrandom`
	* The Win7 version will write "Parameter %s is expected to be a volume!"
	* The Win8 version will write "Parameter %s is expected to be a volume **or a file share path**!"
* Is built with VSS_SERVER preprocessor directive (like the Win7 version)
	* You can see it from the help text printed when executing `vshadow /?`,
	  a lot of options are only shown when VSS_SERVER is enabled, for example the option `-nw`.
