// Make sure that these header files don't include Windows.h and can
// compile without including Windows.h.

#include "base/files/file_util.h"
#include "base/files/platform_file.h"
#include "base/process/process_handle.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "base/win/scoped_handle.h"
#include "base/win/win_util.h"
#include "base/win/registry.h"
#include "base/process/process_handle.h"
#include "base/threading/thread_local_storage.h"

#ifdef _WINDOWS_
#error
#endif

// Make sure these header files can be included in this order.

#include "base/win/windows_types.h"
#include <windows.h>
