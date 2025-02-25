$qsb = "D:\vcpkg\installed\x64-windows\tools\Qt6\bin\qsb.exe"

& $qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o ./shader/vert.qsb ./shader/vert.vert
& $qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o ./shader/frag.qsb ./shader/frag.frag
