// Main header
#include "stdafx.h"

_COM_SMARTPTR_TYPEDEF(IVssAsync, __uuidof(IVssAsync)); // typedef _com_ptr_t<...> IVssAsyncPtr;


void VssClient::CreateSnapshotSet(vector<wstring> volumeList)
{
    FunctionTracer ft(DBG_INFO);

    // Start the shadow set
    CHECK_COM(m_pVssObject->StartSnapshotSet(&m_latestSnapshotSet.id))
    m_latestSnapshotSet.idString = Guid2WString(m_latestSnapshotSet.id);
    ft.WriteLine(L"Creating shadow set %s", m_latestSnapshotSet.idString.c_str());

    // Add the specified volumes to the shadow set
    AddToSnapshotSet(volumeList);

    // Creates the shadow set 
    DoSnapshotSet();

    // Finalize
    SnapshotSetCreated();

    ft.WriteLine(L"Shadow set succesfully created.");
}

// Add volumes to the shadow set
void VssClient::AddToSnapshotSet(vector<wstring> volumeList)
{
    FunctionTracer ft(DBG_INFO);

    _ASSERTE(m_latestSnapshotSet.snapshots.size() == 0);

    // Add volumes to the shadow set 
    for (size_t i = 0; i < volumeList.size(); ++i)
    {
        wstring volume = volumeList[i];
        ft.WriteLine(L"- Adding volume %s [%s] to the shadow set...",
            volume.c_str(),
            GetDisplayNameForVolume(volume).c_str());

        VSS_ID snapshotId;
        CHECK_COM(m_pVssObject->AddToSnapshotSet((LPWSTR)volume.c_str(), GUID_NULL, &snapshotId));

        wstring snapshotIdString = Guid2WString(snapshotId);

        // Preserve this shadow ID for environment later
        m_latestSnapshotSet.snapshots.push_back(SnapshotInfo{ snapshotId, snapshotIdString });
    }
}

// Effectively creating the shadow (calling DoSnapshotSet)
void VssClient::DoSnapshotSet()
{
    FunctionTracer ft(DBG_INFO);

    ft.WriteLine(L"Creating the shadow (DoSnapshotSet) ... ");

    IVssAsyncPtr pAsync;
    CHECK_COM(m_pVssObject->DoSnapshotSet(&pAsync));

    // Waits for the async operation to finish and checks the result
    WaitAndCheckForAsyncOperation(pAsync);
}

void VssClient::SnapshotSetCreated()
{
    FunctionTracer ft(DBG_INFO);
    for (size_t i = 0; i < m_latestSnapshotSet.snapshots.size(); ++i)
    {
        // Get shadow copy device (if the snapshot is there)
        VSS_SNAPSHOT_PROP Snap;
        CHECK_COM(m_pVssObject->GetSnapshotProperties(m_latestSnapshotSet.snapshots[i].id, &Snap));
        // Automatically call VssFreeSnapshotProperties on this structure at the end of scope
        CAutoSnapPointer snapAutoCleanup(&Snap);
        m_latestSnapshotSet.snapshots[i].deviceName = Snap.m_pwszSnapshotDeviceObject;
    }
}

wstring VssClient::MountSnapshots(wstring driveLetters)
{
    FunctionTracer ft(DBG_INFO);
    ft.WriteLine(L"Mounting shadow copies ...");
    for (size_t i = 0; i < m_latestSnapshotSet.snapshots.size(); ++i)
    {
        // Find the drive letter to use
        wchar_t driveLetter;
        if (i < driveLetters.size())
        {
            driveLetter = VerifyAvailableDriveLetter(towupper(driveLetters[i])); // Throws if not available, we don't want to continue when something else is on the drive letter that the exec will assume is the snapshot!
        }
        else
        {
            driveLetter = GetNextAvailableDriveLetter();
        }

        // Create DOS Device
        wstring drive;
        drive += driveLetter;
        drive += L":"; // DefineDosDevice does not work with trailing backslash
        CHECK_WIN32(DefineDosDevice(NULL, drive.c_str(), m_latestSnapshotSet.snapshots[i].deviceName.c_str()));

        // Verify that the DOS device works (DefineDosDevice seems to return success regardless of target being invalid)
        auto hDrive = CreateFile(drive.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        CAutoHandle hDriveAutoCleanup(hDrive);
        if (INVALID_HANDLE_VALUE == hDrive) {
            ft.WriteLine(L"ERROR: Mount %s of %s is not accessible!", drive.c_str(), m_latestSnapshotSet.snapshots[i].deviceName.c_str());
            throw(E_UNEXPECTED);
        }

        // Remember the mount drive to be able to unmount when done
        m_latestSnapshotSet.snapshots[i].mount = drive;
        ft.WriteLine(L"- Mounted %s to %s", drive.c_str(), m_latestSnapshotSet.snapshots[i].deviceName.c_str());

    }

    return driveLetters;
}

void VssClient::UnmountSnapshots()
{
    FunctionTracer ft(DBG_INFO);

    ft.WriteLine(L"Unounting shadow copies ... ");

    // For each added volume add the ShadowRun.exe exposure command
    for (size_t i = 0; i < m_latestSnapshotSet.snapshots.size(); ++i)
    {
        if (!m_latestSnapshotSet.snapshots[i].mount.empty())
        {
            if (DefineDosDevice(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, m_latestSnapshotSet.snapshots[i].mount.c_str(), m_latestSnapshotSet.snapshots[i].deviceName.c_str()))
            {
                ft.WriteLine(L"- Unmounted %s from %s", m_latestSnapshotSet.snapshots[i].mount.c_str(), m_latestSnapshotSet.snapshots[i].deviceName.c_str());
                m_latestSnapshotSet.snapshots[i].mount.clear(); // Set empty so we know it no longer exists
            }
            else
            {
                // Warn, but do not abort - continue unmounting what we can!
                ft.WriteLine(L"- Unable to unmount %s from %s", m_latestSnapshotSet.snapshots[i].mount.c_str(), m_latestSnapshotSet.snapshots[i].deviceName.c_str());
            }
        }
    }
}

void VssClient::UnmountSnapshotsSilent()
{
    try
    {
        for (size_t i = 0; i < m_latestSnapshotSet.snapshots.size(); ++i)
        {
            try
            {
                if (!m_latestSnapshotSet.snapshots[i].mount.empty())
                {
                    if (DefineDosDevice(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, m_latestSnapshotSet.snapshots[i].mount.c_str(), m_latestSnapshotSet.snapshots[i].deviceName.c_str()))
                    {
                        m_latestSnapshotSet.snapshots[i].mount.clear(); // Set empty so we know it no longer exists
                    }
                }
            }
            catch (...) {}
        }
    }
    catch(...) {}
}

// Generate the SETVAR script
void VssClient::GenerateEnvironmentScript(wstring stringFileName)
{
    FunctionTracer ft(DBG_INFO);
    ft.WriteLine(L"Generating the SETVAR script (%s) ... ", stringFileName.c_str());
    wofstream ofile;
    ofile.open(WString2String(stringFileName).c_str());
    ofile << L"@echo.\n";
    ofile << L"@echo [This script is generated by ShadowRun.exe for the shadow set " << m_latestSnapshotSet.idString.c_str() << L"]\n";
    ofile << L"@echo.\n\n";
    ofile << L"SET SHADOW_SET_ID=" << m_latestSnapshotSet.idString.c_str() << L"\n";
    ofile << L"SET SHADOW_SET_COUNT=" << m_latestSnapshotSet.snapshots.size() << L"\n";
    for (size_t i = 0; i < m_latestSnapshotSet.snapshots.size(); ++i)
    {
        ofile << L"SET SHADOW_ID_" << i+1 << L"=" << m_latestSnapshotSet.snapshots[i].idString.c_str() << L"\n";
        ofile << L"SET SHADOW_DEVICE_" << i+1 << L"=" << m_latestSnapshotSet.snapshots[i].deviceName.c_str() << L"\n";
        ofile << L"SET SHADOW_DRIVE_" << i + 1 << L"=" << m_latestSnapshotSet.snapshots[i].mount.c_str() << L"\n"; // Possibly empty, but set it anyway just to be sure!
    }
    ofile.close();
}

// Update environment variables of the current process, which will be inherited by created child process.
void VssClient::SetProcessEnvironment()
{
    FunctionTracer ft(DBG_INFO);
    ft.WriteLine(L"Setting process environment variables ...");
    CHECK_WIN32(SetEnvironmentVariable((LPCWSTR)L"SHADOW_SET_ID", m_latestSnapshotSet.idString.c_str()));
    wostringstream stringBuilder;
    stringBuilder << m_latestSnapshotSet.snapshots.size();
    CHECK_WIN32(SetEnvironmentVariable((LPCWSTR)L"SHADOW_SET_COUNT", stringBuilder.str().c_str()));
    stringBuilder.str(std::wstring{});
    for (size_t i = 0; i < m_latestSnapshotSet.snapshots.size(); ++i)
    {
        stringBuilder << L"SHADOW_ID_" << i+1;
        CHECK_WIN32(SetEnvironmentVariable(stringBuilder.str().c_str(), m_latestSnapshotSet.snapshots[i].idString.c_str()));
        stringBuilder.str(std::wstring{});
        stringBuilder << L"SHADOW_DEVICE_" << i+1;
        CHECK_WIN32(SetEnvironmentVariable(stringBuilder.str().c_str(), m_latestSnapshotSet.snapshots[i].deviceName.c_str()));
        stringBuilder.str(std::wstring{});
        stringBuilder << L"SHADOW_DRIVE_" << i+1;
        if (m_latestSnapshotSet.snapshots[i].mount.empty())
        {
            CHECK_WIN32(SetEnvironmentVariable(stringBuilder.str().c_str(), NULL)); // Remove any existing
        }
        else
        {
            CHECK_WIN32(SetEnvironmentVariable(stringBuilder.str().c_str(), m_latestSnapshotSet.snapshots[i].mount.c_str()));
        }
        stringBuilder.str(std::wstring{});
    }
}