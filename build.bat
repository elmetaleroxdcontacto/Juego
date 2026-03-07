@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo ========================================
echo   Football Manager - Compilador
echo ========================================
echo.

set "SRC_DIR=%~dp0"
set "BUILD_DIR=%SRC_DIR%build"
set "OUTPUT=%SRC_DIR%FootballManager.exe"
set "CXX=g++"
set "CXXFLAGS=-std=c++17 -static -I. -Iinclude -Isrc"
set "LDFLAGS=-std=c++17 -static -lcomctl32 -lgdi32"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

del /q "%BUILD_DIR%\*.o" 2>nul

set "OBJECTS="

for /R "%SRC_DIR%" %%F in (*.cpp) do (
    echo Compilando %%~nxF...
    %CXX% -c "%%~fF" -o "%BUILD_DIR%\%%~nF.o" %CXXFLAGS%
    if errorlevel 1 goto :compile_error
    set "OBJECTS=!OBJECTS! "%BUILD_DIR%\%%~nF.o""
)

if not defined OBJECTS goto :no_sources

echo.
echo Linking...
echo.

%CXX% !OBJECTS! -o "%OUTPUT%" %LDFLAGS%
if errorlevel 1 goto :link_error

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
echo Abriendo juego...
start "" "%OUTPUT%"
exit /b 0

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

