# VanGUI 2026 Release — build record

Public headers and precompiled libraries. Nothing in this folder is generated at
consume time: copy it next to your project and link.

## Toolchain

| | |
|---|---|
| Compiler | MSVC 19.x, Visual Studio 18 (2026) Community, 18.7.11911.148 |
| Generator | Visual Studio 18 2026 |
| CMake | 4.2.2 |
| Built by | `build_sdk.bat` in the VanGUI source tree (default arguments) |
| Date | 2026-09-11 |

## Library matrix

Both archives are required. Link `vangui.lib` **and** `vangui_suite.lib`: the core
carries the guarded `NewFrame` animation hook and the suite provides the symbol it
calls, plus every enhancement module.

| Path | Arch | Config | Size |
|---|---|---|---|
| `lib/win-x64/Release/vangui.lib` | x64 | Release | 3.29 MB |
| `lib/win-x64/Release/vangui_suite.lib` | x64 | Release | 2.46 MB |
| `lib/win-x64/Debug/vangui.lib` | x64 | Debug | 5.02 MB |
| `lib/win-x64/Debug/vangui_suite.lib` | x64 | Debug | 6.41 MB |
| `lib/win-x86/Release/vangui.lib` | x86 | Release | 1.94 MB |
| `lib/win-x86/Release/vangui_suite.lib` | x86 | Release | 1.66 MB |
| `lib/win-x86/Debug/vangui.lib` | x86 | Debug | 4.06 MB |
| `lib/win-x86/Debug/vangui_suite.lib` | x86 | Debug | 5.06 MB |

Architecture was confirmed by reading the COFF machine field out of each archive's
object members, not inferred from the folder name.

## CRT

**Dynamic CRT** (`/MD` Release, `/MDd` Debug), which is the default documented in
`lib/README.md` for an application-linked toolkit. Your application's runtime
library must match these, or the link fails with `LNK2038`.

> This differs from VanHooks and VanGFX, which ship static-CRT (`/MT`) libraries.
> If you need VanGUI static too, rebuild the source with `build_sdk.bat CRT=MT`.

## The frozen config

These are the everything-enabled build. The full `VANGUI_ENABLE_*` set is baked
into the libraries, and **a consumer has to define the same set** or the public
headers expose the zero-cost shims instead of the real declarations — `van.h`
will not compile without them, because its `van::` wrappers call suite functions
unconditionally.

`cmake/VanGUISDKConfig.cmake` applies that macro set for you:

```cmake
list(APPEND CMAKE_PREFIX_PATH "path/to/2026Release/cmake")
find_package(VanGUISDK REQUIRED)
target_link_libraries(my_app PRIVATE VanGUI::suite)
```

Include with the directory prefix, since the include root is `include/`:

```cpp
#include <vangui/vangui.h>
#include <vangui/van.h>
```

Do not change layout-affecting defines in `include/vangui/vanconfig.h` against
these binaries.

## Verified

A consumer project outside this tree was configured with `find_package(VanGUISDK)`,
linked against `VanGUI::suite`, built Release x64 and run. It reported:

```
vangui 1.92.9 WIP | font atlas 512x128 | draw lists 1 | vtx 76
```

so the headers are self-contained, the archives resolve the symbols a consumer
calls, and a frame renders.

## Other targets

ARM64, Linux, macOS, a custom `vanconfig.h`, à-la-carte feature selection, and the
C++20 `import vangui;` module target all require a build from the VanGUI source
repository. See `README.md` and `lib/README.md`.
