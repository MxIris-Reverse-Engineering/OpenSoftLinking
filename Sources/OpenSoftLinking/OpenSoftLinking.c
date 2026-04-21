//
//  OpenSoftLinking.c
//  OpenSoftLinking
//
//  Reverse-engineered 1:1 from Apple SoftLinking-71 (_sl_dlopen @
//  0x1903165d8 in macOS 26.4 dyld_shared_cache).
//

#include <OpenSoftLinking/OpenSoftLinking.h>

void *_osl_dlopen(const char *const *paths, char **errorMessage)
{
    (void)paths;
    (void)errorMessage;
    return 0;
}

void *_osl_dlopen_audited(const char *const *paths)
{
    return _osl_dlopen(paths, 0);
}
