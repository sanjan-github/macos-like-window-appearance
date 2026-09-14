# Safe macOS-like Window Appearance for Windows 11

## What this mod does

This Windhawk mod combines three conservative behaviors:

1. It requests Windows 11's native rounded-corner rendering for ordinary top-level application windows.
2. It suppresses the DWM-drawn outline using Microsoft's documented `DWMWA_COLOR_NONE` border value.
3. It can give each eligible window a **one-time centered golden-ratio starting size** before its first display. After that first layout, the user can move and resize the window normally.

The mod does not modify system files, the registry, input, animations, or ongoing window placement. It does not run a timer, worker thread, polling loop, or window enumeration pass.

## Why the outline may still appear

The previous border setting could appear ineffective for two reasons. First, `DWMWA_COLOR_NONE` only suppresses the border drawn by the **Desktop Window Manager**; an application may draw its own client-area outline, shadow, or custom frame. Second, some applications apply their own DWM attributes after window creation. Version 1.2 reapplies the DWM appearance immediately before `ShowWindow` and still avoids timers or background watchers.

If a line remains after this update, it is probably application-rendered or part of a shadow rather than the DWM border. Removing such a line universally would require application-specific painting hooks and would not be a safe, battery-neutral change.

## Golden-ratio sizing behavior

The default initial width is 62% of the active monitor's work area. Height is calculated using:

```text
height = width / 1.618
```

The result is centered in the monitor work area and constrained to a 92% maximum work-area height. For example, on a 1920×1080 work area, the initial size is approximately 1188×734 pixels before normal Windows frame adjustments.

The layout is applied only once per window. It does not continuously force the golden ratio, so users can customize every window afterward without the mod fighting their changes.

## Before you start

1. Create a Windows restore point or confirm that you have a recent backup.
2. Download Windhawk only from [windhawk.net](https://windhawk.net/).
3. Confirm that you are running Windows 11. The rounded-corner and border attributes are Windows 11 features.
4. Close unsaved work before enabling third-party process modifications.
5. Keep this source as a local mod initially. Do not publish it as an official Windhawk mod without changing its author metadata and following Windhawk's repository rules.

## Installation, one step at a time

### 1. Install Windhawk

Install and open Windhawk. If Windows Defender or another security product reports a problem, stop and investigate the installer rather than bypassing the warning.

### 2. Open the local mod editor

In Windhawk, choose **Create new mod** or the equivalent local-mod editor. The exact label can vary by Windhawk version.

### 3. Paste the source

Open [the Windhawk source file](file:///home/ubuntu/macos-like-window-appearance.wh.cpp), copy the complete contents, and paste it into the editor. The file must remain a single `.wh.cpp` mod source file.

### 4. Compile before enabling

Use Windhawk's **Compile** or **Test compilation** action. Do not enable the mod if compilation fails. The source uses the documented Windhawk hook and settings APIs plus Windows DWM APIs.

### 5. Start with these settings

| Setting | Initial value | Reason |
|---|---:|---|
| Rounding style | `native` | Uses normal Windows 11 rounded corners. |
| Window border | `none` | Requests no DWM outline. |
| Skip tool windows | `true` | Avoids palettes, floating toolbars, and utility windows. |
| Initial golden-ratio size | enabled | Applies the one-time centered starting size. |
| Golden-ratio width percent | `62` | Close to the golden-ratio visual target while leaving usable margins. |

### 6. Enable only after compilation succeeds

Enable the mod and restart one ordinary application, such as Notepad, File Explorer, or Calculator. Verify the result there before using the mod broadly.

### 7. Test the golden-ratio behavior

Open a new window and confirm that it starts centered with a wide, balanced aspect ratio. Resize and move it manually. Confirm that the mod does not snap it back afterward. If you prefer normal Windows initial sizing, disable **Initial golden-ratio size**.

### 8. Test important states

Check the following manually:

- normal restored window;
- maximize and restore;
- snap left/right and unsnap;
- minimize and reopen;
- display scale above 100% if you use it;
- multiple monitors;
- elevated applications, if normally used;
- applications with custom or borderless frames;
- an application that previously showed the thin outline.

A borderless, per-pixel-alpha, custom-region, or heavily themed application may remain square or may draw its own outline. That is expected and safer than forcing a region over the application.

## Performance and battery behavior

The source is designed to minimize overhead:

- no periodic work;
- no worker thread;
- no window enumeration;
- no ongoing foreground-window tracking;
- no continuous resizing or repositioning;
- DWM appearance is applied at creation and immediately before first display;
- golden-ratio geometry is applied at most once per eligible window;
- child, owned, tool, borderless, and non-captioned windows are skipped;
- failure of the optional `ShowWindow` hook does not prevent the creation-time appearance path.

No software modification can guarantee zero CPU/GPU or battery impact on every Windows build. This design avoids the common sources of ongoing overhead. If battery life is critical, use Windhawk's process inclusion/exclusion controls to apply it only to selected applications.

## What it does not provide

The reference image also shows an arranged desktop layout, traffic-light buttons, shadows, and a dock. This source intentionally does not implement:

- continuous automatic tiling;
- macOS-style traffic-light buttons;
- a global dock or launcher;
- custom overlay shadows;
- global active-window tracking;
- window animations.

Those features need different hooks and have higher compatibility risk. For layout experimentation, use a dedicated reversible tool such as Microsoft PowerToys FancyZones rather than adding global window-management logic to this appearance mod.

## Rollback

1. In Windhawk, disable the mod.
2. Close and reopen applications that were modified.
3. If a process does not restore correctly, sign out and sign back in, or restart Windows Explorer from Task Manager.
4. If Windhawk itself causes a problem, disable Windhawk's automatic startup or uninstall Windhawk from **Installed apps**.
5. Do not delete system DLLs or manually edit DWM-related registry values as a rollback method.

The mod does not patch system files and should not require a system restore for normal rollback.

## Troubleshooting

### The mod does not compile

Check that the complete source was pasted, including all metadata blocks. Update Windhawk to a current release and retry. Do not remove safety checks to force compilation.

### The outline is still visible

Confirm that **Window border** is set to `none`, then restart the application. If the line remains, it is likely drawn by the application itself or is a shadow. This mod cannot universally remove application-owned pixels without risky application-specific hooks.

### Windows are not initially golden-ratio sized

Restart the application. The mod intentionally does not enumerate existing windows. Confirm that the window is a normal, unowned, captioned or resizable top-level window and that **Initial golden-ratio size** is enabled.

### A program moves or resizes unexpectedly

Disable **Initial golden-ratio size** first. If the issue continues, disable the whole mod and restart the affected program. Add the program to Windhawk's exclusion list.

### Some windows remain square

Windows 11 treats corner preference as a hint. Maximized windows, custom regions, per-pixel-alpha windows, virtualized windows, and unusual custom frames may not be rounded.

### A different Windhawk mod conflicts

Temporarily disable other appearance, DWM, title-bar, transparency, or corner-radius mods. Run only one mod that controls corner rendering at a time.

## Source references

- [Windhawk: Creating a new mod](https://github.com/ramensoftware/windhawk/wiki/Creating-a-new-mod)
- [Official Windhawk mods repository](https://github.com/ramensoftware/windhawk-mods)
- [Microsoft: Apply rounded corners in desktop apps](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/apply-rounded-corners)
- [Microsoft: DWM window attributes](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute)
- [macOS window-corner design analysis](https://lapcatsoftware.com/articles/2026/3/4.html)

Apple uses different radii for some toolbar and titlebar window styles and does not publish one universal radius. Windows 11's public API exposes a corner preference rather than an arbitrary pixel radius. This mod therefore chooses the public native rounded treatment instead of patching private DWM geometry.
