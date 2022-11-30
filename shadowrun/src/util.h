#pragma once

#include <resapi.h>
#include "stdafx.h"

// COM interface smart pointer types (_com_ptr_t)
_COM_SMARTPTR_TYPEDEF(IVssBackupComponents, __uuidof(IVssBackupComponents)); // typedef _com_ptr_t<...> IVssBackupComponentsPtr;
_COM_SMARTPTR_TYPEDEF(IVssBackupComponentsEx4, __uuidof(IVssBackupComponentsEx4)); // typedef _com_ptr_t<...> IVssBackupComponentsEx4Ptr;


//for IsUNCPath method
#define     UNC_PATH_PREFIX1        (L"\\\\?\\UNC\\")
#define     NONE_UNC_PATH_PREFIX1   (L"\\\\?\\")
#define     UNC_PATH_PREFIX2        (L"\\\\")


/////////////////////////////////////////////////////////////////////////
//  Utility classes 
//

// Used to automatically release a CoTaskMemAlloc allocated pointer when 
// when the instance of this class goes out of scope
// (even if an exception is thrown)
class CAutoComPointer
{
public:
    CAutoComPointer(LPVOID ptr): m_ptr(ptr) {};
    ~CAutoComPointer() { CoTaskMemFree(m_ptr); }
private:
    LPVOID m_ptr;
    
};


// Used to automatically release the contents of a VSS_SNAPSHOT_PROP structure 
// (but not the structure itself)
// when the instance of this class goes out of scope
// (even if an exception is thrown)
class CAutoSnapPointer
{
public:
    CAutoSnapPointer(VSS_SNAPSHOT_PROP * ptr): m_ptr(ptr) {};
    ~CAutoSnapPointer() { ::VssFreeSnapshotProperties(m_ptr); }
private:
    VSS_SNAPSHOT_PROP * m_ptr;
};


// Used to automatically release the given handle
// when the instance of this class goes out of scope
// (even if an exception is thrown)
class CAutoHandle
{
public:
    CAutoHandle(HANDLE h): m_h(h) {};
    ~CAutoHandle() { ::CloseHandle(m_h); }
private:
    HANDLE m_h;
};


// Used to automatically release the given handle
// when the instance of this class goes out of scope
// (even if an exception is thrown)
class CAutoSearchHandle
{
public:
    CAutoSearchHandle(HANDLE h): m_h(h) {};
    ~CAutoSearchHandle() { ::FindClose(m_h); }
private:
    HANDLE m_h;
};



//
//  Wrapper class to convert a wstring to/from a temporary WCHAR
//  buffer to be used as an in/out parameter in Win32 APIs
//
class WString2Buffer
{
public:

    WString2Buffer(wstring & s): 
        m_s(s), m_sv(s.length() + 1, L'\0')
    {
        // Move data from wstring to the temporary vector
        std::copy(m_s.begin(), m_s.end(), m_sv.begin());
    }

    ~WString2Buffer()
    {
        // Move data from the temporary vector to the string
        m_sv.resize(wcslen(&m_sv[0]));
        m_s.assign(m_sv.begin(), m_sv.end());
    }

    // Return the temporary WCHAR buffer 
    operator WCHAR* () { return &(m_sv[0]); }

    // Return the available size of the temporary WCHAR buffer 
    size_t length() { return m_s.length(); }

private: 
    wstring &       m_s;
    vector<WCHAR>   m_sv;
};





/////////////////////////////////////////////////////////////////////////
//  String-related utility functions
//


// Converts a wstring to a string class
inline string WString2String(wstring src)
{
    vector<CHAR> chBuffer;
    int iChars = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
    if (iChars > 0)
    {
        chBuffer.resize(iChars);
        WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, &chBuffer.front(), (int)chBuffer.size(), NULL, NULL);
    }
    
    return std::string(&chBuffer.front());
}


// Converts a wstring into a GUID
inline GUID & WString2Guid(wstring src)
{
    FunctionTracer ft(DBG_INFO);

    // Check if this is a GUID
    static GUID result;
    _bstr_t comstring(src.c_str());
    HRESULT hr = ::CLSIDFromString(comstring.GetBSTR(), &result);
    if (FAILED(hr))
    {
        ft.WriteErrorLine(L"ERROR: The string '%s' is not formatted as a GUID!", src.c_str());
        throw(E_INVALIDARG);
    }

    return result;
}


// Splits a string into a list of substrings separated by the given character
inline vector<wstring> SplitWString(wstring str, WCHAR separator)
{
    FunctionTracer ft(DBG_INFO);

    vector<wstring> strings;

    wstring remainder = str;
    while(true)
    {
        size_t position = remainder.find(separator);
        if (position == wstring::npos)
        {
            // Add the last string
            strings.push_back(remainder);
            break;
        }

        wstring token = remainder.substr(0, position);
        ft.Trace(DBG_INFO, L"Extracting token: '%s' from '%s' between 0..%d", 
            token.c_str(), remainder.c_str(), position);

        // Add this substring and continue with the rest
        strings.push_back(token);
        remainder = remainder.substr(position + 1);
    }

    return strings;
}


// Converts a GUID to a wstring
inline wstring Guid2WString(GUID guid)
{
    FunctionTracer ft(DBG_INFO);

    wstring guidString(100, L'\0');
    CHECK_COM(StringCchPrintfW(WString2Buffer(guidString), guidString.length(), WSTR_GUID_FMT, GUID_PRINTF_ARG(guid)));

    return guidString;
}


// Convert the given BSTR (potentially NULL) into a valid wstring
inline wstring BSTR2WString(BSTR bstr)
{
    return (bstr == NULL)? wstring(L""): wstring(bstr);
}



// Case insensitive comparison
inline bool IsEqual(wstring str1, wstring str2)
{
    return (_wcsicmp(str1.c_str(), str2.c_str()) == 0);
}


// Returns TRUE if the string is already present in the string list  
// (performs case insensitive comparison)
inline bool FindStringInList(wstring str, vector<wstring> stringList)
{
    // Check to see if the volume is already added 
    for (size_t i = 0; i < stringList.size( ); ++i)
        if (IsEqual(str, stringList[i]))
            return true;

    return false;
}


// Append a backslash to the current string 
inline wstring AppendBackslash(wstring str)
{
    if (str.length() == 0)
        return wstring(L"\\");
    if (str[str.length() - 1] == L'\\')
        return str;
    return str.append(L"\\");
}

//This method determins if a given volume is a UNC path, returns true if it has a UNC path prefix and false if it has not
inline bool IsUNCPath(_In_ VSS_PWSZ    pwszVolumeName)
{
    // Check UNC path prefix
    if (_wcsnicmp(pwszVolumeName, UNC_PATH_PREFIX1, wcslen(UNC_PATH_PREFIX1)) == 0) 
        return true;
    else if (_wcsnicmp(pwszVolumeName, NONE_UNC_PATH_PREFIX1, wcslen(NONE_UNC_PATH_PREFIX1)) == 0) 
        return false;
    else if (_wcsnicmp(pwszVolumeName, UNC_PATH_PREFIX2, wcslen(UNC_PATH_PREFIX2)) == 0) 
        return true;
    else
        return false;
}


/////////////////////////////////////////////////////////////////////////
//  Volume, File -related utility functions
//


// Returns TRUE if this is a real volume (for eample C:\ or C:)
// - The backslash termination is optional
inline bool IsVolume(wstring volumePath)
{
    FunctionTracer ft(DBG_INFO);

    bool bIsVolume = false;
    ft.Trace(DBG_INFO, L"Checking if %s is a real volume path...", volumePath.c_str());
    _ASSERTE(volumePath.length() > 0);
    
    // If the last character is not '\\', append one
    volumePath = AppendBackslash(volumePath);

    if (!ClusterIsPathOnSharedVolume(volumePath.c_str()))
    {
        // Get the volume name
        wstring volumeName(MAX_PATH, L'\0');
        if (!GetVolumeNameForVolumeMountPoint( volumePath.c_str(), WString2Buffer(volumeName), (DWORD)volumeName.length()))
        {
            ft.Trace(DBG_INFO, L"GetVolumeNameForVolumeMountPoint(%s) failed with %d", volumePath, GetLastError());
        }
        else
        {
            bIsVolume = true;
        }
    }
    else
    {
        // Note: PathFileExists requires linking to additional dependency shlwapi.lib!
        bIsVolume = ::PathFileExists(volumePath.c_str()) == TRUE;
    }

    ft.Trace(DBG_INFO, L"IsVolume returns %s", (bIsVolume) ? L"TRUE" : L"FALSE");
    return bIsVolume;
}



// Get the unique volume name for the given path
inline wstring GetUniqueVolumeNameForPath(wstring path)
{
    FunctionTracer ft(DBG_INFO);

    _ASSERTE(path.length() > 0);

    wstring volumeRootPath(MAX_PATH, L'\0');
    wstring volumeUniqueName(MAX_PATH, L'\0');

    ft.Trace(DBG_INFO, L"- Get volume path name for %s", path.c_str());

    // Add the backslash termination, if needed
    path = AppendBackslash(path);
    if(!IsUNCPath((VSS_PWSZ)path.c_str()))
    {
        if (ClusterIsPathOnSharedVolume(path.c_str()))
        {
            DWORD cchVolumeRootPath = MAX_PATH;
            DWORD cchVolumeUniqueName = MAX_PATH;
        
            DWORD dwRet = ClusterPrepareSharedVolumeForBackup(
                path.c_str(),
                WString2Buffer(volumeRootPath),
                &cchVolumeRootPath,
                WString2Buffer(volumeUniqueName),
                &cchVolumeUniqueName);

            CHECK_WIN32_ERROR(dwRet, "ClusterPrepareSharedVolumeForBackup");

            ft.Trace(DBG_INFO, L"- Path name: %s", volumeRootPath.c_str());
            ft.Trace(DBG_INFO, L"- Unique volume name: %s", volumeUniqueName.c_str());
        }
        else
        {
            // Get the root path of the volume
        
            CHECK_WIN32(GetVolumePathName((LPCWSTR)path.c_str(), WString2Buffer(volumeRootPath), (DWORD)volumeRootPath.length()));
            ft.Trace(DBG_INFO, L"- Path name: %s", volumeRootPath.c_str());

            // Get the unique volume name
            CHECK_WIN32(GetVolumeNameForVolumeMountPoint((LPCWSTR)volumeRootPath.c_str(), WString2Buffer(volumeUniqueName), (DWORD)volumeUniqueName.length()));
            ft.Trace(DBG_INFO, L"- Unique volume name: %s", volumeUniqueName.c_str());
        }
    }
    else
    {
        IVssBackupComponentsPtr lvssObject;
        IVssBackupComponentsEx4Ptr lvssObject4;
        CHECK_COM( CreateVssBackupComponents(&lvssObject) );
        CHECK_COM(lvssObject->QueryInterface<IVssBackupComponentsEx4>(&lvssObject4));
        VSS_PWSZ pwszVolumeUniqueName = NULL;
        VSS_PWSZ pwszVolumeRootPath = NULL;
        CHECK_COM(lvssObject4->GetRootAndLogicalPrefixPaths((VSS_PWSZ)path.c_str(), &pwszVolumeUniqueName, &pwszVolumeRootPath));

        volumeUniqueName = BSTR2WString(pwszVolumeUniqueName);
        volumeRootPath = BSTR2WString(pwszVolumeRootPath);

        ::CoTaskMemFree(pwszVolumeUniqueName);
        pwszVolumeUniqueName = NULL;
        ::CoTaskMemFree(pwszVolumeRootPath);
        pwszVolumeRootPath = NULL;

        ft.Trace(DBG_INFO, L"- Path name: %s", volumeRootPath.c_str());
        ft.Trace(DBG_INFO, L"- Unique volume name: %s", volumeUniqueName.c_str());
    }
    return volumeUniqueName;
}



// Get the Win32 device for the volume name
inline wstring GetDeviceForVolumeName(wstring volumeName)
{
    FunctionTracer ft(DBG_INFO);

    ft.Trace(DBG_INFO, L"- GetDeviceForVolumeName for '%s'", volumeName.c_str());

    // The input parameter is a valid volume name
    _ASSERTE(wcslen(volumeName.c_str()) > 0);

    // Eliminate the last backslash, if present
    if (volumeName[wcslen(volumeName.c_str()) - 1] == L'\\')
        volumeName[wcslen(volumeName.c_str()) - 1] = L'\0';

    // Eliminate the GLOBALROOT prefix if present
    wstring globalRootPrefix = L"\\\\?\\GLOBALROOT";
    if (IsEqual(volumeName.substr(0,globalRootPrefix.size()), globalRootPrefix))
    {
        wstring kernelDevice = volumeName.substr(globalRootPrefix.size()); 
        ft.Trace(DBG_INFO, L"- GLOBALROOT prefix eliminated. Returning kernel device:  '%s' ", kernelDevice.c_str());

        return kernelDevice;
    }

    // If this is a volume name, get the device 
    wstring dosPrefix = L"\\\\?\\";
    wstring volumePrefix = L"\\\\?\\Volume";
    if (IsEqual(volumeName.substr(0,volumePrefix.size()), volumePrefix))
    {
        // Isolate the DOS device for the volume name (in the format Volume{GUID})
        wstring dosDevice = volumeName.substr(dosPrefix.size());
        ft.Trace(DBG_INFO, L"- DOS device for '%s' is '%s'", volumeName.c_str(), dosDevice.c_str() );

        // Get the real device underneath
        wstring kernelDevice(MAX_PATH, L'\0');
        CHECK_WIN32(QueryDosDevice((LPCWSTR)dosDevice.c_str(), WString2Buffer(kernelDevice), (DWORD)kernelDevice.size()));
        ft.Trace(DBG_INFO, L"- Kernel device for '%s' is '%s'", volumeName.c_str(), kernelDevice.c_str() );

        return kernelDevice;
    }

    return volumeName;
}



// Get the displayable root path for the given volume name
inline wstring GetDisplayNameForVolume(wstring volumeName)
{
    FunctionTracer ft(DBG_INFO);

    DWORD dwRequired = 0;
    wstring volumeMountPoints(MAX_PATH, L'\0');
    if (!GetVolumePathNamesForVolumeName((LPCWSTR)volumeName.c_str(), 
            WString2Buffer(volumeMountPoints), 
            (DWORD)volumeMountPoints.length(), 
            &dwRequired))
    {
            // If not enough, retry with a larger size
            volumeMountPoints.resize(dwRequired, L'\0');
            CHECK_WIN32(!GetVolumePathNamesForVolumeName((LPCWSTR)volumeName.c_str(), 
                WString2Buffer(volumeMountPoints), 
                (DWORD)volumeMountPoints.length(), 
                &dwRequired));
    }

    // compute the smallest mount point by enumerating the returned MULTI_SZ
    wstring mountPoint = volumeMountPoints;
    for(LPWSTR pwszString = (LPWSTR)volumeMountPoints.c_str(); pwszString[0]; pwszString += wcslen(pwszString) + 1)
        if (mountPoint.length() > wcslen(pwszString))
            mountPoint = pwszString;

    return mountPoint;
}



// Execute a command
inline DWORD ExecCommand(wstring command, vector<wstring> arguments, bool quoteArguments, bool expandArguments)
{
    FunctionTracer ft(DBG_INFO);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    //
    // Security Remarks - CreateProcess
    // 
    // The first parameter, lpApplicationName, can be NULL, in which case the 
    // executable name must be in the white space-delimited string pointed to 
    // by lpCommandLine. If the executable or path name has a space in it, there 
    // is a risk that a different executable could be run because of the way the 
    // function parses spaces. The following example is dangerous because the 
    // function will attempt to run "Program.exe", if it exists, instead of "MyApp.exe".
    // 
    // CreateProcess(NULL, "C:\\Program Files\\MyApp", ...)
    // 
    // If a malicious user were to create an application called "Program.exe" 
    // on a system, any program that incorrectly calls CreateProcess 
    // using the Program Files directory will run this application 
    // instead of the intended application.
    // 
    // For this reason we blocked parameters in the executed command.
    //

    // Prepend/append the command with double-quotes. This will prevent adding parameters
    command = wstring(L"\"") + command + wstring(L"\"");
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        wstring argument;
        if (expandArguments)
        {
            // Replace any environment variable references in the form %variableName%
            // with the current value of that environment variable.
            DWORD bufferSize = ExpandEnvironmentStrings((LPCWSTR)arguments[i].c_str(), NULL, 0);
            if (bufferSize == 0)
            {
                CHECK_WIN32(FALSE)
            }
            argument.resize(bufferSize - 1, L'\0');
            // Since src and dst in ExpandEnvironmentStrings must be different buffers we
            // use arguments[i] as src and argument as dst.
            CHECK_WIN32(ExpandEnvironmentStrings((LPCWSTR)arguments[i].c_str(), WString2Buffer(argument), bufferSize))
        }
        else
        {
            argument = arguments[i];
        }
        if (quoteArguments)
        {
            // Automatically add double quotes around the argument, in case it contains spaces etc.
            // This will not always work, e.g. command "cmd" with arguments "/c", "dir" and "c:\program files"
            // will fail ('"dir"' is not recognized as an internal or external command..).
            command += wstring(L" \"") + argument + wstring(L"\"");
        }
        else
        {
            // Add arguments directly. If an argument contain space this means it must be
            // already quoted. When such arguments are supplied into this application it
            // means they must be triple quoted """arg""" (because ShadowRun consumes
            // the outer pair of quotes and the inner quotes must be escaped in the shell
            // by doubling them). It will then work with command "cmd" and arguments
            // "/c", "dir" """c:\program files""".
            command += wstring(L" ") + argument;
        }
    }

    ft.WriteInfoLine(L"Executing command...");
    ft.WriteDebugLine(L"- Command-line: %s", command.c_str());
    ft.WriteInfoLine(L"-----------------------------------------------------");

    // Note: By not setting module name but sending command as first string of the
    // command line it can be name of a batch script and it will be executed directly,
    // without having to explicitely specify "cmd" as the module and "/c <command>"
    // as the command line.

    // Start the child process.
    CHECK_WIN32( CreateProcess( NULL, // No module name (use command line).
        (LPWSTR)command.c_str(), // Command line.
        NULL,             // Process handle not inheritable.
        NULL,             // Thread handle not inheritable.
        FALSE,            // Set handle inheritance to FALSE.
        0,                // No creation flags.
        NULL,             // Use parent's environment block.
        NULL,             // Use parent's starting directory.
        &si,              // Pointer to STARTUPINFO structure.
        &pi ))             // Pointer to PROCESS_INFORMATION structure.

    // Close process and thread handles automatically when we wil leave this function
    CAutoHandle autoCleanupHandleProcess(pi.hProcess);
    CAutoHandle autoCleanupHandleThread(pi.hThread);

    // Wait until child process exits.
    CHECK_WIN32( WaitForSingleObject( pi.hProcess, INFINITE ) == WAIT_OBJECT_0);
    ft.WriteInfoLine(L"-----------------------------------------------------");

    // Checking the exit code
    DWORD dwExitCode = 0;
    CHECK_WIN32( GetExitCodeProcess( pi.hProcess, &dwExitCode ) );
    ft.WriteInfoLine(L"Command completed with exit code %d", dwExitCode);
    return dwExitCode;
}

inline wchar_t GetNextAvailableDriveLetter()
{
    FunctionTracer ft(DBG_INFO);
    auto driveMask = GetLogicalDrives();
    if (!driveMask) CHECK_WIN32_ERROR(GetLastError(), L"GetLogicalDrives");
    wchar_t driveLetter = L'A';
    for (size_t i = 0, n = 8 * sizeof(driveMask);
        i < n && driveMask & (1 << i);
        ++i, ++driveLetter);
    if (driveLetter < L'A' || driveLetter > L'Z')
    {
        ft.WriteErrorLine(L"ERROR: Invalid mount drive letter!");
        throw(E_UNEXPECTED);
    }
    return driveLetter;
}

inline wchar_t VerifyAvailableDriveLetter(wchar_t driveLetter)
{
    FunctionTracer ft(DBG_INFO);
    if (driveLetter < L'A' || driveLetter > L'Z')
    {
        ft.WriteErrorLine(L"ERROR: Invalid mount drive letter!");
        throw(E_UNEXPECTED);
    }
    auto driveMask = GetLogicalDrives();
    if (!driveMask)
    {
        CHECK_WIN32_ERROR(GetLastError(), L"GetLogicalDrives");
    }
    if (driveMask & (1 << (driveLetter - L'A')))
    {
        ft.WriteErrorLine(L"ERROR: Mount drive letter '%c' is already in use!");
        throw(E_UNEXPECTED);
    }
    return driveLetter;
}

