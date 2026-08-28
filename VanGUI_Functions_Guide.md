# VanGUI Functions Guide

Full API reference for VanGUI v1.92.9. All functions live in the `VanGui::` namespace (`#include <vangui/vangui.vg>`). The `van::` facade (`#include <vangui/van.vg>`) is a zero-cost namespace alias — every entry below is equally callable as `van::FunctionName(...)`.

---

## Contents

[Context & Frame Lifecycle](#context--frame-lifecycle) · [Demo & Debug Windows](#demo--debug-windows) · [Style](#style) · [Windows](#windows) · [Child Windows](#child-windows) · [Window State & Sizing](#window-state--sizing) · [Scrolling](#scrolling) · [Layout & Cursor](#layout--cursor) · [ID Stack](#id-stack) · [Style Stack](#style-stack) · [Fonts](#fonts) · [Text](#text) · [Widgets — Buttons](#widgets--buttons) · [Widgets — Images](#widgets--images) · [Widgets — Combo](#widgets--combo) · [Widgets — Drag](#widgets--drag) · [Widgets — Slider](#widgets--slider) · [Widgets — Input](#widgets--input) · [Widgets — Color](#widgets--color) · [Widgets — Trees](#widgets--trees) · [Widgets — Selectables](#widgets--selectables) · [Widgets — List Boxes](#widgets--list-boxes) · [Widgets — Data Plotting](#widgets--data-plotting) · [Widgets — Value Helpers](#widgets--value-helpers) · [Menus](#menus) · [Tooltips](#tooltips) · [Popups & Modals](#popups--modals) · [Tables](#tables) · [Legacy Columns](#legacy-columns) · [Tab Bars](#tab-bars) · [Docking](#docking) · [Logging](#logging) · [Drag & Drop](#drag--drop) · [Clipping & Disabled](#clipping--disabled) · [Focus & Activation](#focus--activation) · [Item Queries](#item-queries) · [Viewports & Draw Lists](#viewports--draw-lists) · [Input — Keyboard](#input--keyboard) · [Input — Mouse](#input--mouse) · [Clipboard](#clipboard) · [Timing & Utilities](#timing--utilities) · [Color Conversion](#color-conversion) · [Memory](#memory) · [Enhancement Suite](#enhancement-suite)

---

## Context & Frame Lifecycle

The context is the root object. Create one at startup, set it current, drive the frame loop.

```cpp
VanGuiContext* CreateContext(VanFontAtlas* shared_font_atlas = nullptr);
void           DestroyContext(VanGuiContext* ctx = nullptr);   // nullptr = current
VanGuiContext* GetCurrentContext();
void           SetCurrentContext(VanGuiContext* ctx);

VanGuiIO&         GetIO();           // frame input/output; fill before NewFrame()
VanGuiStyle&      GetStyle();        // visual style; safe to modify between frames
VanGuiPlatformIO& GetPlatformIO();   // multi-viewport / platform backend hooks

void        NewFrame();              // call after feeding input into GetIO()
void        EndFrame();              // optional explicit end; called by Render() if skipped
void        Render();                // finalise draw data; call after all widgets
VanDrawData* GetDrawData();          // retrieve after Render(); pass to your renderer
```

**Frame loop pattern:**
```cpp
// Each frame:
VanGui::NewFrame();
// ... build your UI ...
VanGui::Render();
RenderDrawData(VanGui::GetDrawData());
```

---

## Demo & Debug Windows

```cpp
void ShowDemoWindow(bool* p_open = nullptr);          // interactive widget showcase
void ShowMetricsWindow(bool* p_open = nullptr);       // draw call count, atlas, font info
void ShowDebugLogWindow(bool* p_open = nullptr);      // internal event log
void ShowIDStackToolWindow(bool* p_open = nullptr);   // ID stack inspector
void ShowAboutWindow(bool* p_open = nullptr);         // version and build info
void ShowStyleEditor(VanGuiStyle* ref = nullptr);     // live theme editor
bool ShowStyleSelector(const char* label);            // combo to pick built-in styles
void ShowFontSelector(const char* label);             // combo to pick loaded fonts
void ShowUserGuide();                                 // inline quick-reference text

const char* GetVersion();                             // e.g. "1.92.9 WIP"
```

---

## Style

```cpp
void StyleColorsDark(VanGuiStyle* dst = nullptr);     // default dark theme
void StyleColorsLight(VanGuiStyle* dst = nullptr);    // light theme
void StyleColorsClassic(VanGuiStyle* dst = nullptr);  // legacy theme
```

Modify `GetStyle()` directly for per-field changes, or use `PushStyleColor` / `PushStyleVar` for scoped overrides (see [Style Stack](#style-stack)).

---

## Windows

Every window must be opened with `Begin` and unconditionally closed with `End`, even when `Begin` returns `false`.

```cpp
bool Begin(const char* name, bool* p_open = nullptr, VanGuiWindowFlags flags = 0);
void End();
```

**`p_open`** — pass a `bool*` to show a close button (×); set to `false` when clicked. Pass `nullptr` for no close button.

**Returns** `true` if the window is visible and not collapsed. Always call `End()` regardless.

**Common `VanGuiWindowFlags`:**

| Flag | Effect |
|---|---|
| `NoTitleBar` | Suppress title bar |
| `NoResize` | Disable resize grip |
| `NoMove` | Lock position |
| `NoScrollbar` | Hide scrollbar |
| `NoCollapse` | Disable collapse by double-click |
| `AlwaysAutoResize` | Fit window to content every frame |
| `NoBackground` | Transparent background |
| `MenuBar` | Reserve space for a menu bar |
| `HorizontalScrollbar` | Enable horizontal scrollbar |
| `NoFocusOnAppearing` | Don't steal focus on first appearance |
| `NoBringToDisplayFront` | Don't raise to front on click |
| `NoNav` | Disable keyboard/gamepad navigation |
| `NoDecoration` | `NoTitleBar \| NoResize \| NoScrollbar \| NoCollapse` |
| `NoInputs` | Pass-through; no interaction |

```cpp
// Position/size helpers (call before Begin):
void SetNextWindowPos(const VanVec2& pos, VanGuiCond cond = 0, const VanVec2& pivot = VanVec2(0,0));
void SetNextWindowSize(const VanVec2& size, VanGuiCond cond = 0);
void SetNextWindowSizeConstraints(const VanVec2& size_min, const VanVec2& size_max,
                                  VanGuiSizeCallback callback = nullptr, void* data = nullptr);
void SetNextWindowContentSize(const VanVec2& size);
void SetNextWindowCollapsed(bool collapsed, VanGuiCond cond = 0);
void SetNextWindowFocus();
void SetNextWindowScroll(const VanVec2& scroll);
void SetNextWindowBgAlpha(float alpha);

// Modify after Begin (use sparingly; prefer SetNext* above):
void SetWindowPos(const VanVec2& pos, VanGuiCond cond = 0);
void SetWindowSize(const VanVec2& size, VanGuiCond cond = 0);
void SetWindowCollapsed(bool collapsed, VanGuiCond cond = 0);
void SetWindowFocus();
void SetWindowFontScale(float scale);

// Named-window overloads (no Begin required):
void SetWindowPos(const char* name, const VanVec2& pos, VanGuiCond cond = 0);
void SetWindowSize(const char* name, const VanVec2& size, VanGuiCond cond = 0);
void SetWindowCollapsed(const char* name, bool collapsed, VanGuiCond cond = 0);
void SetWindowFocus(const char* name);
```

**`VanGuiCond` values:** `Always` · `Once` · `FirstUseEver` · `Appearing`

---

## Child Windows

Embedded scrollable sub-regions inside a parent window.

```cpp
bool BeginChild(const char* str_id, const VanVec2& size = VanVec2(0,0),
                VanGuiChildFlags child_flags = 0, VanGuiWindowFlags window_flags = 0);
bool BeginChild(VanGuiID id, const VanVec2& size = VanVec2(0,0),
                VanGuiChildFlags child_flags = 0, VanGuiWindowFlags window_flags = 0);
void EndChild();
```

`size` components: `0` = remaining space; negative = remaining space minus that amount.

**Common `VanGuiChildFlags`:** `Border` · `AutoResizeX` · `AutoResizeY` · `AlwaysUseWindowPadding` · `FrameStyle`

---

## Window State & Sizing

Query state of the current window (call inside `Begin`/`End`):

```cpp
bool     IsWindowAppearing();
bool     IsWindowCollapsed();
bool     IsWindowFocused(VanGuiFocusedFlags flags = 0);
bool     IsWindowHovered(VanGuiHoveredFlags flags = 0);
VanVec2  GetWindowPos();
VanVec2  GetWindowSize();
float    GetWindowWidth();
float    GetWindowHeight();
bool     IsWindowDocked();
VanGuiID GetWindowDockID();

VanVec2  GetContentRegionAvail();         // remaining space in current window
VanVec2  GetContentRegionMax();
VanVec2  GetWindowContentRegionMin();
VanVec2  GetWindowContentRegionMax();

VanDrawList* GetWindowDrawList();         // draw list for current window
float        GetWindowDpiScale();
```

---

## Scrolling

```cpp
float GetScrollX();
float GetScrollY();
void  SetScrollX(float scroll_x);
void  SetScrollY(float scroll_y);
float GetScrollMaxX();
float GetScrollMaxY();
void  SetScrollHereX(float center_x_ratio = 0.5f);   // scroll so current item is visible
void  SetScrollHereY(float center_y_ratio = 0.5f);
void  SetScrollFromPosX(float local_x, float center_x_ratio = 0.5f);
void  SetScrollFromPosY(float local_y, float center_y_ratio = 0.5f);
```

---

## Layout & Cursor

```cpp
// Primitives
void    Separator();                                        // horizontal rule
void    SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);
void    NewLine();
void    Spacing();
void    Dummy(const VanVec2& size);                         // invisible spacer that occupies layout space
void    Indent(float indent_w = 0.0f);                     // 0 = use style indent
void    Unindent(float indent_w = 0.0f);
void    BeginGroup();                                       // group items for IsItemHovered/bounding-box queries
void    EndGroup();
void    AlignTextToFramePadding();                          // vertically align text next to a framed widget

// Heights
float   GetTextLineHeight();
float   GetTextLineHeightWithSpacing();
float   GetFrameHeight();
float   GetFrameHeightWithSpacing();

// Cursor — local (relative to window)
VanVec2 GetCursorPos();
float   GetCursorPosX();
float   GetCursorPosY();
void    SetCursorPos(const VanVec2& local_pos);
void    SetCursorPosX(float local_x);
void    SetCursorPosY(float local_y);
VanVec2 GetCursorStartPos();

// Cursor — screen (absolute)
VanVec2 GetCursorScreenPos();
void    SetCursorScreenPos(const VanVec2& pos);

// Item width
void    PushItemWidth(float item_width);                    // negative = align to right edge
void    PopItemWidth();
void    SetNextItemWidth(float item_width);                 // applies to next widget only
float   CalcItemWidth();

// Text wrap
void    PushTextWrapPos(float wrap_local_pos_x = 0.0f);    // 0 = window width
void    PopTextWrapPos();

// Clipping (screen-space)
void    PushClipRect(const VanVec2& clip_rect_min, const VanVec2& clip_rect_max,
                     bool intersect_with_current_clip_rect);
void    PopClipRect();

// Rect visibility test
bool    IsRectVisible(const VanVec2& size);
bool    IsRectVisible(const VanVec2& rect_min, const VanVec2& rect_max);
```

---

## ID Stack

VanGUI identifies widgets by hashing their label plus the current ID stack. Use `PushID`/`PopID` to disambiguate multiple widgets sharing the same label (e.g. inside a loop).

```cpp
void     PushID(const char* str_id);
void     PushID(const char* str_id_begin, const char* str_id_end);
void     PushID(const void* ptr_id);
void     PushID(int int_id);
void     PopID();

VanGuiID GetID(const char* str_id);
VanGuiID GetID(const char* str_id_begin, const char* str_id_end);
VanGuiID GetID(const void* ptr_id);
VanGuiID GetID(int int_id);
```

**Pattern — loop disambiguation:**
```cpp
for (int i = 0; i < items.size(); ++i) {
    VanGui::PushID(i);
    VanGui::Button("Delete");
    VanGui::PopID();
}
```

**Label suffix `##`** — text after `##` is part of the ID but not displayed:
```cpp
VanGui::Button("OK##first");
VanGui::Button("OK##second");  // different IDs, same label
```

---

## Style Stack

Scoped style overrides. Every `Push` must be balanced with a matching `Pop`.

```cpp
// Colors
void PushStyleColor(VanGuiCol idx, VanU32 col);
void PushStyleColor(VanGuiCol idx, const VanVec4& col);
void PopStyleColor(int count = 1);

// Floats and VanVec2 style vars
void PushStyleVar(VanGuiStyleVar idx, float val);
void PushStyleVar(VanGuiStyleVar idx, const VanVec2& val);
void PushStyleVarX(VanGuiStyleVar idx, float val_x);   // set only X component
void PushStyleVarY(VanGuiStyleVar idx, float val_y);   // set only Y component
void PopStyleVar(int count = 1);

// Item flags
void PushItemFlag(VanGuiItemFlags option, bool enabled);
void PopItemFlag();

// Query
VanU32         GetColorU32(VanGuiCol idx, float alpha_mul = 1.0f);
VanU32         GetColorU32(const VanVec4& col);
VanU32         GetColorU32(VanU32 col, float alpha_mul = 1.0f);
const VanVec4& GetStyleColorVec4(VanGuiCol idx);
const char*    GetStyleColorName(VanGuiCol idx);
```

---

## Fonts

```cpp
void    PushFont(VanFont* font, float size = 0.0f);  // 0 = font's default size
void    PopFont();
VanFont* GetFont();
float    GetFontSize();
VanVec2  GetFontTexUvWhitePixel();
VanVec2  CalcTextSize(const char* text, const char* text_end = nullptr,
                      bool hide_text_after_double_hash = false, float wrap_width = -1.0f);
```

Fonts are loaded through `GetIO().Fonts` (a `VanFontAtlas`). Call `GetIO().Fonts->AddFontFromFileTTF(...)` before the first `NewFrame`.

---

## Text

```cpp
void TextUnformatted(const char* text, const char* text_end = nullptr);  // fastest; no formatting
void Text(const char* fmt, ...);
void TextV(const char* fmt, va_list args);
void TextColored(const VanVec4& col, const char* fmt, ...);
void TextColoredV(const VanVec4& col, const char* fmt, va_list args);
void TextDisabled(const char* fmt, ...);                  // greyed-out text
void TextDisabledV(const char* fmt, va_list args);
void TextWrapped(const char* fmt, ...);                   // wraps at window/wrap-pos boundary
void TextWrappedV(const char* fmt, va_list args);
void LabelText(const char* label, const char* fmt, ...);  // right-aligned label + value
void LabelTextV(const char* label, const char* fmt, va_list args);
void BulletText(const char* fmt, ...);                    // bullet point + text
void BulletTextV(const char* fmt, va_list args);
void SeparatorText(const char* label);                    // horizontal rule with centred label
void Bullet();                                            // just the bullet dot; no text
bool TextLink(const char* label);                         // returns true on click
void TextLinkOpenURL(const char* label, const char* url = nullptr);  // opens URL on click
```

---

## Widgets — Buttons

All widget functions return `true` when interacted with (clicked, toggled, etc.).

```cpp
bool Button(const char* label, const VanVec2& size = VanVec2(0,0));
bool SmallButton(const char* label);                         // compact, inline-friendly
bool InvisibleButton(const char* str_id, const VanVec2& size, VanGuiButtonFlags flags = 0);
bool ArrowButton(const char* str_id, VanGuiDir dir);         // dir: Left/Right/Up/Down
bool Checkbox(const char* label, bool* v);
bool CheckboxFlags(const char* label, int* flags, int flags_value);
bool CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value);
bool RadioButton(const char* label, bool active);
bool RadioButton(const char* label, int* v, int v_button);   // sets *v = v_button on click
void ProgressBar(float fraction, const VanVec2& size_arg = VanVec2(-FLT_MIN, 0),
                 const char* overlay = nullptr);
```

---

## Widgets — Images

```cpp
void Image(VanTextureID user_texture_id, const VanVec2& image_size,
           const VanVec2& uv0 = VanVec2(0,0), const VanVec2& uv1 = VanVec2(1,1));

void ImageWithBg(VanTextureID user_texture_id, const VanVec2& image_size,
                 const VanVec2& uv0 = VanVec2(0,0), const VanVec2& uv1 = VanVec2(1,1),
                 const VanVec4& bg_col = VanVec4(0,0,0,0),
                 const VanVec4& tint_col = VanVec4(1,1,1,1));

bool ImageButton(const char* str_id, VanTextureID user_texture_id, const VanVec2& image_size,
                 const VanVec2& uv0 = VanVec2(0,0), const VanVec2& uv1 = VanVec2(1,1),
                 const VanVec4& bg_col = VanVec4(0,0,0,0),
                 const VanVec4& tint_col = VanVec4(1,1,1,1));
```

`VanTextureID` is `void*` — cast your renderer's texture handle directly.

---

## Widgets — Combo

```cpp
bool BeginCombo(const char* label, const char* preview_value, VanGuiComboFlags flags = 0);
void EndCombo();

// Convenience overloads (manage open/close internally):
bool Combo(const char* label, int* current_item,
           const char* const items[], int items_count, int popup_max_height_in_items = -1);
bool Combo(const char* label, int* current_item,
           const char* items_separated_by_zeros, int popup_max_height_in_items = -1);
bool Combo(const char* label, int* current_item,
           const char* (*getter)(void* user_data, int idx), void* user_data,
           int items_count, int popup_max_height_in_items = -1);
```

**`BeginCombo` pattern:**
```cpp
if (VanGui::BeginCombo("Mode", current_item)) {
    for (int i = 0; i < IM_ARRAYSIZE(items); ++i) {
        bool selected = (current_item == items[i]);
        if (VanGui::Selectable(items[i], selected))
            current_item = items[i];
        if (selected) VanGui::SetItemDefaultFocus();
    }
    VanGui::EndCombo();
}
```

---

## Widgets — Drag

Click-and-drag to change a value. Ctrl+click to type a value directly.

```cpp
bool DragFloat(const char* label, float* v, float v_speed = 1.0f,
               float v_min = 0.0f, float v_max = 0.0f,
               const char* format = "%.3f", VanGuiSliderFlags flags = 0);
bool DragFloat2(const char* label, float v[2], ...);
bool DragFloat3(const char* label, float v[3], ...);
bool DragFloat4(const char* label, float v[4], ...);
bool DragFloatRange2(const char* label, float* v_current_min, float* v_current_max,
                     float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f,
                     const char* format = "%.3f", const char* format_max = nullptr,
                     VanGuiSliderFlags flags = 0);

bool DragInt(const char* label, int* v, float v_speed = 1.0f,
             int v_min = 0, int v_max = 0,
             const char* format = "%d", VanGuiSliderFlags flags = 0);
bool DragInt2(const char* label, int v[2], ...);
bool DragInt3(const char* label, int v[3], ...);
bool DragInt4(const char* label, int v[4], ...);
bool DragIntRange2(const char* label, int* v_current_min, int* v_current_max, ...);

// Generic (arbitrary data type):
bool DragScalar(const char* label, VanGuiDataType data_type, void* p_data,
                float v_speed = 1.0f, const void* p_min = nullptr, const void* p_max = nullptr,
                const char* format = nullptr, VanGuiSliderFlags flags = 0);
bool DragScalarN(const char* label, VanGuiDataType data_type, void* p_data, int components, ...);
```

**`v_min == v_max == 0`** disables clamping. **`v_speed`** is units-per-pixel dragged.

---

## Widgets — Slider

```cpp
bool SliderFloat(const char* label, float* v, float v_min, float v_max,
                 const char* format = "%.3f", VanGuiSliderFlags flags = 0);
bool SliderFloat2(const char* label, float v[2], float v_min, float v_max, ...);
bool SliderFloat3(const char* label, float v[3], float v_min, float v_max, ...);
bool SliderFloat4(const char* label, float v[4], float v_min, float v_max, ...);
bool SliderAngle(const char* label, float* v_rad,
                 float v_degrees_min = -360.0f, float v_degrees_max = 360.0f,
                 const char* format = "%.0f deg", VanGuiSliderFlags flags = 0);

bool SliderInt(const char* label, int* v, int v_min, int v_max,
               const char* format = "%d", VanGuiSliderFlags flags = 0);
bool SliderInt2(const char* label, int v[2], int v_min, int v_max, ...);
bool SliderInt3(const char* label, int v[3], int v_min, int v_max, ...);
bool SliderInt4(const char* label, int v[4], int v_min, int v_max, ...);

bool SliderScalar(const char* label, VanGuiDataType data_type, void* p_data,
                  const void* p_min, const void* p_max,
                  const char* format = nullptr, VanGuiSliderFlags flags = 0);
bool SliderScalarN(const char* label, VanGuiDataType data_type, void* p_data, int components, ...);

// Vertical sliders
bool VSliderFloat(const char* label, const VanVec2& size, float* v, float v_min, float v_max,
                  const char* format = "%.3f", VanGuiSliderFlags flags = 0);
bool VSliderInt(const char* label, const VanVec2& size, int* v, int v_min, int v_max,
                const char* format = "%d", VanGuiSliderFlags flags = 0);
bool VSliderScalar(const char* label, const VanVec2& size, VanGuiDataType data_type,
                   void* p_data, const void* p_min, const void* p_max, ...);
```

**Common `VanGuiSliderFlags`:** `AlwaysClamp` · `Logarithmic` · `NoRoundToFormat` · `NoInput`

---

## Widgets — Input

```cpp
bool InputText(const char* label, char* buf, size_t buf_size,
               VanGuiInputTextFlags flags = 0,
               VanGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
bool InputTextMultiline(const char* label, char* buf, size_t buf_size,
                        const VanVec2& size = VanVec2(0,0),
                        VanGuiInputTextFlags flags = 0, ...);
bool InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size,
                       VanGuiInputTextFlags flags = 0, ...);

bool InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f,
                const char* format = "%.3f", VanGuiInputTextFlags flags = 0);
bool InputFloat2(const char* label, float v[2], const char* format = "%.3f", ...);
bool InputFloat3(const char* label, float v[3], const char* format = "%.3f", ...);
bool InputFloat4(const char* label, float v[4], const char* format = "%.3f", ...);

bool InputInt(const char* label, int* v, int step = 1, int step_fast = 100,
              VanGuiInputTextFlags flags = 0);
bool InputInt2(const char* label, int v[2], VanGuiInputTextFlags flags = 0);
bool InputInt3(const char* label, int v[3], VanGuiInputTextFlags flags = 0);
bool InputInt4(const char* label, int v[4], VanGuiInputTextFlags flags = 0);

bool InputDouble(const char* label, double* v, double step = 0.0, double step_fast = 0.0,
                 const char* format = "%.6f", VanGuiInputTextFlags flags = 0);

bool InputScalar(const char* label, VanGuiDataType data_type, void* p_data,
                 const void* p_step = nullptr, const void* p_step_fast = nullptr,
                 const char* format = nullptr, VanGuiInputTextFlags flags = 0);
bool InputScalarN(const char* label, VanGuiDataType data_type, void* p_data, int components, ...);
```

**Common `VanGuiInputTextFlags`:** `CharsDecimal` · `CharsHexadecimal` · `CharsUppercase` · `CharsNoBlank` · `AutoSelectAll` · `EnterReturnsTrue` · `CallbackCompletion` · `CallbackHistory` · `CallbackAlways` · `CallbackCharFilter` · `AllowTabInput` · `CtrlEnterForNewLine` · `ReadOnly` · `Password` · `NoUndoRedo` · `EscapeClearsAll`

---

## Widgets — Color

```cpp
bool ColorEdit3(const char* label, float col[3], VanGuiColorEditFlags flags = 0);
bool ColorEdit4(const char* label, float col[4], VanGuiColorEditFlags flags = 0);
bool ColorPicker3(const char* label, float col[3], VanGuiColorEditFlags flags = 0);
bool ColorPicker4(const char* label, float col[4], VanGuiColorEditFlags flags = 0,
                  const float* ref_col = nullptr);
bool ColorButton(const char* desc_id, const VanVec4& col,
                 VanGuiColorEditFlags flags = 0, const VanVec2& size = VanVec2(0,0));
void SetColorEditOptions(VanGuiColorEditFlags flags);   // set global defaults
```

**Common `VanGuiColorEditFlags`:** `NoAlpha` · `NoPicker` · `NoOptions` · `NoSmallPreview` · `NoInputs` · `NoTooltip` · `NoLabel` · `NoBorder` · `AlphaBar` · `AlphaPreview` · `AlphaPreviewHalf` · `HDR` · `DisplayRGB` · `DisplayHSV` · `DisplayHex` · `Uint8` · `Float` · `PickerHueBar` · `PickerHueWheel` · `InputRGB` · `InputHSV`

---

## Widgets — Trees

```cpp
bool TreeNode(const char* label);
bool TreeNode(const char* str_id, const char* fmt, ...);
bool TreeNode(const void* ptr_id, const char* fmt, ...);
bool TreeNodeV(const char* str_id, const char* fmt, va_list args);
bool TreeNodeV(const void* ptr_id, const char* fmt, va_list args);

bool TreeNodeEx(const char* label, VanGuiTreeNodeFlags flags = 0);
bool TreeNodeEx(const char* str_id, VanGuiTreeNodeFlags flags, const char* fmt, ...);
bool TreeNodeEx(const void* ptr_id, VanGuiTreeNodeFlags flags, const char* fmt, ...);

void TreePush(const char* str_id);    // manually push tree indent + ID
void TreePush(const void* ptr_id = nullptr);
void TreePop();
float GetTreeNodeToLabelSpacing();

bool CollapsingHeader(const char* label, VanGuiTreeNodeFlags flags = 0);
bool CollapsingHeader(const char* label, bool* p_visible, VanGuiTreeNodeFlags flags = 0);

void SetNextItemOpen(bool is_open, VanGuiCond cond = 0);
void SetNextItemStorageID(VanGuiID storage_id);
```

**Rule:** call `TreePop()` only when `TreeNode` / `TreeNodeEx` returned `true`.

**Common `VanGuiTreeNodeFlags`:** `Selected` · `Framed` · `AllowOverlap` · `NoTreePushOnOpen` · `NoAutoOpenOnLog` · `DefaultOpen` · `OpenOnDoubleClick` · `OpenOnArrow` · `Leaf` · `Bullet` · `FramePadding` · `SpanAvailWidth` · `SpanFullWidth` · `SpanLabelWidth` · `SpanAllColumns` · `NavLeftJumpsBackHere` · `CollapsingHeader`

---

## Widgets — Selectables

```cpp
bool Selectable(const char* label, bool selected = false,
                VanGuiSelectableFlags flags = 0, const VanVec2& size = VanVec2(0,0));
bool Selectable(const char* label, bool* p_selected,
                VanGuiSelectableFlags flags = 0, const VanVec2& size = VanVec2(0,0));

// Multi-select (range-selection, Ctrl/Shift support):
VanGuiMultiSelectIO* BeginMultiSelect(VanGuiMultiSelectFlags flags,
                                      int selection_size = -1, int items_count = -1);
VanGuiMultiSelectIO* EndMultiSelect();
void SetNextItemSelectionUserData(VanGuiSelectionUserData selection_user_data);
bool IsItemToggledSelection();
```

**Common `VanGuiSelectableFlags`:** `DontClosePopups` · `SpanAllColumns` · `AllowDoubleClick` · `Disabled` · `AllowOverlap`

---

## Widgets — List Boxes

```cpp
bool BeginListBox(const char* label, const VanVec2& size = VanVec2(0,0));
void EndListBox();

// Convenience overloads:
bool ListBox(const char* label, int* current_item,
             const char* const items[], int items_count, int height_in_items = -1);
bool ListBox(const char* label, int* current_item,
             const char* (*getter)(void* user_data, int idx), void* user_data,
             int items_count, int height_in_items = -1);
```

---

## Widgets — Data Plotting

```cpp
void PlotLines(const char* label, const float* values, int values_count,
               int values_offset = 0, const char* overlay_text = nullptr,
               float scale_min = FLT_MAX, float scale_max = FLT_MAX,
               VanVec2 graph_size = VanVec2(0,0), int stride = sizeof(float));
void PlotLines(const char* label,
               float (*values_getter)(void* data, int idx), void* data,
               int values_count, int values_offset = 0, ...);

void PlotHistogram(const char* label, const float* values, int values_count, ...);
void PlotHistogram(const char* label,
                   float (*values_getter)(void* data, int idx), void* data, ...);
```

`scale_min == scale_max == FLT_MAX` auto-scales to the data range.

For richer charts see `vgu::Sparkline` in [Enhancement Suite — widgets_pack.vg](#widgetspackVG).

---

## Widgets — Value Helpers

Shorthand for `LabelText` with typed values:

```cpp
void Value(const char* prefix, bool b);
void Value(const char* prefix, int v);
void Value(const char* prefix, unsigned int v);
void Value(const char* prefix, float v, const char* float_format = nullptr);
```

---

## Menus

```cpp
bool BeginMenuBar();                          // must be inside Begin(..., MenuBar flag)
void EndMenuBar();
bool BeginMainMenuBar();                      // full-width bar at the top of the display
void EndMainMenuBar();
bool BeginMenu(const char* label, bool enabled = true);
void EndMenu();
bool MenuItem(const char* label, const char* shortcut = nullptr,
              bool selected = false, bool enabled = true);
bool MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled = true);
```

---

## Tooltips

```cpp
bool BeginTooltip();
void EndTooltip();
void SetTooltip(const char* fmt, ...);
void SetTooltipV(const char* fmt, va_list args);

bool BeginItemTooltip();                      // opens only when previous item is hovered
void SetItemTooltip(const char* fmt, ...);   // shorthand for BeginItemTooltip + Text + End
void SetItemTooltipV(const char* fmt, va_list args);
```

---

## Popups & Modals

```cpp
// Open (deferred — takes effect next frame):
void OpenPopup(const char* str_id, VanGuiPopupFlags popup_flags = 0);
void OpenPopup(VanGuiID id, VanGuiPopupFlags popup_flags = 0);
void OpenPopupOnItemClick(const char* str_id = nullptr, VanGuiPopupFlags popup_flags = 1);
void CloseCurrentPopup();

// Draw:
bool BeginPopup(const char* str_id, VanGuiWindowFlags flags = 0);
bool BeginPopupModal(const char* name, bool* p_open = nullptr, VanGuiWindowFlags flags = 0);
void EndPopup();

// Context menus (open on right-click):
bool BeginPopupContextItem(const char* str_id = nullptr, VanGuiPopupFlags popup_flags = 1);
bool BeginPopupContextWindow(const char* str_id = nullptr, VanGuiPopupFlags popup_flags = 1);
bool BeginPopupContextVoid(const char* str_id = nullptr, VanGuiPopupFlags popup_flags = 1);

// Query:
bool IsPopupOpen(const char* str_id, VanGuiPopupFlags flags = 0);
```

**`BeginPopupModal`** blocks interaction with anything behind it. Always call `EndPopup()` when `Begin*` returns `true`.

---

## Tables

The tables API supports scrolling, sorting, fixed/flexible column widths, frozen rows/columns, row highlighting, and per-cell background colors.

```cpp
bool BeginTable(const char* str_id, int columns, VanGuiTableFlags flags = 0,
                const VanVec2& outer_size = VanVec2(0,0), float inner_width = 0.0f);
void EndTable();

// Navigation:
void TableNextRow(VanGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f);
bool TableNextColumn();                         // advance to next column; true = visible
bool TableSetColumnIndex(int column_n);         // jump to a specific column

// Setup (call after BeginTable, before first TableNextRow):
void TableSetupColumn(const char* label, VanGuiTableColumnFlags flags = 0,
                      float init_width_or_weight = 0.0f, VanGuiID user_id = 0);
void TableSetupScrollFreeze(int cols, int rows); // freeze first N cols/rows while scrolling

// Headers:
void TableHeader(const char* label);            // single custom header cell
void TableHeadersRow();                         // auto-draw header row from SetupColumn labels
void TableAngledHeadersRow();                   // angled header row

// Sorting:
VanGuiTableSortSpecs* TableGetSortSpecs();      // null if no sort; check Dirty flag

// Column info:
int              TableGetColumnCount();
int              TableGetColumnIndex();
int              TableGetRowIndex();
const char*      TableGetColumnName(int column_n = -1);
VanGuiTableColumnFlags TableGetColumnFlags(int column_n = -1);
void             TableSetColumnEnabled(int column_n, bool v);
int              TableGetHoveredColumn();

// Backgrounds:
void TableSetBgColor(VanGuiTableBgTarget target, VanU32 color, int column_n = -1);
```

**Common `VanGuiTableFlags`:** `Borders` · `BordersInner` · `BordersOuter` · `BordersH` · `BordersV` · `BordersInnerH` · `BordersInnerV` · `BordersOuterH` · `BordersOuterV` · `NoBordersInBody` · `RowBg` · `Resizable` · `Reorderable` · `Hideable` · `Sortable` · `SortMulti` · `SortTristate` · `NoSavedSettings` · `ContextMenuInBody` · `ScrollX` · `ScrollY` · `SizingFixedFit` · `SizingFixedSame` · `SizingStretchProp` · `SizingStretchSame` · `PadOuterX` · `NoPadOuterX` · `NoPadInnerX`

---

## Legacy Columns

Simpler but less capable than the Tables API. Prefer `BeginTable` for new code.

```cpp
void  Columns(int count = 1, const char* id = nullptr, bool border = true);
void  NextColumn();
int   GetColumnIndex();
float GetColumnWidth(int column_index = -1);
void  SetColumnWidth(int column_index, float width);
float GetColumnOffset(int column_index = -1);
void  SetColumnOffset(int column_index, float offset_x);
int   GetColumnsCount();
```

---

## Tab Bars

```cpp
bool BeginTabBar(const char* str_id, VanGuiTabBarFlags flags = 0);
void EndTabBar();
bool BeginTabItem(const char* label, bool* p_open = nullptr, VanGuiTabItemFlags flags = 0);
void EndTabItem();
bool TabItemButton(const char* label, VanGuiTabItemFlags flags = 0);   // non-content tab (button)
void SetTabItemClosed(const char* tab_or_docked_window_label);         // close programmatically
```

**Common `VanGuiTabBarFlags`:** `Reorderable` · `AutoSelectNewTabs` · `TabListPopupButton` · `NoCloseWithMiddleMouseButton` · `NoTabListScrollingButtons` · `NoTooltip` · `DrawSelectedOverline` · `FittingPolicyResizeDown` · `FittingPolicyScroll`

---

## Docking

Multi-window docking support. Requires `VanGuiConfigFlags_DockingEnable` in `GetIO().ConfigFlags`.

```cpp
VanGuiID DockSpace(VanGuiID dockspace_id, const VanVec2& size = VanVec2(0,0),
                   VanGuiDockNodeFlags flags = 0,
                   const VanGuiWindowClass* window_class = nullptr);
VanGuiID DockSpaceOverViewport(VanGuiID dockspace_id = 0,
                                const VanGuiViewport* viewport = nullptr,
                                VanGuiDockNodeFlags flags = 0,
                                const VanGuiWindowClass* window_class = nullptr);
void     SetNextWindowDockID(VanGuiID dock_id, VanGuiCond cond = 0);
void     SetNextWindowClass(const VanGuiWindowClass* window_class);
VanGuiID GetWindowDockID();
bool     IsWindowDocked();
```

---

## Logging

Capture widget text to TTY, a file, or the clipboard for debugging / UI screenshots.

```cpp
void LogToTTY(int auto_open_depth = -1);
void LogToFile(int auto_open_depth = -1, const char* filename = nullptr);
void LogToClipboard(int auto_open_depth = -1);
void LogFinish();
void LogButtons();                            // helper buttons: TTY / File / Clip / Depth
void LogText(const char* fmt, ...);           // append raw text to the current log
void LogTextV(const char* fmt, va_list args);
```

---

## Drag & Drop

```cpp
// Source (inside Begin/End for the dragged item):
bool BeginDragDropSource(VanGuiDragDropFlags flags = 0);
bool SetDragDropPayload(const char* type, const void* data, size_t sz,
                        VanGuiCond cond = 0);
void EndDragDropSource();

// Target:
bool BeginDragDropTarget();
const VanGuiPayload* AcceptDragDropPayload(const char* type,
                                           VanGuiDragDropFlags flags = 0);
void EndDragDropTarget();

// Query current payload (from anywhere):
const VanGuiPayload* GetDragDropPayload();
```

`type` is a user-defined string (max 32 chars) identifying the payload kind. `VanGuiPayload::Data` and `VanGuiPayload::DataSize` carry the raw bytes.

For OS-level file drops see `vgu::DropZone` in [Enhancement Suite — dropzone.vg](#dropzoneMD).

---

## Clipping & Disabled

```cpp
void BeginDisabled(bool disabled = true);
void EndDisabled();
```

Inside `BeginDisabled`/`EndDisabled`, all widgets are visually greyed and non-interactive. The `disabled` parameter lets you conditionally skip disabling without an extra `if`.

---

## Focus & Activation

```cpp
void SetItemDefaultFocus();                         // mark previous item as default focus in popups/menus
void SetKeyboardFocusHere(int offset = 0);          // 0 = next widget, -1 = previous
void SetNextItemAllowOverlap();                     // allow next item to be drawn over the previous
void SetNextItemShortcut(VanGuiKeyChord key_chord, VanGuiInputFlags flags = 0);
void SetItemKeyOwner(VanGuiKey key);
```

---

## Item Queries

Query state of the most recently submitted item:

```cpp
bool     IsItemHovered(VanGuiHoveredFlags flags = 0);
bool     IsItemActive();                  // held (e.g. drag in progress)
bool     IsItemFocused();
bool     IsItemClicked(VanGuiMouseButton mouse_button = 0);
bool     IsItemVisible();                 // within clip rect
bool     IsItemEdited();                  // value changed this frame
bool     IsItemActivated();              // became active this frame
bool     IsItemDeactivated();            // stopped being active this frame
bool     IsItemDeactivatedAfterEdit();   // deactivated and value was edited
bool     IsItemToggledOpen();            // tree node opened/closed this frame

bool     IsAnyItemHovered();
bool     IsAnyItemActive();
bool     IsAnyItemFocused();

VanGuiID GetItemID();
VanVec2  GetItemRectMin();
VanVec2  GetItemRectMax();
VanVec2  GetItemRectSize();
VanGuiItemFlags GetItemFlags();
```

---

## Viewports & Draw Lists

```cpp
VanGuiViewport* GetMainViewport();
VanGuiViewport* GetWindowViewport();
VanGuiViewport* FindViewportByID(VanGuiID id);
VanGuiViewport* FindViewportByPlatformHandle(void* platform_handle);

VanDrawList* GetBackgroundDrawList(VanGuiViewport* viewport = nullptr);
VanDrawList* GetForegroundDrawList(VanGuiViewport* viewport = nullptr);
VanDrawList* GetWindowDrawList();                   // draw list for the current window
VanDrawListSharedData* GetDrawListSharedData();
```

`VanDrawList` exposes primitives for lines, rectangles, circles, triangles, polylines, bezier curves, images, and text at arbitrary screen positions.

---

## Input — Keyboard

```cpp
bool  IsKeyDown(VanGuiKey key);
bool  IsKeyPressed(VanGuiKey key, bool repeat = true);
bool  IsKeyReleased(VanGuiKey key);
bool  IsKeyChordPressed(VanGuiKeyChord key_chord);              // e.g. Mod_Ctrl | Key_S
int   GetKeyPressedAmount(VanGuiKey key, float repeat_delay, float rate);
const char* GetKeyName(VanGuiKey key);
void  SetNextFrameWantCaptureKeyboard(bool want_capture_keyboard);
```

Key values are `VanGuiKey_*` constants (A–Z, F1–F12, Tab, Enter, Escape, arrow keys, modifiers, gamepad inputs, etc.).

---

## Input — Mouse

```cpp
bool    IsMouseDown(VanGuiMouseButton button);
bool    IsMouseClicked(VanGuiMouseButton button, bool repeat = false);
bool    IsMouseReleased(VanGuiMouseButton button);
bool    IsMouseDoubleClicked(VanGuiMouseButton button);
int     GetMouseClickedCount(VanGuiMouseButton button);
bool    IsMouseHoveringRect(const VanVec2& r_min, const VanVec2& r_max, bool clip = true);
bool    IsMousePosValid(const VanVec2* mouse_pos = nullptr);
bool    IsAnyMouseDown();
VanVec2 GetMousePos();
VanVec2 GetMousePosOnOpeningCurrentPopup();
bool    IsMouseDragging(VanGuiMouseButton button, float lock_threshold = -1.0f);
VanVec2 GetMouseDragDelta(VanGuiMouseButton button = 0, float lock_threshold = -1.0f);
void    ResetMouseDragDelta(VanGuiMouseButton button = 0);
VanGuiMouseCursor GetMouseCursor();
void    SetMouseCursor(VanGuiMouseCursor cursor_type);
void    SetNextFrameWantCaptureMouse(bool want_capture_mouse);
```

Mouse buttons: `VanGuiMouseButton_Left` (0), `_Right` (1), `_Middle` (2).

---

## Clipboard

```cpp
const char* GetClipboardText();
void        SetClipboardText(const char* text);
```

---

## Timing & Utilities

```cpp
double   GetTime();           // seconds since context creation
int      GetFrameCount();     // total frames rendered since context creation

void     SetStateStorage(VanGuiStorage* storage);
VanGuiStorage* GetStateStorage();
```

---

## Color Conversion

```cpp
void    ColorConvertRGBtoHSV(float r, float g, float b,
                              float& out_h, float& out_s, float& out_v);
void    ColorConvertHSVtoRGB(float h, float s, float v,
                              float& out_r, float& out_g, float& out_b);
VanU32  ColorConvertFloat4ToU32(const VanVec4& in);
VanVec4 ColorConvertU32ToFloat4(VanU32 in);
```

---

## Memory

Redirect VanGUI's internal allocations to your own heap:

```cpp
void  SetAllocatorFunctions(VanGuiMemAllocFunc alloc_func,
                             VanGuiMemFreeFunc free_func,
                             void* user_data = nullptr);
void  GetAllocatorFunctions(VanGuiMemAllocFunc* p_alloc_func,
                             VanGuiMemFreeFunc* p_free_func,
                             void** p_user_data);
void* MemAlloc(size_t size);
void  MemFree(void* ptr);
```

Call `SetAllocatorFunctions` before `CreateContext`.

---

## Enhancement Suite

Header-only extensions in `include/vangui/`. All are zero-cost when not included.

### `van.vg` — Fluent `van::` facade

```cpp
#include <vangui/van.vg>
namespace van = VanGui;  // entire VanGui:: surface available as van::
```

### `shorthand.vg` — One-liner helpers (`vgu::`)

```cpp
#include <vangui/shorthand.vg>

bool vgu::Shortcut(const char* chord);                               // Ctrl+S etc.
void vgu::HelpMarker(const char* desc);                             // (?) tooltip marker
bool vgu::SearchBox(char* buf, size_t buf_size, const char* hint);
bool vgu::Section(const char* label);                               // collapsible group
void vgu::Kbd(const char* key, ...);                                // keyboard key badge(s)
void vgu::EmptyState(const char* title, const char* body,
                     const char* action_label, auto action_fn);
```

### `widgets_pack.vg` — Extra widgets (`vgu::`) {#widgetspackVG}

```cpp
#include <vangui/widgets_pack.vg>

bool vgu::Segmented(const char* const* labels, int count, int* selected,
                    const VanVec2& size_per_tab = VanVec2(0,0));
bool vgu::ToggleSwitch(const char* label, bool* v);
bool vgu::Chip(const char* label, bool closable, bool* alive);
void vgu::Chips(const char* const* labels, int count);
bool vgu::RatingStars(const char* str_id, int* rating, int max = 5);
void vgu::Sparkline(const char* str_id, const float* values, int count,
                    float min = NAN, float max = NAN, float height = 0.0f);
bool vgu::Breadcrumbs(const char* const* items, int count, int* clicked_index);
bool vgu::Debounced(const char* key, float delay_seconds, bool changed_now);
bool vgu::Throttled(const char* key, float min_interval_seconds);
void vgu::NotificationCenter(const char* title);
```

### `notify.vg` — Toast notifications

```cpp
#include <vangui/notify.vg>

// Post:
VanNotifyID VanGui::NotifyInfo(const char* fmt, ...);
VanNotifyID VanGui::InsertNotification(VanNotifyType type, float duration_ms,
                                       const char* fmt, ...);
// Update:
void VanGui::SetNotificationProgress(VanNotifyID id, float progress);  // -1 = hide bar
// Dismiss:
void VanGui::DismissNotification(VanNotifyID id);
// Position:
void VanGui::SetNotificationsPos(VanNotifyPos pos);
// Render (once per frame, after all other windows):
void VanGui::RenderNotifications();
```

**`VanNotifyType`:** `_Info` · `_Success` · `_Warning` · `_Error`

**`VanNotifyPos`:** `_TopLeft` · `_TopRight` · `_BottomLeft` · `_BottomRight`

### `panels.vg` — Application chrome (`VANGUI_ENABLE_PANELS`)

```cpp
#include <vangui/panels.vg>

bool VanGui::Splitter(const char* id, bool split_vertically, float thickness,
                      float* size1, float* size2, float min1 = 24.f, float min2 = 24.f);
bool VanGui::AccordionSection(const char* label, bool* open);
void VanGui::AccordionEnd();
bool VanGui::BeginStatusBar();
void VanGui::EndStatusBar();
```

### `forms.vg` — Form layout & validation (`VANGUI_ENABLE_FORMS`)

```cpp
#include <vangui/forms.vg>

void VanGui::BeginForm(const char* str_id);
void VanGui::FormRow(const char* label);
void VanGui::FieldError(const char* message);
void VanGui::EndForm();
```

### `dropzone.vg` — File drop zone (`VANGUI_ENABLE_DROPZONE`) {#dropzoneMD}

```cpp
#include <vangui/dropzone.vg>

// In your window:
bool vgu::DropZone(const char* label,
                   const char** out_paths, int max_paths, int* out_count);

// From a backend file-drop callback:
void vgu::NotifyFilesDropped(const char** paths, int count);
```

### `signals.vg` — Signals & slots (`VANGUI_ENABLE_SIGNALS`)

```cpp
#define VANGUI_ENABLE_SIGNALS
#include <vangui/signals.vg>

VanSignal<void(int)> on_value_changed;

VanConnection conn = on_value_changed.connect([](int v) { /* ... */ });
on_value_changed.emit(42);
// conn destructor auto-disconnects

// Thread-safe variant (also define VANGUI_ENABLE_THREAD):
VanConnection queued = on_value_changed.connect_queued([](int v) { /* ... */ });
```

### `loading.vg` — Loading & busy effects (`VANGUI_ENABLE_LOADING`)

Requires `VANGUI_ENABLE_ANIM`. Provides indeterminate spinners, `ProgressRing`, and overlay fades. All stateless (phase from global time) except progress widgets which use the animation substrate.

### `theme_gen.vg` — One-accent theme generation

```cpp
#include <vangui/theme_gen.vg>

VanThemeTokenSet t = VanGui::GenerateTheme(
    VanVec4(0.26f, 0.59f, 0.98f, 1.f),   // accent color
    /*dark=*/true
);
VanGui::ApplyTokenSet(t);
// or animated:
VanGui::TransitionToTokenSet(t, duration_seconds);
```

---

<sub>Built and maintained by [TsyVM](https://github.com/TsyVM) · [TeamVanilla](https://www.teamvanilla.org/)</sub>
