#ifndef FP_MMAP_H
#define FP_MMAP_H

#include <stddef.h>

// Opaque handle for memory-mapped file
typedef struct FPMMap FPMMap;

// Open a file for memory-mapped reading
// Returns handle on success, NULL on failure
// On success, populates *ptr_out with the mapped memory address
// and *size_out with the file size
FPMMap* fp_mmap_open_read(const char* path, void** ptr_out, size_t* size_out);

// Close a memory-mapped file
void fp_mmap_close(FPMMap* map);

#endif // FP_MMAP_H
