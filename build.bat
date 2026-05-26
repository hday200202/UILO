@echo off
REM Windows cmd.exe wrapper that just calls build.ps1.
where pwsh >NUL 2>NUL
if %ERRORLEVEL%==0 (
    pwsh -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
) else (
    powershell -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
)
