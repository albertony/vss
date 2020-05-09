# VSS

This is my repository for VSS related utilities.

For details of the individual utilities, see individual readme files in the respective folders:
* Vshadow (the untouched original repository): [/vshadow](https://github.com/albertony/vss/tree/master/vshadow).
* Shadowrun (my own utility based on vshadow): [/shadowrun](https://github.com/albertony/vss/tree/master/shadowrun).

## Volume Shadow Copy Service (VSS) background information

### Motivation

From [Microsoft TechNet Library](https://docs.microsoft.com/en-gb/windows-server/storage/file-server/volume-shadow-copy-service):
> Backing up and restoring critical business data can be very complex due to the following issues:
>  - The data usually needs to be backed up while the applications that produce the data are still running.
>    This means that some of the data files might be open or they might be in an inconsistent state.
>  - If the data set is large, it can be difficult to back up all of it at one time.

### About

From [Wikipedia](https://en.wikipedia.org/wiki/Shadow_Copy):
> Shadow Copy (also known as Volume Snapshot Service, Volume Shadow Copy Service or VSS) is a technology included
> in Microsoft Windows that can create backup copies or snapshots of computer files or volumes, even when they are
> in use. It is implemented as a Windows service called the Volume Shadow Copy service. A software VSS provider
> service is also included as part of Windows to be used by Windows applications. Shadow Copy technology requires
> either the Windows NTFS or ReFS filesystems in order to create and store shadow copies. Shadow Copies can be
> created on local and external (removable or network) volumes by any Windows component that uses this technology,
> such as when creating a scheduled Windows Backup or automatic System Restore point.

From [Microsoft in Windows Developer Center](https://docs.microsoft.com/en-gb/windows/win32/vss/volume-shadow-copy-service-portal):
> The Volume Shadow Copy Service (VSS) is a set of COM interfaces that implements a framework to allow volume
> backups to be performed while applications on a system continue to write to the volumes.

### Use in Windows

VSS is in use in Windows for various features.

#### System Protection

The System Protection feature in Windows regularly creates what it calls "system restore points", which are basically
VSS shadow copies of your drive. It can be used to restore your entire system, or to restore previous versions of
individual files through the Previous Versions pane in file properties. By default, System Protection is turned
on for system drive (C: drive), and allowed to use up to 5% of its storage capacity.
When the limit is reached, it will automatically delete the oldest restpore points. You can manually delete restore points
from the System Protection pane in the System Properties dialog in Windows, and the Disk Clean-up will also delete old restore points.

#### Backup

Windows has a built-in backup feature, which also utilizes VSS.

### Administration

#### Vssadmin

Very basic command line utility included in Windows which displays basic information about volume shadow copies, and lets you delete
based on different criteria.

```
Delete Shadows        - Delete volume shadow copies
List Providers        - List registered volume shadow copy providers
List Shadows          - List existing volume shadow copies
List ShadowStorage    - List volume shadow copy storage associations
List Volumes          - List volumes eligible for shadow copies
List Writers          - List subscribed volume shadow copy writers
Resize ShadowStorage  - Resize a volume shadow copy storage association
```

(https://docs.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/cc754968(v=ws.11))

#### Vshadow

VShadow is a more advanced command-line tool that you can use to also create and manage many aspects of volume shadow copies.
It is not included in Windows, but in Windows SDK which you would normally install as part of Visual Studio, but can
also download and install from [Microsoft Developer Downloads](https://developer.microsoft.com/en-us/windows/downloads/).

It is actually intended as a sample for demonstrating the use of the [Volume Shadow Copy Service](https://docs.microsoft.com/en-gb/windows/win32/vss/volume-shadow-copy-service-portal) (VSS) COM API.
It even bears the name "Volume Shadow Copy **sample client**", still, but even so it is highly usable.

For more information about the VShadow tool and its command-line options,
see [VShadow Tool and Sample](https://docs.microsoft.com/en-gb/windows/win32/vss/vshadow-tool-and-sample)
and [VShadow Tool Examples](https://docs.microsoft.com/en-gb/windows/win32/vss/vshadow-tool-examples).

The source code (C++) is published in [Windows classic samples](https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/VShadowVolumeShadowCopy) repository, with Visual Studio project file ready
for building (requires ATL, but other than that no external dependencies). License is MIT, with basically the
only demand that a copyright notice are included in "all copies or substantial portions of the software".

Note: The repository contains two slightly different copies of the source code, see [notes below](#vshadow-source-code)

```
Usage:
   VSHADOW [optional flags] [commands]

List of optional flags:
  -?                 - Displays the usage screen
  -p                 - Manages persistent shadow copies
  -nw                - Manages no-writer shadow copies
  -nar               - Creates shadow copies with no auto-recovery
  -tr                - Creates TxF-recovered shadow copies
  -ad                - Creates differential HW shadow copies
  -ap                - Creates plex HW shadow copies
  -scsf              - Creates Shadow Copies for Shared Folders (Client Accessible)
  -t={file.xml}      - Transportable shadow set. Generates also the backup components doc.
  -bc={file.xml}     - Generates the backup components doc for non-transportable shadow set.
  -wi={Writer Name}  - Verify that a writer/component is included
  -wx={Writer Name}  - Exclude a writer/component from set creation or restore
  -mask              - BreakSnapshotSetEx flag: Mask shadow copy luns from system on break.
  -rw                - BreakSnapshotSetEx flag: Make shadow copy luns read-write on break.
  -forcerevert       - BreakSnapshotSetEx flag: Complete operation only if all disk signatures revertable.
  -norevert          - BreakSnapshotSetEx flag: Do not revert disk signatures.
  -revertsig         - Revert to the original disk's signature during resync.
  -novolcheck        - Ignore volume check during resync. Unselected volumes will be overwritten.
  -script={file.cmd} - SETVAR script creation
  -exec={command}    - Custom command executed after shadow creation, import or between break and make-it-write
  -wait              - Wait before program termination or between shadow set break and make-it-write
  -tracing           - Runs VSHADOW.EXE with enhanced diagnostics


List of commands:
  {volume list}                   - Creates a shadow set on these volumes
  -ws                             - List writer status
  -wm                             - List writer summary metadata
  -wm2                            - List writer detailed metadata
  -wm3                            - List writer detailed metadata in raw XML format
  -q                              - List all shadow copies in the system
  -qx={SnapSetID}                 - List all shadow copies in this set
  -s={SnapID}                     - List the shadow copy with the given ID
  -da                             - Deletes all shadow copies in the system
  -do={volume}                    - Deletes the oldest shadow of the specified volume
  -dx={SnapSetID}                 - Deletes all shadow copies in this set
  -ds={SnapID}                    - Deletes this shadow copy
  -i={file.xml}                   - Transportable shadow copy import
  -b={SnapSetID}                  - Break the given shadow set into read-only volumes
  -bw={SnapSetID}                 - Break the shadow set into writable volumes
  -bex={SnapSetID}                - Break using BreakSnapshotSetEx and flags, see options for available flags
  -el={SnapID},dir                - Expose the shadow copy as a mount point
  -el={SnapID},drive              - Expose the shadow copy as a drive letter
  -er={SnapID},share              - Expose the shadow copy as a network share
  -er={SnapID},share,path         - Expose a child directory from the shadow copy as a share
  -r={file.xml}                   - Restore based on a previously-generated Backup Components document
  -rs={file.xml}                  - Simulated restore based on a previously-generated Backup Components doc
  -revert={SnapID}                - Revert a volume to the specified shadow copy
  -addresync={SnapID},drive       - Resync the given shadow copy to the specified volume
  -addresync={SnapID}             - Resync the given shadow copy to it's original volume
  -resync=bcd.xml                 - Perform Resync using the specified BCD
```
(https://docs.microsoft.com/en-us/windows/win32/vss/vshadow-tool-and-sample)

#### Diskshadow

Documented here, but not sure if this is still availble?

https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/diskshadow


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

This source requires ATL, a dependency removed from my own utilities based on the same source.

See separate readme (the one from the original repository) in [/vshadow](https://github.com/albertony/vss/tree/master/vshadow).

#### Building

You can build the application using latest version of Visual Studio. You would need the C++ workload, with at least
MSVC build tools, Windows 10 SDK, as well as ATL. The included project file only include 32-bit (Win32) build configuration,
and as you probably are using a 64-bit version of Windows you need to create a 64-bit (x64) build configuration for the
application to work (the 32-bit build will fail with COM related errors).

### ShadowRun

My own project for a utility based on the VShadow source code, but focused on a very specific feature:
The creation of temporary read-only shadow copy and running of a specified utility that
can work on this, before it is cleaned up.

See separate readme in [/shadowrun](https://github.com/albertony/vss/tree/master/shadowrun).

## Notes

### Vshadow source code

Microsofts [Windows classic samples](https://github.com/microsoft/Windows-classic-samples) repository contains
two slightly different copies of the source code:
* One at [Samples/Win7Samples/winbase/vss/vshadow](https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/Win7Samples/winbase/vss/vshadow)
* Another at [Samples/VShadowVolumeShadowCopy](https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/VShadowVolumeShadowCopy)

The sources are very similar, but with a few differences:
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

I guess the Windows 10 SDK inluded version is based on the Win8 version (if its even built on the same source, although
the Windows 10 SDK included exeuctable still calls itself by the same "sample client" name and with a copyright statement
for with year 2005!) but using a different build configuration from the one published. The project files in both
repositories are for a quite old version of Visual Studio, and they don't even include 64-bit build configuration
which is needed for it to run on 64-bit versions of Windows.

