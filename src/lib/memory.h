//
// Author: Matteo Marescotti
//

#ifndef SMTS_MEMORY_H
#define SMTS_MEMORY_H

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)

#define SMTS_MEMORY_SUPPORTED

#include <sys/time.h>
#include <sys/resource.h>

#else
#warning "measuring memory is not supported for the current OS"
#endif


size_t current_memory();

#endif //SMTS_MEMORY_H
