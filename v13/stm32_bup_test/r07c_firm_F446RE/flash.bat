@echo off

for %%a in (%cd%) do set pj_name=%%~na
set exe_name=wasca

set OPENOCD_CFG=../OpenOCD/stlink-v2-1.cfg
set OPENOCD_TARGET_CFG=../OpenOCD/stm32f4x.cfg
set OPENOCD_ARGS=-f %OPENOCD_CFG%
set OPENOCD_ARGS=%OPENOCD_ARGS% -f %OPENOCD_TARGET_CFG%

set OPENOCD_UPLOAD_CMDS=-c init
set OPENOCD_UPLOAD_CMDS=%OPENOCD_UPLOAD_CMDS% -c "reset halt"
set OPENOCD_UPLOAD_CMDS=%OPENOCD_UPLOAD_CMDS% -c "sleep 10"
set OPENOCD_UPLOAD_CMDS=%OPENOCD_UPLOAD_CMDS% -c "flash write_image erase build/%exe_name%.hex"
set OPENOCD_UPLOAD_CMDS=%OPENOCD_UPLOAD_CMDS% -c "sleep 10"
set OPENOCD_UPLOAD_CMDS=%OPENOCD_UPLOAD_CMDS% -c "reset run"
set OPENOCD_UPLOAD_CMDS=%OPENOCD_UPLOAD_CMDS% -c shutdown

openocd.exe %OPENOCD_ARGS% %OPENOCD_UPLOAD_CMDS%

