#!/usr/bin/env bash
set -euo pipefail

src="$(dirname "$0")/../macos-like-window-appearance.wh.cpp"
readme="$(dirname "$0")/../README.md"

test -s "$src"
test -s "$readme"
grep -q '@version      1.2.0' "$src"
grep -q 'DWMWA_WINDOW_CORNER_PREFERENCE' "$src"
grep -q 'kDwmColorNone' "$src"
grep -q 'ShowWindow_Hook' "$src"
grep -q 'ApplyInitialGoldenRatioSize' "$src"
grep -q 'goldenRatioWidthPercent' "$src"

# Reject high-risk always-on APIs when they appear as code lines.
if grep -En '^[[:space:]]*(CreateThread|std::thread|SetTimer|Sleep\(|EnumWindows|SetWindowLong|WH_KEYBOARD|WH_MOUSE)' "$src"; then
  echo 'Unsafe runtime API pattern found' >&2
  exit 1
fi

echo "Static verification passed."
