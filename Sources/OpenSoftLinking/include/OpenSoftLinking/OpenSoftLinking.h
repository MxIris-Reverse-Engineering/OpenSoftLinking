//
//  OpenSoftLinking.h
//  OpenSoftLinking
//
//  Open-source reimplementation of Apple's private SoftLinking.framework.
//

#ifndef OPENSOFTLINKING_OPENSOFTLINKING_H
#define OPENSOFTLINKING_OPENSOFTLINKING_H

#include <Availability.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Attempt to open a framework by probing a NULL-terminated list of candidate
 * paths. Behavior mirrors Apple's private _sl_dlopen from SoftLinking-71.
 *
 * @param paths         Non-NULL pointer to a NULL-terminated array of C
 *                      strings. Each non-NULL entry is passed to
 *                      dlopen_from() in order; the first handle returned is
 *                      returned from this function.
 * @param errorMessage  Optional out-param. If non-NULL and all paths fail,
 *                      the function writes a heap-allocated '\n'-joined
 *                      string of per-path dlerror() messages. Caller must
 *                      free(). If NULL, failures are silent aside from
 *                      os_log_info output.
 *
 * @return dyld image handle on success; NULL if every path failed.
 */
void *_osl_dlopen(const char *const *paths, char **errorMessage);

/**
 * Equivalent to _osl_dlopen(paths, NULL). Named for parity with Apple's
 * _sl_dlopen_audited, which is likewise a thin tail-call into _sl_dlopen.
 */
void *_osl_dlopen_audited(const char *const *paths);

#ifdef __cplusplus
}
#endif

#include <OpenSoftLinking/SoftLinking.h>

#endif /* OPENSOFTLINKING_OPENSOFTLINKING_H */
