// vangui_loading.h
// -----------------------------------------------------------------------------
// VanGUI Enhancement Suite — Pillar 2: loading & busy-state effects.
//
// Pure consumers of Pillar 1 (vangui_anim) and the VanDrawList API. They add no
// new core state. Indeterminate spinners derive their phase from global time and
// are therefore stateless (they never touch the anim pool); only widgets that
// interpolate a value (ProgressRing, the overlay fade) use the substrate.
//
// DESIGN CONTRACT
//   * Opt-in / zero-cost-when-off: define VANGUI_ENABLE_LOADING to compile the
//     real implementation. When undefined, every entry point is an inline no-op.
//   * VANGUI_ENABLE_LOADING implies VANGUI_ENABLE_ANIM (the progress widgets and
//     overlay fade go through the substrate). Enforced below with #error.
//   * No per-frame heap allocations: strings are caller-owned; draw goes straight
//     to the window/foreground draw list.
// -----------------------------------------------------------------------------

#pragma once

#include <vangui/vangui.h>

#if defined(VANGUI_ENABLE_LOADING) && !defined(VANGUI_ENABLE_ANIM)
#  error "VANGUI_ENABLE_LOADING requires VANGUI_ENABLE_ANIM (Pillar 2 depends on Pillar 1). Define VANGUI_ENABLE_ANIM."
#endif

namespace VanGui {

// ---------------------------------------------------------------------------
// Shared low-level progress-bar primitive (DRY).
// ---------------------------------------------------------------------------
// Header-only so both vangui_loading and vangui_notify draw progress through the
// exact same code path without a link dependency between the two modules.
namespace Detail {
inline void ProgressBarPrim(VanDrawList* dl, const VanVec2& p_min, const VanVec2& p_max,
                            float fraction, VanU32 track_col, VanU32 fill_col,
                            float rounding = 2.0f)
{
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    dl->AddRectFilled(p_min, p_max, track_col, rounding);
    if (fraction > 0.0f)
    {
        const VanVec2 fill_max(p_min.x + (p_max.x - p_min.x) * fraction, p_max.y);
        dl->AddRectFilled(p_min, fill_max, fill_col, rounding);
    }
}
} // namespace Detail

#ifdef VANGUI_ENABLE_LOADING

// --- Indeterminate spinners (stateless: phase from GetTime) -----------------

// Classic arc spinner. `color`==0 uses the current text color.
VANGUI_API void Spinner(const char* id, float radius, float thickness, VanU32 color = 0, float speed = 1.0f);

// Ring of fading dots. `color`==0 uses the current text color.
VANGUI_API void SpinnerDots(const char* id, float radius, int count = 8, VanU32 color = 0);

// Row of pulsing bars. `color`==0 uses the current text color.
VANGUI_API void SpinnerBars(const char* id, VanVec2 size, VanU32 color = 0);

// --- Indeterminate sliding bar (gradient sweep) -----------------------------
VANGUI_API void IndeterminateBar(const char* id, VanVec2 size, VanU32 color = 0);

// --- Determinate circular progress (fraction 0..1; smoothed via AnimFloat) --
VANGUI_API void ProgressRing(const char* id, float fraction, float radius, float thickness, VanU32 color = 0);

// --- Skeleton placeholders with shimmer (shimmer phase from GetTime) --------
VANGUI_API void Skeleton(VanVec2 size, float rounding = 4.0f);
VANGUI_API void SkeletonText(int lines, float line_height = 0.0f);

// --- Dim-and-cover overlay; fade via AnimBool. RAII variant below. ----------
// Returns true while the overlay is at least partially visible.
VANGUI_API bool BeginLoadingOverlay(const char* id, bool busy);
VANGUI_API void EndLoadingOverlay();

// RAII guard for BeginLoadingOverlay/EndLoadingOverlay.
struct VanLoadingOverlayScope
{
    bool Visible;
    explicit VanLoadingOverlayScope(const char* id, bool busy) { Visible = BeginLoadingOverlay(id, busy); }
    ~VanLoadingOverlayScope() { EndLoadingOverlay(); }
    explicit operator bool() const { return Visible; }
    VanLoadingOverlayScope(const VanLoadingOverlayScope&) = delete;
    VanLoadingOverlayScope& operator=(const VanLoadingOverlayScope&) = delete;
};

#else // ----------------------------- shims -----------------------------------

inline void Spinner(const char*, float, float, VanU32 = 0, float = 1.0f) {}
inline void SpinnerDots(const char*, float, int = 8, VanU32 = 0) {}
inline void SpinnerBars(const char*, VanVec2, VanU32 = 0) {}
inline void IndeterminateBar(const char*, VanVec2, VanU32 = 0) {}
inline void ProgressRing(const char*, float, float, float, VanU32 = 0) {}
inline void Skeleton(VanVec2, float = 4.0f) {}
inline void SkeletonText(int, float = 0.0f) {}
inline bool BeginLoadingOverlay(const char*, bool) { return false; }
inline void EndLoadingOverlay() {}

struct VanLoadingOverlayScope
{
    bool Visible = false;
    explicit VanLoadingOverlayScope(const char*, bool) {}
    ~VanLoadingOverlayScope() {}
    explicit operator bool() const { return false; }
};

#endif // VANGUI_ENABLE_LOADING

} // namespace VanGui
