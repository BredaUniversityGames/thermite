@echo off
setlocal enabledelayedexpansion

echo.
echo ================================================
echo   Unreferenced Asset Finder
echo ================================================
echo.

REM Script must be copied to the project root folder (e.g. projects/game/)
set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"
set "ASSETS_DIR=%PROJECT_DIR%\assets"
set "SCENES_DIR=%ASSETS_DIR%\scenes"

if not exist "%ASSETS_DIR%\" (
    echo ERROR: No 'assets' folder found.
    echo Expected: %ASSETS_DIR%
    echo.
    echo Copy this script to your project root folder ^(e.g. projects/game/^) and run it.
    echo.
    pause
    exit /b 1
)

echo Project : %PROJECT_DIR%
echo Assets  : %ASSETS_DIR%
echo.
echo Note: Files in assets\scenes\ are skipped ^(always referenced from code^).
echo.
echo Scanning... ^(this may take a moment^)
echo.

set "TOTAL=0"
set "UNREF=0"

REM Store unreferenced file paths in a temp file so we can delete them later
set "TMPFILE=%TEMP%\unref_assets_%RANDOM%.txt"
if exist "%TMPFILE%" del "%TMPFILE%"

echo ================================================
echo   Unreferenced Files:
echo ================================================

for /r "%ASSETS_DIR%" %%F in (*.*) do (
    set "ASSET_FULL=%%F"
    set "ASSET_NAME=%%~nxF"

    REM Skip files inside the scenes folder
    set "IS_SCENE_DIR=0"
    echo "%%F" | findstr /i /c:"%SCENES_DIR%\" >nul 2>&1
    if not errorlevel 1 set "IS_SCENE_DIR=1"

    if "!IS_SCENE_DIR!"=="0" (
        set /a TOTAL+=1

        REM Build relative path from assets dir, then convert to forward slashes for JSON matching
        set "REL=%%F"
        set "REL=!REL:%ASSETS_DIR%\=!"
        set "REL_FWD=!REL:\=/!"

        set "FOUND=0"

        REM --- Search in .scene files ---
        for /r "%ASSETS_DIR%" %%S in (*.scene) do (
            if "!FOUND!"=="0" if /i not "%%F"=="%%S" (
                findstr /i /c:"!REL_FWD!" "%%S" >nul 2>&1
                if not errorlevel 1 set "FOUND=1"
            )
        )

        REM --- Search in .prefab files ---
        if "!FOUND!"=="0" (
            for /r "%ASSETS_DIR%" %%P in (*.prefab) do (
                if "!FOUND!"=="0" if /i not "%%F"=="%%P" (
                    findstr /i /c:"!REL_FWD!" "%%P" >nul 2>&1
                    if not errorlevel 1 set "FOUND=1"
                )
            )
        )

        REM --- Search in .json config files ---
        if "!FOUND!"=="0" (
            for /r "%ASSETS_DIR%" %%J in (*.json) do (
                if "!FOUND!"=="0" if /i not "%%F"=="%%J" (
                    findstr /i /c:"!REL_FWD!" "%%J" >nul 2>&1
                    if not errorlevel 1 set "FOUND=1"
                )
            )
        )

        REM --- Search in C++ source files ---
        REM Check both relative path and bare filename for assets hardcoded in source.
        if "!FOUND!"=="0" (
            for /r "%PROJECT_DIR%" %%C in (*.cpp *.hpp *.h) do (
                if "!FOUND!"=="0" (
                    findstr /i /c:"!REL_FWD!" "%%C" >nul 2>&1
                    if not errorlevel 1 (
                        set "FOUND=1"
                    ) else (
                        findstr /i /c:"!ASSET_NAME!" "%%C" >nul 2>&1
                        if not errorlevel 1 set "FOUND=1"
                    )
                )
            )
        )

        if "!FOUND!"=="0" (
            set /a UNREF+=1
            echo   !REL!
            echo %%F>>"%TMPFILE%"
        )
    )
)

if "%UNREF%"=="0" echo   ^(none -- all assets are referenced^)

set /a REF=TOTAL-UNREF

echo.
echo ================================================
echo   Summary
echo ================================================
echo   Total assets  : %TOTAL%
echo   Referenced    : %REF%
echo   Unreferenced  : %UNREF%
echo ================================================
echo.

REM --- Offer to delete unreferenced files ---
if "%UNREF%"=="0" goto :cleanup

set /p "ANSWER=Delete all %UNREF% unreferenced file(s)? This cannot be undone. [y/N]: "
if /i not "!ANSWER!"=="y" goto :cleanup

echo.
echo Deleting...
set "DELETED=0"
set "FAILED=0"

for /f "usebackq delims=" %%D in ("%TMPFILE%") do (
    del /f /q "%%D" >nul 2>&1
    if not errorlevel 1 (
        set /a DELETED+=1
        echo   Deleted: %%D
    ) else (
        set /a FAILED+=1
        echo   FAILED:  %%D
    )
)

echo.
echo Done. Deleted: %DELETED%   Failed: %FAILED%
echo.

:cleanup
if exist "%TMPFILE%" del "%TMPFILE%"
pause
