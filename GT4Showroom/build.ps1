param([string]$CompilerRoot,[string]$BuildDirectory,[switch]$RunTests)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
if(!$BuildDirectory){$BuildDirectory=Join-Path $root 'build'}
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$cc=Join-Path $CompilerRoot 'bin\i686-w64-mingw32-clang.exe'
$cpp=Join-Path $CompilerRoot 'bin\i686-w64-mingw32-clang++.exe'
$mh=Join-Path $root 'third_party\minhook';$objects=@()
foreach($unit in @('buffer.c','hook.c','trampoline.c','hde\hde32.c')){
 $obj=Join-Path $BuildDirectory ([IO.Path]::GetFileNameWithoutExtension($unit)+'.o')
 & $cc -O2 -c (Join-Path $mh "src\$unit") -o $obj
 if($LASTEXITCODE -ne 0){throw "Could not compile $unit"}
 $objects+=$obj
}
& $cpp -std=c++17 -O2 -Wall -Wextra -Werror -shared -static -I (Join-Path $mh 'include') (Join-Path $root 'source\GT4Showroom.cpp') @objects -ld3d9 -ldsound -ldxguid -lwinmm -lgdi32 -luser32 '-Wl,--kill-at' -o (Join-Path $root 'install\GT4Showroom.asi')
if($LASTEXITCODE -ne 0){throw 'ASI compilation failed'}
Write-Output 'Standalone GT4Showroom.asi compiled.'

if($RunTests){
 foreach($name in @('controller_v220_tests','logging_tests','studio_v210_tests','studio_v200_tests','interaction_v200_tests','stability_tests','realbig_v190_tests','secret_unlock_tests','prize_tests','presentation_tests','logic_tests','search_filter_tests','qol_tests','native_preview_tests','frontend_tests','config_tests','binary_smoke','audio_tests','font_tests')){
  $exe=Join-Path $BuildDirectory "$name.exe"
  & $cpp -std=c++17 -O2 -Wall -Wextra -Werror -static (Join-Path $root "tests\$name.cpp") -ld3d9 -ldsound -ldxguid -lwinmm -lgdi32 -luser32 -o $exe
  if($LASTEXITCODE -ne 0){throw "$name compilation failed"}
  if($name -eq 'presentation_tests'){& $exe (Join-Path $root 'tests\fixtures\carcols-full-names.dat')}
  elseif($name -eq 'config_tests'){& $exe (Join-Path $root 'install\GT4Showroom.ini')}
  elseif($name -eq 'binary_smoke'){& $exe (Join-Path $root 'install\GT4Showroom.asi')}
  elseif($name -eq 'audio_tests'){& $exe ((Join-Path $root 'install\GT4Showroom\assets\audio')+'\')}
  elseif($name -eq 'font_tests'){& $exe (Join-Path $root 'install\GT4Showroom\assets\fonts\Michroma-Regular.ttf') (Join-Path $root 'install\GT4Showroom\assets\fonts\LiberationSans-Bold.ttf')}
  else{& $exe}
  if($LASTEXITCODE -ne 0){throw "$name failed"}
 }
}

if($RunTests){
 foreach($kind in @('pass','preview')){
  $defines=@();if($kind -eq 'preview'){$defines=@('-DPREVIEW_PEER')}
  & $cpp -std=c++17 -O2 -Wall -Wextra -Werror -shared -static @defines -I (Join-Path $mh 'include') (Join-Path $root 'tests\render_chain_peer.cpp') @objects -ld3d9 -lgdi32 -luser32 '-Wl,--kill-at' -o (Join-Path $BuildDirectory "render-$kind.dll")
  if($LASTEXITCODE -ne 0){throw "Render chain $kind DLL compilation failed"}
 }
 & $cpp -std=c++17 -O2 -Wall -Wextra -Werror -static (Join-Path $root 'tests\render_chain_tests.cpp') -luser32 -o (Join-Path $BuildDirectory 'render_chain_tests.exe')
 if($LASTEXITCODE -ne 0){throw 'Render chain test compilation failed'}
 & (Join-Path $BuildDirectory 'render_chain_tests.exe') $BuildDirectory
 if($LASTEXITCODE -ne 0){throw 'Render chain integration failed'}
}



