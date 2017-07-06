#include <malloc.h>

#include "jemalloc/include/jemalloc/jemalloc.h"

#ifdef __cplusplus
extern "C" {
#endif

JEMALLOC_EXPORT struct mallinfo je_mallinfo();

#ifdef __cplusplus
}
#endif
