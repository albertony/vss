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

#include <shlwapi.h>
//#define _COM_SMARTPTR
#include <comdef.h>

// STL includes
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

// Used for safe string manipulation
#include <strsafe.h>

#include "shadow.h"
