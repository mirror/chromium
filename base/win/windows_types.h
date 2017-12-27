#ifndef BASE_WIN_WINDOWS_TYPES_H
#define BASE_WIN_WINDOWS_TYPES_H

#include <sal.h>
#include <concurrencysal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long DWORD;
typedef long LONG;
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;

#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef int INT;
#endif
typedef void* PVOID;
typedef void* HANDLE;

#if defined(_WIN64)
typedef __int64 INT_PTR, *PINT_PTR;
typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

typedef __int64 LONG_PTR, *PLONG_PTR;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#define __int3264   __int64

#else
typedef _W64 int INT_PTR, *PINT_PTR;
typedef _W64 unsigned int UINT_PTR, *PUINT_PTR;

typedef _W64 long LONG_PTR, *PLONG_PTR;
typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;

#define __int3264   __int32

#endif

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

struct _RTL_SRWLOCK;
typedef _RTL_SRWLOCK RTL_SRWLOCK;
typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;


#define DECLSPEC_IMPORT __declspec(dllimport)
#define WINBASEAPI DECLSPEC_IMPORT
#define WINAPI __stdcall

struct CHROME_SRWLOCK {
	PVOID Ptr;
};

struct CHROME_CONDITION_VARIABLE {
	PVOID Ptr;
};

WINBASEAPI
_Releases_exclusive_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockExclusive(
	_Inout_ PSRWLOCK SRWLock
);

#ifdef __cplusplus
}
#endif

#endif  // BASE_WIN_WINDOWS_TYPES_H
