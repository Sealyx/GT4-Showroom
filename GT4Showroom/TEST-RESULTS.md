# GT4 Showroom2.2.0 validation

Warnings-as-errors Win32 C++17 build passed. All19 isolated executable suites and
separate-DLL MinHook render chains in both load orders passed. New tests exercise
production StudioControls via PadActions: open/eligibility/held-release gates,
native plus idle-XInput merging, scoped native Escape pulses and native clear,
controller-only sidebar reload, country filter, Favorites, search keyboard,
paint/livery/back, cinematic pause/orbit/zoom and South Korea/legacy INI spelling.
Existing logging, state ownership, 10,000 physical-field updates, native capture,
streaming, palettes, livery, audio/fonts, loader and unsupported-host checks pass.

Fresh production D3D9 audit: clean/contaminated pixels identical and caller state
restored. Reset/menu/cinematic/controller-keyboard checks pass at800x600,
1920x1080,2560x1080 and5120x1440;2xMSAA host target passes. At1280x720,
120 warmed renderer submissions averaged1.1769ms, maximum1.9780ms,
52,365 triangles/294draws/zero new uploads. This is CPU submission timing in the
standalone harness, not GTA FPS or a comparison benchmark.

A660frame/22second controller-driven video uses the actual UI/input/renderer code.
Model services are controlled fixtures, including private locally exported car
meshes and native Sultan paintjob pixels; GTA and physical controller hardware do
not execute. All-frame submission average1.973ms/max522.029ms includes cold font/
GPU preparation; the warm audit above isolates steady-state submission. Neither
figure establishes gameplay performance. Private model fixtures are not packaged.

The prior2.1.1 baseline was accepted and marked Finished by the user. This new2.2.0
binary is UNTESTED IN GTA. Retest with an actual native mapped pad and an XInput
pad: View/Select opening on foot, in-car/cutscene gates, all sidebar/submenus,
Reload settings, search typing/Find/Delete/Cancel, Favorites and empty view,
colors/liveries/spawn/prize, cinematic controls, Start pause/resume, held buttons
on exit, Alt-Tab, disconnect/reconnect, reset and normal closed gameplay.
No live installation or GTA launch was performed.
