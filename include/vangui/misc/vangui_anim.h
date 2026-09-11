// vangui_anim.h
// -----------------------------------------------------------------------------
// VanGUI Enhancement Suite — Pillar 1: shared animation & transition substrate.
//
// This is the single, authoritative place where VanGUI turns "a target value"
// into "a value that moves toward the target over time". Every higher-level
// enhancement (loading effects, dialog open/close, theme cross-fades, toast
// slides, widget motion) is built on top of this — nothing reimplements
// easing or time integration. (Engineering Constitution, Ch.1: DRY.)
//
// DESIGN CONTRACT
//   * Opt-in / zero-cost-when-off:
//       - Define VANGUI_ENABLE_ANIM to compile the real implementation.
//       - When it is NOT defined, every public function below is an inline
//         no-op shim that snaps to the target instantly and allocates nothing,
//         so calling code compiles and runs identically either way.
//   * Immediate-mode preserved:
//       - No persistent user objects. State is keyed by VanGuiID (the existing
//         ID stack) and stored context-side, exactly like tables/windows.
//   * No per-frame heap allocations: the state pool grows once and recycles
//     slots; eviction marks slots free.
//   * Fallible nothing: these calls cannot fail. They are noexcept-friendly
//     and return by value. (No exceptions; cf. Constitution Ch. Error Handling.)
//
// USAGE
//   1. #define VANGUI_ENABLE_ANIM (in vanconfig.h or your build).
//   2. Add a single guarded call at the end of VanGui::NewFrame():
//          #ifdef VANGUI_ENABLE_ANIM
//              VanGui::Anim::NewFrameUpdate();
//          #endif
//      (This is the ONLY edit the substrate requires in core vangui.cpp.)
//   3. In your UI code, pull the animated value each frame:
//          float a = VanGui::Anim::AnimBool("panel_open", show_panel,
//                                           { .Duration = 0.18f });
//          // use `a` (0..1) to drive alpha / scale / slide
//
// THREADING / CONTEXT
//   State is associated with the active VanGui context. Switching the current
//   context (CreateContext/DestroyContext) transparently resets the pool. Using
//   two VanGui contexts *simultaneously* from interleaved code is not supported
//   in this revision (documented follow-up; see docs/ENHANCEMENT_ROADMAP.md).
// -----------------------------------------------------------------------------

#pragma once

#include <vangui/vangui.h>   // VanGuiID, VanVec4, VanU32, VANGUI_API, VanGui::GetID

namespace VanGui {
namespace Anim {

// ---------------------------------------------------------------------------
// Easing curves
// ---------------------------------------------------------------------------
// `t` is a normalized progress in [0,1]; the return value is the shaped
// progress. Some curves intentionally overshoot 1.0 (BackOut, ElasticOut) —
// AnimColor()/AnimBool() clamp afterwards; AnimFloat() preserves the overshoot.

enum VanEasing : int
{
    VanEasing_Linear = 0,
    VanEasing_QuadIn,    VanEasing_QuadOut,    VanEasing_QuadInOut,
    VanEasing_CubicIn,   VanEasing_CubicOut,   VanEasing_CubicInOut,
    VanEasing_ExpoOut,   VanEasing_CircOut,
    VanEasing_BackOut,   VanEasing_ElasticOut, VanEasing_BounceOut,
    VanEasing_COUNT
};

// Polynomial curves are constexpr (compile-time validated via static_assert in
// the .cpp). Transcendental curves (Expo/Elastic/Bounce) are evaluated at run
// time. `Ease` dispatches to the right one.
[[nodiscard]] constexpr float EaseLinear  (float t) noexcept { return t; }
[[nodiscard]] constexpr float EaseQuadIn  (float t) noexcept { return t * t; }
[[nodiscard]] constexpr float EaseQuadOut (float t) noexcept { return t * (2.0f - t); }
[[nodiscard]] constexpr float EaseCubicIn (float t) noexcept { return t * t * t; }
[[nodiscard]] constexpr float EaseCubicOut(float t) noexcept { const float u = t - 1.0f; return u * u * u + 1.0f; }

#ifdef VANGUI_ENABLE_ANIM
[[nodiscard]] VANGUI_API float Ease(VanEasing fn, float t) noexcept;
#else
[[nodiscard]] inline float Ease(VanEasing, float t) noexcept { return t; }
#endif

// ---------------------------------------------------------------------------
// Parameters
// ---------------------------------------------------------------------------

struct VanAnimParams
{
    float     Duration = 0.20f;               // seconds for a tween segment
    VanEasing Easing   = VanEasing_CubicOut;  // shaping curve
    float     Delay    = 0.0f;                // seconds to wait before moving
    bool      Unscaled = false;               // reserved: ignore app time-scale
};

struct VanSpringParams
{
    float Stiffness = 170.0f;   // higher = snappier
    float Damping   = 26.0f;    // higher = less overshoot
    float Eps       = 0.001f;   // settle threshold (value & velocity)
};

// ===========================================================================
//  REAL IMPLEMENTATION  (VANGUI_ENABLE_ANIM defined)
// ===========================================================================
#ifdef VANGUI_ENABLE_ANIM

// --- Animated scalars / colors, keyed by ID --------------------------------
// Each returns the *current* value for this frame and advances internal state
// toward `target` using VanGui::GetIO().DeltaTime.

[[nodiscard]] VANGUI_API float   AnimFloat(VanGuiID id, float target,   const VanAnimParams& p = {});
[[nodiscard]] VANGUI_API VanVec4 AnimColor(VanGuiID id, VanVec4 target, const VanAnimParams& p = {});

// Entrance/exit driver: returns 0..1 progress as `open` flips. Use it to drive
// fades, scale-ins, slide-ins for popups, toasts, dialogs, list rows, etc.
[[nodiscard]] VANGUI_API float   AnimBool (VanGuiID id, bool open,      const VanAnimParams& p = {});

// Spring (framerate-independent, semi-implicit). No fixed duration; settles by
// physics. Good for drag handles, reorder, knobs.
[[nodiscard]] VANGUI_API float   SpringFloat(VanGuiID id, float target, const VanSpringParams& p = {});

// --- Convenience string-id overloads (hash via the active ID stack) ---------
[[nodiscard]] inline float   AnimFloat(const char* id, float target,   const VanAnimParams& p = {}) { return AnimFloat(VanGui::GetID(id), target, p); }
[[nodiscard]] inline VanVec4 AnimColor(const char* id, VanVec4 target, const VanAnimParams& p = {}) { return AnimColor(VanGui::GetID(id), target, p); }
[[nodiscard]] inline float   AnimBool (const char* id, bool open,      const VanAnimParams& p = {}) { return AnimBool (VanGui::GetID(id), open,  p); }
[[nodiscard]] inline float   SpringFloat(const char* id, float target, const VanSpringParams& p = {}) { return SpringFloat(VanGui::GetID(id), target, p); }

// --- Lifecycle / queries ----------------------------------------------------

// Advance the frame, evict stale state. Call once per frame from NewFrame().
VANGUI_API void NewFrameUpdate();

// True while any animation touched this frame is still moving. Backends can use
// this to keep rendering continuously, and idle/sleep when it returns false.
[[nodiscard]] VANGUI_API bool IsAnimating();

// Forget the state for one id (e.g. to hard-cut instead of animate).
VANGUI_API void Reset(VanGuiID id);

// Free all animation state for the active context (also called on context swap).
VANGUI_API void Shutdown();

// Frames an entry may go untouched before it is evicted (default 60).
VANGUI_API void SetEvictionFrames(int frames);

// Testing aid: force a fixed DeltaTime for deterministic stepping. Pass a
// negative value to resume using VanGui::GetIO().DeltaTime (the default).
VANGUI_API void SetDeltaTimeOverrideForTesting(float dt);

// Introspection for metrics/debug overlays.
VANGUI_API int  PoolActiveCount();   // tweens touched/alive
VANGUI_API int  PoolCapacity();      // slots allocated in the pool

// ===========================================================================
//  ZERO-COST SHIMS  (VANGUI_ENABLE_ANIM not defined)
// ===========================================================================
#else

[[nodiscard]] inline float   AnimFloat(VanGuiID, float target,   const VanAnimParams& = {}) { return target; }
[[nodiscard]] inline VanVec4 AnimColor(VanGuiID, VanVec4 target, const VanAnimParams& = {}) { return target; }
[[nodiscard]] inline float   AnimBool (VanGuiID, bool open,      const VanAnimParams& = {}) { return open ? 1.0f : 0.0f; }
[[nodiscard]] inline float   SpringFloat(VanGuiID, float target, const VanSpringParams& = {}) { return target; }

[[nodiscard]] inline float   AnimFloat(const char*, float target,   const VanAnimParams& = {}) { return target; }
[[nodiscard]] inline VanVec4 AnimColor(const char*, VanVec4 target, const VanAnimParams& = {}) { return target; }
[[nodiscard]] inline float   AnimBool (const char*, bool open,      const VanAnimParams& = {}) { return open ? 1.0f : 0.0f; }
[[nodiscard]] inline float   SpringFloat(const char*, float target, const VanSpringParams& = {}) { return target; }

inline void NewFrameUpdate() {}
[[nodiscard]] inline bool IsAnimating() { return false; }
inline void Reset(VanGuiID) {}
inline void Shutdown() {}
inline void SetEvictionFrames(int) {}
inline void SetDeltaTimeOverrideForTesting(float) {}
inline int  PoolActiveCount() { return 0; }
inline int  PoolCapacity()    { return 0; }

#endif // VANGUI_ENABLE_ANIM

} // namespace Anim
} // namespace VanGui
