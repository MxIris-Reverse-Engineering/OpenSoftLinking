# OpenSoftLinking

An open-source reimplementation of Apple's private
`SoftLinking.framework` (internally tagged `SoftLinking-71` on macOS
26.4), plus the full WebKit-derived `SOFT_LINK_*` macro suite
re-prefixed `OPEN_SOFT_LINK_*`.

## What it does

Loads Apple frameworks at runtime with fallback paths and
Catalyst-aware path resolution (delegated to dyld via
`dlopen_from`). Provides macros to soft-link frameworks, classes,
functions, pointers, constants, and variables the same way UIKit,
AVFoundation, and WebKit do internally.

## Requirements

- Swift Package Manager (swift-tools 6.3+)
- macOS 10.15+ / iOS 13+ / tvOS 13+ / watchOS 6+ / visionOS 1+ / Mac Catalyst 13+

## Installation

```swift
dependencies: [
    .package(url: "https://github.com/<owner>/OpenSoftLinking", from: "0.1.0"),
]
```

In a C / Objective-C source file:

```objc
#import <OpenSoftLinking/OpenSoftLinking.h>

OPEN_SOFT_LINK_FRAMEWORK(UIKit)
OPEN_SOFT_LINK_CLASS(UIKit, UIApplication)
OPEN_SOFT_LINK_POINTER(UIKit, UIApp, UIApplication *)

// Later…
Class appClass = getUIApplicationClass();
```

Swift consumers bridge via a bridging header:

```c
// MyBridgingHeader.h
#import <OpenSoftLinking/OpenSoftLinking.h>
OPEN_SOFT_LINK_FRAMEWORK(UIKit)
```

## Runtime API

```c
void *_osl_dlopen(const char *const *paths, char **errorMessage);
void *_osl_dlopen_audited(const char *const *paths);
```

`paths` is a NULL-terminated array of candidate framework paths. On
success, returns the dyld handle of the first successful load. On
failure with `errorMessage` non-NULL, writes a `\n`-joined string of
per-path `dlerror()` messages that the caller must `free()`.

## Relation to Apple's SoftLinking

| Aspect | Apple SoftLinking.framework | OpenSoftLinking |
|---|---|---|
| Symbol names | `_sl_dlopen`, `_sl_dlopen_audited` | `_osl_dlopen`, `_osl_dlopen_audited` |
| Macro names | N/A (internal) | `OPEN_SOFT_LINK_*` |
| dlopen flags | `RTLD_LAZY \| RTLD_FIRST` | Same |
| Caller-aware dyld lookup | Via `dlopen_from` + LR | Same |
| Catalyst path resolution | Delegated to dyld | Same |
| Error aggregation | `\n`-joined strings + `os_log_info` | Same |
| License | Proprietary | LGPL-2.1 |

## License

LGPL-2.1. The `SoftLinking.h` macro suite is derived from WebKit's
`Source/WTF/wtf/cocoa/SoftLinking.h` (LGPL-2.1). See `LICENSE`.

## Known limitations

- `calloc` failure on the slow-path (error aggregation) triggers
  `abort()`, matching Apple's `__assert_rtn` behavior; not unit
  tested.
- `dlopen_from` is a stable SPI since macOS 10.15 / iOS 13; no formal
  guarantee against future removal.
- Byte-equivalent parity with Apple's SoftLinking binary is not a
  goal — only behavioral parity.
