# VanGUI Suite — Functions Guide

The complete public surface of the VanGUI SDK: the immediate-mode core, the
fluent `van::` facade, and every enhancement pillar with the enable-macro it
rides on. Header paths are relative to `include/vangui/`.

- **Core** is the Dear ImGui API, renamed `ImGui→VanGui`, in namespace `VanGui`.
- **Suite** functions live in `VanGui::` (and some in `van::`), and are declared
  only when their `VANGUI_ENABLE_*` macro is defined. In this SDK the whole set
  is defined for you by `VanGUI::suite` / `VanGUISDKConfig.cmake` — see the
  frozen-config contract in the README. When a macro is *off* (source builds),
  the same headers declare `inline` no-op shims, so call sites compile unchanged
  and cost nothing.

---

## 1. Core

```cpp
#include <vangui/vangui.h>       // everything in namespace VanGui::
```

Identical in shape to Dear ImGui 1.92.x: `Begin/End`, `Button`, `SliderFloat`,
`InputText`, `BeginTable`, docking, draw lists, fonts, `ShowDemoWindow()`, etc.
Types are `VanVec2`, `VanVec4`, `VanColor`, `VanGuiID`, `VanGuiWindowFlags`, …
Compile-time configuration is in `vanconfig.h` (frozen for the prebuilt libs).

---

## 2. The `van::` facade  (header-only)

```cpp
#include <vangui/van.h>
using namespace van;
```

RAII scopes, fluent widget results, references instead of pointers, and
designated-initializer option structs. Zero per-frame allocation; wraps the core
without reimplementing any widget.

### Scopes — `if (auto s = van::xxx(...))`
`window(title, WindowOpts)` · `child(id, ChildOpts)` · `group()` ·
`disabled(on)` · `popup(id)` · `modal(title,&open)` · `menu(label)` ·
`menu_bar()` · `main_menu_bar()` · `tab_bar(id)` · `tab(label)` · `tooltip()` ·
`combo(label,preview)` · `list_box(label)` · `table(id,cols,TableOpts)` ·
`tree(label)` · `style_color(idx,col)` · `style_var(idx,val)` · `id(...)` ·
`item_width(w)` · `indent()`.

The scope's `bool` is the open/visible state; it closes itself on block exit
(`End`/`Pop` runs unconditionally where the core requires it, conditionally
otherwise).

### Widgets — return a fluent `Response`
`button(label, ButtonOpts)` · `small_button` · `checkbox(label, bool&)` ·
`radio` · `selectable` · `menu_item` · `slider(float&|int&, …)` ·
`drag(...)` · `input(float&|int&)` · `input_text(char*/std::string&)` ·
`color_edit(Vec4&|Color&)` · `combo(label,int&,items,count)`.

`Response` methods: `clicked()` / `changed()`, `hovered()`, `active()`, and the
chainable `on_click(fn)`, `on_change(fn)`, `on_hover(fn)`, `tooltip(text)`.

```cpp
button("Save", { .primary = true }).on_click(save).tooltip("write to disk");
checkbox("V-Sync", cfg.vsync);
slider("Volume", cfg.volume, 0.f, 100.f, "%.0f%%");
```

### Layout & text
`row([&]{…})` / `column([&]{…})` auto-arrange `van::` widgets (no manual
`SameLine`); `grid(columns, count, [&](int i){…})`; `separator`, `spacing`,
`same_line`, `new_line`, `dummy`; `text`, `text_colored`, `text_disabled`,
`heading`, `bullet`.

### `std::expected` helpers (when `<expected>` is available)
`on_value(exp, fn)` · `on_error(exp, fn)` · `value_or(exp, fallback)`.

The facade also exposes thin `van::` wrappers for the pillars and the Tier 1–3
modules below whenever the matching module is linked — e.g. `toast_*`, `spinner`,
`list_view`, `node_graph`, and for the tooling modules `property_grid` /
`property(...)`, `curve_editor` / `gradient_editor`, `date_picker` / `calendar` /
`time_picker`, `markdown`, `rich_text`, `plot` / `plot_values`, `timeline` /
`timeline_track`, `data_grid`, `banner` / `snackbar`, `perf_hud`, `reorder_item`,
`tr(...)`, plus the `van::Console` / `van::HexEditor` / `van::Repl` aliases. RAII
scope wrappers (`property_grid`, `property_category`, `plot`, `timeline`) close
themselves like the other `van::` scopes.

---

## 3. Enhancement pillars

Each entry lists the macro (defined for you in this SDK), the header, and the
primary entry points. The header is authoritative for exact signatures.

### 3.1 Animation substrate — `VANGUI_ENABLE_ANIM`  ·  `misc/vangui_anim.h`
The one module with a core hook (`Anim::NewFrameUpdate()` in `VanGui::NewFrame`).
Pure easing + ID-keyed tween/spring state; no per-frame allocations at steady
state; evicts idle slots.
```cpp
float   VanGui::Anim::AnimFloat(VanGuiID id, float target, const VanAnimParams& = {});
VanVec4 VanGui::Anim::AnimColor(VanGuiID id, VanVec4 target, const VanAnimParams& = {});
float   VanGui::Anim::AnimBool (VanGuiID id, bool open,      const VanAnimParams& = {});
float   VanGui::Anim::SpringFloat(VanGuiID id, float target, const VanSpringParams& = {});
float   VanGui::Anim::Ease(VanEasing fn, float t);   // constexpr, noexcept
bool    VanGui::Anim::IsAnimating();                 // drives idle/power integration
```

### 3.2 Loading effects — `VANGUI_ENABLE_LOADING`  ·  `misc/vangui_loading.h`
`Spinner`, `SpinnerDots`, `SpinnerBars`, `IndeterminateBar`, `ProgressRing`,
`Skeleton`, `SkeletonText`, `BeginLoadingOverlay`/`EndLoadingOverlay`. Spinners
are stateless (phase from global time); only interpolating widgets touch anim.
Facade: `van::spinner(...)`, `van::progress_ring(...)`, `van::loading_overlay(...)` (RAII).

### 3.3 Notifications — `VANGUI_ENABLE_NOTIFY`  ·  `misc/vangui_notify.h`
Toasts with progress bars and four anchor corners.
```cpp
VanGui::NotifyInfo/NotifySuccess/NotifyWarning/NotifyError("%s", msg);
VanGui::RenderNotifications();        // called for you by RenderExtras()
```
Facade: `van::toast_info/success/warning/error(msg)`, `van::toast_on_error(exp)`.

### 3.4 Theme engine — `VANGUI_ENABLE_THEME_ENGINE`  ·  `misc/vangui_theme_engine.h`
12 semantic tokens, animated `TransitionToTheme`, push/pop scopes, file
hot-reload. `RenderThemeTransition()` runs from `NewFrameExtras()`.
`GenerateTheme(accent, dark)` (`van_kit.h` / `vangui_theme_gen.h`) builds a full
token set from one accent color (pure math, unit-testable).

### 3.5 Named themes — `VANGUI_ENABLE_THEMES`  ·  `misc/vangui_themes.h`
Presets (dark, light, classic, dracula, nord, monokai, gruvbox) + save/load.
`VanGui::LoadTheme(id|name)`, `SaveThemeToFile`, `LoadThemeFromFile`.
Facade: `van::load_theme(...)`, `van::save_theme_to_file(...)`.

### 3.6 Stylesheet (.vss) — `VANGUI_ENABLE_STYLESHEET`  ·  `misc/vangui_style_sheet.h`
Declarative QSS-like front-end over the theme engine. Parser errors return
`std::expected` with line numbers; hot-reload cross-fades via the theme engine.
```cpp
std::expected<void,const char*> VanGui::LoadStyleSheet(const char* path);
std::expected<void,const char*> VanGui::LoadStyleSheetFromMemory(const char*, size_t);
void VanGui::PollStyleSheetChanges();   // from NewFrameExtras()
```
(The `.vss` grammar implementation ships compiled — the parser `.inl` stays in
the private source.)

### 3.7 Standard dialogs — `VANGUI_ENABLE_DIALOGS`  ·  `misc/vangui_dialogs.h`
```cpp
VanDialogResult VanGui::MessageBox(title, message, buttons = Ok);
std::expected<std::string,VanDialogResult> VanGui::GetOpenFileName(filters = nullptr);
std::expected<std::string,VanDialogResult> VanGui::GetSaveFileName(default_name, filters);
```
Facade: `van::confirm(title)`.

### 3.8 Layout helpers — `VANGUI_ENABLE_LAYOUT`  ·  `misc/vangui_layout.h`
`BeginHBox`/`BeginVBox`/`BeginGrid`/`EndBox`, `Stretch(weight)`. Pure cursor
math, no retained layout tree.

### 3.9 Signals / slots — `VANGUI_ENABLE_SIGNALS`  ·  `misc/vangui_signals.h` (header-only)
```cpp
VanGui::VanSignal<Args...>;         // emit(...), connect(slot)->VanConnection, disconnect_all()
VanGui::VanConnection;              // move-only, auto-disconnects in destructor (RAII)
```
Instance-based, no global state, no codegen. With `VANGUI_ENABLE_THREAD` also
linked, cross-thread `connect_queued()` activates.

### 3.10 Model/View — `VANGUI_ENABLE_VIEWS`  ·  `misc/vangui_views.h`
`VanListModel` / `VanTreeModel` (struct-of-function-pointers), virtualized
through the core list clipper (a million rows render O(visible)).
`BeginListView`/`EndListView`, `BeginTreeView`/`EndTreeView`.
Facade: `van::list_view(...)`, `van::tree_view(...)`.

### 3.11 Actions registry — `VANGUI_ENABLE_ACTIONS`  ·  `misc/vangui_actions.h`
Retained metadata unifying menu items, global shortcuts, and palette entries
behind one `Action { Id, Label, Category, Icon, Shortcut, Run, UserData,
Enabled }`. `Register`/`Invoke`/`Find`/`SetEnabled`/`ProcessShortcuts()`.
Function-pointer callbacks — no `std::function`, no allocations at steady state.

### 3.12 Command palette — `VANGUI_ENABLE_COMMAND_PALETTE`  ·  `misc/vangui_command_palette.h`
Ctrl+K registry with fuzzy subsequence match and animated open.
`RenderCommandPalette()` runs from `RenderExtras()`.

### 3.13 Node graph — `VANGUI_ENABLE_NODE_GRAPH`  ·  `misc/vangui_node_graph.h`
`CreateNodeGraphContext`/`Destroy…`, `BeginNodeGraph`/`EndNodeGraph`,
`BeginNode`/`EndNode`, `NodeTitle`, `NodePin`, `NodeLink`, `IsLinkCreated/Deleted`,
`IsNodeSelected`. Facade mirrors each under `van::`.

### 3.14 Extended widgets — `VANGUI_ENABLE_WIDGETS_EXT`  ·  `misc/vangui_widgets_ext.h`
`Toggle`, `SegmentedControl`, `Chip`, `Badge`, `Breadcrumb`, `Stepper`,
`StarRating`, `SearchBox`. Header-only companions in `misc/vangui_widgets_pack.h`
(no macro): `Segmented`, `ToggleSwitch`, `Chips`, `RatingStars`, `Sparkline`,
`Breadcrumbs`, `Debounced`, `Throttled`, `NotificationCenter`.

### 3.15 Charts — `VANGUI_ENABLE_CHARTS`  ·  `misc/vangui_charts.h`
`Sparkline`, `BarChart`, `Gauge`. Facade: `van::sparkline/bars/gauge`.

### 3.16 Feedback — `VANGUI_ENABLE_FEEDBACK`  ·  `misc/vangui_feedback.h`
`AnimatedValue`, `RippleButton`, `ElevatedButton` (ride the anim substrate).

### 3.17 Forms — `VANGUI_ENABLE_FORMS`  ·  `misc/vangui_forms.h`
`BeginForm`/`EndForm`, `FormRow`, `FieldError`, `FieldHint`, `PushInvalid`/
`PopInvalid`, `ValidNotEmpty`, `ValidInRange`. Facade: `van::form(...)` (RAII),
`van::invalid_if(bool)`.

### 3.18 Panels / app chrome — `VANGUI_ENABLE_PANELS`  ·  `misc/vangui_panels.h`
`Splitter`, `AccordionSection`/`AccordionEnd`, `BeginStatusBar`/`EndStatusBar`,
`BeginToolbar`/`EndToolbar`, `ToolbarSeparator`.

### 3.19 Toolbar builder — `VANGUI_ENABLE_TOOLBAR`  ·  `misc/vangui_toolbar.h`
Declarative toolbar with automatic overflow into a `">>"` popup:
`tb.Button/Toggle/Sep/Text/Render()`.

### 3.20 Drop zone — `VANGUI_ENABLE_DROPZONE`  ·  `misc/vangui_dropzone.h`
Bordered drop target. Internal payloads via `BeginDragDropTarget`; OS file drops
via a backend-called `NotifyFilesDropped(paths, count)` (e.g. from
`glfwSetDropCallback`). `HasPendingDrop()`.

### 3.21 Wizard / stepper — `VANGUI_ENABLE_WIZARD`  ·  `misc/vangui_wizard.h`
Multi-page modal: numbered header stepper, Back/Next/Cancel/Finish, per-step
`SetStepValid(bool)` gate, terminal `WizardResult { None, Cancelled, Finished }`.

### 3.22 Shortcuts — `VANGUI_ENABLE_SHORTCUTS`  ·  `misc/vangui_shortcuts.h`
Keyboard-shortcut registry; optionally drives the command palette.

### 3.23 .vui loader — `VANGUI_ENABLE_VUI`  ·  `misc/vangui_vui.h`
Declarative UI loader (parser + renderer over the core widgets). Parser `.inl`
ships compiled — private source.

### 3.24 Thread pool — `VANGUI_ENABLE_THREAD`  ·  `misc/vangui_thread.h`
`InitThreadPool`/`ShutdownThreadPool`, `PostToMainThread`, `Async`,
`AsyncFuture<T>`, `VanFuture<T>`, `DrainMainThreadQueue()` (from
`NewFrameExtras()`). Facade: `van::init_threads/async/async_future/post`. Without
the macro, everything runs synchronously with identical signatures.

### 3.25 Theme editor — `VANGUI_ENABLE_THEME_EDITOR`  ·  `misc/vangui_theme_editor.h`
`ShowThemeEditor(&open)`, `ShowThemeEditorWindow(&open)`.

---

## 4. Header-only utilities (`van_kit.h`)

Available whenever their headers are present — no link dependency.

- **`VanGui::VanUndoStack<T>`** / **`VanCommandStack`** — snapshot and
  command-based undo/redo (`misc/vangui_undo.h`).
- **`VanGui::VanSettings`** — typed INI key/value store with two-way `bind`/
  `pull`/`push` (`misc/vangui_settings.h`).
- **`VanGui::GenerateTheme(accent, dark)`** — semantic theme from one color
  (`misc/vangui_theme_gen.h`).
- **`van::Inspect(title, obj)` + `van::Reflector`** — auto-inspector that builds
  an editing UI from a struct's field types via an ADL `van_describe`, or the
  `VAN_REFLECT_BEGIN/VAN_FIELD/VAN_GROUP/VAN_REFLECT_END` macros
  (`misc/vangui_reflect.h`).
- **`vgu::` RAII scope guards** — `misc/vangui_scoped.h` (no macro): a guard for
  every Begin/End pair.
- **`vgu::` shorthand** — `misc/vangui_shorthand.h`: `Shortcut`, `Section`,
  `SearchBox`, `Kbd`, `HelpMarker`, `Property/Bind`, and subsystem-gated
  `Toast::*`, `Confirm/AskYesNo`, `Async` helpers.

---

## 5. Per-frame drivers (`misc/vangui_enhance.h`)

```cpp
VanGui::NewFrame();
VanGui::NewFrameExtras();     // theme transition + .vss reload + thread drain
// ... your UI ...
VanGui::RenderExtras();       // toasts + command palette
VanGui::Render();

bool VanGui::EnhanceWantsRedraw();     // true while animating or work pending
void VanGui::ShowEnhanceMetrics(&open);// anim pool + thread pool metrics
```

All fully guarded — with a module unlinked, its step is compiled out.

---

## 6. Tooling & application modules (Tier 1–3)

Later additions aimed at real tools and desktop-app surfaces. Same rules as the
pillars: each is gated by a `VANGUI_ENABLE_*` macro (defined for you in this
SDK), keyed by `VanGuiID`, no per-frame heap at steady state, draw-list based.
Header is authoritative for exact signatures.

### 6.1 Log / console — `VANGUI_ENABLE_CONSOLE` · `misc/vangui_console.h`
Retained, filterable, virtualized log view. `VanConsole` object owns its buffer;
`AddLog(level, fmt, ...)`, `Draw(title,&open)` / `DrawContents()`, level mask +
search filter, auto-scroll. Rendering is O(visible rows) via `VanGuiListClipper`.

### 6.2 Property grid — `VANGUI_ENABLE_PROPERTY_GRID` · `misc/vangui_property_grid.h`
Two-column inspector over core tables: `BeginPropertyGrid`/`EndPropertyGrid`,
collapsible `BeginPropertyCategory`, and typed rows `PropertyFloat/Int/Bool/
Color/Text/Combo` + read-only `PropertyLabel`. Complements `van_kit.h`'s reflect.

### 6.3 Curve & gradient editor — `VANGUI_ENABLE_CURVE_EDITOR` · `misc/vangui_curve_editor.h`
Draggable control points: `CurveEditor(...)` + `CurveValue(...)` (piecewise
linear), `GradientEditor(...)` + `GradientSample(...)` + display-only `GradientBar`.
Double-click adds, right-click removes, double-click a stop opens a color picker.

### 6.4 Date/time picker — `VANGUI_ENABLE_DATETIME` · `misc/vangui_datetime.h`
`CalendarWidget`, `DatePicker` (calendar popup), `TimePicker`, plus pure helpers
`VanDaysInMonth` / `VanDayOfWeek` (Sakamoto). Structs `VanDate` / `VanTime`.

### 6.5 Markdown — `VANGUI_ENABLE_MARKDOWN` · `misc/vangui_markdown.h`
`Markdown(text, on_link, ud)` — headings, bold/italic, inline + fenced code,
lists, blockquotes, rules, `[text](url)` links. Font-independent; for tooltips,
About boxes, help. Off-shim renders the raw text.

### 6.6 Hex editor — `VANGUI_ENABLE_HEX_EDITOR` · `misc/vangui_hex_editor.h`
`VanHexEditor`: address/hex/ASCII columns, `VanGuiListClipper`-virtualized,
click-to-edit bytes, highlight range. `Draw(...)` / `DrawContents(...)`.

### 6.7 Real-time plot — `VANGUI_ENABLE_PLOT` · `misc/vangui_plot.h`
`BeginPlot`/`EndPlot`, `PlotXY`, `PlotValues` (x=index), `PlotHLine` — framed
axes, grid, hover crosshair with readout. One active plot at a time; caller owns
the data (maintain your own ring for scrolling telemetry).

### 6.8 Timeline / sequencer — `VANGUI_ENABLE_TIMELINE` · `misc/vangui_timeline.h`
`BeginTimeline` (draggable playhead) / `TimelineTrack` (draggable keyframes;
double-click a lane to add, right-click a key to remove) / `EndTimeline`.

### 6.9 Data grid — `VANGUI_ENABLE_DATA_GRID` · `misc/vangui_data_grid.h`
`DataGrid(id, cols, col_count, model, size, &selected)` over a callback model:
filter box + click-to-sort + selection, virtualized via a per-id cached
filtered/sorted index (O(visible) steady state). `VanDataGridColumn` /
`VanDataGridModel`.

### 6.10 Banners & snackbars — `VANGUI_ENABLE_BANNERS` · `misc/vangui_banners.h`
In-flow `Banner(id, type, text, action, &open)` (severity-colored, optional
action/close) and a managed `Snackbar(text, action, duration)` queue drawn by
`RenderSnackbars()` (returns a clicked snackbar id). Companion to `notify`.

### 6.11 Command console / REPL — `VANGUI_ENABLE_REPL` · `misc/vangui_repl.h`
`VanRepl`: scrolling output + input line with history (Up/Down) and Tab
completion. `AddOutput`, `SetCompletions`, `Draw(id, size, on_exec, ud)`.

### 6.12 Rich text — `VANGUI_ENABLE_RICHTEXT` · `misc/vangui_richtext.h`
`RichText("<c=RRGGBB>..</c> <b>..</b> <i>..</i> <u>..</u> <code>..</code>")` +
`CalcRichTextSize`. Font-independent (faux-bold, tinted italic). For legends,
labels, status lines.

### 6.13 Perf HUD — `VANGUI_ENABLE_PERF_HUD` · `misc/vangui_perf_hud.h`
`PerfHUD(&open)` corner overlay / `PerfHUDContents()` — FPS, frame-time, min/max,
a rolling graph and a 60 fps reference line. Fixed ring, no per-frame allocation.

### 6.14 Drag-to-reorder — header-only · `misc/vangui_reorder.h`
`ReorderItem(payload_type, index, &from, &to)` right after an item turns it into
a drag source + drop target; `ReorderApply(arr, count, from, to)` performs the
move. No macro gate.

### 6.15 Localization-lite — header-only · `misc/vangui_i18n.h`
`VanStringCatalog` (`Set`/`Tr`/`Has`/`LoadFromMemory`) + process-wide
`GlobalCatalog()` and `Tr("key")`. Label swapping — not full i18n (RTL/shaping
stay out of scope). No macro gate.
