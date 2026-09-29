<#
.SYNOPSIS
  Captures the 9 Quick Start Guide screenshots for CALSS's Help carousel
  (gui_help_carousel.cpp's HELP_STEPS[]) from the real running app window.

.DESCRIPTION
  Finds the CALSS main window by its actual Win32 class name
  (AuthorshipScorerMainWindow, per gui.cpp's WINDOW_CLASS_NAME - not
  guessed), captures ONLY that window via PrintWindow (not a blind
  desktop-region grab, so it's correct regardless of window position or
  overlapping windows), center-crops to exactly 16:10 (never stretches),
  resizes to a fixed target resolution for a consistent set, and saves
  each capture as PNG under assets\help\ next to the built exe.

  This script does NOT click, type, or otherwise drive the app's UI. You
  (or whoever is at the keyboard) navigate CALSS to the right screen by
  hand, then trigger a capture. Two trigger modes:

    - Single-shot: -Step N captures exactly one named step right now.
    - Walkthrough:  no -Step given -> prompts through all 9 in order,
                     waiting for Enter before each capture.

.PARAMETER Step
  1-9. Capture only this step's screenshot, once, and exit. Omit to run
  the full 9-step walkthrough instead.

.PARAMETER OutDir
  Destination folder for the PNGs. Defaults to assets\help\ next to this
  script's repo root (..\assets\help relative to tools\).

.PARAMETER TargetWidth / TargetHeight
  Final saved PNG size. Must stay 16:10 (the carousel's image frame,
  HELP_IMG_FRAME_W_DESIGN=584 / HELP_IMG_FRAME_H_DESIGN=365, is exactly
  16:10). Defaults to 1600x1000.

.PARAMETER DryRun
  Capture whatever window currently matches the CALSS class, save it as
  _dryrun_test.png in OutDir, and exit - for verifying the capture/crop
  mechanics work before doing a real numbered step. Never overwrites a
  real numbered filename.

.PARAMETER Now
  Only valid with -Step. Skips the Read-Host wait and captures
  immediately - for scripted/orchestrated driving (e.g. an agent
  controlling the app itself) rather than a human at the keyboard, where
  nothing will ever supply the Enter keypress the normal prompt waits
  for. Not the default path; the human-in-the-loop walkthrough above is.

.EXAMPLE
  .\capture_help_screenshots.ps1 -DryRun
  .\capture_help_screenshots.ps1 -Step 3
  .\capture_help_screenshots.ps1 -Step 3 -Now
  .\capture_help_screenshots.ps1
#>
[CmdletBinding()]
param(
    [ValidateRange(1,9)]
    [int]$Step,

    [string]$OutDir = (Join-Path (Split-Path -Parent $PSScriptRoot) "assets\help"),

    [int]$TargetWidth  = 1600,
    [int]$TargetHeight = 1000,

    [switch]$DryRun,

    [switch]$Now
)

$ErrorActionPreference = "Stop"

if (($TargetWidth / $TargetHeight) -ne 1.6) {
    throw "TargetWidth/TargetHeight must be exactly 16:10 (e.g. 1600x1000, 1168x730). Got $TargetWidth x $TargetHeight."
}

# -- Win32 interop ----------------------------------------------------
# RECT is declared as its own top-level type (not nested inside Native)
# because PowerShell 5.1's parser trips over the CLR "Outer+Nested" type
# syntax when it appears inside a [...] type-literal or New-Object call.
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace CalssCapture {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }

    public static class Native {
        [DllImport("user32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
        public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        [DllImport("user32.dll")]
        public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll")]
        public static extern bool IsWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern bool IsIconic(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern bool SetForegroundWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        [DllImport("user32.dll")]
        public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);

        [DllImport("dwmapi.dll")]
        public static extern int DwmGetWindowAttribute(IntPtr hwnd, int dwAttribute, out RECT pvAttribute, int cbAttribute);
    }
}
'@

Add-Type -AssemblyName System.Drawing

$WINDOW_CLASS_NAME = "AuthorshipScorerMainWindow"          # gui.cpp:43 - read, not guessed
$WINDOW_TITLE      = "Code Authorship Likelihood System"   # gui.cpp:44 - read, not guessed
$PW_RENDERFULLCONTENT = 0x00000002
$SW_RESTORE = 9
$DWMWA_EXTENDED_FRAME_BOUNDS = 9

# -- The 9 real carousel steps (filenames copied from HELP_STEPS[] in
#    gui_help_carousel.cpp - see the confirmation block this script
#    prints at startup) -----------------------------------------------
$StepsDef = @(
    @{ N=1; File="calss_help_01_load_submissions.png";
       Prompt="Empty/load state: folder / import files / Google Classroom sync options all visible." }
    @{ N=2; File="calss_help_02_run_analysis.png";
       Prompt="Sidebar mid-setup: AI-summary checkbox + Run Analysis button visible, right before/during a run." }
    @{ N=3; File="calss_help_03_overview_summary.png";
       Prompt="Overview tab: hero stat row visible (pairs flagged / high similarity / max score)." }
    @{ N=4; File="calss_help_04_overview_ai_briefing.png";
       Prompt="Overview tab: AI summary section, at least one clickable 'Pair N' deep-link visible in the text." }
    @{ N=5; File="calss_help_05_classes_grid.png";
       Prompt="Classes tab: student card grid, Style DNA strips visible." }
    @{ N=6; File="calss_help_06_classes_student_detail.png";
       Prompt="A single student's detail page, opened from a card." }
    @{ N=7; File="calss_help_07_flagged_pairs_list.png";
       Prompt="Flagged Pairs tab: list view, severity filter visible." }
    @{ N=8; File="calss_help_08_flagged_pairs_detail.png";
       Prompt="A single flagged pair expanded: side-by-side comparison + Deviations block visible." }
    @{ N=9; File="calss_help_09_generate_report.png";
       Prompt="Export/report-generation control visible." }
)

function Get-CalssWindow {
    # Both class AND title are passed together (not class-with-null-title)
    # -- FindWindow's "NULL means match any" semantics did not marshal
    # reliably from PowerShell in testing (returned not-found even though
    # the live window's class name verified as an exact match via
    # GetClassName); passing both known-good literal constants together
    # is what actually works.
    $hwnd = [CalssCapture.Native]::FindWindow($WINDOW_CLASS_NAME, $WINDOW_TITLE)
    if ($hwnd -eq [IntPtr]::Zero) {
        throw "CALSS window not found (class '$WINDOW_CLASS_NAME', title '$WINDOW_TITLE'). Is authorship.exe running?"
    }
    if ([CalssCapture.Native]::IsIconic($hwnd)) {
        [CalssCapture.Native]::ShowWindow($hwnd, $SW_RESTORE) | Out-Null
        Start-Sleep -Milliseconds 200
    }
    return $hwnd
}

function Get-WindowBounds([IntPtr]$hwnd) {
    # Prefer DWM extended frame bounds - this app defines a custom
    # WM_NCCALCSIZE frame, and on Win10/11 GetWindowRect can include a
    # few px of invisible click-resize padding that DWM bounds excludes.
    $rect = New-Object CalssCapture.RECT
    $rectSize = 16  # RECT = 4 x Int32, sequential layout - fixed, no reflection needed
    $hr = [CalssCapture.Native]::DwmGetWindowAttribute($hwnd, $DWMWA_EXTENDED_FRAME_BOUNDS, [ref]$rect, $rectSize)
    if ($hr -ne 0) {
        if (-not [CalssCapture.Native]::GetWindowRect($hwnd, [ref]$rect)) {
            throw "GetWindowRect failed."
        }
    }
    return $rect
}

function Capture-CalssWindow {
    $hwnd = Get-CalssWindow
    $rect = Get-WindowBounds $hwnd
    $w = $rect.Right - $rect.Left
    $h = $rect.Bottom - $rect.Top
    if ($w -le 0 -or $h -le 0) { throw "Captured window has zero/negative size ($w x $h)." }

    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $gr  = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gr.GetHdc()
    try {
        $ok = [CalssCapture.Native]::PrintWindow($hwnd, $hdc, $PW_RENDERFULLCONTENT)
        if (-not $ok) { throw "PrintWindow returned false." }
    } finally {
        $gr.ReleaseHdc($hdc)
        $gr.Dispose()
    }
    return $bmp
}

function Crop-To16x10([System.Drawing.Bitmap]$src) {
    $srcW = $src.Width
    $srcH = $src.Height
    $targetAspect = 16.0 / 10.0
    $srcAspect = $srcW / [double]$srcH

    if ($srcAspect -gt $targetAspect) {
        # Too wide - crop left/right equally, keep full height.
        $cropW = [Math]::Round($srcH * $targetAspect)
        $cropH = $srcH
        $x = [Math]::Round(($srcW - $cropW) / 2.0)
        $y = 0
    } else {
        # Too tall (or already exact) - crop top/bottom equally, keep full width.
        $cropW = $srcW
        $cropH = [Math]::Round($srcW / $targetAspect)
        $x = 0
        $y = [Math]::Round(($srcH - $cropH) / 2.0)
    }

    $cropRect = New-Object System.Drawing.Rectangle($x, $y, $cropW, $cropH)
    $cropped = $src.Clone($cropRect, $src.PixelFormat)
    return $cropped
}

function Resize-To([System.Drawing.Bitmap]$src, [int]$w, [int]$h) {
    $dst = New-Object System.Drawing.Bitmap($w, $h)
    $gr = [System.Drawing.Graphics]::FromImage($dst)
    $gr.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $gr.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $gr.PixelOffsetMode   = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $gr.DrawImage($src, 0, 0, $w, $h)
    $gr.Dispose()
    return $dst
}

function Save-Capture([string]$destPath) {
    $raw = Capture-CalssWindow
    try {
        $cropped = Crop-To16x10 $raw
        try {
            $final = Resize-To $cropped $TargetWidth $TargetHeight
            try {
                $final.Save($destPath, [System.Drawing.Imaging.ImageFormat]::Png)
            } finally { $final.Dispose() }
        } finally { $cropped.Dispose() }
    } finally { $raw.Dispose() }

    $info = Get-Item $destPath
    Write-Host "  Saved: $destPath  ($TargetWidth x $TargetHeight, $([Math]::Round($info.Length/1KB)) KB)" -ForegroundColor Green
}

# -- Main ------------------------------------------------------------
if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
    Write-Host "Created $OutDir"
}

if ($DryRun) {
    Write-Host "DRY RUN - capturing current CALSS window state to _dryrun_test.png" -ForegroundColor Yellow
    Save-Capture (Join-Path $OutDir "_dryrun_test.png")
    exit 0
}

if ($PSBoundParameters.ContainsKey('Step')) {
    $def = $StepsDef | Where-Object { $_.N -eq $Step }
    Write-Host ""
    Write-Host "Step $($def.N)/9 - $($def.File)" -ForegroundColor Cyan
    Write-Host "  Needs: $($def.Prompt)"
    if (-not $Now) {
        Write-Host "  Navigate CALSS to that screen now, then press Enter here to capture..."
        Read-Host | Out-Null
    }
    Save-Capture (Join-Path $OutDir $def.File)
    exit 0
}

Write-Host "CALSS Help Carousel screenshot walkthrough - 9 steps." -ForegroundColor Cyan
Write-Host "Output folder: $OutDir"
Write-Host "Target size: ${TargetWidth}x${TargetHeight} (16:10)"
Write-Host ""

foreach ($def in $StepsDef) {
    Write-Host "Step $($def.N)/9 - $($def.File)" -ForegroundColor Cyan
    Write-Host "  Needs: $($def.Prompt)"
    Write-Host "  Navigate CALSS to that screen now, then press Enter here to capture..."
    Read-Host | Out-Null
    Save-Capture (Join-Path $OutDir $def.File)
    Write-Host ""
}

Write-Host "All 9 steps captured. Review each PNG in $OutDir before wiring them into the app." -ForegroundColor Green
