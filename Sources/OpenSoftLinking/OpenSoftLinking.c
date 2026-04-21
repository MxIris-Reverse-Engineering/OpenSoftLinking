#include <OpenSoftLinking/OpenSoftLinking.h>
#include <dlfcn.h>
#include <os/log.h>
#include <ptrauth.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

extern void *dlopen_from(const char *path, int mode, const void *callerAddress);

#define OSL_DLOPEN_FLAGS (RTLD_LAZY | RTLD_FIRST)

__attribute__((noinline))
void *_osl_dlopen(const char *const *paths, char **errorMessage)
{
    const void *caller = ptrauth_strip(__builtin_return_address(0),
                                        ptrauth_key_return_address);

    /* Fast path: no error collection. */
    if (errorMessage == NULL) {
        for (const char *const *p = paths; *p != NULL; ++p) {
            void *handle = dlopen_from(*p, OSL_DLOPEN_FLAGS, caller);
            if (handle != NULL) {
                return handle;
            }
        }
        return NULL;
    }

    /* Slow path: collect per-path dlerror() messages. */
    size_t count = 0;
    for (const char *const *p = paths; *p != NULL; ++p) ++count;

    if (count == 0) {
        *errorMessage = calloc(1, 1);  /* empty string; free() valid */
        if (*errorMessage == NULL) {
            abort();
        }
        return NULL;
    }

    char **errors = (char **)calloc(count, sizeof(char *));
    if (errors == NULL) {
        abort();
    }
    size_t totalLength = 0;

    for (size_t i = 0; i < count; ++i) {
        void *handle = dlopen_from(paths[i], OSL_DLOPEN_FLAGS, caller);
        if (handle != NULL) {
            for (size_t k = 0; k < i; ++k) {
                free(errors[k]);
            }
            free(errors);
            return handle;
        }
        const char *raw = dlerror();
        char *copied = strdup(raw ? raw : "unknown");
        if (copied == NULL) {
            abort();
        }
        errors[i] = copied;
        totalLength += strlen(copied) + 1;  /* +1 for '\n' or NUL */
        os_log_info(OS_LOG_DEFAULT,
                    "SoftLinking client failed to load dependency: %{public}s",
                    copied);
    }

    char *combined = (char *)calloc(totalLength, 1);
    if (combined == NULL) {
        abort();
    }
    for (size_t i = 0; i < count; ++i) {
        strlcat(combined, errors[i], totalLength);
        if (i + 1 < count) {
            strlcat(combined, "\n", totalLength);
        }
        free(errors[i]);
    }
    free(errors);

    *errorMessage = combined;
    return NULL;
}

void *_osl_dlopen_audited(const char *const *paths)
{
    return _osl_dlopen(paths, NULL);
}
