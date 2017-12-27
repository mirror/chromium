// Make sure that these header files don't include Windows.h and can
// compile without including Windows.h.

//#include "base/files/file_util.h" // Includes time.h
//#include "base/files/platform_file.h" // Includes time.h
#include "base/process/process_handle.h"
//#include "base/synchronization/condition_variable.h"
//#include "base/synchronization/lock.h" // Indirectly includes time.h
//#include "base/threading/platform_thread.h" // Includes time.h
#include "base/win/scoped_handle.h"

#ifdef _WINDOWS_
#error
#endif
