// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifdef WIN32

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

// Entry point for V8 Windows DLL

extern "C" BOOL WINAPI DllMain(HINSTANCE hinst,
                               DWORD reason,
                               LPVOID reserverd) {
  return TRUE;
}

#endif
