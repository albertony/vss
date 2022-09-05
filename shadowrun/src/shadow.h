#pragma once


// VSS includes
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <vsmgmt.h>

// VDS includes
#include <vds.h>

// Our includes
#include "tracing.h"
#include "util.h"
#include "vssclient.h"


// Shadow Copy Provider GUID {b5946137-7b9f-4925-af80-51abd60b20d5}
const GUID VSS_SWPRV_ProviderId = { 0xb5946137, 0x7b9f, 0x4925, { 0xaf, 0x80, 0x51, 0xab, 0xd6, 0xb, 0x20, 0xd5 } };


/////////////////////////////////////////////////////////////////////////
//  The main command line parser
//

class CommandLineParser
{
public:

    CommandLineParser();

    ~CommandLineParser();

    int Run(vector<wstring>& arguments);

private:

    int MainRoutine(vector<wstring>& arguments);

    // Returns TRUE if the argument is in the following formats
    //  -xxxx
    //  /xxxx
    // where xxxx is the option pattern
    bool MatchArgument(const wstring& arg, const wstring& optionPattern);

    // Returns TRUE if the argument is in the following formats
    //  -xxxx=yyyy
    //  /xxxx=yyyy
    // where xxxx is the option pattern and yyyy the additional parameter (optionally enclosed double quotes)
    bool MatchArgument(const wstring& argument, const wstring& optionPattern, wstring& additionalParameter, bool dequote, bool lower);


    // The VSS client
    VssClient   m_vssClient;
};






