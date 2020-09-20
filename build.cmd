@echo off
setlocal

set SCRIPT_DIR=%~dp0

if "%VSINSTALLDIR%"=="" (
    echo Run VsDevCmd.bat beforehand. Probably at:
    echo "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"
    goto error
)

if "%BOOST_ROOT%"=="" (
    echo Set BOOST_ROOT to the root directory of the boost library.
    goto error
)

if "%KPISDK_ROOT%"=="" (
    echo Set KPISDK_ROOT to the root directory of the KbMedia Player plugin SDK.
    goto error
)

MSBuild /nologo /p:Platform=x86 /p:Configuration=Release /v:Quiet %SCRIPT_DIR%\src\KbAsciiMml.sln
MSBuild /nologo /p:Platform=x64 /p:Configuration=Release /v:Quiet %SCRIPT_DIR%\src\KbAsciiMml.sln

exit /b

:error
exit /b 1