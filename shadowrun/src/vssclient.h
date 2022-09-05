#pragma once


/////////////////////////////////////////////////////////////////////////
//  Regular VSS client class 
//
//  This class implements a high-level VSS API. It is not dependent on 
//  ShadowRun.exe command-line interface - It can be called from an UI program as well.
//

struct SnapshotInfo
{
    VSS_ID id = GUID_NULL;
    wstring idString;
    wstring deviceName;
    wstring mount;
};
struct SnapshotSetInfo
{
    VSS_ID id = GUID_NULL;
    wstring idString;
    vector<SnapshotInfo> snapshots;
};

class VssClient
{
public:

    // Constructor
    VssClient();

    // Destructor
    ~VssClient();

    // Initialize the internal pointers
    void Initialize();

    //
    //  Shadow copy creation related methods
    //

    // Method to create a shadow copy set with the given volumes
    void CreateSnapshotSet(vector<wstring> volumeList);

    // Add volumes to the shadow copy set
    void AddToSnapshotSet(vector<wstring> volumeList);
    
    // Effectively creating the shadow copy (calling DoSnapshotSet)
    void DoSnapshotSet();

    // Finalizing snapshot set information
    void SnapshotSetCreated();

    // Generate the SETVAR script for this shadow copy set
    void GenerateEnvironmentScript(wstring stringFileName);

    // Set environment variables for this shadow copy set
    void SetProcessEnvironment();

    // Mount volume snapshots as drives
    wstring MountSnapshots(wstring driveLetters);

    // Unmount any mounted volume snapshots
    void UnmountSnapshots();

private:

    void SetProcessEnvironmentVariable(LPCWSTR name, LPCWSTR value);

    // Waits for the async operation to finish
    void WaitAndCheckForAsyncOperation(IVssAsync*  pAsync);

    void UnmountSnapshotsSilent();

private:
    
    //
    //  Data members
    //

    // TRUE if CoInitialize() was already called 
    // Needed to pair each succesfull call to CoInitialize with a corresponding CoUninitialize
    bool                            m_bCoInitializeCalled;

    // The IVssBackupComponents interface is automatically released when this object is destructed.
    // Needed to issue VSS calls 
    IVssBackupComponentsPtr         m_pVssObject;

    // Latest shadow copy set
    SnapshotSetInfo                 m_latestSnapshotSet;

};

