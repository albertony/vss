// Main header
#include "stdafx.h"



// Constructor
VssClient::VssClient()
{
    m_bCoInitializeCalled = false;
}


// Destructor
VssClient::~VssClient()
{

    // Try to unmount any still mounted snapshots
    UnmountSnapshotsSilent();

    // Release the IVssBackupComponents interface 
    // WARNING: this must be done BEFORE calling CoUninitialize()
    m_pVssObject = NULL;
    
    // Call CoUninitialize if the CoInitialize was performed sucesfully
    if (m_bCoInitializeCalled)
        CoUninitialize();
}


// Initialize the COM infrastructure and the internal pointers
void VssClient::Initialize()
{
    FunctionTracer ft(DBG_INFO);

    // Initialize COM 
    CHECK_COM(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
    m_bCoInitializeCalled = true;

    // Initialize COM security
    CHECK_COM( 
        CoInitializeSecurity(
            NULL,                           //  Allow *all* VSS writers to communicate back!
            -1,                             //  Default COM authentication service
            NULL,                           //  Default COM authorization service
            NULL,                           //  reserved parameter
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  //  Strongest COM authentication level
            RPC_C_IMP_LEVEL_IMPERSONATE,    //  Minimal impersonation abilities 
            NULL,                           //  Default COM authentication settings
            EOAC_DYNAMIC_CLOAKING,          //  Cloaking
            NULL                            //  Reserved parameter
            ) );

    // Create the internal backup components object
    CHECK_COM(CreateVssBackupComponents(&m_pVssObject));
    
    // Initialize for backup
    CHECK_COM(m_pVssObject->InitializeForBackup())

    // Set the context, different than the default context
    DWORD dwContext = VSS_CTX_FILE_SHARE_BACKUP; // Specifies an auto-release, nonpersistent shadow copy created without writer involvement.
    ft.WriteLine(L"- Setting the VSS context to: 0x%08lx", dwContext);
    CHECK_COM(m_pVssObject->SetContext(dwContext));

    // Set various properties per backup components instance
    CHECK_COM(m_pVssObject->SetBackupState(true, true, VSS_BT_FULL, false));
}



// Waits for the completion of the asynchronous operation
void VssClient::WaitAndCheckForAsyncOperation(IVssAsync* pAsync)
{
    FunctionTracer ft(DBG_INFO);

    ft.WriteLine(L"(Waiting for the asynchronous operation to finish...)");

    // Wait until the async operation finishes
    CHECK_COM(pAsync->Wait());

    // Check the result of the asynchronous operation
    HRESULT hrReturned = S_OK;
    CHECK_COM(pAsync->QueryStatus(&hrReturned, NULL));

    // Check if the async operation succeeded...
    if(FAILED(hrReturned))
    {
        ft.WriteLine(L"Error during the last asynchronous operation.");
        ft.WriteLine(L"- Returned HRESULT = 0x%08lx", hrReturned);
        ft.WriteLine(L"- Error text: %s", FunctionTracer::HResult2String(hrReturned).c_str());
        ft.WriteLine(L"- Please re-run SHADOWRUN.EXE with the /tracing option to get more details");
        throw(hrReturned);
    }
}
