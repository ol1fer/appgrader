# AppGrader

Per-monitor and per-window colour grading for KDE Plasma 6 on Linux.

You get exposure, contrast, tone, colour wheels, HSL, curves and external
.cube LUTs, plus screen-space stuff like vignette, noise, sharpen, clarity
and bloom. Everything goes through a small KWin compositor effect, so the
grade ends up in screenshots, screencasts, OBS etc. No DLL injection or
window hooks involved, so it stays out of the way of games and anti-cheat.

## Status

Linux + KDE Plasma 6 (Wayland or X11). Windows port is on the wishlist but
DWM doesn't have an equivalent compositor effect API, so it's not a quick
porting job.

## Build

You'll need Qt 6, KF6 CoreAddons, KWin development headers, ECM, and
pkexec for the install step.

Ubuntu / Kubuntu (Plasma 6):

    sudo apt install build-essential cmake ninja-build pkg-config \
        qt6-base-dev qt6-base-private-dev \
        libgl1-mesa-dev libegl-dev libxkbcommon-dev \
        extra-cmake-modules libkf6coreaddons-dev kwin-dev

Arch:

    sudo pacman -S base-devel cmake ninja qt6-base \
        extra-cmake-modules kcoreaddons kwin

Then:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build

That gives you `build/appgrader` (the GUI) and
`build/src/kwin_effect/appgrader_lut.so` (the compositor effect).

## First run

1. Run `build/appgrader`.
2. Click "KWin Effect..." in the bottom toolbar.
3. Hit "Install / Update". pkexec will ask for your password and copy the
   .so into `/usr/lib/qt6/plugins/kwin/effects/plugins/`.
4. Log out and log back in. KWin caches plugin factories, so a freshly
   installed .so won't pick up until KWin actually restarts.
5. Move some sliders. The grade should appear on the chosen target.

If something goes badly wrong, kill the effect with:

    qdbus org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect appgrader_lut

## Releases

Tagged releases get a tarball attached on the Releases page. They're built
against the Qt 6 / Plasma 6 ABI on the latest Ubuntu LTS. If your distro is
on a different KWin minor version the bundled .so might fail to load, in
which case build from source, it isn't slow.
