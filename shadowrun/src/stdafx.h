// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


// General includes
#define STRICT // Enable STRICT Type Checking in Windows headers
#define WIN32_LEAN_AND_MEAN // To speed the build process exclude rarely-used services from Windows headers
#define NOMINMAX // Exclude min/max macros from Windows header
#include <windows.h>

// _ASSERTE declaration (used by ATL) and otehr macros
#include "macros.h"

#include <comdef.h>
#include <shlwapi.h>


// STL includes
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

// Used for safe string manipulation
// Needed for a single call, to StringCchPrintfW, in implementation of Guid2WString(GUID guid).
#include <strsafe.h>

// Base header for this application
#include "shadow.h"
