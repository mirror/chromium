#ifndef BASE_WIN_WINDOWS_ATL_H
#define BASE_WIN_WINDOWS_ATL_H

#include "base/win/windows_full.h"

// Make sure this is defined - atlwin.h needs it.
#define PostMessage PostMessageW
#include <atlbase.h>
#include <atlcrack.h>
#include <atlwin.h>

// Cause the problematic macros to be undefined.
#include "base/win/windows_full.h"

#endif  // BASE_WIN_WINDOWS_ATL_H
