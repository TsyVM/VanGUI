<div align="center">
<img src="vangui-logo.png" width="600" alt="VanGUI"/>
    
<p><em>Bloat-free Immediate Mode GUI for C++23</em></p>

[![License: MIT](https://img.shields.io/badge/License-MIT-D2B48C?style=for-the-badge&labelColor=1C1008)](LICENSE.txt)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-D2B48C?style=for-the-badge&labelColor=1C1008&logo=cplusplus&logoColor=D2B48C)](https://en.cppreference.com/w/cpp/23)
[![Windows](https://img.shields.io/badge/Windows-x86%20%7C%20x64-D2B48C?style=for-the-badge&labelColor=1C1008&logo=windows&logoColor=D2B48C)](libs/)
[![Linux](https://img.shields.io/badge/Linux-x64%20%7C%20ARM64-D2B48C?style=for-the-badge&labelColor=1C1008&logo=linux&logoColor=D2B48C)](libs/)
[![macOS](https://img.shields.io/badge/macOS-x64%20%7C%20ARM64-D2B48C?style=for-the-badge&labelColor=1C1008&logo=apple&logoColor=D2B48C)](libs/)
[![TeamVanilla](https://img.shields.io/badge/Team-TeamVanilla-D2B48C?style=for-the-badge&labelColor=1C1008)](https://www.teamvanilla.org/)

<br/>

[![Stars](https://img.shields.io/github/stars/tsyvm/vangui?style=for-the-badge&color=D2B48C&labelColor=1C1008)](../../stargazers)
[![Issues](https://img.shields.io/github/issues/tsyvm/vangui?style=for-the-badge&color=D2B48C&labelColor=1C1008)](../../issues)
[![Last Commit](https://img.shields.io/github/last-commit/tsyvm/vangui?style=for-the-badge&color=D2B48C&labelColor=1C1008)](../../commits)

</div>

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:1C1008,50:6B4226,100:1C1008&height=3"/>

VanGUI is a **bloat-free graphical user interface library for C++23**. It outputs optimized vertex buffers that you can render anytime in your 3D-pipeline-enabled application. It is fast, portable, renderer-agnostic, and self-contained (no external dependencies).

VanGUI is designed to **enable fast iterations** and to **empower programmers** to create content creation tools and visualization / debug tools. It favors simplicity and productivity and lacks certain features found in higher-level libraries (full RTL, bidirectional text, accessibility).

VanGUI is distributed as **precompiled static libraries with public API headers**. Drop the headers and the matching `.lib` / `.a` into your project and link — no source compilation required.

<div align="center">

### Contents

[Quick Start](#-quick-start) · [Installation](#-installation) · [Enhancement Suite](#-enhancement-suite) · [C++23 Modules](#-c23-modules) · [Demo](#-demo) · [Requirements](#-requirements) · [License](#-license)

</div>

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:1C1008,50:6B4226,100:1C1008&height=3"/>

## ⚡ Quick Start

```cpp
#include <vangui/vangui.vg>   // or: import vangui;

VanGui::Text("Hello, world %d", 123);
if (VanGui::Button("Save"))
    MySaveFunction();
VanGui::InputText("string", buf, VAN_COUNTOF(buf));
VanGui::SliderFloat("float", &f, 0.0f, 1.0f);
```

Or use the fluent `van::` facade from `van.vg`:

```cpp
#include <vangui/van.vg>

van::window("My Tool", [&] {
    van::text("value = {}", my_value);
    if (van::button("Reset")) my_value = 0;
});
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:1C1008,50:6B4226,100:1C1008&height=3"/>

## 📦 Installation

VanGUI is distributed as **precompiled static libraries with public headers**. Copy `include/` into your project, link against the `.lib` for your target, and you're done. All libraries use a static CRT — no Redistributable required.

### Library paths

| Target | Path |
|---|---|
| **Windows x64 — Release** | `libs/MSVC/win-x64/Release/vangui.lib` |
| **Windows x86 — Release** | `libs/MSVC/win-x86/Release/vangui.lib` |

### MSVC project setup

1. **Additional Include Directories** → `include\`
2. **Additional Library Directories** → `libs\MSVC\win-x64\Release\`
3. **Additional Dependencies** → `vangui.lib`
4. **Runtime Library** → `Multi-threaded (/MT)` for Release, `Multi-threaded Debug (/MTd)` for Debug

### CMake (drop-in)

```cmake
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(VANGUI_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/MSVC/win-x64")
    else()
        set(VANGUI_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/MSVC/win-x86")
    endif()
    set(VANGUI_LIB_NAME "vangui.lib")
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(VANGUI_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/Clang/macos-arm64")
    else()
        set(VANGUI_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/Clang/macos-x64")
    endif()
    set(VANGUI_LIB_NAME "libvangui.a")
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(VANGUI_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/GCC/linux-arm64")
    else()
        set(VANGUI_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/GCC/linux-x64")
    endif()
    set(VANGUI_LIB_NAME "libvangui.a")
endif()

add_library(VanGUI::vangui STATIC IMPORTED)
set_target_properties(VanGUI::vangui PROPERTIES
    IMPORTED_LOCATION_RELEASE "${VANGUI_LIB_DIR}/Release/${VANGUI_LIB_NAME}"
    IMPORTED_LOCATION_DEBUG   "${VANGUI_LIB_DIR}/Debug/${VANGUI_LIB_NAME}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(my_target PRIVATE VanGUI::vangui)
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:1C1008,50:6B4226,100:1C1008&height=3"/>

## 🧩 Enhancement Suite

VanGUI ships a set of optional header-only extensions under `include/vangui/`. Just `#include` what you need — no extra build steps.

### `van.vg` — Fluent `van::` facade

A thin namespace alias over `VanGui::` for callsite brevity. No new symbols, no overhead, no state.

```cpp
#include <vangui/van.vg>

van::window("My Tool", [&] {
    van::text("value = {}", my_value);
    if (van::button("Reset")) my_value = 0;
});
```

### `shorthand.vg` — One-liner sugar (`vgu::`)

Common multi-line patterns collapsed to single calls. Zero allocations, zero retained state.

```cpp
#include <vangui/shorthand.vg>

if (vgu::Shortcut("Ctrl+S")) SaveDocument();
vgu::HelpMarker("Tooltip text here.");
vgu::SearchBox(query_buf, sizeof(query_buf), "Search commands...");
if (auto s = vgu::Section("Advanced")) { /* ... */ }
vgu::Kbd("Ctrl", "Shift", "P");
vgu::EmptyState("No items", "Add one to get started", "Add item", []{ AddItem(); });
```

### `widgets_pack.vg` — Extra widgets (`vgu::`)

Small composable widgets not in the core API. Zero allocations at steady state.

| Widget | Description |
|---|---|
| `vgu::Segmented(labels, count, &selected)` | Pill-shaped horizontal tab strip |
| `vgu::ToggleSwitch(label, &v)` | Material-style on/off toggle |
| `vgu::Chip(label, closable, &alive)` | Bordered tag with optional × |
| `vgu::Chips(labels, count)` | Read-only chip row |
| `vgu::RatingStars("##r", &rating, max)` | Click-to-rate star row |
| `vgu::Sparkline("##s", values, count, ...)` | Inline mini line chart |
| `vgu::Breadcrumbs(items, count, &clicked)` | Click-to-navigate breadcrumb row |
| `vgu::NotificationCenter("Notifications")` | Read-only notification tray listing |

### `notify.vg` — Toast notifications

Overlay toast system rendered once per frame after all other windows.

```cpp
#include <vangui/notify.vg>

// Post a toast
VanNotifyID id = VanGui::NotifyInfo("Loaded %d items", count);
VanGui::InsertNotification(VanNotifyType_Error, 5000.f, "Failed: %d", err);

// Update a live progress bar on an existing toast
VanGui::SetNotificationProgress(id, 0.75f);  // 0..1
VanGui::SetNotificationProgress(id, -1.f);   // hide bar

// Dismiss programmatically
VanGui::DismissNotification(id);

// Control position
VanGui::SetNotificationsPos(VanNotifyPos_TopRight);

// Render (call once per frame, after all other windows)
VanGui::RenderNotifications();
```

### `panels.vg` — Application chrome (`VANGUI_ENABLE_PANELS`)

Splitter, accordion, status bar, and toolbar — lightweight compositions over the core API.

```cpp
// Draggable splitter between two regions
VanGui::Splitter("##split", /*vertical=*/true, 4.f, &left_w, &right_w);

// Collapsible section with animated chevron
if (VanGui::AccordionSection("Advanced", &open)) { /* content */ }
VanGui::AccordionEnd();

// Viewport-pinned status bar
if (VanGui::BeginStatusBar()) {
    VanGui::Text("Ready");
    VanGui::EndStatusBar();
}
```

### `forms.vg` — Form layout & validation (`VANGUI_ENABLE_FORMS`)

Aligned label/field rows with inline error messages.

```cpp
VanGui::BeginForm("settings");
VanGui::FormRow("Name");    VanGui::InputText("##n", name, sizeof name);
if (name[0] == 0) VanGui::FieldError("Name is required");
VanGui::FormRow("Volume");  VanGui::SliderFloat("##v", &vol, 0, 100);
VanGui::EndForm();
```

### `dropzone.vg` — File drop zone (`VANGUI_ENABLE_DROPZONE`)

Bordered region that reacts to drag hover and reports dropped items. Supports both VanGUI internal drag-drop and OS-level file drops.

```cpp
const char* dropped[16]; int n = 0;
if (vgu::DropZone("Drop images here", dropped, 16, &n)) {
    for (int i = 0; i < n; ++i) LoadImage(dropped[i]);
}

// From a backend (SDL/GLFW/Win32) file-drop callback:
const char* paths[] = { "/tmp/a.png", "/tmp/b.jpg" };
vgu::NotifyFilesDropped(paths, 2);
```

### `signals.vg` — Signals & slots (`VANGUI_ENABLE_SIGNALS`)

RAII instance-based signal/slot system. Optionally thread-safe (`VANGUI_ENABLE_THREAD`).

```cpp
#define VANGUI_ENABLE_SIGNALS
#include <vangui/signals.vg>

VanSignal<void(int)> on_value_changed;
VanConnection conn = on_value_changed.connect([](int v) { printf("changed: %d\n", v); });
on_value_changed.emit(42);
// conn destructor auto-disconnects
```

### `loading.vg` — Loading & busy-state effects (`VANGUI_ENABLE_LOADING`)

Indeterminate spinners, progress rings, and overlay fades. Stateless for spinners (phase from global time); progress widgets use the animation substrate.

### `theme_gen.vg` — One-accent theme generation

Generate a full semantic theme from a single accent color. Pure color math; no VanGUI runtime calls.

```cpp
#include <vangui/theme_gen.vg>

VanThemeTokenSet t = VanGui::GenerateTheme(VanVec4(0.26f, 0.59f, 0.98f, 1.f), /*dark=*/true);
VanGui::ApplyTokenSet(t);
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:1C1008,50:6B4226,100:1C1008&height=3"/>

## 🔷 C++23 Modules

MSVC v143 and CMake 3.28+ only. Link `vangui_module` instead of `vangui`:

```cpp
import vangui;
VanGui::Text("Hello from a module!");
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:1C1008,50:6B4226,100:1C1008&height=3"/>

## 🎬 Demo

Call `VanGui::ShowDemoWindow()` to open the built-in interactive demo showcasing widgets, layouts, and extension features.

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:1C1008,50:6B4226,100:1C1008&height=3"/>

## 🛠️ Requirements

| | |
|---|---|
| **Compiler** | GCC 13+, Clang 17+, or MSVC 19.38+ with `/std:c++23` |
| **CMake** | 3.28+ (optional — drop-in linking works without CMake) |
| **Dependencies** | None — fully self-contained |

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:1C1008,50:6B4226,100:1C1008&height=3"/>

## 📄 License

VanGUI is licensed under the MIT License — see [LICENSE.txt](LICENSE.txt).

<div align="center">

<sub>Built and maintained by <a href="https://github.com/TsyVM">TsyVM</a> · <a href="https://www.teamvanilla.org/">TeamVanilla</a></sub>

<img width="100%" src="https://capsule-render.vercel.app/api?type=waving&color=0:6B4226,100:1C1008&height=80&section=footer"/>

</div>
