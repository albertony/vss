// Main header
#include "stdafx.h"
#include "version.h"
#include <VersionHelpers.h>


///////////////////////////////////////////////////////////////////
//
//  Main routine
//
//  Default return values:
//      0 - Success
//      1 - Object not found
//      2 - Runtime Error
//      3 - Memory allocation error
//
extern "C" int __cdecl wmain(__in int argc, __in_ecount(argc) WCHAR ** argv)
{
    (void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    FunctionTracer ft(DBG_INFO);
    CommandLineParser obj;

    try
    {
        // Build the argument vector
        vector<wstring> arguments;
        for(int i = 1; i < argc; i++)
            arguments.push_back(argv[i]);

        // Run actions based on arguments
        return obj.Run(arguments);
    }
    catch(bad_alloc ex)
    {
        // Generic STL allocation error
        ft.WriteErrorLine(L"ERROR: Memory allocation error!");
        return 3;
    }
    catch(exception ex)
    {
        // We should never get here (unless we have a bug)
        _ASSERTE(false);
        ft.WriteErrorLine(L"ERROR: STL Exception caught!");
        ft.WriteErrorLine(L"- Exception text: %S", ex.what());
        return 2;
    }
    catch(HRESULT hr)
    {
        ft.Trace(DBG_INFO, L"HRESULT Error caught: 0x%08lx", hr);
        return 2;
    }
}


// Verify that OS version is supported
bool OSVersionCheck()
{
    bool bIsWin8 = FALSE;
    bIsWin8 = IsWindows8OrGreater();
    return !bIsWin8;
}


// Print logo
void PrintLogo()
{
    FunctionTracer ft(DBG_INFO);
    ft.WriteInfoLine(
        L"ShadowRun.exe (Volume Shadow Copy Runner) version " VER_PRODUCTVERSION_STR ".\n"
        L"Copyright (C) 2022 Albertony. All rights reserved.\n"
    );
}


//  Prints the command line options
void PrintUsage(bool startWithNewline)
{
    FunctionTracer ft(DBG_INFO);
    if (startWithNewline)
    {
        ft.WriteInfoLine(L"");
    }
    ft.WriteInfoLine(
        L"Usage:\n"
        L"   ShadowRun [flags] [volumes]\n"
        L"\n"
        L"List of flags:\n"
        L"  -?                 - Displays this usage screen\n"
        L"  -h                 - Displays this usage screen\n" // Added (not from orginal vshadow)
        L"  -nw                - Create no-writer shadow copies (implied and deprecated, only for vshadow syntax compat)\n" // Implied, but kept for backwards compatibility with vshadow
        L"  -script={file.cmd} - Environment variable configuration script creation\n"
        L"  -exec={command}    - Custom command executed after shadow creation\n"
        L"  -wait              - Wait before program termination\n"
        L"  -tracing           - Runs ShadowRun.exe with enhanced diagnostics\n" // Deprecated, should use -log-level=trace instead, but kept for backwards compatibility with vshadow
        L"  -env               - Set process environment variables\n" // Added (not from orginal vshadow)
        L"  -mount             - Mount shadow copies as temporary drives\n" // Added (not from orginal vshadow)
        L"  -drive={ABC}       - Specific drive letters to use for mounting\n" // Added (not from orginal vshadow)
        L"  -errorcode=1       - First exit code value to use for internal errors, not from the command\n" // Added (not from orginal vshadow)
        L"  -nq                - Do not force quotes around arguments specified with -arg or after -- with quotes\n" // Added (not from orginal vshadow)
        L"  -arg={string}      - Argument to append after the -exec command, repeat or use -- for multiple arguments\n" // Added (not from orginal vshadow)
        L"  -log-level={debug} - Log level, one of: trace, debug, info (default), notice (unused), error or silent\n" // Added (not from orginal vshadow), replaces tracing from original vshadow
        L"  -- {args}...       - Special flag that makes all following arguments being passed directly to the -exec\n" // Added (not from orginal vshadow)
    );
}


// Constructor
CommandLineParser::CommandLineParser()
{
    FunctionTracer ft(DBG_INFO);
}


// Destructor
CommandLineParser::~CommandLineParser()
{
    FunctionTracer ft(DBG_INFO);
}


int CommandLineParser::Run(vector<wstring>& arguments)
{
    FunctionTracer ft(DBG_INFO);

    // Check platform support
    if (OSVersionCheck())
    {
        FunctionTracer::SetLogLevel(FunctionTracer::LOGLEVEL_INFO); // Force log level to make sure error is actually shown, although info is currently default
        PrintLogo();
        ft.WriteErrorLine(L"ERROR: This version of Windows is not supported!");
        throw(E_INVALIDARG);
    }

    // Check for help request (no arguments, or single argument -? or -h)
    if (arguments.size() == 0 || (arguments.size() == 1 && (MatchArgument(arguments[0], L"?") || MatchArgument(arguments[0], L"h"))))
    {
        FunctionTracer::SetLogLevel(FunctionTracer::LOGLEVEL_INFO); // Force log level to make sure help is actually shown, although info is currently default
        PrintLogo();
        PrintUsage(false);
        return EXIT_SUCCESS; // Default value: 0
    }

    // Handle -log-level or -tracing argument
    int logLevel = -1;
    for (vector<wstring>::iterator it = arguments.begin(); it != arguments.end(); )
    {
        wstring value;
        if (MatchArgument(*it, L"log-level", value, true, true))
        {
            if (value == L"trace")
            {
                logLevel = FunctionTracer::LOGLEVEL_TRACE;
            }
            else if (value == L"debug")
            {
                logLevel = FunctionTracer::LOGLEVEL_DEBUG;
            }
            else if (value == L"info")
            {
                logLevel = FunctionTracer::LOGLEVEL_INFO;
            }
            else if (value == L"notice")
            {
                logLevel = FunctionTracer::LOGLEVEL_NOTICE;
            }
            else if (value == L"error")
            {
                logLevel = FunctionTracer::LOGLEVEL_ERROR;
            }
            else if (value == L"silent")
            {
                logLevel = FunctionTracer::LOGLEVEL_SILENT;
            }
            else
            {
                PrintLogo();
                ft.WriteErrorLine(L"ERROR: The parameter '%s' must be trace, debug, info, notice, error or silent!", value.c_str());
                PrintUsage(true);
                throw(E_INVALIDARG);
            }
            FunctionTracer::SetLogLevel(logLevel);
            PrintLogo(); // Note: Postponed until log level is set!
            ft.WriteDebugLine(L"Log level %s", value.c_str()); // Will only be logged if the level is debug or trace
            it = arguments.erase(it);
            break;
        }
        else if (MatchArgument(*it, L"tracing"))
        {
            logLevel = FunctionTracer::LOGLEVEL_TRACE;
            FunctionTracer::SetLogLevel(logLevel);
            PrintLogo(); // Note: Postponed until log level is set!
            ft.WriteDebugLine(L"Log level trace - consider using newer option -log-level=trace instead of -tracing");
            it = arguments.erase(it);
            break;
        }
        else
        {
            ++it;
        }
    }
    if (logLevel < 0)
    {
        PrintLogo();
        ft.WriteDebugLine(L"Using default log level"); // Will not be logged, as long as default log level is info
    }

    return MainRoutine(arguments);
}


//
//  Main routine
//  - First, parse the command-line options
//  - Then execute the appropriate client calls
//
//  WARNING: 
//  - The routine does not check for mutually exclusive flags/options
//
//  On error, this function throws
//
int CommandLineParser::MainRoutine(vector<wstring>& arguments)
{
    FunctionTracer ft(DBG_INFO);

    // Exit codes:
    // Customizable offset with -errorcode flag, to separate from executed command.
    // Defaults (and if errors before -errorcode is parsed) are:
    // 0 - Success
    // 1 - Object not found
    // 2 - Runtime Error
    // 3 - Memory allocation error
    DWORD exitCode = EXIT_SUCCESS;
    DWORD errorCodeStart = EXIT_FAILURE;

    // the volume for which to delete the oldest shadow copy
    wstring stringVolume;

    // The script file prefix 
    // Non-empty if scripts have to be generated 
    wstring environmentScript;

    // Modify process environment option corresponding to environment script
    bool setProcessEnvironment = false;

    // Mount shadow copy as drives
    bool mountSnapshots = false;

    // Drive letters to use for mounting. Default is to use find next available.
    wstring mountDriveLetters;

    // Command/script to execute between snapshot creation and backup complete
    wstring execCommand;

    // Additional arguments to the execCommand
    vector<wstring> execArguments;

    // Force quotes around arguments specified with -arg or after -- with quotes
    bool addExecArgumentQuotes = true;

    // Argument handling mode where all remaining arguments are just passed into execArguments.
    bool passThrough = false;

    // Wait after shadow copy has been created
    bool waitBeforeCleanup = false;

    ft.WriteDebugLine(L"Checking options...");

    try
    {
        // Enumerate each argument
        for(size_t argIndex = 0; argIndex < arguments.size(); ++argIndex)
        {
            //
            //  Flags
            //

            // Check for the script generation option
            if (MatchArgument(arguments[argIndex], L"script", environmentScript, true, false))
            {
                ft.WriteDebugLine(L"- Environment variable configuration script creation '%s'", environmentScript.c_str());
                continue;
            }

            // Check for the command execution option
            if (MatchArgument(arguments[argIndex], L"exec", execCommand, true, false)) // Note: Dequote because quotes are always added on execution due to security reasons
            {
                ft.WriteDebugLine(L"- Execute binary/script after shadow creation '%s'", execCommand.c_str());

                // Check if the command is a valid CMD/EXE file
                DWORD dwAttributes = GetFileAttributes((LPWSTR)execCommand.c_str());
                if ((dwAttributes == INVALID_FILE_ATTRIBUTES) || ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
                {
                    ft.WriteErrorLine(L"ERROR: The parameter '%s' must be an existing file!", execCommand.c_str());
                    ft.WriteErrorLine(L"- Note: For parameters use '-arg' or '--' options");
                    throw(E_INVALIDARG);
                }
                continue;
            }

            // Wait after shadow copy has been created
            if (MatchArgument(arguments[argIndex], L"wait"))
            {
                ft.WriteDebugLine(L"- Wait after shadow copy has been created");
                waitBeforeCleanup = true;
                continue;
            }

            // Check for the errorcode option
            wstring value;
            if (MatchArgument(arguments[argIndex], L"errorcode", value, true, false))
            {
                errorCodeStart = _wtoi(value.c_str());
                if (errno == ERANGE || errorCodeStart == 0)
                {
                    ft.WriteErrorLine(L"ERROR: The parameter '%s' must be a non-zero integer!", value.c_str());
                    throw(E_INVALIDARG);
                }
                ft.WriteDebugLine(L"- Program errors will be reported with exit codes starting at %d", errorCodeStart);
                continue;
            }

            // Check for the environment option
            if (MatchArgument(arguments[argIndex], L"env"))
            {
                ft.WriteDebugLine(L"- Set process environment");
                setProcessEnvironment = true;
                continue;
            }

            // Check for the mount option
            if (MatchArgument(arguments[argIndex], L"mount"))
            {
                ft.WriteDebugLine(L"- Mount shadow copies as temporary drives");
                mountSnapshots = true;
                continue;
            }

            // Check for the drive option
            if (MatchArgument(arguments[argIndex], L"drive", mountDriveLetters, true, false))
            {
                ft.WriteDebugLine(L"- Mount shadow copies as temporary drives '%s'", mountDriveLetters.c_str());
                mountSnapshots = true; // Implied
                continue;
            }

            // Check for the exec arguments option
            // Note: Will by default remove optional surrounding double quotes and then forcefully
            // add then when executing the command. This can be reversed by the -nq option: Then
            // any quoting are kept as is, so that only the arguments actually specified with quotes
            // will be quoted when executing the command, but must then quote only the value
            // not "-arg=value", but -arg="value").
            if (MatchArgument(arguments[argIndex], L"arg", value, addExecArgumentQuotes, false))
            {
                execArguments.push_back(value);
                ft.WriteDebugLine(L"- Including additional argument when executing binary/script '%s'", value.c_str());
                continue;
            }

            // Do not force quotes around arguments specified with -arg or after --, when executing the command.
            // Note: For -arg this only has affect for the ones not already specified!
            if (MatchArgument(arguments[argIndex], L"nq"))
            {
                ft.WriteDebugLine(L"- Do not force quotes around arguments specified with -arg or after -- with quotes");
                addExecArgumentQuotes = false; // Variable logic reversed, so setting quoting=false
                continue;
            }

            // Create no-writer shadow copies (deprecated option kept only for compatibility with vshadow)
            if (MatchArgument(arguments[argIndex], L"nw"))
            {
                ft.WriteDebugLine(L"- No-writers option detected, deprecated option kept only for compatibility with vshadow");
                continue;
            }

            // Check if rest (or until --) are unnamed arguments are volumes or file share paths. If yes, try to create the shadow copy set.
            if (IsVolume(arguments[argIndex]) || IsUNCPath((VSS_PWSZ)arguments[argIndex].c_str()))
            {
                ft.Trace(DBG_INFO, L"Volume %s was added as unnamed parameter", arguments[argIndex].c_str());

                // Make sure that all the arguments are volumes
                vector<wstring> volumeList;
                volumeList.push_back(GetUniqueVolumeNameForPath(arguments[argIndex]));

                // Process the rest of the arguments
                for (++argIndex; argIndex < arguments.size(); ++argIndex)
                {
                    if (passThrough)
                    {
                        // Pass argument directly on to the exec command
                        execArguments.push_back(arguments[argIndex]);
                        continue;
                    }
                    else if (MatchArgument(arguments[argIndex], L"-"))
                    {
                        // Enable pass through mode
                        passThrough = true;
                    }
                    else if (!(IsVolume(arguments[argIndex]) || IsUNCPath((VSS_PWSZ)arguments[argIndex].c_str())))
                    {
                        // No match. Print an error and the usage
                        ft.WriteErrorLine(L"\nERROR: invalid parameters %s", GetCommandLine());
                        ft.WriteErrorLine(L"- Parameter %s is expected to be a volume or a file share path!", arguments[argIndex].c_str());
                        ft.WriteErrorLine(L"- Example: ShadowRun C:");
                        PrintUsage(true);
                        return errorCodeStart; // Default value: 1
                    }
                    else
                    {
                        // Add the volume to the list
                        volumeList.push_back(GetUniqueVolumeNameForPath(arguments[argIndex]));
                    }
                }

                // Initialize the VSS client
                m_vssClient.Initialize();

                // Create the shadow copy set
                m_vssClient.CreateSnapshotSet(volumeList);

                // Mount drives (optional) - do it before environmentScript and setProcessEnvironment because they include the mounted drives information!
                if (mountSnapshots)
                    m_vssClient.MountSnapshots(mountDriveLetters);

                // Generate management scripts (optional)
                if (environmentScript.length() > 0)
                    m_vssClient.GenerateEnvironmentScript(environmentScript);

                // Set process environment (optional)
                if (setProcessEnvironment)
                    m_vssClient.SetProcessEnvironment();

                // Executing the custom command (optional)
                if (execCommand.length() > 0)
                    exitCode = ExecCommand(execCommand, execArguments, addExecArgumentQuotes);

                if (waitBeforeCleanup)
                {
                    ft.WriteInfoLine(L"Suspending ShadowRun while leaving any created shadow copies available for use");
                    ft.Write(L"Press <ENTER> to continue..."); // Note: Write regardless of log-level!
                    (void)getchar();
                }

                if (mountSnapshots)
                    m_vssClient.UnmountSnapshots();

                ft.WriteInfoLine(L"ShadowRun completed with exit code %d", exitCode);

                return exitCode; // Default value: 0
            }

            // No match. Print an error and the usage.
            // Report as "not epected" instead of "not valid" parameter, in case it is one
            // of the arguments only supported as stand-alone argument, i.e. -log-level, -tracing, -? or -h.
            ft.WriteErrorLine(L"ERROR: Parameter '%s' was not expected!", arguments[argIndex].c_str());
            PrintUsage(true);
            return errorCodeStart;  // Default value: 1
        }

        ft.WriteErrorLine(L"ERROR: Missing volume parameter!");
        PrintUsage(true);
        return errorCodeStart; // Default value: 1
    }
    catch(bad_alloc ex)
    {
        // Generic STL allocation error
        ft.WriteErrorLine(L"ERROR: Memory allocation error!");
        return errorCodeStart + 2; // Default value: 3
    }
    catch(exception ex)
    {
        // We should never get here (unless we have a bug)
        _ASSERTE(false);
        ft.WriteErrorLine(L"ERROR: STL Exception caught!");
        ft.WriteErrorLine(L"- Exception text: %S", ex.what());
        return errorCodeStart + 1; // Default value: 2
    }
    catch(HRESULT hr)
    {
        ft.Trace(DBG_INFO, L"HRESULT Error caught: 0x%08lx", hr);
        return errorCodeStart + 1; // Default value: 2
    }
}



// Returns TRUE if the argument is in the following formats
//  -xxxx
//  /xxxx
// where xxxx is the option pattern
bool CommandLineParser::MatchArgument(const wstring& argument, const wstring& optionPattern)
{
    FunctionTracer ft(DBG_INFO);

    ft.Trace(DBG_INFO, L"Matching Arg: '%s' with '%s'\n", argument.c_str(), optionPattern.c_str());

    bool retVal = (IsEqual(argument, wstring(L"/") + optionPattern) || IsEqual(argument, wstring(L"-") + optionPattern) );

    ft.Trace(DBG_INFO, L"Return: %s\n", BOOL2TXT(retVal));
    return retVal;
}


// Returns TRUE if the argument is in the following formats
//  -xxxx=yyyy
//  /xxxx=yyyy
// where xxxx is the option pattern and yyyy the additional parameter (optionally enclosed double quotes)
bool CommandLineParser::MatchArgument(const wstring& argument, const wstring& optionPattern, wstring& additionalParameter, bool dequote, bool lower)
{
    FunctionTracer ft(DBG_INFO);

    ft.Trace(DBG_INFO, L"Matching Arg: '%s' with '%s'", argument.c_str(), optionPattern.c_str());

    _ASSERTE(argument.length() > 0);

    if ((argument[0] != L'/') && (argument[0] != L'-'))
        return false;

    // Find the '=' separator between the option and the additional parameter
    size_t equalSignPos = argument.find(L'=');
    if ((equalSignPos == wstring::npos) || (equalSignPos == 0))
        return false;

    ft.Trace(DBG_INFO, L"%s %d", argument.substr(1, equalSignPos - 1).c_str(), equalSignPos);

    // Check to see if this is our option
    if (!IsEqual(argument.substr(1, equalSignPos - 1), optionPattern))
        return false;

    // Isolate the additional parameter
    additionalParameter = argument.substr(equalSignPos + 1);

    ft.Trace(DBG_INFO, L"- Additional Param: [%s]", additionalParameter.c_str());

    // We cannot have an empty additional parameter
    if (additionalParameter.length() == 0)
        return false;

    if (dequote)
    {
        // Eliminate the enclosing quotes, if any
        size_t lastPos = additionalParameter.length() - 1;
        if ((additionalParameter[0] == L'"') && (additionalParameter[lastPos] == L'"'))
            additionalParameter = additionalParameter.substr(1, additionalParameter.length() - 2);
    }
    if (lower)
    {
        // Make lowercase. Note that another alternative is to use IsEqual for case insensitive comparison.
        transform(additionalParameter.begin(), additionalParameter.end(), additionalParameter.begin(), towlower);
    }
    ft.Trace(DBG_INFO, L"Return true; (additional param = %s)", additionalParameter.c_str());
    
    return true;
}
