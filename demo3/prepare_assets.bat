setlocal

set model=6_0

mkdir %1\assets
%~dp0..\tools\dxc.exe %DXC_FLAGS% -T vs_%model% -E VSMain %~dp0assets\shaders.hlsl -Fo %1\assets\shaders.vs.bin
%~dp0..\tools\dxc.exe %DXC_FLAGS% -T ps_%model% -E PSMain %~dp0assets\shaders.hlsl -Fo %1\assets\shaders.ps.bin
copy %~dp0assets\*.wav %1\assets
copy %~dp0assets\*.png %1\assets
python %~dp0..\tools\packfiles.py %1\assets > %1\demo.pak.cpp
