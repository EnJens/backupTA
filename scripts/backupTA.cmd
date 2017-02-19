@echo off
cd /d %~dp0

for /F "delims=" %%a in ('windows\adb.exe shell getprop ro.product.cpu.abi') do set ARCH=%%a
for /F "delims=" %%a in ('windows\adb.exe shell getprop ro.product.device') do set DEVICE=%%a
for /F "delims=" %%a in ('windows\adb.exe shell getprop ro.serialno') do set SERIAL=%%a

set SUFFIX=32
IF %ARCH% == "arm64-v8a" (
   set SUFFIX=64
)

for /f "tokens=1-8 delims=.:,/ " %%a in ("%date% %time%") do set DateNtime=%%a%%b%%c-%%d%%e
set TAIMG=TA_%DEVICE%_%SERIAL%_%DateNtime%.img

windows\adb.exe push files/dirtycow%SUFFIX% /data/local/tmp/dirtycow
windows\adb.exe push files/run-as%SUFFIX% /data/local/tmp/run-as
windows\adb.exe push files/exploitta%SUFFIX% /data/local/tmp/exploitta
windows\adb.exe push files/dumpta%SUFFIX% /sdcard/dumpta
windows\adb.exe push files/backupTA.sh /data/local/tmp
windows\adb.exe shell "chmod 755 /data/local/tmp/*"
windows\adb.exe shell "/data/local/tmp/backupTA.sh %TAIMG%"
windows\adb.exe pull /data/local/tmp/%TAIMG% %TAIMG%
windows\adb.exe shell "rm -f /data/local/tmp/dirtycow /data/local/tmp/run-as /data/local/tmp/exploitta /sdcard/dumpta /data/local/tmp/backupTA.sh"
echo TA Sucessfully pulled to %TAIMG%
pause
