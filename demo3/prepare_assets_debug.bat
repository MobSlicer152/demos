@echo off

set "DXC_FLAGS=-Zi -Qembed_debug -Qsource_in_debug_module"
call %~dp0prepare_assets.bat %*
