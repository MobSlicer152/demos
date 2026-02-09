for %%x in (x64 x86) do (
    mkdir %~dp0%%x
    link /lib /nologo /def:%~dp0ntdll_libc.def /machine:%%x /out:%~dp0%%x\ntdll_libc.lib
    del %~dp0%%x\ntdll_libc.exp
)
