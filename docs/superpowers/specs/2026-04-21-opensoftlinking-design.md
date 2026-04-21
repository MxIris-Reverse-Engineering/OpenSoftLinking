# OpenSoftLinking — Design Spec

**Date**: 2026-04-21
**Status**: Approved (brainstorming phase)
**Next step**: writing-plans skill → implementation plan

---

## 0. Summary

OpenSoftLinking is an open-source re-implementation of Apple's private
`SoftLinking.framework` (`/System/Library/PrivateFrameworks/SoftLinking.framework`,
internal version tag `SoftLinking-71` on macOS 26.4).

It gives consumers the ability to load an Apple framework at runtime with the
same path-probing + Catalyst-aware path resolution behavior that Apple's
`_sl_dlopen` provides, plus the full WebKit-style `SOFT_LINK_*` macro surface
(re-prefixed `OPEN_SOFT_LINK_*`). The runtime is a 1:1 reverse-engineered
reimplementation based on IDA decompilation of `SoftLinking.i64`.

Scope boundary: this package is **Apple-platform only**. All work happens on
top of `dlopen_from` (dyld SPI, available on macOS 10.15+ / iOS 13+ and
equivalent).

## 1. Goals & Non-Goals

### Goals

1. Provide a symbol-level replacement for Apple `SoftLinking.framework`'s
   public-ish contract (`_osl_dlopen` / `_osl_dlopen_audited`) that behaves
   identically to the decompiled `_sl_dlopen` from macOS 26.4 `SoftLinking-71`.
2. Provide the full WebKit `SoftLinking.h` macro suite under `OPEN_SOFT_LINK_*`
   prefix, routed through our runtime (not raw `dlopen`).
3. Ship as a single Swift Package with one library target + one test target.
4. Cover all Apple platforms supported by `dlopen_from` (macOS 10.15+, iOS 13+,
   tvOS 13+, watchOS 6+, visionOS 1+, Mac Catalyst 13+).
5. Not collide with Apple's private symbols when linked into a process that
   already links against `SoftLinking.framework` (e.g. UIKitMacHelper
   consumers).

### Non-Goals

1. Not a pure-Swift API. Consumers use C / Objective-C; Swift consumers bridge
   via a bridging header.
2. Not a re-implementation of Apple path rewriting rules. Catalyst path
   resolution is delegated to dyld via `dlopen_from(path, flags, caller)`.
3. Not a header for `dlsym`. `dlsym()` remains a libc call; OpenSoftLinking
   only owns the `dlopen` side.
4. Not a linter or deprecation tool. We don't warn consumers about using soft
   linking.
5. Not shipping pre-built `.xcframework`. Build from source via SwiftPM only.

## 2. Reverse-Engineering Basis

Decompilation source: `/Volumes/Code/Dump/DyldSharedCaches/macOS/26.4/SoftLinking.i64`.

Key facts established from IDA analysis:

| Fact | Evidence |
|---|---|
| Only 2 exported functions | `_sl_dlopen` @ `0x1903165d8` (616 bytes), `_sl_dlopen_audited` @ `0x1903165d0` (8-byte thunk) |
| No `_sl_dlsym` | Not present in exports. `dlsym()` is a libc call from the consumer. |
| Underlying call is `dlopen_from` | Single import: `_dlopen_from` @ `0x1903168ac` |
| dlopen flags = `0x101` | `MOV W1, #0x101` at `0x190316630` and `0x1903166a4` = `RTLD_LAZY | RTLD_FIRST` |
| Caller address = stripped LR | `MOV X24, X30` at `0x1903165fc`, then `XPACI X21` at `0x190316628` |
| Descriptor = NULL-terminated `const char **` | Loop structure: `LDR X0, [X19,#8]!` + `CBZ X0, ...` |
| No path remapping inside SoftLinking | No string manipulation before `dlopen_from` call |
| Error combine uses `\n` separator | `strlcat(combined, "\n", …)` at `0x19031677c` |
| Cold path triggers `__assert_rtn` | `_sl_dlopen.cold.1` @ `0x190316840`: `__assert_rtn("_sl_dlopen", "SoftLinking.c", 66, "errorMessage")` |
| Log format | `"SoftLinking client failed to load dependency: %{public}s"` @ `0x190316968`, `os_log_info` via `_os_log_impl` |
| Version marker | `@(#)PROGRAM:SoftLinking  PROJECT:SoftLinking-71` in `__const` @ `0x190316920` |

Implication: **Catalyst's "iOS path → iOSSupport" rewriting lives in dyld**, not
in SoftLinking. SoftLinking's value is purely: (a) try multiple fallback
paths, (b) invoke `dlopen_from` with the consumer's PC so dyld sees the
consumer's ABI, (c) optionally aggregate errors.

## 3. Architecture

### 3.1 Package layout

```
OpenSoftLinking/
├── Package.swift
├── README.md
├── LICENSE
├── docs/superpowers/specs/2026-04-21-opensoftlinking-design.md  (this file)
├── Sources/
│   └── OpenSoftLinking/
│       ├── OpenSoftLinking.c              — runtime (reverse-engineered)
│       ├── OpenSoftLinking_Private.h      — internal-only declarations
│       └── include/OpenSoftLinking/
│           ├── OpenSoftLinking.h          — umbrella + runtime prototypes
│           └── SoftLinking.h              — WebKit-derived macro suite
└── Tests/
    └── OpenSoftLinkingTests/
        ├── OSLRuntimeTests.m
        ├── OSLMacroFrameworkTests.m
        ├── OSLMacroClassTests.m
        ├── OSLMacroFunctionTests.m
        ├── OSLMacroPointerTests.m
        ├── OSLMacroConstantTests.m
        └── OSLPlatformBehaviorTests.m
```

### 3.2 Two-layer architecture

**Runtime layer** (`OpenSoftLinking.c`, ~100 lines): a 1:1 translation of
Apple's `_sl_dlopen` decompilation. Two exported functions:

```c
void *_osl_dlopen(const char *const *paths, char **errorMessage);
void *_osl_dlopen_audited(const char *const *paths);
```

**Macro layer** (`SoftLinking.h`, ~1000 lines): adapted from WebKit
`Source/WTF/wtf/cocoa/SoftLinking.h`. All `SOFT_LINK_*` macros renamed to
`OPEN_SOFT_LINK_*` and their internal `dlopen(path, RTLD_NOW)` call sites
replaced with `_osl_dlopen(descriptor, NULL)`.

### 3.3 ABI surface

**C functions** (no prefix collision with Apple private):

| Symbol | Purpose |
|---|---|
| `_osl_dlopen(paths, errorMessage)` | Core probe-load function |
| `_osl_dlopen_audited(paths)` | Thunk, tail-calls `_osl_dlopen(paths, NULL)` |

**Macros** (no prefix collision with WebKit):

| Group | Macros |
|---|---|
| Framework / Library loaders | `OPEN_SOFT_LINK_FRAMEWORK`, `_PRIVATE_FRAMEWORK`, `_FRAMEWORK_IN_UMBRELLA`, `_FRAMEWORK_OPTIONAL`, `_FRAMEWORK_OPTIONAL_PREFLIGHT`, `_LIBRARY`, `_LIBRARY_OPTIONAL` |
| Header/Source split (for cross-TU linking) | `OPEN_SOFT_LINK_FRAMEWORK_FOR_HEADER`, `_FOR_SOURCE`, `_FOR_SOURCE_OPTIONAL_WITH_EXPORT`, private-framework variants |
| Class | `OPEN_SOFT_LINK_CLASS`, `_OPTIONAL`, `_FOR_HEADER`, `_FOR_SOURCE`, `_FOR_SOURCE_OPTIONAL` |
| Function | `OPEN_SOFT_LINK`, `_MAY_FAIL`, `_OPTIONAL`, `_FUNCTION_FOR_HEADER`, `_FUNCTION_FOR_SOURCE`, `_FUNCTION_MAY_FAIL_FOR_*` |
| Pointer | `OPEN_SOFT_LINK_POINTER`, `_OPTIONAL`, `_FOR_HEADER`, `_FOR_SOURCE` |
| Constant | `OPEN_SOFT_LINK_CONSTANT`, `_MAY_FAIL`, `_OPTIONAL`, `_FOR_HEADER`, `_FOR_SOURCE` |
| Variable | `OPEN_SOFT_LINK_VARIABLE`, `_FOR_HEADER`, `_FOR_SOURCE` |

### 3.4 Package.swift

```swift
// swift-tools-version: 6.3
import PackageDescription

let package = Package(
    name: "OpenSoftLinking",
    platforms: [
        .macOS(.v10_15),
        .iOS(.v13),
        .tvOS(.v13),
        .watchOS(.v6),
        .visionOS(.v1),
        .macCatalyst(.v13),
    ],
    products: [
        .library(name: "OpenSoftLinking", targets: ["OpenSoftLinking"]),
    ],
    targets: [
        .target(
            name: "OpenSoftLinking",
            publicHeadersPath: "include"
        ),
        .testTarget(
            name: "OpenSoftLinkingTests",
            dependencies: ["OpenSoftLinking"]
        ),
    ],
    swiftLanguageModes: [.v6]
)
```

## 4. Runtime Layer Specification

### 4.1 `_osl_dlopen` contract

```c
/**
 * Attempt to dlopen a framework by probing a NULL-terminated list of paths.
 *
 * @param paths         Non-NULL pointer to a NULL-terminated array of C
 *                      strings. Each non-NULL entry is tried in order until
 *                      one succeeds or all are exhausted.
 * @param errorMessage  If non-NULL, on all-paths-failed the function allocates
 *                      a newline-joined concatenation of all dlerror() strings
 *                      and writes the pointer to *errorMessage. Caller must
 *                      free(). On success *errorMessage is not written.
 *
 * @return dyld image handle on success; NULL on failure.
 *
 * Semantics reverse-engineered from Apple SoftLinking-71 (_sl_dlopen @
 * 0x1903165d8 in macOS 26.4 dyld_shared_cache).
 */
void *_osl_dlopen(const char *const *paths, char **errorMessage);

/** Thunk equivalent to _osl_dlopen(paths, NULL). */
void *_osl_dlopen_audited(const char *const *paths);
```

### 4.2 Reference implementation (C pseudocode, faithful to IDA)

```c
#include <dlfcn.h>
#include <os/log.h>
#include <ptrauth.h>
#include <stdlib.h>
#include <string.h>

extern void *dlopen_from(const char *path, int mode, const void *callerAddress);

#define OSL_DLOPEN_FLAGS (RTLD_LAZY | RTLD_FIRST)  /* 0x101 */

__attribute__((noinline))
void *_osl_dlopen(const char *const *paths, char **errorMessage)
{
    const void *caller = ptrauth_strip(__builtin_return_address(0),
                                        ptrauth_key_return_address);

    /* Fast path: no error collection */
    if (errorMessage == NULL) {
        for (const char *const *p = paths; *p != NULL; ++p) {
            void *handle = dlopen_from(*p, OSL_DLOPEN_FLAGS, caller);
            if (handle != NULL) {
                return handle;
            }
        }
        return NULL;
    }

    /* Slow path: collect per-path dlerror strings and join with '\n' */
    size_t count = 0;
    for (const char *const *p = paths; *p != NULL; ++p) count++;

    const char **errors = calloc(count, sizeof(char *));
    size_t totalLength = 0;

    for (size_t i = 0; i < count; i++) {
        void *handle = dlopen_from(paths[i], OSL_DLOPEN_FLAGS, caller);
        if (handle != NULL) {
            /* Free any partial errors we collected so far */
            for (size_t k = 0; k < i; k++) free((void *)errors[k]);
            free(errors);
            return handle;
        }
        char *err = strdup(dlerror());
        errors[i] = err;
        totalLength += strlen(err) + 1; /* +1 for '\n' separator */
        os_log_info(OS_LOG_DEFAULT,
                    "SoftLinking client failed to load dependency: %{public}s",
                    err);
    }

    char *combined = calloc(totalLength, 1);
    if (combined == NULL) {
        /* Matches Apple __assert_rtn cold path */
        abort();
    }

    for (size_t i = 0; i < count; i++) {
        strlcat(combined, errors[i], totalLength);
        if (i + 1 < count) {
            strlcat(combined, "\n", totalLength);
        }
        free((void *)errors[i]);
    }
    free(errors);

    *errorMessage = combined;
    return NULL;
}

void *_osl_dlopen_audited(const char *const *paths)
{
    return _osl_dlopen(paths, NULL);
}
```

### 4.3 Divergences from WebKit behavior

| Aspect | WebKit `SoftLinking.h` | OpenSoftLinking |
|---|---|---|
| Underlying load | `dlopen(path, RTLD_NOW)` | `dlopen_from(path, RTLD_LAZY \| RTLD_FIRST, caller)` |
| Catalyst path support | None (pass literal path, caller trusts dyld) | Explicit: dyld resolves based on caller ABI passed to `dlopen_from` |
| Fallback paths | Single literal path per macro expansion | NULL-terminated descriptor with iOS-flat primary + macOS-Contents fallback |
| Error aggregation | Per-macro `RELEASE_ASSERT` at call site | Joined with `\n` separator in `_osl_dlopen` errorMessage out-param |
| Log channel | Generic | `OS_LOG_DEFAULT` with Apple's exact format string |
| Symbol namespacing | `SOFT_LINK_*` (collides w/ Apple) | `OPEN_SOFT_LINK_*` (unique) + `_osl_` runtime prefix |

## 5. Macro Layer Specification

### 5.1 Derivation procedure

Source material: WebKit `Source/WTF/wtf/cocoa/SoftLinking.h` (LGPL-2.1).
Mechanical transformation applied:

1. Rename every `SOFT_LINK_*` macro identifier to `OPEN_SOFT_LINK_*`.
2. Replace every internal `dlopen(<path>, <flags>)` call with
   `_osl_dlopen((const char *const[]){ <primary>, <fallback>, NULL }, NULL)`.
3. Replace every `RELEASE_ASSERT_WITH_MESSAGE` with an equivalent
   `os_log_fault` + `__builtin_trap()` pair guarded by an internal
   `OSL_RELEASE_ASSERT` macro (defined once at top of `SoftLinking.h`).
4. Drop WebKit-specific includes (`<wtf/…>`, `WTF_EXTERN_C_BEGIN/END`).
5. Keep path layout logic: primary = iOS-flat, fallback = macOS-Contents.
6. Add LICENSE attribution header preserving LGPL-2.1 inheritance.

### 5.2 Framework descriptor format (generated inline by macros)

For a public framework `FOO`:

```c
static const char *const FOOPaths[] = {
    "/System/Library/Frameworks/FOO.framework/FOO",
    "/System/Library/Frameworks/FOO.framework/Contents/MacOS/FOO",
    NULL
};
```

For a private framework `FOO`:

```c
static const char *const FOOPaths[] = {
    "/System/Library/PrivateFrameworks/FOO.framework/FOO",
    "/System/Library/PrivateFrameworks/FOO.framework/Contents/MacOS/FOO",
    NULL
};
```

For a framework `FOO` inside umbrella `BAR`:

```c
static const char *const FOOPaths[] = {
    "/System/Library/Frameworks/BAR.framework/Frameworks/FOO.framework/FOO",
    "/System/Library/Frameworks/BAR.framework/Versions/Current/Frameworks/FOO.framework/Versions/Current/FOO",
    NULL
};
```

The primary entry uses iOS-flat framework layout; the fallback uses macOS's
versioned `Contents/MacOS` layout. dyld picks whichever matches the running
platform; on Catalyst the caller-aware `dlopen_from` rewrites to iOSSupport
automatically.

### 5.3 Library descriptor (for `OPEN_SOFT_LINK_LIBRARY`)

```c
static const char *const <name>Paths[] = {
    "/usr/lib/lib<library>.dylib",
    NULL
};
```

### 5.4 Generated helper functions (non-OPTIONAL framework case)

```c
/* OPEN_SOFT_LINK_FRAMEWORK(UIKit) expands approximately to: */
static void *UIKitLibrary(void)
{
    static void *framework;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        static const char *const paths[] = {
            "/System/Library/Frameworks/UIKit.framework/UIKit",
            "/System/Library/Frameworks/UIKit.framework/Contents/MacOS/UIKit",
            NULL,
        };
        char *error = NULL;
        framework = _osl_dlopen(paths, &error);
        if (framework == NULL) {
            os_log_fault(OS_LOG_DEFAULT,
                         "OpenSoftLinking: failed to load UIKit: %{public}s",
                         error ? error : "unknown");
            __builtin_trap();
        }
    });
    return framework;
}
```

`_OPTIONAL` variants omit the trap and return `framework` (which may be NULL).

### 5.5 Generated helpers for Class / Pointer / Constant / Function

Follows WebKit semantics verbatim (cached `static` via `dispatch_once`,
`objc_getClass` for classes, `dlsym` + one-level deref for pointers, `dlsym`
direct for functions). Only the underlying `dlopen` path differs.

## 6. Error Handling

### 6.1 Runtime layer contract summary

| Case | Return | `*errorMessage` | Side effects |
|---|---|---|---|
| Any path succeeds | handle | untouched | none |
| All fail, `errorMessage == NULL` | NULL | n/a | `os_log_info` per failure |
| All fail, `errorMessage != NULL` | NULL | heap-allocated joined string | `os_log_info` per failure; caller must `free()` |
| `calloc` fails on slow path | no return | n/a | `abort()` (matches Apple `__assert_rtn`) |
| Empty descriptor (`paths[0] == NULL`) | NULL | implementation-defined: result of `calloc(0, 1)` — may be NULL or a 0-byte allocation. If `calloc` returns NULL on slow path, `abort()` (matches Apple cold path). | none |

### 6.2 Macro layer strategy

- **Non-`_OPTIONAL` macros**: failure is a programming error. Use
  `OSL_RELEASE_ASSERT` → `os_log_fault` + `__builtin_trap()`.
- **`_OPTIONAL` macros**: failure returns NULL / nil. Consumer checks.
- **`_MAY_FAIL` macros** (function + constant variants): return bool, success
  writes to out-param.

### 6.3 ARM64e PAC handling

- `__builtin_return_address(0)` returns a PAC'd pointer on ARM64e.
- Must call `ptrauth_strip(ptr, ptrauth_key_return_address)` before passing to
  `dlopen_from`.
- Omitting this step on ARM64e causes dyld to fail module lookup for the
  caller, disabling Catalyst path rewriting — breaking the exact guarantee the
  framework exists to provide.

### 6.4 Thread safety

- `_osl_dlopen` itself is reentrant, no shared state.
- Macro-generated `XxxLibrary()` / `get_*()` caches use `dispatch_once` for
  atomic one-time initialization.

## 7. Testing Strategy

### 7.1 Test target

XCTest, Objective-C (not Swift), dependent on `OpenSoftLinking` target.

### 7.2 Runtime tests (`OSLRuntimeTests.m`)

Selected test framework: `Foundation.framework` (present on all Apple
platforms).

| Test | Scenario | Expectation |
|---|---|---|
| `test_dlopen_primaryPath_returnsHandle` | single valid path | handle ≠ NULL |
| `test_dlopen_fallbackUsed_whenPrimaryMissing` | bogus primary, valid fallback | handle ≠ NULL |
| `test_dlopen_allFail_returnsNull_noError` | all invalid, errorMessage=NULL | returns NULL, no crash |
| `test_dlopen_allFail_errorMessageSet` | all invalid, errorMessage ≠ NULL | `*err ≠ NULL`, contains `\n` |
| `test_dlopen_allFail_errorContainsDlerror` | all invalid | `*err` substring-matches typical dlerror text |
| `test_dlopen_emptyPathList_returnsNull` | `paths = { NULL }`, errorMessage=NULL | returns NULL, no crash |
| `test_dlopen_emptyPathList_errorMessage_doesNotCrash` | `paths = { NULL }`, errorMessage ≠ NULL | returns NULL; `*err` either NULL or 0-byte string (libc-dependent), no crash, no leak |
| `test_dlopen_audited_equivalenceTest` | `_osl_dlopen_audited(p)` | returns same handle as `_osl_dlopen(p, NULL)` |
| `test_dlopen_errorMessage_canBeFreed` | allocate + free | no crash / leak |
| `test_dlopen_handle_isUsableWithDlsym` | dlsym `NSObjectClass` | returns valid class via symbol |

### 7.3 Macro tests

**`OSLMacroFrameworkTests.m`**: `OPEN_SOFT_LINK_FRAMEWORK(Foundation)` →
`FoundationLibrary()` non-NULL; `_OPTIONAL` w/ bogus framework returns NULL;
cached pointer identity across calls; concurrent calls thread-safe.

**`OSLMacroClassTests.m`**: `OPEN_SOFT_LINK_CLASS(Foundation, NSObject)` →
matches `[NSObject class]`; `_OPTIONAL` with missing class returns Nil.

**`OSLMacroFunctionTests.m`**: `OPEN_SOFT_LINK(CoreFoundation, CFStringGetLength, …)` →
matches direct-link result; `_MAY_FAIL` with missing symbol returns false;
`_OPTIONAL` returns NULL.

**`OSLMacroPointerTests.m`**: `OPEN_SOFT_LINK_POINTER(Foundation, NSDefaultRunLoopMode, NSRunLoopMode)` →
equal via `isEqual:` to direct-link value.

**`OSLMacroConstantTests.m`**: similar pattern with `CoreFoundation`
constants; `_MAY_FAIL` variant behavior.

### 7.4 Platform behavior tests (`OSLPlatformBehaviorTests.m`)

- **Catalyst (`#if TARGET_OS_MACCATALYST`)**: `_osl_dlopen` with
  `/System/Library/Frameworks/UIKit.framework/UIKit` succeeds; `dladdr`
  returned path contains `/System/iOSSupport/`. macOS native target skips via
  `XCTSkip`.
- **ARM64e PAC**: indirect test — successful load from custom helper function
  (different caller site) proves PAC strip + caller propagation work.
- **Concurrency**: 100-thread concurrent `_osl_dlopen` of same descriptor,
  all return same handle, no crash.

### 7.5 Explicit non-tests

Documented in `README.md` "Known Limitations":

1. `calloc` failure path (can't reliably induce in XCTest).
2. `dlopen_from` SPI future removal (runtime-detect only, not in unit tests).
3. Byte-equivalent parity with Apple binary (infeasible).

### 7.6 CI destinations

```bash
swift test 2>&1 | xcsift                                          # macOS native
swift test --destination '<catalyst>' 2>&1 | xcsift               # Catalyst
xcodebuild test -scheme OpenSoftLinking -destination '...iOS...'  # iOS Sim
# equivalent for tvOS / watchOS / visionOS simulators
```

Preferred via XcodeBuildMCP CLI. See `README.md` for exact commands.

## 8. Open Questions / Deferred

1. **`dlopen_from` alternative path for watchOS**. It is declared available,
   but we should verify on a physical-device destination or at minimum on
   simulator that it resolves. If it doesn't, degrade by `#if watchOS → dlopen`
   fallback (with known Catalyst-style rewriting loss, which watchOS doesn't
   need).
2. **`SOFT_LINK_FRAMEWORK_FOR_HEADER` namespace parameter semantics**. WebKit
   uses this for cross-TU linking in header/source splits. We should verify
   our 1:1 rename preserves the expansion identifiers exactly.
3. **LGPL-2.1 vs project license**. Since the macro layer is adapted from
   WebKit (LGPL-2.1), OpenSoftLinking either must remain LGPL-2.1 compatible,
   or the macro file ships as a separately-licensed component with clear
   attribution. Resolve before shipping.

## 9. Appendices

### 9.1 Key IDA addresses referenced

| Symbol | Address | Notes |
|---|---|---|
| `_sl_dlopen` | `0x1903165d8` | 616 bytes |
| `_sl_dlopen_audited` | `0x1903165d0` | 8 bytes (thunk) |
| `_sl_dlopen.cold.1` | `0x190316840` | `__assert_rtn` caller |
| `dlopen_from` import | `0x1903168ac` | dyld SPI |
| log format string | `0x190316968` | `"SoftLinking client failed to load dependency: %{public}s"` |
| `SoftLinking.c` string | `0x1903169ac` | assert source file name |
| `errorMessage` string | `0x1903169ba` | assert description |
| Version marker | `0x190316920` | `@(#)PROGRAM:SoftLinking PROJECT:SoftLinking-71` |
| `__const` descriptor base | `0x190316920` | 72 bytes |

### 9.2 ARM64 instruction landmarks in `_sl_dlopen`

| Address | Instruction | Meaning |
|---|---|---|
| `0x1903165d8` | `PACIBSP` | sign LR into frame |
| `0x1903165fc` | `MOV X24, X30` | save LR (caller return address) |
| `0x190316628` | `XPACI X21` | strip PAC from caller address |
| `0x190316630` | `MOV W1, #0x101` | RTLD_LAZY \| RTLD_FIRST |
| `0x190316638` | `BL _dlopen_from` | fast-path call |
| `0x19031677c` | `BL _strlcat` | `"\n"` separator concat |
| `0x190316834` | `RETAB` | authenticate LR and return |
| `0x190316838` | `BL __sl_dlopen.cold.1` | malloc-failed assert |

### 9.3 Reference

- Apple SoftLinking.framework macOS 26.4 @ dyld_shared_cache (internal tag
  `SoftLinking-71`).
- WebKit open-source macro reference:
  <https://github.com/WebKit/WebKit/blob/main/Source/WTF/wtf/cocoa/SoftLinking.h>.
- MacCatalyst architecture research (private, same author):
  `/Volumes/Code/Personal/UIFoundation/Researchs/MacCatalyst-Architecture-Research.md`,
  §9 SoftLinking.
- dyld `dlopen_from` signature:
  `extern void *dlopen_from(const char *path, int mode, const void *callerAddress);`
  (libdyld export, macOS 10.15+ / iOS 13+).
