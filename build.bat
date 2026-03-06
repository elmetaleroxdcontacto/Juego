@echo off
echo ========================================
echo   Football Manager - Compilador
echo ========================================
echo.

set SRC_DIR=%~dp0
set BUILD_DIR=%SRC_DIR%build
set OUTPUT=%SRC_DIR%FootballManager.exe

echo Compilando archivos...
echo.

rem Compilar cada archivo fuente
g++ -c "%SRC_DIR%main.cpp" -o "%BUILD_DIR%\main.o" -std=c++17 -static
g++ -c "%SRC_DIR%io.cpp" -o "%BUILD_DIR%\io.o" -std=c++17 -static
g++ -c "%SRC_DIR%models.cpp" -o "%BUILD_DIR%\models.o" -std=c++17 -static
g++ -c "%SRC_DIR%competition.cpp" -o "%BUILD_DIR%\competition.o" -std=c++17 -static
g++ -c "%SRC_DIR%simulation.cpp" -o "%BUILD_DIR%\simulation.o" -std=c++17 -static
g++ -c "%SRC_DIR%ui.cpp" -o "%BUILD_DIR%\ui.o" -std=c++17 -static
g++ -c "%SRC_DIR%utils.cpp" -o "%BUILD_DIR%\utils.o" -std=c++17 -static

if errorlevel 1 (
    echo.
    echo [ERROR] Error en la compilacion!
    pause
    exit /b 1
)

echo Linking...
echo.

rem Enlazar todos los objetos
g++ "%BUILD_DIR%\main.o" "%BUILD_DIR%\io.o" "%BUILD_DIR%\models.o" "%BUILD_DIR%\competition.o" "%BUILD_DIR%\simulation.o" "%BUILD_DIR%\ui.o" "%BUILD_DIR%\utils.o" -o "%OUTPUT%" -std=c++17 -static

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

