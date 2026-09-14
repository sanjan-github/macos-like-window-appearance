// ==WindhawkMod==
// @id           macos-like-window-appearance-safe
// @name         macOS-like Window Appearance (Safe)
// @description  Applies polished Windows 11 rounded corners, suppresses the DWM outline, and optionally gives ordinary app windows a one-time centered golden-ratio size without polling, timers, window enumeration, or system-file changes.
// @version      1.2.0
// @author       sanjan-github
// @github       https://github.com/sanjan-github/macos-like-window-appearance
// @include      *
// @exclude      dwm.exe
// @architecture x86-64
// @compilerOptions -ldwmapi
// ==/WindhawkMod==

// This mod intentionally changes appearance and optional initial geometry only.
// It does not continuously move, resize, tile, animate, subclass, or inspect
// windows after their first display.
//
// Windows 11 exposes a supported corner preference API, but not a supported
// API for choosing an arbitrary pixel radius from a Windhawk app-process hook.
// We therefore use the native rounded style, which is the closest supported
// equivalent to the softer macOS appearance without private DWM hooks.
// The border is separately suppressed using Microsoft's documented
// DWMWA_COLOR_NONE value so no thin outline is drawn around the rounded frame.

// ==WindhawkModReadme==
/*
# macOS-like Window Appearance (Safe)

This mod applies Windows 11's native rounded-corner treatment to ordinary
captioned app windows, suppresses the DWM outline, and optionally gives each
eligible window a one-time centered golden-ratio starting size.

## Deliberate safety limits

- No timer, worker thread, polling loop, or `EnumWindows` scan.
- No continuous window movement, resizing, tiling, snapping, animation, or
  input hooks.
- Child windows, owned windows, tool windows, popups, and maximized windows are
  ignored by the initial-layout path.
- The border can be disabled, or reset to Windows default, per settings.
- The golden-ratio size is applied once immediately before first display. After
  that, the user can move and resize the window normally.
- If a hook or DWM API is unavailable, original behavior is preserved.
- Disabling the mod removes the hooks; no system files or registry values are
  modified by this mod.

## Important scope note

This is an appearance and initial-layout mod, not a complete macOS window
manager. A reliable continuously managed tiling engine would need global
foreground-window, work-area, monitor, and hotkey logic. That is intentionally
not included to protect compatibility, performance, and battery life.

## Compatibility

Windows 11's native corner preference is a hint. Apps that use custom regions,
per-pixel alpha, or unusual frames may remain square. The visible line around
a window may also be drawn by the application itself or may be a shadow; the
DWM border setting can only suppress the DWM-drawn border.

Apple uses different radii for some toolbar and titlebar window styles and does
not publish one universal radius. Windows 11's public API exposes a corner
preference rather than an arbitrary pixel radius, so this mod chooses the
public native rounded treatment instead of patching private DWM geometry.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- rounding: native
  $name: Rounding style
  $description: Native uses normal Windows 11 rounded corners; small uses the native small-corner style.
  $options:
  - native: Normal rounded corners
  - small: Small rounded corners
  - default: Let Windows decide
- border: none
  $name: Window border
  $description: None removes the DWM outline, preventing a thin line around the rounded window; default restores Windows behavior.
  $options:
  - none: No DWM outline
  - default: Windows default outline
- skipToolWindows: true
  $name: Skip tool windows
  $description: Do not change utility, palette, floating-toolbar, or task-switcher windows.
- goldenRatioSize: true
  $name: Initial golden-ratio size
  $description: Before the first display, size eligible windows to about 62 percent of the monitor work area width with a 1.618 to 1 width-to-height ratio, then never touch them again.
- goldenRatioWidthPercent: 62
  $name: Golden-ratio width percent
  $description: Initial window width as a percentage of the monitor work area. Values are clamped between 40 and 85.
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <dwmapi.h>
#include <windhawk_api.h>

namespace {

enum class RoundingStyle {
    Native,
    Small,
    Default,
};

struct Settings {
    RoundingStyle rounding = RoundingStyle::Native;
    bool suppressBorder = true;
    bool skipToolWindows = true;
    bool goldenRatioSize = true;
    int goldenRatioWidthPercent = 62;
};

Settings g_settings;

using CreateWindowExW_t = HWND(WINAPI*)(
    DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int,
    HWND, HMENU, HINSTANCE, LPVOID);
CreateWindowExW_t CreateWindowExW_Original = nullptr;

using ShowWindow_t = BOOL(WINAPI*)(HWND, int);
ShowWindow_t ShowWindow_Original = nullptr;

constexpr wchar_t kInitialLayoutProperty[] =
    L"macos_like_window_appearance_initial_layout_1";

bool IsOrdinaryWindow(HWND hwnd) {
    if (hwnd == nullptr || GetWindow(hwnd, GW_OWNER) != nullptr) {
        return false;
    }

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    if (g_settings.skipToolWindows && (exStyle & WS_EX_TOOLWINDOW) != 0) {
        return false;
    }

    // Restrict behavior to conventional application frames. This avoids
    // menus, message-only windows, overlays, and borderless render surfaces.
    if ((style & WS_CAPTION) == 0 && (style & WS_THICKFRAME) == 0) {
        return false;
    }

    return true;
}

bool IsOrdinaryCreation(DWORD exStyle, DWORD style, HWND parent) {
    if (parent != nullptr) {
        return false;
    }

    if (g_settings.skipToolWindows && (exStyle & WS_EX_TOOLWINDOW) != 0) {
        return false;
    }

    return (style & WS_CAPTION) != 0 || (style & WS_THICKFRAME) != 0;
}

void ApplyWindowAppearance(HWND hwnd) {
    if (hwnd == nullptr) {
        return;
    }

    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DEFAULT;
    switch (g_settings.rounding) {
        case RoundingStyle::Native:
            preference = DWMWCP_ROUND;
            break;
        case RoundingStyle::Small:
            preference = DWMWCP_ROUNDSMALL;
            break;
        case RoundingStyle::Default:
            preference = DWMWCP_DEFAULT;
            break;
    }

    // DwmSetWindowAttribute is a hint. Ignore failures so unsupported builds
    // and unusual windows continue with their original behavior.
    (void)DwmSetWindowAttribute(
        hwnd,
        DWMWA_WINDOW_CORNER_PREFERENCE,
        &preference,
        sizeof(preference));

    // DWMWA_BORDER_COLOR is 34 in the Windows 11 SDK. Numeric constants keep
    // this source compatible with older Windhawk SDK headers. Microsoft
    // documents DWMWA_COLOR_NONE as 0xFFFFFFFE to suppress the DWM border.
    constexpr DWORD kDwmwaBorderColor = 34;
    constexpr COLORREF kDwmColorNone = 0xFFFFFFFEu;
    constexpr COLORREF kDwmColorDefault = 0xFFFFFFFFu;
    COLORREF borderColor = g_settings.suppressBorder
                                ? kDwmColorNone
                                : kDwmColorDefault;
    (void)DwmSetWindowAttribute(
        hwnd,
        kDwmwaBorderColor,
        &borderColor,
        sizeof(borderColor));
}

void ApplyInitialGoldenRatioSize(HWND hwnd) {
    if (!g_settings.goldenRatioSize || !IsOrdinaryWindow(hwnd) ||
        GetPropW(hwnd, kInitialLayoutProperty) != nullptr) {
        return;
    }

    // Mark before changing geometry so re-entrant show paths cannot apply the
    // initial layout twice.
    if (!SetPropW(hwnd, kInitialLayoutProperty, reinterpret_cast<HANDLE>(1))) {
        return;
    }

    if (IsZoomed(hwnd) || IsIconic(hwnd)) {
        return;
    }

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitorInfo)) {
        return;
    }

    RECT work = monitorInfo.rcWork;
    int workWidth = work.right - work.left;
    int workHeight = work.bottom - work.top;
    int widthPercent = g_settings.goldenRatioWidthPercent;
    if (widthPercent < 40) {
        widthPercent = 40;
    } else if (widthPercent > 85) {
        widthPercent = 85;
    }

    // Golden ratio: width / height ~= 1.618. Keep a safety margin so rounded
    // corners and resize borders do not touch the work-area edge.
    int width = (workWidth * widthPercent) / 100;
    int height = (width * 1000) / 1618;
    int maxHeight = (workHeight * 92) / 100;
    if (height > maxHeight) {
        height = maxHeight;
        width = (height * 1618) / 1000;
    }

    int x = work.left + (workWidth - width) / 2;
    int y = work.top + (workHeight - height) / 2;
    (void)SetWindowPos(
        hwnd, nullptr, x, y, width, height,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

HWND WINAPI CreateWindowExW_Hook(
    DWORD exStyle,
    LPCWSTR className,
    LPCWSTR windowName,
    DWORD style,
    int x,
    int y,
    int width,
    int height,
    HWND parent,
    HMENU menu,
    HINSTANCE instance,
    LPVOID param) {
    HWND hwnd = CreateWindowExW_Original(
        exStyle, className, windowName, style, x, y, width, height,
        parent, menu, instance, param);

    if (hwnd != nullptr && IsOrdinaryCreation(exStyle, style, parent)) {
        ApplyWindowAppearance(hwnd);
    }

    return hwnd;
}

BOOL WINAPI ShowWindow_Hook(HWND hwnd, int command) {
    if (hwnd != nullptr && IsOrdinaryWindow(hwnd)) {
        // Some applications overwrite DWM attributes after CreateWindowExW.
        // Reapply immediately before display, without a timer or watcher.
        ApplyWindowAppearance(hwnd);
        ApplyInitialGoldenRatioSize(hwnd);
    }

    return ShowWindow_Original(hwnd, command);
}

RoundingStyle LoadRoundingStyle() {
    PCWSTR value = Wh_GetStringSetting(L"rounding");
    RoundingStyle result = RoundingStyle::Native;

    if (value != nullptr) {
        if (_wcsicmp(value, L"small") == 0) {
            result = RoundingStyle::Small;
        } else if (_wcsicmp(value, L"default") == 0) {
            result = RoundingStyle::Default;
        }
        Wh_FreeStringSetting(value);
    }

    return result;
}

void LoadSettings() {
    g_settings.rounding = LoadRoundingStyle();

    PCWSTR border = Wh_GetStringSetting(L"border");
    g_settings.suppressBorder = true;
    if (border != nullptr) {
        if (_wcsicmp(border, L"default") == 0) {
            g_settings.suppressBorder = false;
        }
        Wh_FreeStringSetting(border);
    }

    g_settings.skipToolWindows =
        Wh_GetIntSetting(L"skipToolWindows", 1) != 0;
    g_settings.goldenRatioSize =
        Wh_GetIntSetting(L"goldenRatioSize", 1) != 0;
    g_settings.goldenRatioWidthPercent =
        Wh_GetIntSetting(L"goldenRatioWidthPercent", 62);
}

}  // namespace

BOOL Wh_ModInit() {
    LoadSettings();

    if (!Wh_SetFunctionHook(
            reinterpret_cast<void*>(&CreateWindowExW),
            reinterpret_cast<void*>(&CreateWindowExW_Hook),
            reinterpret_cast<void**>(&CreateWindowExW_Original))) {
        Wh_Log(L"Failed to hook CreateWindowExW; leaving the process unchanged.");
        return FALSE;
    }

    if (!Wh_SetFunctionHook(
            reinterpret_cast<void*>(&ShowWindow),
            reinterpret_cast<void*>(&ShowWindow_Hook),
            reinterpret_cast<void**>(&ShowWindow_Original))) {
        // Appearance still works at creation time if this optional hook is
        // unavailable. Do not fail the complete mod for the layout hook.
        Wh_Log(L"ShowWindow hook unavailable; first-show reapplication disabled.");
    }

    return TRUE;
}

void Wh_ModSettingsChanged() {
    // New settings affect windows created or shown after the change. Existing
    // windows are intentionally not enumerated to preserve low overhead.
    LoadSettings();
}

void Wh_ModUninit() {
    // Windhawk removes registered hooks automatically after this callback.
}
