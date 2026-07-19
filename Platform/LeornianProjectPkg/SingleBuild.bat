@echo off
setlocal EnableExtensions EnableDelayedExpansion

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

for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
if not defined ROOT set "ROOT=%CD%"

:: Project Package
set PROJECT_PACKAGE=LeornianProjectPkg

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

:: Default Platform
set DEFAULT_DSC=%ROOT%\Platform\TestProjectPkg\TestProjectPkg.dsc

:: ============================================================
:: Parse Arguments
:: ============================================================

set BUILD_MODE=PACKAGE
set BUILD_PACKAGE=%DEFAULT_DSC%
set BUILD_MODULE=

if "%~1"=="" goto START

if /I "%~1"=="package" (
    if "%~2"=="" goto USAGE
    set "BUILD_MODE=PACKAGE"
    set "BUILD_PACKAGE=%~2"
    goto START
)

if /I "%~1"=="module" (
    if "%~2"=="" goto USAGE
    if "%~3"=="" goto USAGE
    set "BUILD_MODE=MODULE"
    set "BUILD_PACKAGE=%~2"
    set "BUILD_MODULE=%~3"
    goto START
)

goto USAGE


:START

echo.
echo ============================================================
echo Build Configuration
echo ============================================================

echo Mode      : %BUILD_MODE%
echo Package   : %BUILD_PACKAGE%

if /I "%BUILD_MODE%"=="MODULE" (
    echo Module    : %BUILD_MODULE%
)

echo Target    : %TARGET%
echo ToolChain : %TOOLCHAIN%
echo Arch      : %ARCH%

echo.
echo ============================================================
echo Setting up EDK2
echo ============================================================

if not exist "%EDK2%\edksetup.bat" (
    echo [ERROR] edksetup.bat not found at "%EDK2%\edksetup.bat"
    exit /b 1
)

pushd "%EDK2%" || (
    echo [ERROR] Cannot enter EDK2 directory: "%EDK2%"
    exit /b 1
)

call edksetup.bat Rebuild
if errorlevel 1 (
    echo.
    echo [ERROR] edksetup failed.
    popd
    exit /b 1
)

popd

echo.
echo ============================================================
echo Building BaseTools
echo ============================================================

pushd "%EDK_TOOLS_PATH%" || (
    echo [ERROR] Cannot enter BaseTools directory: "%EDK_TOOLS_PATH%"
    exit /b 1
)

nmake
if errorlevel 1 (
    echo.
    echo [ERROR] BaseTools build failed.
    popd
    exit /b 1
)

popd

:: Resolve relative package/module paths against the workspace root
if not exist "%BUILD_PACKAGE%" (
    if exist "%WORKSPACE%\%BUILD_PACKAGE%" (
        set "BUILD_PACKAGE=%WORKSPACE%\%BUILD_PACKAGE%"
    )
)

if /I "%BUILD_MODE%"=="MODULE" (
    if not exist "%BUILD_MODULE%" (
        if exist "%WORKSPACE%\%BUILD_MODULE%" (
            set "BUILD_MODULE=%WORKSPACE%\%BUILD_MODULE%"
        )
    )
)

echo.
echo ============================================================
echo Building...
echo ============================================================

if /I "%BUILD_MODE%"=="PACKAGE" (
    call build ^
         -p "%BUILD_PACKAGE%" ^
         -a %ARCH% ^
         -t %TOOLCHAIN% ^
         -b %TARGET% ^
         -y %ROOT%\Build\BuildReport.log ^
         -j Build.log
) else (
    call build ^
         -p "%BUILD_PACKAGE%" ^
         -m "%BUILD_MODULE%" ^
         -a %ARCH% ^
         -t %TOOLCHAIN% ^
         -b %TARGET% ^
         -y %ROOT%\Build\BuildReport.log ^
         -j Build.log
)

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo ============================================================
echo Build finished successfully.
echo ============================================================

pause
endlocal
exit /b 0


:USAGE
echo.
echo ============================================================
echo Usage
echo ============================================================
echo.
echo Build default platform:
echo.
echo     build.bat
echo.
echo Build package:
echo.
echo     build.bat package Platform\TestProjectPkg\TestProjectPkg.dsc
echo.
echo     build.bat package Features\TestUefiPkg\TestUefi.dsc
echo.
echo Build module:
echo.
echo     build.bat module Platform\TestProjectPkg\TestProjectPkg.dsc Platform\TestProjectPkg\Drivers\TestDriver\TestDriver.inf
echo.
echo     build.bat module Features\TestUefiPkg\TestUefi.dsc Features\TestUefiPkg\Application\HelloWorld\HelloWorld.inf
echo.
exit /b 1