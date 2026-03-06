@echo off
setlocal
echo ========================================
echo   Football Manager - Compilador
echo ========================================
echo.

set SRC_DIR=%~dp0
set BUILD_DIR=%SRC_DIR%build
set OUTPUT=%SRC_DIR%FootballManager.exe

echo Compilando archivos...
echo.

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

call :compile "%SRC_DIR%main.cpp" "%BUILD_DIR%\main.o" || goto :compile_error
call :compile "%SRC_DIR%io.cpp" "%BUILD_DIR%\io.o" || goto :compile_error
call :compile "%SRC_DIR%models.cpp" "%BUILD_DIR%\models.o" || goto :compile_error
call :compile "%SRC_DIR%competition.cpp" "%BUILD_DIR%\competition.o" || goto :compile_error
call :compile "%SRC_DIR%app_services.cpp" "%BUILD_DIR%\app_services.o" || goto :compile_error
call :compile "%SRC_DIR%gui.cpp" "%BUILD_DIR%\gui.o" || goto :compile_error
call :compile "%SRC_DIR%simulation.cpp" "%BUILD_DIR%\simulation.o" || goto :compile_error
call :compile "%SRC_DIR%ui.cpp" "%BUILD_DIR%\ui.o" || goto :compile_error
call :compile "%SRC_DIR%utils.cpp" "%BUILD_DIR%\utils.o" || goto :compile_error
call :compile "%SRC_DIR%validators.cpp" "%BUILD_DIR%\validators.o" || goto :compile_error

echo Linking...
echo.

rem Enlazar todos los objetos
g++ "%BUILD_DIR%\main.o" "%BUILD_DIR%\io.o" "%BUILD_DIR%\models.o" "%BUILD_DIR%\competition.o" "%BUILD_DIR%\app_services.o" "%BUILD_DIR%\gui.o" "%BUILD_DIR%\simulation.o" "%BUILD_DIR%\ui.o" "%BUILD_DIR%\utils.o" "%BUILD_DIR%\validators.o" -o "%OUTPUT%" -std=c++17 -static -lcomctl32 -lgdi32

if errorlevel 1 (
    echo.
    echo [ERROR] Error en el linking!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Compilacion exitosa!
echo   Ejecutable: %OUTPUT%
echo ========================================
echo.
pause
exit /b 0

:compile
g++ -c %1 -o %2 -std=c++17 -static
exit /b %errorlevel%

:compile_error
echo.
echo [ERROR] Error en la compilacion!
pause
exit /b 1

