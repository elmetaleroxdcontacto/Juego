@echo off
setlocal EnableExtensions

echo ========================================
echo   Football Manager - Compilador
echo ========================================
echo.

set "SRC_DIR=%~dp0"
set "BUILD_DIR=%SRC_DIR%build"
set "OUTPUT=%SRC_DIR%FootballManager.exe"
set "CXX=g++"
set "CXXFLAGS=-std=c++17 -static"
set "LDFLAGS=-std=c++17 -static -lcomctl32 -lgdi32"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

for %%F in (main io models competition app_services gui simulation ui utils validators) do (
    echo Compilando %%F.cpp...
    %CXX% -c "%SRC_DIR%%%F.cpp" -o "%BUILD_DIR%\%%F.o" %CXXFLAGS%
    if errorlevel 1 goto :compile_error
)

echo.
echo Linking...
echo.

%CXX% ^
    "%BUILD_DIR%\main.o" ^
    "%BUILD_DIR%\io.o" ^
    "%BUILD_DIR%\models.o" ^
    "%BUILD_DIR%\competition.o" ^
    "%BUILD_DIR%\app_services.o" ^
    "%BUILD_DIR%\gui.o" ^
    "%BUILD_DIR%\simulation.o" ^
    "%BUILD_DIR%\ui.o" ^
    "%BUILD_DIR%\utils.o" ^
    "%BUILD_DIR%\validators.o" ^
    -o "%OUTPUT%" %LDFLAGS%
if errorlevel 1 goto :link_error

echo.
echo ========================================
echo   Compilacion exitosa!
echo   Ejecutable: %OUTPUT%
echo ========================================
echo.
echo Abriendo juego...
start "" "%OUTPUT%"
exit /b 0

:compile_error
echo.
echo [ERROR] Error en la compilacion.
exit /b 1

:link_error
echo.
echo [ERROR] Error en el linking.
exit /b 1

