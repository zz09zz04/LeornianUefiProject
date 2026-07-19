@echo off
setlocal

:: ============================================================
:: Encoding
:: ============================================================
set CL=/utf-8
chcp 65001 >nul
set PYTHONIOENCODING=utf-8
set PYTHONUTF8=1
set LC_ALL=en_US.UTF-8

:: ============================================================
:: Project Configuration
:: ============================================================
set ROOT=%~dp0..\..

:: Project Package
set PROJECT_NAME=Leornian
set PROJECT_PACKAGE=%PROJECT_NAME%ProjectPkg

:: EDK2 Root
set EDK2=%ROOT%\Edk2

:: Build Environment
set WORKSPACE=%ROOT%
set PACKAGES_PATH=%EDK2%;%ROOT%\Features;%ROOT%\Platform
set EDK_TOOLS_PATH=%EDK2%\BaseTools
set CONF_PATH=%EDK2%\Conf

:: Tool Settings
set IASL_PREFIX=%ROOT%\Platform\%PROJECT_PACKAGE%\Tools\Asl\iasl-win-20250404\
set NASM_PREFIX=%ROOT%\Platform\%PROJECT_PACKAGE%\Tools\Nasm\nasm-3.02\

:: Build Options
set TARGET=DEBUG
set TOOLCHAIN=VS2022
set ARCH=X64
set BUILD_MACROS=

:: Validate quoted macro values only when arguments were supplied.
if not "%~1"=="" (
    call :validate_macro_arguments %*
    if errorlevel 1 goto usage
)

:: Command line arguments
:: %1 : TARGET (e.g. DEBUG/RELEASE/NOOPT)
:: %2..: Macro names/values to be passed as -D <macro>
::       Standalone flag  : MACRO_NAME
::       Key=Value macro  : "MACRO_NAME=VALUE"  (quotes required, = is a cmd delimiter)
if not "%~1"=="" (
    set TARGET=%~1
    shift
)

set VALID_TARGET=
for %%T in (DEBUG RELEASE NOOPT) do (
    if /I "%TARGET%"=="%%T" set VALID_TARGET=1
)
if not defined VALID_TARGET (
    set ERR_MSG=[ERROR] Invalid TARGET: %TARGET%
    goto usage
)

:parse_macros
if "%~1"=="" goto args_done
set BUILD_MACROS=%BUILD_MACROS% -D %~1
shift
goto parse_macros

:args_done
echo BUILD_MACROS=%BUILD_MACROS%

:: DSC
set DSC=%ROOT%\Platform\%PROJECT_PACKAGE%\%PROJECT_PACKAGE%.dsc

:: Project output directory (must match OUTPUT_DIRECTORY in DSC)
set OUTPUT_DIRECTORY=%ROOT%\Build\%PROJECT_PACKAGE%

:: Feature Packages
:: ============================================================
:: Detect feature submodules under Features\ and generate a small
:: DSC fragment with conditional includes. This logic is separated
:: into GenerateFeatureIncludes.bat to keep the main build script clean.
:: ============================================================

call "%ROOT%\Platform\%PROJECT_PACKAGE%\GenerateFeatureIncludes.bat"


echo.
echo ============================================================
echo Setting up EDK2
echo ============================================================

pushd "%EDK2%"
call edksetup.bat Rebuild
if errorlevel 1 (
    echo.
    echo [ERROR] edksetup failed.
    popd
    pause
    exit /b 1
)
popd

::echo.
::echo ============================================================
::echo Building BaseTools
::echo ============================================================
::
::pushd "%EDK_TOOLS_PATH%"
::nmake
::if errorlevel 1 (
::    echo.
::    echo [ERROR] BaseTools build failed.
::    popd
::    pause
::    exit /b 1
::)
::popd

echo.
echo ============================================================
echo Building Project
echo ============================================================

call build ^
    -p %DSC% ^
    -a %ARCH% ^
    -t %TOOLCHAIN% ^
    -b %TARGET% ^
    -y %OUTPUT_DIRECTORY%\BuildReport.log ^
    -j Build.log ^
    %BUILD_MACROS%

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo ============================================================
echo Build finished successfully.
echo ============================================================

pause
endlocal
exit /b 0

:usage
echo.
if defined ERR_MSG echo %ERR_MSG%
echo Usage: %~nx0 [DEBUG^|RELEASE^|NOOPT] [MACRO1] ["MACRO2=VALUE"] ...
echo        NOTE: macros with '=' must be quoted to avoid cmd.exe splitting them.
pause
endlocal
exit /b 1

:validate_macro_arguments
setlocal EnableDelayedExpansion
set "ARGUMENTS=%*"
set /a INDEX=0, IN_QUOTES=0

:validate_next_character
set "CHAR=!ARGUMENTS:~%INDEX%,1!"
if not defined CHAR (
    endlocal
    exit /b 0
)
set "CHAR_WITHOUT_QUOTE=!CHAR:"=!"
if not defined CHAR_WITHOUT_QUOTE set /a IN_QUOTES=1-IN_QUOTES

if "!CHAR!"=="=" if !IN_QUOTES!==0 (
    endlocal & set "ERR_MSG=[ERROR] Macro containing '=' must be enclosed in double quotes. Example: "MACRO=VALUE""
    exit /b 1
)
set /a INDEX+=1
goto validate_next_character
