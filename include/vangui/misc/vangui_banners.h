// vangui_banners.h
// -----------------------------------------------------------------------------
// VanGUI Enhancement Suite — inline banners + snackbars.
//
// In-flow alert banners (persistent, colored by severity, optional action/close)
// and transient bottom-anchored snackbars with an optional action button. A
// lightweight companion to vangui_notify's corner toasts. Opt-in / zero-cost via
// VANGUI_ENABLE_BANNERS.
// -----------------------------------------------------------------------------

#pragma once

#include <vangui/vangui.h>

namespace VanGui {

enum VanBannerType : int { VanBanner_Info = 0, VanBanner_Success, VanBanner_Warning, VanBanner_Error };

#ifdef VANGUI_ENABLE_BANNERS

// Inline, in-flow banner. Returns true the frame the action button is clicked.
// If p_open is set, a close button clears *p_open.
VANGUI_API bool Banner(const char* id, VanBannerType type, const char* text,
                       const char* action_label = nullptr, bool* p_open = nullptr);

// Queue a transient snackbar (bottom-center). Call RenderSnackbars() each frame.
VANGUI_API void Snackbar(const char* text, const char* action_label = nullptr, float duration = 4.0f);
// Draw + expire queued snackbars. Returns the id of a snackbar whose action was
// clicked this frame, or -1.
VANGUI_API int  RenderSnackbars();

#else // ------------------------------- shims ---------------------------------

inline bool Banner(const char*, VanBannerType, const char*, const char* = nullptr, bool* = nullptr) { return false; }
inline void Snackbar(const char*, const char* = nullptr, float = 4.0f) {}
inline int  RenderSnackbars() { return -1; }

#endif // VANGUI_ENABLE_BANNERS

} // namespace VanGui
