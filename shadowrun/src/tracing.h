#pragma once


/////////////////////////////////////////////////////////////////////////
//  Generic tracing/logger class
//


// Very simple tracing/logging class 
class FunctionTracer
{
public:
    FunctionTracer(wstring fileName, INT lineNumber, wstring functionName);
    ~FunctionTracer();

    enum LogLevel
    {
        LOGLEVEL_SILENT,
        LOGLEVEL_ERROR,
        LOGLEVEL_NOTICE, // Warnings and significant events
        LOGLEVEL_INFO,
        LOGLEVEL_DEBUG,
        LOGLEVEL_TRACE,
    };

    // Tracing routine
    void Trace(wstring file, int line, wstring functionName, wstring format, ...);

    // Console logging routine for debug messages
    void WriteDebugLine(wstring format, ...);

    // Console logging routine for info messages
    void WriteInfoLine(wstring format, ...);

    // Console logging routine for error messages
    void WriteErrorLine(wstring format, ...);

    // Console logging routine
    void WriteLine(wstring format, ...);

    // Console logging routine
    void Write(wstring format, ...);

    // Converts a HRESULT into a printable message
    static wstring  HResult2String(HRESULT hrError);

    // Set log level
    static void SetLogLevel(int level);

private:

    //
    //  Data members
    //

    static int  m_logLevel;

    wstring     m_fileName;
    int         m_lineNumber;
    wstring     m_functionName;

};

