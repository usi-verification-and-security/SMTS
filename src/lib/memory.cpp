//
// Author: Matteo Marescotti
//

#include "Exception.h"
#include "memory.h"

size_t current_memory() {
#ifdef SMTS_MEMORY_SUPPORTED
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        throw Exception("getrusage() error");
    }

#ifdef __linux__
    return usage.ru_maxrss * 1024;
#else
    return usage.ru_maxrss;
#endif

#else
    return 0;
#endif
}
