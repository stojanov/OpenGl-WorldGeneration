#include <assert.h>
#include "Log.h"

#ifdef PRISM_DEBUG
#define PR_CONST_ASSERT(x, ...) { if constexpr (!(x)) { PR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); assert(x); } }
#define PR_ASSERT(x, ...) { if(!(x)) { PR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); assert(x); } }
#define PR_ERROR_ASSERT(x, err)
#else
#define PR_ASSERT(x, ...)
#define PR_CONST_ASSERT(x, ...)
#endif