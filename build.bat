@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo ========================================
echo   Football Manager - Build
echo ========================================
echo.

for %%I in ("%~dp0.") do set "ROOT_DIR=%%~fI"
set "BUILD_DIR=%ROOT_DIR%\build"
set "CMAKE_BUILD_DIR=%ROOT_DIR%\build-cmake"
set "SRC_DIR=%ROOT_DIR%\src"
set "CXX=g++"
set "FALLBACK_CXXFLAGS=-std=c++17 -static -Iinclude -Isrc"

set "CMAKE_TARGETS=FootballManager"
set "PRIMARY_TARGET=FootballManager"
set "RUN_MODE=gui"
set "RUN_VALIDATE=0"
set "RUN_TESTS=0"
set "BUILD_ROUTE=Sin definir"
set "FORWARD_ARGS="
set "OUTPUT=%ROOT_DIR%\FootballManager.exe"
set "TEST_OUTPUT=%CMAKE_BUILD_DIR%\bin\FootballManagerTests.exe"

:parse_args
if "%~1"=="" goto :after_parse
if /i "%~1"=="--gui" (
    set "CMAKE_TARGETS=FootballManager"
    set "PRIMARY_TARGET=FootballManager"
    set "RUN_MODE=gui"
) else if /i "%~1"=="--cli" (
    set "CMAKE_TARGETS=FootballManagerCLI"
    set "PRIMARY_TARGET=FootballManagerCLI"
    set "RUN_MODE=console"
) else if /i "%~1"=="--validate" (
    set "CMAKE_TARGETS=FootballManagerCLI"
    set "PRIMARY_TARGET=FootballManagerCLI"
    set "RUN_MODE=validate"
    set "RUN_VALIDATE=1"
    set "FORWARD_ARGS=!FORWARD_ARGS! --validate"
) else if /i "%~1"=="--tests" (
    set "CMAKE_TARGETS=FootballManagerTests"
    set "PRIMARY_TARGET=FootballManagerTests"
    set "RUN_MODE=tests"
    set "RUN_TESTS=1"
) else if /i "%~1"=="--all" (
    set "CMAKE_TARGETS=FootballManager FootballManagerCLI FootballManagerTests"
    set "PRIMARY_TARGET=FootballManager"
    set "RUN_MODE=gui"
    set "RUN_TESTS=1"
) else if /i "%~1"=="--run-tests" (
    set "RUN_TESTS=1"
    if /i "!CMAKE_TARGETS!"=="FootballManager" set "CMAKE_TARGETS=FootballManager FootballManagerTests"
    if /i "!CMAKE_TARGETS!"=="FootballManagerCLI" set "CMAKE_TARGETS=FootballManagerCLI FootballManagerTests"
) else (
    set "FORWARD_ARGS=!FORWARD_ARGS! %~1"
)
shift
goto :parse_args

:after_parse
if /i "%PRIMARY_TARGET%"=="FootballManagerCLI" set "OUTPUT=%ROOT_DIR%\FootballManagerCLI.exe"
if /i "%PRIMARY_TARGET%"=="FootballManagerTests" set "OUTPUT=%ROOT_DIR%\FootballManagerTests.exe"
if /i "%PRIMARY_TARGET%"=="FootballManager" (
    set "FALLBACK_LDFLAGS=-std=c++17 -static -mwindows -lcomctl32 -lgdi32 -lwinmm"
) else (
    set "FALLBACK_LDFLAGS=-std=c++17 -static -lcomctl32 -lgdi32 -lwinmm"
)

if /i "%FM_FORCE_FALLBACK%"=="1" goto :legacy_build

where cmake >nul 2>nul
if errorlevel 1 goto :legacy_build

where mingw32-make >nul 2>nul
if errorlevel 1 goto :legacy_build

echo Usando CMake como sistema principal de compilacion...
if not exist "%CMAKE_BUILD_DIR%" mkdir "%CMAKE_BUILD_DIR%"
cmake -S "%ROOT_DIR%" -B "%CMAKE_BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=%CXX% > "%CMAKE_BUILD_DIR%\cmake-config.log" 2>&1
if errorlevel 1 goto :legacy_build_after_cmake_config

cmake --build "%CMAKE_BUILD_DIR%" --config Release --target %CMAKE_TARGETS% > "%CMAKE_BUILD_DIR%\cmake-build.log" 2>&1
if errorlevel 1 goto :legacy_build_after_cmake_build

set "BUILD_ROUTE=CMake"
set "OUTPUT=%CMAKE_BUILD_DIR%\bin\%PRIMARY_TARGET%.exe"
if not exist "%OUTPUT%" set "OUTPUT=%CMAKE_BUILD_DIR%\%PRIMARY_TARGET%.exe"
if not exist "%TEST_OUTPUT%" set "TEST_OUTPUT=%CMAKE_BUILD_DIR%\FootballManagerTests.exe"
goto :post_build

:legacy_build_after_cmake_config
echo CMake no pudo configurarse correctamente en este entorno.
echo Revisa "%CMAKE_BUILD_DIR%\cmake-config.log" para mas detalle.
goto :legacy_build

:legacy_build_after_cmake_build
echo CMake se configuro, pero la compilacion fallo en este entorno.
echo Revisa "%CMAKE_BUILD_DIR%\cmake-build.log" para mas detalle.

:legacy_build
set "BUILD_ROUTE=Fallback directo g++"
echo CMake no esta disponible o no pudo completarse. Se usa fallback directo con g++.
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

del /q "%BUILD_DIR%\*.o" 2>nul
set "OBJECTS="

for /R "%SRC_DIR%" %%F in (*.cpp) do (
    set "SKIP_FILE="
    if /i "%PRIMARY_TARGET%"=="FootballManager" if /i "%%~nxF"=="main.cpp" set "SKIP_FILE=1"
    if /i "%PRIMARY_TARGET%"=="FootballManagerCLI" if /i "%%~nxF"=="winmain.cpp" set "SKIP_FILE=1"
    if /i "%PRIMARY_TARGET%"=="FootballManagerTests" if /i "%%~nxF"=="main.cpp" set "SKIP_FILE=1"
    if /i "%PRIMARY_TARGET%"=="FootballManagerTests" if /i "%%~nxF"=="winmain.cpp" set "SKIP_FILE=1"

    if not defined SKIP_FILE (
        echo Compilando %%~nxF...
        %CXX% -c "%%~fF" -o "%BUILD_DIR%\%%~nF.o" %FALLBACK_CXXFLAGS%
        if errorlevel 1 goto :compile_error
        set "OBJECTS=!OBJECTS! "%BUILD_DIR%\%%~nF.o""
    )
)

if /i "%PRIMARY_TARGET%"=="FootballManagerTests" (
    echo Compilando project_tests.cpp...
    %CXX% -c "%ROOT_DIR%\tests\project_tests.cpp" -o "%BUILD_DIR%\project_tests.o" %FALLBACK_CXXFLAGS%
    if errorlevel 1 goto :compile_error
    set "OBJECTS=!OBJECTS! "%BUILD_DIR%\project_tests.o""
)

if not defined OBJECTS goto :no_sources

echo.
echo Linking...
echo.

%CXX% !OBJECTS! -o "%OUTPUT%" %FALLBACK_LDFLAGS%
if errorlevel 1 goto :link_error

if "%RUN_TESTS%"=="1" if /i not "%PRIMARY_TARGET%"=="FootballManagerTests" (
    echo Aviso: la ruta fallback compilo solo el target principal solicitado. Los tests automaticos requieren la ruta CMake.
)

:post_build
echo.
echo ========================================
echo   Compilacion exitosa
echo ========================================
echo Ruta usada : %BUILD_ROUTE%
echo Targets    : %CMAKE_TARGETS%
echo Principal  : %OUTPUT%
if exist "%TEST_OUTPUT%" echo Tests      : %TEST_OUTPUT%
echo ========================================
echo.

if /i "%FM_SKIP_RUN%"=="1" (
    echo FM_SKIP_RUN=1 detectado. Se omite la ejecucion posterior.
    exit /b 0
)

if "%RUN_TESTS%"=="1" (
    if exist "%TEST_OUTPUT%" (
        echo Ejecutando FootballManagerTests...
        pushd "%ROOT_DIR%"
        "%TEST_OUTPUT%"
        set "TEST_EXIT=%ERRORLEVEL%"
        popd
        if not "%TEST_EXIT%"=="0" exit /b %TEST_EXIT%
    ) else if /i "%PRIMARY_TARGET%"=="FootballManagerTests" (
        echo Ejecutando FootballManagerTests...
        pushd "%ROOT_DIR%"
        "%OUTPUT%"
        set "TEST_EXIT=%ERRORLEVEL%"
        popd
        if not "%TEST_EXIT%"=="0" exit /b %TEST_EXIT%
    )
)

if /i "%RUN_MODE%"=="tests" exit /b 0
if /i "%RUN_MODE%"=="console" goto :run_console
if /i "%RUN_MODE%"=="validate" goto :run_console

echo Abriendo juego desde la raiz del proyecto...
start "" /D "%ROOT_DIR%" "%OUTPUT%" %FORWARD_ARGS%
exit /b 0

:run_console
echo Ejecutando juego en esta consola desde %ROOT_DIR%...
pushd "%ROOT_DIR%"
"%OUTPUT%" %FORWARD_ARGS%
set "RUN_EXIT=%ERRORLEVEL%"
popd
exit /b %RUN_EXIT%

:compile_error
echo.
echo [ERROR] Error en la compilacion.
exit /b 1

:no_sources
echo.
echo [ERROR] No se encontraron archivos .cpp para compilar.
exit /b 1

:link_error
echo.
echo [ERROR] Error en el linking.
exit /b 1
