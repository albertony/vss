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
    
    // tracing routine
    void Trace(wstring file, int line, wstring functionName, wstring format, ...);
    
    // console logging routine
    void WriteLine(wstring format, ...);
    
    // Converts a HRESULT into a printable message
    static wstring  HResult2String(HRESULT hrError);

    // Enables tracing
    static void EnableTracingMode();

private:

    //
    //  Data members
    //

    static bool m_traceEnabled;

    wstring     m_fileName;
    int         m_lineNumber;
    wstring     m_functionName;

};

