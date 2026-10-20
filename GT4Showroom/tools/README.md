# Shared renderer tools

preview_prize.cpp is the standalone Win32 Direct3D9 host for the shipped
Model/Renderer/Prize code. The1.9.1 preview uses the restored stationary BGRA
backdrop and the same lower-floor UV map, camera, actual car/reflection/shadow,
text and controls as the ASI. A short preparation wait and green return gameplay
are host stand-ins. GTA streaming/tuning/physics/native plugins are not executed.

Build with Win32 C++17 Clang and link d3d9,gdiplus,gdi32,user32. Arguments:
fixture folder, frame output, frame count, Michroma file, starting index,
normal/benchmark, hardware/software/mixed, geometry multiplier, catalogue size,
country override(-1 normal), paint-page flag, motion time, prize/loading,
width,height, optional INI path.1080 frames makes the36s movie.

Existing export_addon_prize.py and export_prize_meshes.py use Blender/DragonFF
and shared plan helpers to export actual supplied car/upgrade fixtures. The
editable Calico .blend is retained. No new Blender export was needed for1.9.1.
See prize-framing-v191.md and previews/provenance.json for current presentation.
Earlier research notes are historical, not current game-test evidence.
