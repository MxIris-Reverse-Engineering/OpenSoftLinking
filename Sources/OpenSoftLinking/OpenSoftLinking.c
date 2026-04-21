#include <OpenSoftLinking/OpenSoftLinking.h>
#include <dlfcn.h>
#include <ptrauth.h>
#include <stddef.h>

/* dyld SPI — declared by Apple in dyld private headers; exported by libSystem.
 * Stable since macOS 10.15 / iOS 13. */
extern void *dlopen_from(const char *path, int mode, const void *callerAddress);

#define OSL_DLOPEN_FLAGS (RTLD_LAZY | RTLD_FIRST)  /* 0x101, matches Apple */

__attribute__((noinline))
void *_osl_dlopen(const char *const *paths, char **errorMessage)
{
    (void)errorMessage; /* slow path added in Task 1.4 */

    const void *caller = ptrauth_strip(__builtin_return_address(0),
                                        ptrauth_key_return_address);

    for (const char *const *p = paths; *p != NULL; ++p) {
        void *handle = dlopen_from(*p, OSL_DLOPEN_FLAGS, caller);
        if (handle != NULL) {
            return handle;
        }
    }
    return NULL;
}

void *_osl_dlopen_audited(const char *const *paths)
{
    return _osl_dlopen(paths, NULL);
}
