/////////////////////////////////////////////////////////////////////////
// Copyright � Microsoft Corporation. All rights reserved.
// 
//  This file may contain preliminary information or inaccuracies, 
//  and may not correctly represent any associated Microsoft 
//  Product as commercially released. All Materials are provided entirely 
//  �AS IS.� To the extent permitted by law, MICROSOFT MAKES NO 
//  WARRANTY OF ANY KIND, DISCLAIMS ALL EXPRESS, IMPLIED AND STATUTORY 
//  WARRANTIES, AND ASSUMES NO LIABILITY TO YOU FOR ANY DAMAGES OF 
//  ANY TYPE IN CONNECTION WITH THESE MATERIALS OR ANY INTELLECTUAL PROPERTY IN THEM. 
// 



// Main header
#include "stdafx.h"




/////////////////////////////////////////////////////////////////////////
//  Tracing class (very simple implementation) 
//


bool    FunctionTracer::m_traceEnabled = false;


// Console logging routine
// Can throw HRESULT on invalid formats
void FunctionTracer::WriteLine(wstring format, ...)
{
    wstring buffer;
    VPRINTF_VAR_PARAMS(buffer, format);

    wprintf(L"%s\n", buffer.c_str());
    Trace(m_fileName, m_lineNumber, m_functionName, L"OUTPUT: %s", buffer.c_str());
}


// Tracing routine
// Can throw HRESULT on invalid formats
void FunctionTracer::Trace(wstring file, int line, wstring functionName, wstring format, ...)
{
    if (m_traceEnabled)
    {
        wstring buffer;
        VPRINTF_VAR_PARAMS(buffer, format);
    
        size_t pos = file.find_last_of(L"\\");
        wstring fileName = (pos == wstring::npos)? file: file.substr(pos+1);

        // TODO - put here your own implementation of a tracing routine, if needed
        wprintf(L"[[%40s @ %10s:%4d]] %s\n", functionName.c_str(), fileName.c_str(), line, buffer.c_str());
    }
}


// Constructor
FunctionTracer::FunctionTracer(wstring fileName, INT lineNumber, wstring functionName):
    m_fileName(fileName), m_lineNumber(lineNumber), m_functionName(functionName)
{
    if (m_traceEnabled)
        Trace(m_fileName, m_lineNumber, m_functionName, L"ENTER %s", m_functionName.c_str());
}


// Destructor
FunctionTracer::~FunctionTracer()
{
    if (m_traceEnabled)
        Trace(m_fileName, m_lineNumber, m_functionName, L"EXIT %s", m_functionName.c_str());;
}


// Enable tracing mode
void FunctionTracer::EnableTracingMode()
{
    m_traceEnabled = true;
}


// Convert a failure type into a string
wstring FunctionTracer::HResult2String(HRESULT hrError)
{
    switch (hrError)
    {

    CHECK_CASE_FOR_CONSTANT(VSS_E_BAD_STATE	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_UNEXPECTED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_PROVIDER_ALREADY_REGISTERED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_PROVIDER_NOT_REGISTERED);
    CHECK_CASE_FOR_CONSTANT(VSS_E_PROVIDER_VETO	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_PROVIDER_IN_USE);
    CHECK_CASE_FOR_CONSTANT(VSS_E_OBJECT_NOT_FOUND);
    CHECK_CASE_FOR_CONSTANT(VSS_S_ASYNC_PENDING	);
    CHECK_CASE_FOR_CONSTANT(VSS_S_ASYNC_FINISHED);
    CHECK_CASE_FOR_CONSTANT(VSS_S_ASYNC_CANCELLED);
    CHECK_CASE_FOR_CONSTANT(VSS_E_VOLUME_NOT_SUPPORTED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_OBJECT_ALREADY_EXISTS	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_UNEXPECTED_PROVIDER_ERROR	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_INVALID_XML_DOCUMENT	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_FLUSH_WRITES_TIMEOUT	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_HOLD_WRITES_TIMEOUT	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_UNEXPECTED_WRITER_ERROR	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_SNAPSHOT_SET_IN_PROGRESS	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITER_INFRASTRUCTURE	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITER_NOT_RESPONDING	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITER_ALREADY_SUBSCRIBED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_UNSUPPORTED_CONTEXT	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_VOLUME_IN_USE	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_INSUFFICIENT_STORAGE	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_NO_SNAPSHOTS_IMPORTED	);
    CHECK_CASE_FOR_CONSTANT(VSS_S_SOME_SNAPSHOTS_NOT_IMPORTED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_MAXIMUM_NUMBER_OF_REMOTE_MACHINES_REACHED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_REMOTE_SERVER_UNAVAILABLE	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_REMOTE_SERVER_UNSUPPORTED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_REVERT_IN_PROGRESS	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_REVERT_VOLUME_LOST	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_REBOOT_REQUIRED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_TRANSACTION_FREEZE_TIMEOUT	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_TRANSACTION_THAW_TIMEOUT	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITERERROR_OUTOFRESOURCES);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITERERROR_TIMEOUT	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITERERROR_RETRYABLE);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITERERROR_NONRETRYABLE);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITERERROR_RECOVERY_FAILED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_BREAK_REVERT_ID_FAILED);
    CHECK_CASE_FOR_CONSTANT(VSS_E_LEGACY_PROVIDER);
    CHECK_CASE_FOR_CONSTANT(VSS_E_MISSING_DISK	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_MISSING_HIDDEN_VOLUME	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_MISSING_VOLUME	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_AUTORECOVERY_FAILED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_DYNAMIC_DISK_ERROR	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_NONTRANSPORTABLE_BCD	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_CANNOT_REVERT_DISKID	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_RESYNC_IN_PROGRESS	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_CLUSTER_ERROR         );
    CHECK_CASE_FOR_CONSTANT(VSS_E_ASRERROR_DISK_ASSIGNMENT_FAILED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_ASRERROR_DISK_RECREATION_FAILED	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_ASRERROR_NO_ARCPATH	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_ASRERROR_MISSING_DYNDISK	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_ASRERROR_SHARED_CRIDISK	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_ASRERROR_DATADISK_RDISK0	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_ASRERROR_RDISK0_TOOSMALL	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_ASRERROR_CRITICAL_DISKS_TOO_SMALL	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_WRITER_STATUS_NOT_AVAILABLE	);
    CHECK_CASE_FOR_CONSTANT(VSS_E_UNSELECTED_VOLUME             );
    CHECK_CASE_FOR_CONSTANT(VSS_E_SNAPSHOT_NOT_IN_SET           );
    CHECK_CASE_FOR_CONSTANT(VSS_E_NESTED_VOLUME_LIMIT           );


    // Regular COM errors
    CHECK_CASE_FOR_CONSTANT(S_OK);
    CHECK_CASE_FOR_CONSTANT(S_FALSE);
    CHECK_CASE_FOR_CONSTANT(E_UNEXPECTED);
    CHECK_CASE_FOR_CONSTANT(E_OUTOFMEMORY);
    
    default:
        break;
    }

    PWCHAR pwszBuffer = NULL;
    DWORD dwRet = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER 
        | FORMAT_MESSAGE_FROM_SYSTEM 
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, hrError, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPWSTR)&pwszBuffer, 0, NULL);

    // No message found for this error. Just return <Unknown>
    if (dwRet == 0)
        return wstring(L"<Unknown error code>");

    // Convert the message into wstring
    wstring errorText(pwszBuffer);
    LocalFree(pwszBuffer);
    
    return errorText;
}

