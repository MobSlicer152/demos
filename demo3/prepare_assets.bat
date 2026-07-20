setlocal

set model=6_0

mkdir %1\assets
%~dp0..\tools\dxc.exe -T vs_%model% -E VSMain %~dp0shaders.hlsl -Fo %1\assets\shaders.vs.bin
%~dp0..\tools\dxc.exe -T ps_%model% -E PSMain %~dp0shaders.hlsl -Fo %1\assets\shaders.ps.bin
python %~dp0..\tools\packfiles.py %1\assets > %1\demo.pak.cpp
