// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains defines and typedefs that allow popular Windows types to
// be used without the overhead of including windows.h.

#ifndef BASE_WIN_WINDOWS_TYPES_H
#define BASE_WIN_WINDOWS_TYPES_H

// Needed for function prototypes.
#include <concurrencysal.h>
#include <sal.h>
#include <specstrings.h>

#ifdef __cplusplus
extern "C" {
#endif

// typedef and define the most commonly used Windows integer types.

typedef unsigned long DWORD;
typedef long LONG;
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;

#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int* PUINT;
typedef void* LPVOID;
typedef void* PVOID;
typedef void* HANDLE;
typedef int BOOL;
typedef unsigned char BYTE;
typedef BYTE BOOLEAN;
typedef DWORD ULONG;
typedef unsigned short WORD;
typedef WORD UWORD;

#if defined(_WIN64)
typedef __int64 INT_PTR, *PINT_PTR;
typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

typedef __int64 LONG_PTR, *PLONG_PTR;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;
#else
typedef _W64 int INT_PTR, *PINT_PTR;
typedef _W64 unsigned int UINT_PTR, *PUINT_PTR;

typedef _W64 long LONG_PTR, *PLONG_PTR;
typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;
#endif

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
#define LRESULT LONG_PTR

typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;

typedef DWORD ACCESS_MASK;
typedef ACCESS_MASK REGSAM;

// Forward declare Windows compatible handles.

#define CHROME_DECLARE_HANDLE(name) \
  struct name##__;                  \
  typedef struct name##__* name
CHROME_DECLARE_HANDLE(HKEY);
CHROME_DECLARE_HANDLE(HWND);
CHROME_DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
#undef CHROME_DECLARE_HANDLE

// Forward declare some Windows struct/typedef sets.

typedef struct _OVERLAPPED OVERLAPPED;
typedef struct tagMSG MSG, *PMSG, *NPMSG, *LPMSG;

typedef struct _RTL_SRWLOCK RTL_SRWLOCK;
typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;

typedef struct _GUID GUID;
typedef GUID CLSID;

typedef struct tagLOGFONTW LOGFONTW, *PLOGFONTW, *NPLOGFONTW, *LPLOGFONTW;
typedef LOGFONTW LOGFONT;

typedef struct _FILETIME FILETIME;

// Declare Chrome versions of some Windows structures. These are needed for
// when we need a concrete type but don't want to pull in Windows.h. We can't
// declare the Windows types so we declare our types and cast to the Windows
// types in a few places.

struct CHROME_SRWLOCK {
  PVOID Ptr;
};

struct CHROME_CONDITION_VARIABLE {
  PVOID Ptr;
};

// Define some commonly used Windows constants. Note that the layout of these
// macros - including internal spacing - must be 100% consistent with windows.h.

#define HKEY_CURRENT_USER (( HKEY ) (ULONG_PTR)((LONG)0x80000001) )
#define HKEY_LOCAL_MACHINE (( HKEY ) (ULONG_PTR)((LONG)0x80000002) )
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#define HTNOWHERE 0
#define MAX_PATH 260
#define ERROR_SUCCESS 0L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_ACCESS_DENIED 5L
#define ERROR_INVALID_HANDLE 6L
#define ERROR_SHARING_VIOLATION 32L
#define ERROR_LOCK_VIOLATION 33L

// Define some macros needed when prototyping Windows functions.

#define DECLSPEC_IMPORT __declspec(dllimport)
#define WINBASEAPI DECLSPEC_IMPORT
#define WINUSERAPI DECLSPEC_IMPORT
#define WINAPI __stdcall
#define CALLBACK __stdcall

// Needed for optimal lock performance.
WINBASEAPI _Releases_exclusive_lock_(*SRWLock) VOID WINAPI
    ReleaseSRWLockExclusive(_Inout_ PSRWLOCK SRWLock);

// Needed to support protobuf's GetMessage macro magic.
WINUSERAPI BOOL WINAPI GetMessageW(_Out_ LPMSG lpMsg,
                                   _In_opt_ HWND hWnd,
                                   _In_ UINT wMsgFilterMin,
                                   _In_ UINT wMsgFilterMax);

// Needed for thread_local_storage.h
WINBASEAPI LPVOID WINAPI TlsGetValue(_In_ DWORD dwTlsIndex);

// Needed for scoped_handle.h
WINBASEAPI _Check_return_ _Post_equals_last_error_ DWORD WINAPI
    GetLastError(VOID);

WINBASEAPI VOID WINAPI SetLastError(_In_ DWORD dwErrCode);

#ifdef __cplusplus
}
#endif

// These macros are all defined by windows.h and are also used as the names of
// functions in the Chromium code base. Add to this list as needed whenever
// there is a Windows macro which causes a function call to be renamed. This
// ensures that the same renaming will happen everywhere. Includes of this file
// can be added wherever needed to ensure this consistent renaming.

#define CopyFile CopyFileW
#define CreateDirectory CreateDirectoryW
#define CreateEvent CreateEventW
#define CreateFile CreateFileW
#define CreateService CreateServiceW
#define DeleteFile DeleteFileW
#define DispatchMessage DispatchMessageW
#define DrawText DrawTextW
#define GetComputerName GetComputerNameW
#define GetCurrentDirectory GetCurrentDirectoryW
#define GetCurrentTime() GetTickCount()
#define GetFileAttributes GetFileAttributesW
#define GetMessage GetMessageW
#define GetUserName GetUserNameW
#define LoadIcon LoadIconW
#define LoadImage LoadImageW
#define PostMessage PostMessageW
#define ReplaceFile ReplaceFileW
#define ReportEvent ReportEventW
#define SendMessage SendMessageW
#define SendMessageCallback SendMessageCallbackW
#define SetCurrentDirectory SetCurrentDirectoryW
#define StartService StartServiceW

#endif  // BASE_WIN_WINDOWS_TYPES_H
