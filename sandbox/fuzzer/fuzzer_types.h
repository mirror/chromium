// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_FUZZER_TYPES_H_
#define SANDBOX_FUZZER_TYPES_H_

// Kill exceptions.
#define __try if(true)
#define __except(...) if(false)

// Windows types.
typedef int64_t __int64;
typedef int32_t __int32;
#ifdef __cplusplus
typedef bool BOOL;
#else
typedef uint32_t BOOL;
#endif  // __cplusplus
typedef void* HMODULE;
typedef void* LPVOID;
typedef void* PVOID;
typedef PVOID HANDLE;
typedef HANDLE HWND;
typedef HANDLE HINSTANCE;
typedef void* HDC;
typedef void* HBITMAP;
typedef void* HICON;
typedef void* HFONT;
typedef void* HGDIOBJ;
typedef void* HBRUSH;
typedef void* HMMIO;
typedef void* HACMSTREAM;
typedef void* HACMDRIVER;
typedef void* HIC;
typedef void* HACMOBJ;
typedef HACMSTREAM* LPHACMSTREAM;
typedef void* HACMDRIVERID;
typedef void* LPHACMDRIVER;
typedef unsigned char BYTE;
typedef BYTE* LPBYTE;
typedef char TCHAR;
typedef TCHAR* LPTSTR;
typedef const TCHAR* LPCTSTR;
typedef char* LPSTR;
typedef LPSTR LPOLESTR;
typedef const char* LPCSTR;
typedef LPCSTR LPCOLESTR;
typedef wchar_t WCHAR;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef UINT MMRESULT;
typedef uint32_t DWORD;
typedef DWORD COLORREF;
typedef DWORD FOURCC;
typedef DWORD HRESULT;
typedef DWORD* LPDWORD;
typedef DWORD* DWORD_PTR;
typedef int32_t LONG;
typedef int32_t* LONG_PTR;
typedef LONG_PTR LRESULT;
typedef uint32_t ULONG;
typedef uint32_t* ULONG_PTR;
typedef uint64_t _fsize_t;
typedef LONG NTSTATUS;
typedef void PROCESS_INFORMATION;

// __stdcall is used in one place.
#define __stdcall

#endif  // SANDBOX_FUZZER_TYPES_H_
