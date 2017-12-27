#ifndef BASE_WIN_WINDOWS_FULL_H
#define BASE_WIN_WINDOWS_FULL_H

#include <windows.h>
// windows.h #defines this (only on x64). This causes problems because the
// public atomicops API also uses MemoryBarrier at the public name for this
// fence. So, on X64, undef it, and call its documented
// (http://msdn.microsoft.com/en-us/library/windows/desktop/ms684208.aspx)
// implementation directly.
#undef MemoryBarrier

// Undefine all the other defines which end up used as function names
// in Chromium.
#undef CopyFile
#undef CreateDirectory
#undef CreateService
#undef DeleteFile
#undef GetCurrentDirectory
#undef PostMessage

// Make sure our typedefs are compatible with Microsoft's.
#include "base/win/windows_types.h"

#endif  // BASE_WIN_WINDOWS_FULL_H
