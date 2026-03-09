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
set "OUTPUT=%ROOT_DIR%\FootballManager.exe"
set "GAME_ARGS=%*"
set "CXX=g++"
set "FALLBACK_CXXFLAGS=-std=c++17 -static -Iinclude -Isrc"
set "FALLBACK_LDFLAGS=-std=c++17 -static -lcomctl32 -lgdi32"

if /i "%FM_FORCE_FALLBACK%"=="1" goto :legacy_build

where cmake >nul 2>nul
if errorlevel 1 goto :legacy_build

where mingw32-make >nul 2>nul
if errorlevel 1 goto :legacy_build

echo Usando CMake como sistema principal de compilacion...
if not exist "%CMAKE_BUILD_DIR%" mkdir "%CMAKE_BUILD_DIR%"
cmake -S "%ROOT_DIR%" -B "%CMAKE_BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=%CXX% > "%CMAKE_BUILD_DIR%\cmake-config.log" 2>&1
if errorlevel 1 goto :legacy_build_after_cmake_config

cmake --build "%CMAKE_BUILD_DIR%" --config Release > "%CMAKE_BUILD_DIR%\cmake-build.log" 2>&1
if errorlevel 1 goto :legacy_build_after_cmake_build

set "OUTPUT=%CMAKE_BUILD_DIR%\bin\FootballManager.exe"
if not exist "%OUTPUT%" set "OUTPUT=%CMAKE_BUILD_DIR%\FootballManager.exe"
goto :run_game

:legacy_build_after_cmake_config
echo CMake no pudo configurarse correctamente en este entorno.
echo Revisa "%CMAKE_BUILD_DIR%\cmake-config.log" para mas detalle.
goto :legacy_build

:legacy_build_after_cmake_build
echo CMake se configuro, pero la compilacion fallo en este entorno.
echo Revisa "%CMAKE_BUILD_DIR%\cmake-build.log" para mas detalle.

:legacy_build
echo CMake no esta disponible o no pudo configurarse. Se usa fallback directo con g++.
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

del /q "%BUILD_DIR%\*.o" 2>nul

set "OBJECTS="

for /R "%SRC_DIR%" %%F in (*.cpp) do (
    echo Compilando %%~nxF...
    %CXX% -c "%%~fF" -o "%BUILD_DIR%\%%~nF.o" %FALLBACK_CXXFLAGS%
    if errorlevel 1 goto :compile_error
    set "OBJECTS=!OBJECTS! "%BUILD_DIR%\%%~nF.o""
)

if not defined OBJECTS goto :no_sources

echo.
echo Linking...
echo.

%CXX% !OBJECTS! -o "%OUTPUT%" %FALLBACK_LDFLAGS%
if errorlevel 1 goto :link_error

:run_game
echo.
echo ========================================
echo   Compilacion exitosa!
echo   Ejecutable: %OUTPUT%
echo ========================================
echo.
if /i "%FM_SKIP_RUN%"=="1" (
    echo FM_SKIP_RUN=1 detectado. Se omite la ejecucion del juego.
    exit /b 0
)
if /i "%~1"=="--cli" goto :run_console
if /i "%~1"=="--validate" goto :run_console

echo Abriendo juego desde la raiz del proyecto...
start "" /D "%ROOT_DIR%" "%OUTPUT%" %GAME_ARGS%
exit /b 0

:run_console
echo Ejecutando juego en esta consola desde %ROOT_DIR%...
pushd "%ROOT_DIR%"
"%OUTPUT%" %GAME_ARGS%
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
