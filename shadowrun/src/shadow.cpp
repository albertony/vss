// Main header
#include "stdafx.h"
#include <VersionHelpers.h>

bool OSVersionCheck()
{
    bool bIsWin8 = FALSE;
    bIsWin8 = IsWindows8OrGreater();
    return !bIsWin8;
}


///////////////////////////////////////////////////////////////////
//
//  Main routine 
//
//  Return values:
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
        if (OSVersionCheck())
        {
            ft.WriteLine(L"This version of shadowrun is not supported on this version of Windows." );
            return 2;
        }

        ft.WriteLine(
            L"\n"
            L"SHADOWRUN.EXE 1.0 - Volume Shadow Copy Runner.\n"
            L"Copyright (C) 2020 Albertony. All rights reserved.\n"
            L"\n"
            );

        // Build the argument vector 
        vector<wstring> arguments;
        for(int i = 1; i < argc; i++)
            arguments.push_back(argv[i]);

        // Process the arguments and execute the main options
        return obj.MainRoutine(arguments);
    }
    catch(bad_alloc ex)
    {
        // Generic STL allocation error
        ft.WriteLine(L"ERROR: Memory allocation error");
        return 3;
    }
    catch(exception ex)
    {
        // We should never get here (unless we have a bug)
        _ASSERTE(false);
        ft.WriteLine(L"ERROR: STL Exception caught: %S", ex.what());
        return 2;
    }
    catch(HRESULT hr)
    {
        ft.Trace(DBG_INFO, L"HRESULT Error caught: 0x%08lx", hr);
        return 2;
    }
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
int CommandLineParser::MainRoutine(vector<wstring> arguments)
{
    FunctionTracer ft(DBG_INFO);

    // the volume for which to delete the oldest shadow copy
    wstring stringVolume;

    // The script file prefix 
    // Non-empty if scripts have to be generated 
    wstring stringFileName;

    // Script to execute between snapshot creation and backup complete
    wstring execCommand;

    // Enumerate each argument
    for(unsigned argIndex = 0; argIndex < arguments.size(); argIndex++)
    {
        //
        //  Flags
        //

        // Check for the script generation option
        if (MatchArgument(arguments[argIndex], L"script", stringFileName))
        {
            ft.WriteLine(L"(Option: Generate SETVAR script '%s')", stringFileName.c_str());
            continue;
        }

        // Check for the command execution option
        if (MatchArgument(arguments[argIndex], L"exec", execCommand))
        {
            ft.WriteLine(L"(Option: Execute binary/script after shadow creation '%s')", execCommand.c_str());

            // Check if the command is a valid CMD/EXE file
            DWORD dwAttributes = GetFileAttributes((LPWSTR)execCommand.c_str());
            if ((dwAttributes == INVALID_FILE_ATTRIBUTES) || ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
            {
                ft.WriteLine(L"ERROR: the parameter '%s' must be an existing file!", execCommand.c_str());
                ft.WriteLine(L"- Note: the -exec command cannot have parameters!");
                throw(E_INVALIDARG);
            }

            continue;
        }

        // Check for the tracing option
        if (MatchArgument(arguments[argIndex], L"tracing"))
        {
            ft.WriteLine(L"(Option: Enable tracing)");
            FunctionTracer::EnableTracingMode();
            continue;
        }

        // Check for /? or -?
        if (MatchArgument(arguments[argIndex], L"?"))
            break;


        // Check if the arguments are volumes or file share paths. If yes, try to create the shadow copy set 
        if (IsVolume(arguments[argIndex]) || IsUNCPath((VSS_PWSZ)arguments[argIndex].c_str()))
        {
            ft.WriteLine(L"(Option: Create shadow copy set)");

            ft.Trace(DBG_INFO, L"\nAttempting to create a shadow copy set... (volume %s was added as parameter)", arguments[argIndex].c_str());

            // Make sure that all the arguments are volumes
            vector<wstring> volumeList; 
            volumeList.push_back(GetUniqueVolumeNameForPath(arguments[argIndex], true));

            // Process the rest of the arguments
            for(unsigned i = argIndex + 1; i < arguments.size(); i++)
            {
                if (!(IsVolume(arguments[i]) || IsUNCPath((VSS_PWSZ)arguments[i].c_str())))
                {
                    // No match. Print an error and the usage
                    ft.WriteLine(L"\nERROR: invalid parameters %s", GetCommandLine());
                    ft.WriteLine(L"- Parameter %s is expected to be a volume or a file share path!  (shadow copy creation is assumed)", arguments[i].c_str());
                    ft.WriteLine(L"- Example: SHADOWRUN C:");
                    PrintUsage();
                    return 1;
                }

                // Add the volume to the list
                volumeList.push_back(GetUniqueVolumeNameForPath(arguments[i], true));
            }
            
            // Initialize the VSS client
            m_vssClient.Initialize();

            // Create the shadow copy set
            m_vssClient.CreateSnapshotSet(volumeList);

            try
            {
                // Generate management scripts if needed
                if (stringFileName.length() > 0)
                    m_vssClient.GenerateSetvarScript(stringFileName);

                // Executing the custom command if needed
                if (execCommand.length() > 0)
                    ExecCommand(execCommand);

            }
            catch(HRESULT)
            {
                throw;
            }

            ft.WriteLine(L"\nSnapshot creation done.");
            
            return 0;
        }

        // No match. Print an error and the usage
        ft.WriteLine(L"\nERROR: invalid parameter '%s'\n", arguments[argIndex].c_str());
        PrintUsage();
        return 1;
    }

    PrintUsage();
    return 0;
}



// Returns TRUE if the argument is in the following formats
//  -xxxx
//  /xxxx
// where xxxx is the option pattern
bool CommandLineParser::MatchArgument(wstring argument, wstring optionPattern)
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
// where xxxx is the option pattern and yyyy the additional parameter (eventually enclosed in ' or ")
bool CommandLineParser::MatchArgument(wstring argument, wstring optionPattern, wstring & additionalParameter)
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

    // Eliminate the enclosing quotes, if any
    size_t lastPos = additionalParameter.length() - 1;
    if ((additionalParameter[0] == L'"') && (additionalParameter[lastPos] == L'"'))
        additionalParameter = additionalParameter.substr(1, additionalParameter.length() - 2);
    
    ft.Trace(DBG_INFO, L"Return true; (additional param = %s)", additionalParameter.c_str());
    
    return true;
}


// 
//  Prints the command line options
//
void CommandLineParser::PrintUsage()
{
    FunctionTracer ft(DBG_INFO);
    ft.WriteLine(
        L"Usage:\n"
        L"   SHADOWRUN [flags] [volumes]\n"
        L"\n"
        L"List of flags:\n"
        L"  -?                 - Displays the usage screen\n"
        L"  -p                 - Manages persistent shadow copies\n"
        L"  -script={file.cmd} - SETVAR script creation\n"
        L"  -exec={command}    - Custom command executed after shadow creation, import or between break and make-it-write\n"
        L"  -tracing           - Runs SHADOWRUN.EXE with enhanced diagnostics\n"
        L"\n" );
}

CommandLineParser::CommandLineParser()
{
}

// Destructor
CommandLineParser::~CommandLineParser()
{
    FunctionTracer ft(DBG_INFO);
}
