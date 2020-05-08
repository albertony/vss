# VSS

VShadow is a command-line tool that you can use to create and manage volume shadow copies.
It is actually intended as a sample, and calls itself "Volume Shadow Copy **sample client**",
for demonstrating the use of the [Volume Shadow Copy Service](https://docs.microsoft.com/en-gb/windows/win32/vss/volume-shadow-copy-service-portal)
(VSS) COM API.

For more information about the VShadow tool and its command-line options,
see [VShadow Tool and Sample](https://docs.microsoft.com/en-gb/windows/win32/vss/vshadow-tool-and-sample)
and [VShadow Tool Examples](https://docs.microsoft.com/en-gb/windows/win32/vss/vshadow-tool-examples).

The source code is published in [Windows classic samples](https://github.com/microsoft/Windows-classic-samples) repository,
with Visual Studio project file ready for building! License is MIT, with basically the only demand
that a copyright notice are included in "all copies or substantial portions of the Software".

**Note:** The repository contains two slightly different copies of the source code:
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

## VSS administration utilities

The "Volume Shadow Copy sample client" (vshadow.exe) utility can be used for some basic
administration of volume shadow copies, like listing the existing volume shadow copies
on the system, delete specific ones etc. Windows 10 includes a utility called
"Volume Shadow Copy Service administrative command-line tool" (vssadmin.exe),
which can do some of the same most basic this (list and delete). It does not support
the creation of new ones though, there are a lot more functionality in vshadow.

## Projects in this repository

### VShadow

Contains the original source code of the `Volume Shadow Copy sample client` from Microsoft'same'
[Windows classic samples](https://github.com/microsoft/Windows-classic-samples) repository - the
Windows 8 version: https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/VShadowVolumeShadowCopy.

This source requires ATL.


### ShadowRun

A small utility based on the VShadow source code, but focused on a very specific feature:
The creation of temporary read-only shadow copy and running of a specified utility that
can work on this, before it is cleaned up.

The changes include:
* The `-nw` ("no-writer") option is implied, and the flags `-exec={command}` and `-script={file.cmd}`
are required. Flag `-tracing` are optional. No other options are supported.
* Removed dependency to ATL

Forced no-writer shadow copies (same as with command line argument `-nw` in the original version).

**TODO:**
* Still uses PathFileExists, which requires linking shlwapi.lib. Could we replace this with something more standard that is 100% equivalent?