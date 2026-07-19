#ifndef FP_PROGRESS_H
#define FP_PROGRESS_H

#include <stdint.h>

typedef int (*fp_progress_callback)(void* user, uint64_t current, uint64_t total, const char* phase);

typedef struct {
    fp_progress_callback callback;
    void* user;
} fp_progress_t;

static inline int fp_progress_emit(fp_progress_t progress, uint64_t current, uint64_t total, const char* phase) {
    if (progress.callback) {
        return progress.callback(progress.user, current, total, phase);
    }
    return 1;
}

#endif /* FP_PROGRESS_H */
