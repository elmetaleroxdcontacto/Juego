# Football Manager Chile Simulator

Simulador de gestion futbolistica en C++ centrado en carreras de largo plazo, simulacion de partidos, mercado de fichajes y progreso de club dentro de la estructura chilena.

El proyecto ya es jugable y en esta etapa prioriza tres cosas:

- una arquitectura modular que permita seguir creciendo sin rehacer todo
- una simulacion mas coherente entre tactica, moral, fatiga, salud y resultados
- una GUI Win32 mas clara, usable y estable para jugar sin depender de la consola

## Estado actual

El repositorio esta en desarrollo activo, pero ya incluye una base funcional bastante amplia:

- modo carrera con multiples divisiones chilenas
- simulacion semanal de temporada
- motor de partidos modular
- fichajes, renovaciones, precontratos y shortlist
- progresion de jugadores y cantera
- lesiones, fatiga, moral y contexto competitivo
- finanzas, objetivos de directiva y reportes
- interfaz grafica Win32 y modo consola
- validacion de datos y suite de tests automatizados

## Lo mas importante que ya trae

### Simulacion

- contexto previo al partido
- fases de partido y eventos tipados
- impacto de tactica, presion, moral y fatiga
- estadisticas y postproceso del encuentro
- ajustes en vivo del match center
- consecuencias medicas y deportivas tras el partido

### Gestion de club

- carrera por divisiones chilenas
- plantel, once titular e instrucciones
- scouting, seguimiento y radar de mercado
- renovaciones, ventas y precontratos
- presupuesto, salarios e infraestructura
- confianza de directiva y objetivos mensuales

### Desarrollo de jugadores

- generacion y manejo de cantera
- progresion mensual
- impacto del entrenamiento
- condicion del jugador unificada
- lesiones y riesgo medico integrados en la toma de decisiones

### GUI Win32

- ejecutable GUI sin consola innecesaria
- arranque maximizado y soporte DPI
- modo de pantalla por boton y `F11`
- ciclo entre ventana restaurada, maximizado y fullscreen sin borde
- dashboard con tarjetas KPI e insights contextuales
- tarjetas clicables para abrir scouting, finanzas, directiva y otras vistas
- tablas con autosize por contexto y memoria visual por vista

## Ejecutables

Con CMake en Windows se generan estos binarios:

- `build-cmake/bin/FootballManager.exe`
  - version GUI principal
- `build-cmake/bin/FootballManagerCLI.exe`
  - version consola
- `build-cmake/bin/FootballManagerTests.exe`
  - suite de pruebas

## Requisitos

### Windows

- CMake 3.16 o superior
- MinGW con `g++`
- `mingw32-make`

### Linux u otros entornos

El proyecto puede compilar con CMake tambien fuera de Windows, pero la GUI Win32 es especifica de Windows. En esos casos el flujo principal cae en el modo consola.

## Compilacion

### Opcion recomendada: CMake

```powershell
cmake -S . -B build-cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build-cmake --config Release --target FootballManager
```

Para compilar tambien CLI y tests:

```powershell
cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests
```

### Opcion rapida en Windows: `build.bat`

```powershell
build.bat
```

No ejecutar automaticamente al terminar:

```powershell
$env:FM_SKIP_RUN='1'
cmd /c build.bat
```

Forzar fallback directo con `g++` si el arbol de CMake esta inestable:

```powershell
$env:FM_SKIP_RUN='1'
$env:FM_FORCE_FALLBACK='1'
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

Tambien puedes lanzar el modo consola desde el binario principal:

```powershell
.\build-cmake\bin\FootballManager.exe --cli
```

## Validacion

La validacion del proyecto se ejecuta por CLI:

```powershell
.\build-cmake\bin\FootballManagerCLI.exe --validate
```

El flujo de aplicacion tambien soporta:

```powershell
.\build-cmake\bin\FootballManager.exe --validate
```

## Tests

Compilar los tests:

```powershell
cmake --build build-cmake --config Release --target FootballManagerTests
```

Ejecutarlos directamente:

```powershell
.\build-cmake\bin\FootballManagerTests.exe
```

O usar `ctest`:

```powershell
ctest --test-dir build-cmake --output-on-failure
```

La suite actual cubre, entre otras cosas:

- validacion de datos y carga inicial
- estructura del match engine
- fatiga tactica y urgencia competitiva
- mercado, shortlist y negociacion
- lesiones, riesgo medico y reemplazos
- progreso mensual y servicios de carrera
- guardado/carga y transicion de temporada

## Arquitectura

### Capas principales

- `engine`
  - flujo principal del juego, estado de carrera y modelos base
- `simulation`
  - contexto de partido, fases, eventos, resolucion, estadisticas y postproceso
- `career`
  - servicios de carrera, reportes, inbox, desarrollo, temporada y world state
- `ai`
  - gestion de partido, planificacion de plantilla y evaluacion de mercado
- `transfers`
  - mercado, negociacion y tipos de transferencia
- `development`
  - progresion, entrenamiento, cantera y desarrollo mensual
- `finance`
  - caja, masa salarial y proyeccion semanal
- `gui`
  - layout, runtime, acciones y vistas de la GUI Win32
- `ui`
  - flujo legado/consola y pantallas de texto
- `io`
  - carga, guardado y serializacion
- `validators`
  - validacion de datos y consistencia de proyecto

### Estructura del repo

```text
include/
src/
data/
docs/
tests/
saves/
tools/
```

### Carpetas de datos

Actualmente la base jugable sigue viviendo principalmente en:

- `data/LigaChilena/`

Ademas existe una capa de configuracion mas estructurada para seguir desacoplando datos del codigo:

- `data/configs/`
- `data/leagues/`
- `data/teams/`
- `data/players/`

## Flujo de entrada

### En Windows

- `src/winmain.cpp` entra por GUI
- `src/main.cpp` entra por consola
- `src/engine/game_controller.cpp` decide si abrir GUI, CLI o validacion

### Objetivo de esa separacion

Permite mantener:

- la GUI como capa de presentacion
- la consola como fallback funcional
- los servicios de carrera y simulacion reutilizables sin depender de una UI concreta

## Documentacion adicional

- [Arquitectura](docs/ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Limpieza de datos](docs/data_cleanup_report.md)

## Roadmap cercano

- mejorar accesibilidad e interaccion de la GUI
- seguir separando codigo grande que aun concentra demasiada logica
- profundizar negociaciones, historial y memoria de temporada
- expandir base de datos externa para facilitar modding
- reforzar analiticas, scouting y feedback tactico

## Contribuir

Las contribuciones son bienvenidas, especialmente en:

- C++ y arquitectura
- simulacion de partidos
- interfaces Win32 / UX
- datos de equipos, ligas y jugadores
- validacion, testing y balance

Flujo sugerido:

1. haz un fork del repositorio
2. crea una rama tematica
3. realiza tus cambios
4. valida build y tests
5. abre un Pull Request

## Repositorio

GitHub:

https://github.com/elmetaleroxdcontacto/Juego
