#pragma once


/////////////////////////////////////////////////////////////////////////
//  Regular VSS client class 
//
//  This class implements a high-level VSS API. It is not dependent on 
//  SHADOWRUN.EXE command-line interface - It can be called from an UI program as well.
//


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

    // Set environment variables for this shadow copy set
    void Setvar();

    // Generate the SETVAR script for this shadow copy set
    void GenerateSetvarScript(wstring stringFileName);


private:

    // Waits for the async operation to finish
    void WaitAndCheckForAsyncOperation(IVssAsync*  pAsync);



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

    // List of shadow copy IDs from the latest shadow copy creation process
    vector<VSS_ID>                  m_latestSnapshotIdList;

    // Latest shadow copy set ID
    VSS_ID                          m_latestSnapshotSetID;

};

