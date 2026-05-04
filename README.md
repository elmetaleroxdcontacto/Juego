# Chilean Footballito

Simulador de gestion futbolistica en C++ inspirado en la profundidad de juegos tipo Football Manager, con foco en carrera, scouting, vestuario, mercado y motor de partido.

El proyecto ya es jugable y hoy combina tres capas que trabajan juntas:

- frontend inicial `Chilean Footballito` con portada, `Continuar`, `Jugar`, `Cargar guardado`, `Creditos` y configuraciones compartidas
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
- guardado/carga con versionado `save_version = 1`, validacion de integridad y rechazo de saves corruptos
- desarrollo mensual, progresion juvenil y manejo de carga
- vestuario con moral, promesas, tension social y reuniones de plantel
- atributos avanzados por jugador: personalidad, disciplina, liderazgo, propension a lesiones, adaptacion y forma

### Profundidad tipo Football Manager

- mesa de staff con recomendaciones semanales priorizadas
- centro del manager con agenda, scouting, mercado e inbox combinado
- scouting por cobertura regional, foco posicional y confianza de informe
- lectura de proyecto de club en negociaciones: estilo, cantera, politica, clima interno y seguridad del cargo
- dashboard con contexto de rival, microciclo, decisiones sugeridas y narrativa semanal
- datos externos tolerantes a campos faltantes en `data/teams.json`, `data/players.json`, `data/leagues.json` y overlays en `mods/`

### Motor de partido

- contexto previo al partido y snapshots de cada equipo
- partido dividido en seis fases estructuradas
- posesion, progresiones, ataques, ocasiones, xG y eventos tipados
- influencia de tactica, instruccion de partido, moral, disponibilidad y fatiga
- lesiones con cambio forzado real
- reportes post partido con explicacion tactica y recomendaciones

### GUI Win32

- menu de inicio con presentacion tipo dashboard y `Chilean Footballito` como portada central
- portada Win32 con `Continuar`, `Jugar`, `Cargar guardado`, `Configuraciones`, `Creditos` y `Salir`
- configuraciones persistentes entre sesiones para volumen, dificultad, velocidad, idioma, texto, perfil visual y musica del frontend
- reproduccion del tema versionado `assets/audio/Los Miserables - El Crack  Video Oficial (HD Remastered).mp3` mientras la portada principal o el frontend activo lo permiten
- `FootballManager.exe` sin consola innecesaria
- arranque maximizado con soporte DPI
- boton y `F11` para alternar ventana, maximizado y fullscreen sin borde
- dashboard owner-draw con KPIs e insights clicables
- tablas con autosize por vista y suavizado entre refrescos
- cambio de pagina diferido con cache basica para vistas pesadas como `Fichajes`, `Finanzas` y `Noticias`

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

### VS Code

1. Instala las extensiones de C/C++ y CMake Tools.
2. Abre la carpeta del repo.
3. Configura el kit de MinGW si CMake Tools lo solicita.
4. Ejecuta `CMake: Configure` con `build-cmake` como carpeta de build.
5. Ejecuta `CMake: Build Target` y elige `FootballManager`, `FootballManagerCLI` o `FootballManagerTests`.

### Opcion rapida en Windows

```powershell
build.bat
```

Si solo quieres compilar sin ejecutar automaticamente:

```powershell
$env:FM_SKIP_RUN='1'
cmd /c build.bat
```

### Flags utiles de `build.bat`

```powershell
build.bat --gui
build.bat --cli
build.bat --tests --run-tests
build.bat --all --run-tests
build.bat --validate
build.bat --verify
```

El script ahora informa si uso la ruta CMake o la fallback, que targets compilo y donde quedaron los binarios.
Tambien mantiene sincronizados los ejecutables visibles de la raiz del repo y `build-cmake/bin`, incluso si la compilacion cae al fallback directo con `g++`.
`--verify` compila GUI, CLI y tests; ejecuta CTest; y luego corre la validacion completa de datos/proyecto sin abrir la GUI.

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
- guardado, carga, integridad de saves, migracion legacy, JSON/mods y transicion de temporada

## Arquitectura

### Modulos principales

- `engine`: estado base, modelos y controlador del juego
- `simulation`: contexto de partido, fases, eventos, resolucion y reportes
- `career`: servicios de carrera, managers ligeros, inbox, staff, narrativas, semana y mundo
- `ai`: planificacion de plantilla, ajustes CPU y evaluacion de mercado
- `transfers`: mercado, tipos y negociacion
- `development`: entrenamiento, progresion y cantera
- `finance`: proyecciones y buffer economico
- `gui`: runtime, layout, acciones y vistas Win32
- `ui`: flujo de consola y pantallas de texto
- `io`: guardado, carga y serializacion
- `validators`: chequeos de integridad y arranque

### Datos JSON y mods

El juego sigue cargando la base historica desde `data/LigaChilena`, pero ahora tambien lee:

- `data/leagues.json`: ligas adicionales o experimentales.
- `data/teams.json`: equipos nuevos o ajustes livianos sobre equipos existentes.
- `data/players.json`: jugadores externos con atributos avanzados opcionales.
- `mods/leagues.json`, `mods/teams.json`, `mods/players.json`: overlays locales para agregar contenido sin tocar la base.

Los campos faltantes usan fallback seguro; un jugador JSON con solo `team`, `name`, `position`, `age` y `market_value` ya puede entrar al juego.

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

### Estado actual

- La carrera mantiene compatibilidad con el agregado `Career`, pero ya expone `TeamId`, `TeamRepository`, `SeasonManager`, `FinanceManager`, `TransferManager`, `InboxManager` y `NewsManager` como capa progresiva.
- La simulacion semanal esta dividida en fases: partidos, estado fisico, transferencias, finanzas, noticias/sistemas y calendario.
- El mundo guarda campeones, records, tabla historica del club controlado y fichajes importantes dentro del historial persistente.
- Los tres ejecutables principales siguen vigentes: GUI, CLI y tests.
- Los archivos temporales de ejecucion CLI se ignoran y no forman parte del codigo fuente.

## Flujo de entrada

- `src/winmain.cpp`: entrada GUI en Windows
- `src/main.cpp`: entrada consola
- `src/engine/game_controller.cpp`: seleccion de GUI, CLI o validacion
- `src/engine/front_menu.cpp`: frontend compartido para el menu principal y configuraciones
- `src/engine/game_settings.cpp`: persistencia de configuraciones del frontend
- `src/gui/gui_audio.cpp`: musica del menu, manifiesto de assets y fade de audio

## Documentacion adicional

- [Arquitectura](docs/ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Reporte de limpieza de datos](docs/data_cleanup_report.md)
- [Changelog](CHANGELOG.md)
- [Audio del frontend](assets/audio/README.md)

## Roadmap natural

- seguir profundizando el motor de partido con mas lectura rival y balon parado
- ampliar staff y networking del mercado con mas memoria del mundo
- enriquecer cantera, intake juvenil y narrativa de largo plazo
- pulir la GUI con mas widgets de comparacion y centros de informacion
- migrar usos internos de `Team*` hacia `TeamId` en reportes, UI y validadores
- mover mas logica financiera y de mercado desde `week_simulation.cpp` hacia managers dedicados

## Repositorio

GitHub: https://github.com/elmetaleroxdcontacto/Juego
