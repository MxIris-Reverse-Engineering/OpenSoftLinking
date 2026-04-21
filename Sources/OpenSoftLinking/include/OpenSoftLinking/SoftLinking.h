//
//  SoftLinking.h
//  OpenSoftLinking
//
//  WebKit-derived soft-linking macros, prefixed OPEN_ and routed through
//  the _osl_dlopen runtime. Adapted from:
//  https://github.com/WebKit/WebKit/blob/main/Source/WTF/wtf/cocoa/SoftLinking.h
//  (LGPL-2.1). Transformation rules documented in
//  docs/superpowers/specs/2026-04-21-opensoftlinking-design.md §5.1.
//

#ifndef OPENSOFTLINKING_SOFTLINKING_H
#define OPENSOFTLINKING_SOFTLINKING_H

/* C-compatible system headers only — Foundation/NSObject are not imported
 * here because SoftLinking.h is a public C header (publicHeadersPath).
 * Consumers that are Objective-C translation units already import Foundation
 * before including OpenSoftLinking/OpenSoftLinking.h. */
#include <dispatch/dispatch.h>
#include <objc/runtime.h>
#include <os/log.h>
#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime dependency (forward-declared here for macro expansions that include
 * only SoftLinking.h). */
void *_osl_dlopen(const char *const *paths, char **errorMessage);

#ifdef __cplusplus
}
#endif

/* ---------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------*/

/* Abort the current process with a diagnostic. Used by non-OPTIONAL macro
 * variants when a symbol cannot be resolved. */
#define OSL_RELEASE_ASSERT(cond, fmt, ...)                                   \
    do {                                                                     \
        if (__builtin_expect(!(cond), 0)) {                                  \
            os_log_fault(OS_LOG_DEFAULT, fmt, ##__VA_ARGS__);                \
            __builtin_trap();                                                \
        }                                                                    \
    } while (0)

/* Stringification. */
#define OSL_STR_(x) #x
#define OSL_STR(x) OSL_STR_(x)

/* ---------------------------------------------------------------------------
 * Framework loaders
 * -------------------------------------------------------------------------*/

#define OPEN_SOFT_LINK_FRAMEWORK(framework)                                  \
    static void *framework##Library(void)                                    \
    {                                                                        \
        static void *frameworkLibrary;                                       \
        static dispatch_once_t onceToken;                                    \
        dispatch_once(&onceToken, ^{                                         \
            static const char *const paths[] = {                             \
                "/System/Library/Frameworks/" #framework ".framework/" #framework, \
                "/System/Library/Frameworks/" #framework ".framework/Contents/MacOS/" #framework, \
                NULL                                                         \
            };                                                               \
            char *error = NULL;                                              \
            frameworkLibrary = _osl_dlopen(paths, &error);                   \
            OSL_RELEASE_ASSERT(frameworkLibrary != NULL,                     \
                "OpenSoftLinking: failed to load %{public}s: %{public}s",   \
                #framework, error ? error : "unknown");                      \
        });                                                                  \
        return frameworkLibrary;                                             \
    }

#define OPEN_SOFT_LINK_FRAMEWORK_OPTIONAL(framework)                         \
    static void *framework##Library(void)                                    \
    {                                                                        \
        static void *frameworkLibrary;                                       \
        static dispatch_once_t onceToken;                                    \
        dispatch_once(&onceToken, ^{                                         \
            static const char *const paths[] = {                             \
                "/System/Library/Frameworks/" #framework ".framework/" #framework, \
                "/System/Library/Frameworks/" #framework ".framework/Contents/MacOS/" #framework, \
                NULL                                                         \
            };                                                               \
            frameworkLibrary = _osl_dlopen(paths, NULL);                     \
        });                                                                  \
        return frameworkLibrary;                                             \
    }

#define OPEN_SOFT_LINK_PRIVATE_FRAMEWORK(framework)                          \
    static void *framework##Library(void)                                    \
    {                                                                        \
        static void *frameworkLibrary;                                       \
        static dispatch_once_t onceToken;                                    \
        dispatch_once(&onceToken, ^{                                         \
            static const char *const paths[] = {                             \
                "/System/Library/PrivateFrameworks/" #framework ".framework/" #framework, \
                "/System/Library/PrivateFrameworks/" #framework ".framework/Contents/MacOS/" #framework, \
                NULL                                                         \
            };                                                               \
            char *error = NULL;                                              \
            frameworkLibrary = _osl_dlopen(paths, &error);                   \
            OSL_RELEASE_ASSERT(frameworkLibrary != NULL,                     \
                "OpenSoftLinking: failed to load private %{public}s: %{public}s", \
                #framework, error ? error : "unknown");                      \
        });                                                                  \
        return frameworkLibrary;                                             \
    }

#define OPEN_SOFT_LINK_PRIVATE_FRAMEWORK_OPTIONAL(framework)                 \
    static void *framework##Library(void)                                    \
    {                                                                        \
        static void *frameworkLibrary;                                       \
        static dispatch_once_t onceToken;                                    \
        dispatch_once(&onceToken, ^{                                         \
            static const char *const paths[] = {                             \
                "/System/Library/PrivateFrameworks/" #framework ".framework/" #framework, \
                "/System/Library/PrivateFrameworks/" #framework ".framework/Contents/MacOS/" #framework, \
                NULL                                                         \
            };                                                               \
            frameworkLibrary = _osl_dlopen(paths, NULL);                     \
        });                                                                  \
        return frameworkLibrary;                                             \
    }

#define OPEN_SOFT_LINK_FRAMEWORK_IN_UMBRELLA(umbrella, framework)            \
    static void *framework##Library(void)                                    \
    {                                                                        \
        static void *frameworkLibrary;                                       \
        static dispatch_once_t onceToken;                                    \
        dispatch_once(&onceToken, ^{                                         \
            static const char *const paths[] = {                             \
                "/System/Library/Frameworks/" #umbrella ".framework/Frameworks/" #framework ".framework/" #framework, \
                "/System/Library/Frameworks/" #umbrella ".framework/Versions/Current/Frameworks/" #framework ".framework/Versions/Current/" #framework, \
                NULL                                                         \
            };                                                               \
            char *error = NULL;                                              \
            frameworkLibrary = _osl_dlopen(paths, &error);                   \
            OSL_RELEASE_ASSERT(frameworkLibrary != NULL,                     \
                "OpenSoftLinking: failed to load %{public}s (umbrella %{public}s): %{public}s", \
                #framework, #umbrella, error ? error : "unknown");           \
        });                                                                  \
        return frameworkLibrary;                                             \
    }

#define OPEN_SOFT_LINK_FRAMEWORK_IN_UMBRELLA_OPTIONAL(umbrella, framework)   \
    static void *framework##Library(void)                                    \
    {                                                                        \
        static void *frameworkLibrary;                                       \
        static dispatch_once_t onceToken;                                    \
        dispatch_once(&onceToken, ^{                                         \
            static const char *const paths[] = {                             \
                "/System/Library/Frameworks/" #umbrella ".framework/Frameworks/" #framework ".framework/" #framework, \
                "/System/Library/Frameworks/" #umbrella ".framework/Versions/Current/Frameworks/" #framework ".framework/Versions/Current/" #framework, \
                NULL                                                         \
            };                                                               \
            frameworkLibrary = _osl_dlopen(paths, NULL);                     \
        });                                                                  \
        return frameworkLibrary;                                             \
    }

/* ---------------------------------------------------------------------------
 * Library loaders (/usr/lib/lib<name>.dylib)
 * -------------------------------------------------------------------------*/

#define OPEN_SOFT_LINK_LIBRARY(library)                                      \
    static void *library##Library(void)                                      \
    {                                                                        \
        static void *libraryHandle;                                          \
        static dispatch_once_t onceToken;                                    \
        dispatch_once(&onceToken, ^{                                         \
            static const char *const paths[] = {                             \
                "/usr/lib/lib" #library ".dylib",                            \
                NULL                                                         \
            };                                                               \
            char *error = NULL;                                              \
            libraryHandle = _osl_dlopen(paths, &error);                      \
            OSL_RELEASE_ASSERT(libraryHandle != NULL,                        \
                "OpenSoftLinking: failed to load lib%{public}s.dylib: %{public}s", \
                #library, error ? error : "unknown");                        \
        });                                                                  \
        return libraryHandle;                                                \
    }

#define OPEN_SOFT_LINK_LIBRARY_OPTIONAL(library)                             \
    static void *library##Library(void)                                      \
    {                                                                        \
        static void *libraryHandle;                                          \
        static dispatch_once_t onceToken;                                    \
        dispatch_once(&onceToken, ^{                                         \
            static const char *const paths[] = {                             \
                "/usr/lib/lib" #library ".dylib",                            \
                NULL                                                         \
            };                                                               \
            libraryHandle = _osl_dlopen(paths, NULL);                        \
        });                                                                  \
        return libraryHandle;                                                \
    }

/* ---------------------------------------------------------------------------
 * Class macros
 * -------------------------------------------------------------------------*/

#define OPEN_SOFT_LINK_CLASS(framework, className)                            \
    static Class get##className##Class(void)                                  \
    {                                                                         \
        static Class cls;                                                     \
        static dispatch_once_t onceToken;                                     \
        dispatch_once(&onceToken, ^{                                          \
            (void)framework##Library();                                       \
            cls = objc_getClass(#className);                                  \
            OSL_RELEASE_ASSERT(cls != Nil,                                    \
                "OpenSoftLinking: class %{public}s not found in %{public}s", \
                #className, #framework);                                      \
        });                                                                   \
        return cls;                                                           \
    }

#define OPEN_SOFT_LINK_CLASS_OPTIONAL(framework, className)                   \
    static Class get##className##Class(void)                                  \
    {                                                                         \
        static Class cls;                                                     \
        static dispatch_once_t onceToken;                                     \
        dispatch_once(&onceToken, ^{                                          \
            (void)framework##Library();                                       \
            cls = objc_getClass(#className);                                  \
        });                                                                   \
        return cls;                                                           \
    }

/* ---------------------------------------------------------------------------
 * Function macros
 * -------------------------------------------------------------------------*/

/* Non-optional soft-link. Aborts on missing symbol.
 *
 * Expansion creates <functionName>_soft(params) which on first call resolves
 * the symbol via dlsym() and thereafter calls it directly.
 */
#define OPEN_SOFT_LINK(framework, functionName, resultType, parameterDeclarations, parameterNames) \
    static resultType (*functionName##_soft_ptr) parameterDeclarations;                            \
    static resultType functionName##_soft parameterDeclarations                                    \
    {                                                                                              \
        static dispatch_once_t onceToken;                                                          \
        dispatch_once(&onceToken, ^{                                                               \
            functionName##_soft_ptr = (resultType (*) parameterDeclarations)                       \
                dlsym(framework##Library(), #functionName);                                        \
            OSL_RELEASE_ASSERT(functionName##_soft_ptr != NULL,                                    \
                "OpenSoftLinking: function %{public}s not found in %{public}s",                   \
                #functionName, #framework);                                                        \
        });                                                                                        \
        return functionName##_soft_ptr parameterNames;                                             \
    }

/* Optional soft-link for a function whose presence is uncertain.
 *
 * Expansion creates two functions:
 *   - canLoad_<framework>_<functionName>() -> BOOL
 *   - <functionName>_soft(params) -> calls real fn if present; caller must
 *     check canLoad before calling.
 */
#define OPEN_SOFT_LINK_MAY_FAIL(framework, functionName, resultType, parameterDeclarations, parameterNames) \
    static resultType (*functionName##_soft_ptr) parameterDeclarations;                            \
    static BOOL canLoad_##framework##_##functionName(void)                                         \
    {                                                                                              \
        static dispatch_once_t onceToken;                                                          \
        dispatch_once(&onceToken, ^{                                                               \
            functionName##_soft_ptr = (resultType (*) parameterDeclarations)                       \
                dlsym(framework##Library(), #functionName);                                        \
        });                                                                                        \
        return functionName##_soft_ptr != NULL;                                                    \
    }                                                                                              \
    static resultType functionName##_soft parameterDeclarations                                    \
    {                                                                                              \
        return functionName##_soft_ptr parameterNames;                                             \
    }

/* Optional soft-link variant. The WebKit name kept for compatibility; the
 * callingConvention token is accepted but not emitted (compilers infer the
 * ABI from the function pointer type).
 *
 * Expansion provides only canLoad_<framework>_<functionName>() — consumers
 * must pair it with a manual call site, e.g.:
 *     if (canLoad_Foundation_foo()) { ... }
 */
#define OPEN_SOFT_LINK_OPTIONAL(framework, functionName, resultType, callingConvention, parameterDeclarations) \
    static resultType (*functionName##_soft_ptr) parameterDeclarations;                            \
    static BOOL canLoad_##framework##_##functionName(void)                                         \
    {                                                                                              \
        static dispatch_once_t onceToken;                                                          \
        dispatch_once(&onceToken, ^{                                                               \
            functionName##_soft_ptr = (resultType (*) parameterDeclarations)                       \
                dlsym(framework##Library(), #functionName);                                        \
        });                                                                                        \
        return functionName##_soft_ptr != NULL;                                                    \
    }

/* Further macros added by subsequent tasks:
 *  - Function macros: Task 2.3
 *  - Pointer macros: Task 2.4
 *  - Constant macros: Task 2.5
 *  - Variable macros: Task 2.6
 *  - Header/Source split variants: Task 2.7
 */

#endif /* OPENSOFTLINKING_SOFTLINKING_H */
