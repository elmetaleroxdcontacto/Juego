# Football Manager Chile Simulator

Simulador de gestion futbolistica en C++ inspirado en la profundidad de juegos tipo Football Manager, con foco en carrera, scouting, vestuario, mercado y motor de partido.

El proyecto ya es jugable y hoy combina tres capas que trabajan juntas:

- frontend inicial `Chilean Footballito` con menu principal y configuraciones compartidas
- simulacion de partidos con fases, contexto tactico, xG, riesgo y fatiga
- modo carrera con directiva, finanzas, scouting, contratos, cantera y staff
- GUI Win32 para jugar sin consola, mas una CLI para validacion y depuracion

## Que trae hoy

### Carrera y gestion

- multiples divisiones chilenas
- calendario semanal y transicion de temporada
- objetivos de directiva y confianza del cargo
- finanzas, deuda, sponsor, salarios e infraestructura
- mercado con shortlist, scouting, precontratos, cesiones y negociacion estructurada
- desarrollo mensual, progresion juvenil y manejo de carga
- vestuario con moral, promesas, tension social y reuniones de plantel

### Profundidad tipo Football Manager

- mesa de staff con recomendaciones semanales priorizadas
- centro del manager con agenda, scouting, mercado e inbox combinado
- scouting por cobertura regional, foco posicional y confianza de informe
- lectura de proyecto de club en negociaciones: estilo, cantera, politica, clima interno y seguridad del cargo
- dashboard con contexto de rival, microciclo, decisiones sugeridas y narrativa semanal

### Motor de partido

- contexto previo al partido y snapshots de cada equipo
- partido dividido en seis fases estructuradas
- posesion, progresiones, ataques, ocasiones, xG y eventos tipados
- influencia de tactica, instruccion de partido, moral, disponibilidad y fatiga
- lesiones con cambio forzado real
- reportes post partido con explicacion tactica y recomendaciones

### GUI Win32

- menu de inicio con presentacion tipo dashboard y `Chilean Footballito` como portada central
- portada Win32 con `Jugar`, `Configuraciones`, chips de estado y roadmap visual para futuras opciones
- `FootballManager.exe` sin consola innecesaria
- arranque maximizado con soporte DPI
- boton y `F11` para alternar ventana, maximizado y fullscreen sin borde
- dashboard owner-draw con KPIs e insights clicables
- tablas con autosize por vista y suavizado entre refrescos

## Ejecutables

Los binarios principales quedan en `build-cmake/bin/`:

- `FootballManager.exe`: version GUI principal
- `FootballManagerCLI.exe`: version consola
- `FootballManagerTests.exe`: suite automatizada

## Requisitos

### Windows

- CMake 3.16 o superior
- MinGW con `g++`
- `mingw32-make`

### Otros entornos

El proyecto puede compilarse tambien fuera de Windows con CMake, pero la GUI actual es Win32. En esos casos el flujo principal es la CLI.

## Compilacion

### Opcion recomendada: CMake

```powershell
cmake -S . -B build-cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests
```

### Opcion rapida en Windows

```powershell
build.bat
```

Si solo quieres compilar sin ejecutar automaticamente:

```powershell
$env:FM_SKIP_RUN='1'
cmd /c build.bat
```

## Ejecucion

### GUI

```powershell
.\build-cmake\bin\FootballManager.exe
```

### Consola

```powershell
.\build-cmake\bin\FootballManagerCLI.exe
```

La CLI abre primero el frontend textual `Chilean Footballito`, con paneles ASCII, navegacion por `W/S`, `Enter`, numeros y acceso al submenu de configuraciones.

### Validacion de datos y proyecto

```powershell
.\build-cmake\bin\FootballManagerCLI.exe --validate
```

## Tests

Compilar y ejecutar:

```powershell
cmake --build build-cmake --config Release --target FootballManagerTests
.\build-cmake\bin\FootballManagerTests.exe
```

La suite valida, entre otras cosas:

- integridad de datos y carga inicial
- estructura del motor de partido
- urgencia competitiva, fatiga y perfil tactico
- scouting, shortlist y negociacion
- riesgo medico y reemplazos por lesion
- desarrollo mensual y servicios de carrera
- guardado, carga y transicion de temporada

## Arquitectura

### Modulos principales

- `engine`: estado base, modelos y controlador del juego
- `simulation`: contexto de partido, fases, eventos, resolucion y reportes
- `career`: servicios de carrera, inbox, staff, narrativas, semana y mundo
- `ai`: planificacion de plantilla, ajustes CPU y evaluacion de mercado
- `transfers`: mercado, tipos y negociacion
- `development`: entrenamiento, progresion y cantera
- `finance`: proyecciones y buffer economico
- `gui`: runtime, layout, acciones y vistas Win32
- `ui`: flujo de consola y pantallas de texto
- `io`: guardado, carga y serializacion
- `validators`: chequeos de integridad y arranque

### Estructura del repo

```text
include/
src/
tests/
data/
docs/
saves/
tools/
```

## Flujo de entrada

- `src/winmain.cpp`: entrada GUI en Windows
- `src/main.cpp`: entrada consola
- `src/engine/game_controller.cpp`: seleccion de GUI, CLI o validacion
- `src/engine/front_menu.cpp`: frontend compartido para el menu principal y configuraciones

## Documentacion adicional

- [Arquitectura](docs/ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Reporte de limpieza de datos](docs/data_cleanup_report.md)

## Roadmap natural

- seguir profundizando el motor de partido con mas lectura rival y balon parado
- ampliar staff y networking del mercado con mas memoria del mundo
- enriquecer cantera, intake juvenil y narrativa de largo plazo
- pulir la GUI con mas widgets de comparacion y centros de informacion

## Repositorio

GitHub: https://github.com/elmetaleroxdcontacto/Juego
