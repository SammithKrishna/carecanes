@echo off
setlocal
set TASK_NAME=CareCaneTwilioServer
set SCRIPT_PATH=%~dp0start_server.bat

schtasks /Create /TN "%TASK_NAME%" /TR "\"%SCRIPT_PATH%\"" /SC ONLOGON /RL LIMITED /F
if %ERRORLEVEL% EQU 0 (
  echo Autostart task created: %TASK_NAME%
) else (
  echo Failed to create task. Try running this file as Administrator.
)
endlocal
