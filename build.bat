@echo off
setlocal

set VERSION=v0.0.13

set KEIL_DIR=%LOCALAPPDATA%\Keil_v5\UV4
set PROJECT_DIR=%~dp0\Project\MDK5
set GPG_PATH=C:\Program Files (x86)\GnuPG\bin

:: Check if Keil uVision4 is installed
if not exist "%KEIL_DIR%\UV4.exe" (
    echo Keil uVision4 not found in %KEIL_DIR%. Please install Keil uVision4.
    exit /b 1
)

:: Build the project using Keil uVision4 in command line mode
"%KEIL_DIR%\UV4.exe" -b "%PROJECT_DIR%\ALCM.uvprojx" -t"ALCM Multi" -o "%PROJECT_DIR%\alcm-multi-build.log" -j0
move /Y "%PROJECT_DIR%\Objects\ALCM.hex" "%PROJECT_DIR%\ALCM-Multi_%VERSION%.hex"

"%KEIL_DIR%\UV4.exe" -b "%PROJECT_DIR%\ALCM.uvprojx" -t"ALCM PintV" -o "%PROJECT_DIR%\alcm-pintv-build.log" -j0
move /Y "%PROJECT_DIR%\Objects\ALCM.hex" "%PROJECT_DIR%\ALCM-PintV_%VERSION%.hex"

"%KEIL_DIR%\UV4.exe" -b "%PROJECT_DIR%\ALCM.uvprojx" -t"ALCM XRV" -o "%PROJECT_DIR%\alcm-xrv-build.log" -j0
move /Y "%PROJECT_DIR%\Objects\ALCM.hex" "%PROJECT_DIR%\ALCM-XRV_%VERSION%.hex"