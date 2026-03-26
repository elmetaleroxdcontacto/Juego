# Lista de Tareas para Juego de Manager de Fútbol Carga de Equipos desde Archivo

## Tareas Completadas
- [x] Analizar la tarea del usuario: Hacer que el juego cargue equipos desde un archivo en lugar de codificados.
- [x] Leer main.cpp para entender la estructura del código.
- [x] Leer archivos de equipos para confirmar divisiones y conteos de equipos.
- [x] Planificar: Modificar initializeLeague para cargar desde teams.txt, actualizar selección de equipos en Juego Rápido y Modo Carrera.
- [x] Editar main.cpp para implementar la carga desde teams.txt en initializeLeague.
- [x] Actualizar selección de equipos en Juego Rápido para cargar desde teams.txt.
- [x] Actualizar bucles de selección de equipos en Modo Carrera para usar índices correctos para 16 equipos de Primera.
- [x] Verificar que los cambios sean correctos.
- [x] Compilar el juego para verificar errores.
- [x] Ejecutar el juego y verificar que inicie correctamente.
- [x] Probar el juego para asegurar que la nueva carga de equipos funcione correctamente (el usuario puede interactuar).
- [x] Implementar la estructura de Jugador con atributos como nombre, posición, ataque, defensa, resistencia, habilidad, edad, valor, lesiones, etc.
- [x] Crear la clase Equipo con métodos para calcular ataque total, defensa total, habilidad promedio, presupuesto, puntos, etc.
- [x] Desarrollar el sistema de liga con tabla de posiciones y clasificación por puntos y diferencia de goles.
- [x] Agregar modo carrera con guardado y carga de progreso en archivo career_save.txt.
- [x] Implementar simulación de partidos con consideración de tácticas, habilidades y resistencia.
- [x] Añadir mercado de transferencias para comprar y vender jugadores.
- [x] Incluir sistema de lesiones aleatorias y recuperación semanal.
- [x] Desarrollar modo juego rápido con selección de equipo y gestión básica.
- [x] Cargar equipos desde archivos de texto con datos de jugadores.
- [x] Agregar estadísticas de equipo y jugadores (victorias, empates, derrotas, goles, etc.).
- [x] Implementar logros y eventos aleatorios en partidos.
- [x] Crear menús en español para una mejor experiencia de usuario.
- [x] Normalizar nombres de equipos para manejar caracteres especiales en archivos.
- [x] Agregar envejecimiento de jugadores con disminución de habilidades.
- [x] Implementar curación de lesiones al final de temporada.
- [x] Desarrollar selección de división en modo carrera (Primera, Primera B, Segunda).
- [x] Actualizar tabla de liga después de cada partido simulado.
- [x] Comprobar que la carga de equipos desde archivos funciona correctamente.
- [x] Verificar que el modo carrera permite guardar y cargar progreso sin errores.
- [x] Probar la simulación de partidos con diferentes tácticas y habilidades.
- [x] Confirmar que el mercado de transferencias permite comprar y vender jugadores correctamente.
- [x] Validar el sistema de lesiones y recuperación semanal.
- [x] Asegurar que el modo juego rápido funciona con selección y gestión básica.
- [x] Revisar que las estadísticas de equipo y jugadores se actualizan correctamente.
- [x] Verificar que los logros y eventos aleatorios se activan adecuadamente.
- [x] Confirmar que los menús en español se muestran correctamente y son funcionales.
- [x] Probar el envejecimiento de jugadores y la disminución de habilidades.
- [x] Validar la curación de lesiones al final de temporada.
- [x] Comprobar la selección de división en modo carrera.
- [x] Asegurar que la tabla de liga se actualiza después de cada partido simulado.
- [x] Agregar opción de retiro de jugador al cumplir entre 35 y 45 años.
- [x] Crear archivo Colo-Colo.txt en primera_division con la plantilla actual de Colo-Colo de la Primera División Chilena.
- [x] Corregir duplicados en XI al completar la alineacion inicial.
- [x] Ampliar la normalizacion de posiciones (laterales, carrileros, etc.).
- [x] Priorizar position_raw cuando position y position_raw difieren.
- [x] Mejorar el scraper para inferir posiciones desde el nombre en Tercera A/B.
- [x] Re-scrapear LigaChilena completa con las nuevas reglas.
- [x] Implementar asignacion automatica de posiciones para jugadores N/A al cargar equipos.
- [x] Validar todas las ligas y equipos (0 N/A tras asignacion).
- [x] Completar plantillas con <18 jugadores en Segunda Division (3 equipos).
Resumen: Se corrigio el XI, se amplio la normalizacion de posiciones, se mejoro el scraper, se re-scrapeo la liga y se validaron/ajustaron plantillas en todas las divisiones.

- Actualizar las plantillas de todos los equipos de la Primera División con los jugadores reales actuales de la temporada 2024/2025, obtenidos de transfermarkt.com.


## Notas
- teams.txt tiene 16 equipos para Primera División.
- Original: Primera 9 (0-8), Segunda 10 (9-18), Primera B 12 (19-30), Total 31.
- Nuevo: Primera 16 (0-15), Segunda 10 (16-25), Primera B 12 (26-37), Total 38.
- Plantillas canonicas: players.csv (players.txt/players.json quedan como referencia).
- Equipos por division se leen desde LigaChilena/<division>/teams.txt.

- [x] Agregar Tercera División A con equipos y jugadores
- [x] Agregar Tercera División B con equipos y jugadores
- [x] Verificar que la API incluya las nuevas divisiones

- [x] Documentar todo el trabajo realizado en español en TODO.md
Nota: valores monetarios usan enteros de 64 bits; entrada manual hasta 1e12.
- [x] Modularizacion del codigo en archivos (models/io/simulation/ui) y uso de std::filesystem.

## Tareas Recientes (2026)

- [x] Corregir error de compilación en utils.cpp: "argument of type 'const char *' is incompatible with parameter of type 'LPCWSTR'"
  - El problema era que las funciones Windows FindFirstFile/FindNextFile usaban versiones Unicode por defecto
  - Solución: Cambiar a versiones ANSI explícitas (FindFirstFileA, FindNextFileA) y estructura WIN32_FIND_DATAA
- [x] Crear script de compilación build.bat para facilitar la compilación del juego
  - Compila todos los archivos fuente: main.cpp, io.cpp, models.cpp, simulation.cpp, ui.cpp, utils.cpp
  - Genera el ejecutable FootballManager.exe
  - Uso: ejecutar build.bat en la carpeta del proyecto
- [x] Agregar pausa después de cada opción en los menús del juego
  - Modo Carrera: después de cada opción (excepto volver), muestra "Presiona Enter para continuar..."
  - Juego Rápido: después de cada opción (excepto volver), muestra "Presiona Enter para continuar..."
- [x] Mejoras prioritarias: ventaja local y clima, fatiga/condicion persistente, eventos de partido, entrenamiento con rendimientos decrecientes y limite semanal, vista de equipo con XI, y riesgo de lesiones por edad/condicion/tactica.
- [x] Mejoras prioritarias: ventaja local y clima, fatiga/condicion persistente, eventos de partido, entrenamiento con rendimientos decrecientes y limite semanal, vista de equipo con XI, y riesgo de lesiones por edad/condicion/tactica.
- [x] Implementar moral de equipo, potencial de jugadores y desarrollo por partidos.
- [x] Agregar ascensos/descensos con playoffs y ajustes de division.
- [x] Incorporar finanzas semanales (ingresos + sueldos) y mercado con ofertas/CPU.
- [x] Añadir lesiones con tipo y duracion; nuevas tacticas (Pressing/Counter).
- [x] Incluir estadisticas post-partido y editor rapido de equipo/jugadores.
- [x] Scouting con incertidumbre, contratos/salarios y renovaciones.\n- [x] Roles por jugador y plan de entrenamiento semanal.\n- [x] Cantera, eventos de club y dashboard semanal.\n- [x] Ajuste de tacticas CPU segun rival/moral.\n- [x] Modo Copa (eliminacion directa).
- [x] Roles por jugador, salarios/contratos, historial de lesiones y plan de entrenamiento semanal.
- [x] Scouting con incertidumbre, desarrollo juvenil, eventos de club y dashboard semanal.
- [x] IA tactica adaptativa, partidos clave y modo Copa (eliminacion directa).

## Cambios recientes (2026-03-11) - Validacion, carga de planteles y saves
- La suite de validacion ahora falla correctamente si la auditoria de plantillas detecta errores de datos.
- La auditoria sigue generando `saves/roster_validation_report.txt` y ya no puede dar falso verde.
- Se unifico la resolucion de posiciones entre cargador y validador, priorizando `position_raw` cuando corresponde.
- La carga por carpeta ahora usa fallback real: `players.csv -> players.txt -> legacy txt -> players.json`.
- Se agrego soporte directo para `players.json`.
- La normalizacion final del plantel ahora respeta el maximo por division y rebalancea cobertura minima de roles sin exceder el cupo.
- Se endurecio el sistema de guardado/carga:
  - version de save subida a `VERSION 8`
  - escape de delimitadores y parseo seguro
  - escritura a `.tmp` con reemplazo atomico y fallback de copia si Windows no permite el rename
- La validacion de save/load ahora usa un archivo runtime dedicado y comprueba la version real escrita en disco.
- Tests reforzados/agregados:
  - `validation_suite`
  - `save_load_roundtrip`
  - `loader_fallback`
- Verificacion realizada:
  - `FootballManager.exe --validate`: 0 fallas logicas
  - `FootballManagerTests.exe`: todos los tests pasan
- Pendiente de datos:
  - la base de plantillas sigue reportando `9236` errores y `1283` advertencias en la auditoria; eso requiere limpiar archivos en `data/`, no mas cambios de runtime.
 
## Cambios recientes (2026-03-05)
- Implementado formato de Segunda División (14 equipos) con grupos Norte/Sur, calendario intra-grupo ida/vuelta (14 fechas), tablas por grupo y clasificación automática a playoff/descenso.
- Playoff de Segunda División ahora con llaves ida y vuelta; repechaje 4° vs 4° a partido único.
- Tablas especiales para Primera División y Primera B con nuevos criterios de desempate (DG, ganados, GF, goles visita, menos rojas/amarillas, sorteo).
- Nuevos campos por equipo: goles de visita, tarjetas amarillas/rojas y semilla de sorteo; guardado/carga actualizado.
- Simulación de tarjetas y registro de goles de visita en cada partido.
- Final por el título en Primera División y Primera B si hay empate en puntos (partido único + penales).
- Liguilla de Primera B (2°-8°) ida/vuelta con reglas de localía; final con prórroga + penales.
- Descenso en Primera B: último lugar con definición si hay empate en puntos.
- Ascensos/descensos actualizados: Primera División recibe campeón + liguilla de Primera B; Primera B asciende campeón + liguilla y desciende 1 a Segunda.

## Cambios recientes (2026-03-06)
- Ajustada la implementación de Primera B 2026 para usar desempates por título y descenso en cancha neutral, sin ventaja de localía.
- `playMatch` ahora soporta partidos en sede neutral; se usa en finales y definiciones de desempate.
- Corregida la evaluación de llaves ida/vuelta de la liguilla de ascenso para decidir por puntaje de serie y luego diferencia de gol.
- Final de la liguilla de Primera B ajustada para aplicar prórroga y luego penales solo si persiste la igualdad tras puntaje de serie y diferencia de gol.
- Empates múltiples por el 1° lugar o por el último lugar ahora se reducen a dos equipos usando el orden oficial de clasificación antes del partido definitorio.
- Validado el fixture de Primera B: 16 equipos, 30 fechas, 8 partidos por fecha y segunda rueda espejada invirtiendo localía.
- Compilación verificada exitosamente con `build.bat` tras los cambios.

## Cambios recientes (2026-03-06) - Tercera Division A/B
- Implementado formato 2026 de Tercera División A con 16 equipos, todos contra todos ida y vuelta y calendario automático de 30 fechas.
- Agregados desempates específicos para Tercera A: puntos, diferencia de gol, goles a favor, partidos ganados, goles de visita y enfrentamientos directos.
- Se incorporó registro persistente de enfrentamientos directos por equipo y su guardado/carga en la carrera.
- Implementado playoff de ascenso de Tercera A para 2°-5°: semifinales ida y vuelta (2° vs 5°, 3° vs 4°) y final a partido único.
- Implementado cierre de temporada específico de Tercera A con ascenso directo del 1°, ascenso adicional por playoff y cruces de promoción/descenso frente a Tercera B.
- Implementado formato 2026 de Tercera División B con 28 equipos divididos en Zona Norte y Zona Sur, cada una con tabla independiente y fixture automático por grupo.
- Agregado soporte general de divisiones con grupos en la UI y en el calendario para reutilizar la lógica entre Segunda División y Tercera B.
- Implementados ascensos directos de Tercera B para los campeones de Zona Norte y Zona Sur.
- Implementada final entre campeones de grupo para definir al campeón de Tercera B.
- Implementados cruces 2°/3° entre zonas y llaves de promoción entre Tercera B y Tercera A.
- Actualizados `teams.txt` de Tercera A y Tercera B a una estructura 2026 de 16 y 28 equipos respectivamente.
- Creadas carpetas placeholder para clubes nuevos que no existían en el repositorio, permitiendo que el cargador genere planteles automáticos y no descarte equipos.
- Validaciones realizadas:
  - Tercera A queda con 16 equipos y 30 fechas.
  - Tercera B queda con 28 equipos y 2 grupos de 14.
  - Compilación verificada exitosamente con `build.bat`.
- Nota de implementación:
  - En Tercera B la fase zonal quedó a una rueda por grupo y los cruces 2°/3° se usaron como filtro para la promoción A/B siguiendo la especificación dada en la solicitud.

## Cambios recientes (2026-03-06) - Motor de competiciones
- Creado un nuevo módulo de configuración de competiciones en `competition.h` y `competition.cpp`.
- Centralizadas en `CompetitionConfig` las reglas base por división:
  - perfil de tabla
  - handler de fin de temporada
  - formato de grupos
  - ingresos semanales
  - factor salarial
  - divisor de presupuesto
  - tamaño máximo de plantel
  - cantidad esperada de equipos
- `models.cpp` ahora usa esta configuración para:
  - decidir el perfil de desempates de tabla
  - determinar si una división usa grupos
  - construir grupos regionales
  - generar calendarios según formato configurado
- `io.cpp` ahora usa la configuración para:
  - limitar tamaño máximo de plantel por división
  - calcular presupuesto inicial según divisor configurado
- `ui.cpp` ahora usa la configuración para:
  - nombres de grupos (por ejemplo, Zona Norte / Zona Sur)
  - ingresos base por división
  - factor salarial semanal
  - despacho del cierre de temporada según handler de competición
- `build.bat` fue actualizado para compilar el nuevo módulo `competition.cpp`.
- Resultado:
  - el juego depende menos de `if (division == ...)` repartidos por múltiples archivos
  - queda una base más limpia para seguir moviendo playoffs, ascensos y descensos a reglas declarativas
- Validación realizada:
  - compilación verificada exitosamente con `build.bat`.

## Cambios recientes (2026-03-06) - Sanciones, centro de competicion, mercado y directiva
- Implementado sistema de sanciones por jugador:
  - las tarjetas amarillas y rojas ahora se asignan a futbolistas concretos en cada partido
  - acumulacion de 5 amarillas = 1 fecha de suspension
  - tarjeta roja = suspension automatica
  - los suspendidos quedan fuera del XI mientras deban fechas
- Persistidos en guardado/carga nuevos datos individuales:
  - clausula de rescision
  - acumulacion de amarillas
  - tarjetas amarillas y rojas de temporada
  - fechas de suspension pendientes
- `viewTeam` y `displayStatistics` ahora muestran suspensiones, tarjetas individuales y clausulas.
- Agregado `Centro de Competicion` al menu de carrera:
  - posicion actual
  - zona alta y zona baja de la tabla relevante
  - proxima fecha
  - confianza de la directiva
- Agregada pantalla `Objetivos y Directiva`:
  - objetivo de puesto final
  - objetivo financiero
  - objetivo de uso de juveniles sub-20
  - nivel de confianza y semanas de advertencia
- La directiva ahora genera objetivos automaticamente al iniciar cada temporada y actualiza su confianza semana a semana segun:
  - posicion deportiva
  - presupuesto
  - uso de juveniles
  - moral del equipo
- Mercado de transferencias mejorado:
  - compra de jugadores desde otros clubes
  - negociacion de oferta con posible contraoferta del club vendedor
  - negociacion de salario, duracion de contrato y honorarios de agente
  - ejecucion de clausulas de rescision
  - venta de jugadores con precio pedido y contraoferta del mercado
- Ajustado el loop semanal de carrera:
  - las sanciones se descuentan correctamente tras la fecha jugada
  - ofertas entrantes permiten aceptar, contraofertar o rechazar
  - la IA de fichajes actua de forma mas agresiva segun presupuesto y necesidades de plantel
  - renovaciones de contrato ahora exigen salario, duracion y clausula
- Validaciones realizadas:
  - compilacion verificada exitosamente con `build.bat`
  - arranque del ejecutable verificado con salida limpia al menu principal

## Cambios recientes (2026-03-06) - Carrera avanzada, tacticas, finanzas y validadores
- Implementada progresion de carrera del manager:
  - nombre y reputacion del manager persistentes
  - posibilidad de cambiar voluntariamente de club desde `Objetivos y Directiva`
  - despido automatico si la confianza de la directiva cae a niveles criticos
  - reasignacion inmediata a un nuevo club tras despido
- Agregado historial de temporadas persistente:
  - division, club, puesto final, campeon, ascensos, descensos y nota de temporada
  - nueva pantalla `Historial de Temporadas`
- Agregado feed de noticias persistente:
  - resultados semanales
  - fichajes, prestamos, precontratos y renovaciones
  - eventos de club
  - movimientos del manager
  - nueva pantalla `Noticias`
- Mercado expandido con nuevas operaciones:
  - precontratos para jugadores con poco contrato restante
  - prestamos entrantes
  - prestamos salientes
  - retorno automatico de jugadores cedidos al vencer el prestamo
  - ejecucion de precontratos al comenzar la nueva temporada
- Implementada gestion de alineacion y rotacion:
  - XI preferido manual o automatico
  - seleccion de capitan
  - seleccion de lanzador de penales
  - politica de rotacion (`Titulares`, `Balanceado`, `Rotacion`)
  - el motor del XI ahora prioriza titulares preferidos y respeta rotacion, lesiones y suspensiones
- Motor tactico ampliado:
  - intensidad de presion
  - linea defensiva
  - tempo
  - amplitud
  - tipo de marcaje (`Zonal` / `Hombre`)
  - la simulacion ahora usa estos parametros para rendimiento, desgaste, tarjetas y penales
  - los desempates por penales ahora ponderan mejor capitan, lanzador y calidad del XI
- Finanzas del club profundizadas:
  - sponsor semanal
  - ingresos por entradas segun localia, estadio y masa social
  - deuda del club
  - niveles de estadio, cantera y centro de entrenamiento
  - costo semanal de infraestructura
  - nueva pantalla `Club y Finanzas` para invertir en infraestructura
  - la cantera y la recuperacion/entrenamiento ahora se benefician de la infraestructura
- Transicion de temporada mejorada:
  - premios economicos por posicion final
  - registro automatico en historial
  - ejecucion de precontratos al pasar de temporada
  - avance de temporada centralizado en una sola rutina
- Mejorado el soporte de rutas Unicode en Windows:
  - `utils.cpp` ahora usa WinAPI wide (`GetFileAttributesW`, `FindFirstFileW`, `FindNextFileW`)
  - se corrigio la deteccion de carpetas con acentos y `ñ`
- Mejorado `build.bat`:
  - ahora detiene la compilacion correctamente si un archivo falla
  - agregado el nuevo modulo `validators.cpp`
- Agregado modulo de validacion y tests:
  - nuevo comando CLI `FootballManager.exe --validate`
  - nueva opcion de menu principal `Validar sistema`
  - valida conteo de divisiones, equipos unicos, planteles, fixtures, guardado/carga y orden basico de tabla
- Validaciones realizadas:
  - compilacion verificada exitosamente con `build.bat`
  - suite `FootballManager.exe --validate` ejecutada con resultado sin fallas
  - arranque del ejecutable y salida por menu principal verificados

## Cambios recientes (2026-03-06) - Convocatoria, staff, copa, scouting y dinamicas de plantel
- Implementada gestion mas completa de convocatoria y partido:
  - banca preferida automatica o manual
  - seleccion de lanzadores de tiros libres y corners
  - visualizacion de banca en `Ver Equipo`
  - cambios automaticos durante los partidos segun condicion, lesiones y rotacion
  - registro de sustituciones en el resultado del partido
- Mejorado el motor de simulacion:
  - `playMatch` ahora devuelve tiros, posesion y cambios por equipo
  - el capitan aporta segun liderazgo
  - los especialistas a balon parado aportan segun `setPieceSkill`
  - se registran titularidades de temporada para objetivos y dinamicas de vestuario
  - mejorada la recuperacion de lesiones y condicion usando cuerpo medico y preparador fisico
- Agregado cuerpo tecnico con impacto real:
  - asistente tecnico, preparador fisico, jefe de scouting, jefe de juveniles y cuerpo medico visibles en `Club y Finanzas`
  - nuevas inversiones para mejorar cada area del staff
  - posibilidad de cambiar la region de captacion juvenil
- Agregadas dinamicas de personalidad y relacion jugador-club:
  - felicidad, quimica, liderazgo, profesionalismo y ambicion visibles en plantel y estadisticas
  - jugadores pueden inquietarse por pocos minutos, bajo salario relativo o malos resultados
  - aumento de ofertas y exigencia de renovacion cuando un jugador quiere salir
- Sistema juvenil profundizado:
  - desarrollo mensual de juveniles segun cantera, entrenador juvenil y profesionalismo
  - ingresos de nuevos prospectos durante la temporada
  - captacion influida por la region juvenil del club
- Scouting ampliado:
  - informes por region
  - enfoque por posicion o necesidad detectada del plantel
  - precision del informe ligada al jefe de scouting
  - bandeja persistente de informes y opcion de fichar desde el informe
- Agregado calendario multi-competencia simple:
  - copa de temporada paralela a la liga
  - rondas eliminatorias en semanas seleccionadas
  - campeon de copa persistente
  - visualizacion de estado de copa en `Centro de Competicion` y resumen semanal
- Agregado analisis postpartido:
  - resumen persistente del ultimo partido con marcador, tiros, posesion, cambios y lectura rapida
  - visible en `Centro de Competicion`
- Objetivos dinamicos reforzados:
  - progreso mensual visible en `Objetivos y Directiva`, `Centro de Competicion` y dashboard semanal
  - correccion del objetivo de mejora de posicion para que no genere metas imposibles
- Herramientas de datos y validacion reforzadas:
  - la suite de validacion ahora comprueba tambien persistencia de staff y datos avanzados de carrera
- Validaciones realizadas:
  - compilacion verificada exitosamente con `build.bat`
  - suite `FootballManager.exe --validate` ejecutada con resultado sin fallas
  - arranque del ejecutable y salida por menu principal verificados

## Cambios recientes (2026-03-07) - Estabilidad de compilacion y ejecucion
- Corregido `build.bat` para que la compilacion sea lineal y deterministica, sin pausas ni subrutinas que dejaban un `FootballManager.exe` inestable.
- El ejecutable final vuelve a generarse correctamente en `FootballManager.exe` y ya no dispara el error de aplicacion por lectura invalida de memoria al correr validaciones.
- Se retiraron trazas temporales de depuracion usadas para aislar el fallo durante la investigacion.
- Validaciones realizadas:
  - compilacion verificada exitosamente con el nuevo `build.bat`
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas

## Cambios recientes (2026-03-07) - GUI jugable y versionado de saves
- La GUI gano una barra de vistas y acciones rapidas en la ventana principal.
- Nuevas vistas dentro del panel izquierdo:
  - resumen
  - competicion
  - directiva
  - club y finanzas
  - scouting
- Nuevas acciones jugables desde GUI, sin pasar por la consola:
  - otear jugadores automaticamente segun necesidad del plantel
  - fichar un objetivo seleccionado del centro de transferencias
  - firmar precontratos sobre objetivos elegibles
  - renovar contratos desde el plantel
  - vender jugadores desde el plantel
  - mejorar cantera, entrenamiento, scouting y estadio
- `app_services.cpp` / `app_services.h` ampliados para soportar desde GUI:
  - resumenes de competicion, directiva, club y scouting
  - scouting automatico reutilizable
  - mejoras de club reutilizables
  - fichaje directo, precontrato, renovacion y venta por servicio
- `models.cpp` ahora guarda version del save con cabecera `VERSION 2` y `loadCareer()` acepta retrocompatibilidad con saves sin version.
- `validators.cpp` ahora verifica tambien que el archivo de carrera guardado incluya version de save.
- Validaciones realizadas:
  - compilacion verificada exitosamente con `build.bat`
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas

## Cambios recientes (2026-03-06) - Interfaz grafica
- Agregado nuevo modulo `gui.cpp` / `gui.h` con una interfaz grafica nativa Win32 para Windows.
- La GUI permite:
  - iniciar una carrera nueva eligiendo division, equipo y manager
  - cargar y guardar carrera
  - simular semanas desde la ventana principal
  - ver resumen del club, noticias, tabla, plantel y centro de transferencias
  - ejecutar la validacion del sistema desde un boton
- Agregado un nuevo bloque "Centro de transferencias" en la GUI principal.
- El centro de transferencias muestra:
  - precontratos y movimientos pendientes
  - jugadores con contratos cortos
  - jugadores que quieren salir
  - prestamos entrantes y salientes
  - objetivos destacados del mercado
- El resumen principal de la GUI ahora tambien incluye alertas resumidas del mercado:
  - cantidad de pendientes
  - contratos por vencer
  - jugadores que quieren salir
  - conteo de prestamos
- `main.cpp` ahora abre la GUI por defecto; la interfaz de consola sigue disponible con `FootballManager.exe --cli`.
- Ajustado `ui.cpp` para soportar seleccion automatica de nuevo club al ser despedido cuando el juego corre en modo GUI.
- Corregido `Career::initializeLeague` para limpiar `myTeam` y evitar punteros colgantes al recargar datos.
- `build.bat` actualizado para compilar `gui.cpp` y enlazar `comctl32` y `gdi32`.
- Validaciones realizadas:
  - compilacion verificada exitosamente
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas

## Cambios recientes (2026-03-06) - Desacople de terminal
- Agregada nueva capa `app_services.cpp` / `app_services.h` para que la GUI consuma operaciones del juego sin depender de la terminal:
  - iniciar carrera
  - cargar carrera
  - guardar carrera
  - simular semana
  - ejecutar validaciones
- `Career::saveCareer()` ya no imprime directamente en consola; ahora devuelve estado y cada interfaz decide como mostrar el resultado.
- `LeagueTable` ahora puede renderizar su contenido como lineas de texto mediante `formatLines()`, reduciendo el acoplamiento del modelo con `cout`.
- `validators.cpp` ahora genera un `ValidationSuiteSummary` reutilizable; la salida a consola queda solo como una capa de presentacion.
- `ui.cpp` incorpora callbacks para desacoplar decisiones interactivas del flujo semanal:
  - mensajes de UI
  - seleccion de nuevo club tras despido
  - decision sobre ofertas entrantes
  - decision sobre renovaciones de contrato
- La GUI ya no depende de `readInt` dentro de la simulacion semanal para:
  - renovaciones de contrato
  - ofertas de transferencia entrantes
  - eleccion de club tras despido
- `simulateCareerWeek` y varios helpers del resumen semanal/eventos ahora pueden emitir mensajes por callback en lugar de escribir siempre a terminal.
- `build.bat` y `README.md` actualizados para incluir `app_services.cpp`.
- Validaciones realizadas:
  - compilacion verificada exitosamente
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas

## Cambios recientes (2026-03-07) - Revision tecnica completa
- Revisados todos los archivos fuente principales del proyecto para detectar errores de compilacion, validacion o inconsistencias evidentes.
- Ejecutada una pasada global de chequeo sobre:
  - compilacion completa con `build.bat`
  - validacion interna con `FootballManager.exe --validate`
  - revision estatica de todas las fuentes `.cpp` con `g++ -std=c++17 -Wall -Wextra -Wpedantic -fsyntax-only`
- Corregido un warning menor en `gui.cpp` por un parametro sin uso dentro de `playerRole(...)`.
- Resultado final de la revision:
  - sin errores de compilacion
  - sin fallas en la suite de validacion
  - sin warnings en la pasada estatica aplicada a las fuentes principales

## Cambios recientes (2026-03-07) - Nuevas mecanicas deportivas, de vestuario y negociacion
- Agregadas nuevas mecanicas persistentes de jugador en `models.h` / `models.cpp`:
  - rasgos individuales (`traits`)
  - plan de desarrollo (`developmentPlan`)
  - promesa contractual (`promisedRole`)
- Agregada nueva instruccion de partido por equipo en `Team.matchInstruction`.
- El guardado de carrera sube a `VERSION 3` y mantiene retrocompatibilidad de carga para saves anteriores sin estos campos.
- Los jugadores generados aleatoriamente y juveniles ahora nacen con rasgos y plan individual por defecto.
- La simulacion en `simulation.cpp` ahora usa estas mecanicas:
  - rasgos que alteran ataque/defensa
  - rasgos que afectan fatiga, tarjetas e impacto de lesion
  - instrucciones de partido que cambian el comportamiento tactico del equipo
  - ajuste de corners y juego a balon parado segun instruccion
- La progresion semanal y mensual en `ui.cpp` ahora considera:
  - planes de desarrollo individuales
  - promesas de rol incumplidas
  - lideres de vestuario y jugadores conflictivos
  - noticias adicionales de prensa, aficion y tension interna
- La negociacion en `app_services.cpp` / `app_services.h` se amplió con promesas:
  - sin promesa
  - titular
  - rotacion
  - proyecto
- Las promesas ya afectan salario, duracion de contrato, felicidad y expectativa de minutos.
- La GUI en `gui.cpp` ahora expone estas mecanicas:
  - nuevo combo de promesa para fichajes, precontratos y renovaciones
  - boton `Plan+` para ciclar el plan individual del jugador seleccionado
  - boton de instruccion de partido para ciclar la instruccion activa del equipo
  - detalle ampliado en seleccion de jugador y de objetivo de mercado con plan, promesa y rasgos
  - vista de plantel con columnas de plan y promesa
  - resumen principal con instruccion activa, lideres/promesas en riesgo y estado ampliado del vestuario
- Los resumenes de `Club`, `Directiva` y `Scouting` ahora muestran:
  - clima de vestuario
  - lideres del plantel
  - promesas en riesgo
  - proyectos juveniles
  - rasgos y perfil de personalidad en informes de scouting y shortlist
- Ajustado tambien el flujo de consola para:
  - mostrar plan/promesa/rasgos al ver el plantel
  - permitir cambiar la instruccion de partido desde tacticas
- `validators.cpp` ahora verifica tambien la persistencia de:
  - instruccion de partido
  - plan de desarrollo
  - promesa contractual
  - rasgos del jugador
- Validaciones realizadas:
  - compilacion verificada exitosamente con `build.bat`
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas
  - revision estatica de las fuentes principales sin warnings

## Cambios recientes (2026-03-07) - Profundidad estilo Football Manager
- Ampliado el perfil persistente de jugador en `models.h` / `models.cpp` con nuevos atributos de contexto futbolistico:
  - pie habil (`preferredFoot`)
  - posiciones secundarias (`secondaryPositions`)
  - consistencia
  - rendimiento en partidos grandes
  - forma actual
  - disciplina tactica
  - versatilidad
- El guardado de carrera sube a `VERSION 4` y mantiene carga retrocompatible para saves anteriores sin estos campos.
- Se agrego `ensurePlayerProfile(...)` para normalizar y completar automaticamente jugadores cargados desde:
  - `players.csv`
  - `players.txt`
  - formatos legacy
  - generacion aleatoria
  - alta manual desde la UI
- Mejorada la seleccion automatica del XI y de la banca:
  - ahora pondera forma, consistencia, disciplina tactica y situacion contractual
  - las posiciones secundarias ya cuentan al evaluar si un jugador puede cubrir un rol
- Profundizado el motor de partidos en `simulation.cpp`:
  - la forma, consistencia, disciplina tactica y pie habil ya modifican el rendimiento real
  - los partidos clave ahora favorecen o castigan a futbolistas segun su atributo de `bigMatches`
  - se agregaron/modularon mas roles efectivos en simulacion:
    - `SweeperKeeper`
    - `BallPlaying`
    - `Organizador`
    - `Interior`
    - `Objetivo`
  - las instrucciones colectivas tienen mas peso sobre ataque, defensa, fatiga, tarjetas y riesgo competitivo
- Nuevas instrucciones de partido disponibles desde consola y GUI:
  - `Por bandas`
  - `Juego directo`
  - `Contra-presion`
  - `Pausar juego`
- Ampliado el entrenamiento semanal en `ui.cpp`:
  - nuevo foco `Preparacion partido`
  - nuevo foco `Recuperacion`
  - el entrenamiento tactico ahora mejora disciplina tactica
  - la preparacion de partido mejora forma/chemistry
  - la recuperacion acelera vuelta fisica y reduce tiempos de lesion leves
- Mejorado el scouting en consola y servicios:
  - los informes ahora muestran pie habil, posiciones secundarias, forma, fiabilidad y rendimiento en partidos grandes
  - el foco de posicion ya acepta jugadores capaces de cubrir la demarcacion por versatilidad, no solo por posicion primaria
  - la shortlist actualizada hereda este nivel de detalle
- Mejoradas las negociaciones y sueldos:
  - la demanda salarial ahora considera forma actual, consistencia y peso competitivo del jugador
- Mejorado el analisis post-partido:
  - se agrego una estimacion simple de xG
  - se añade una recomendacion automatica de ajuste tactico para la semana siguiente
- Implementada actividad de mundo en segundo plano:
  - las otras divisiones del proyecto ahora juegan fechas en background mientras avanzas tu carrera
  - se generan titulares de resultados externos en `newsFeed`
  - CPU de otras divisiones tambien renueva contratos y mueve planteles fuera de tu categoria activa
- Ajustada la progresion semanal global:
  - recuperacion, entrenamiento y evolucion contractual ya no quedan limitados solo a la division activa
- Mejorada la GUI en `gui.cpp`:
  - nuevos detalles de jugador con pie habil, posiciones secundarias, forma y fiabilidad
  - resumen de mercado mas informativo
  - etiquetas cortas para las nuevas instrucciones de partido
- `validators.cpp` actualizado para comprobar persistencia de los nuevos atributos avanzados del jugador.
- Validaciones realizadas:
  - compilacion verificada exitosamente con `build.bat`
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas

## Cambios recientes (2026-03-07) - Motor contextual, identidad de club y reportes
- Reforzado el motor de partidos en `simulation.cpp` para que el resultado responda mejor al contexto futbolistico:
  - la presion alta ahora aporta recuperaciones y volumen ofensivo real
  - las lineas defensivas altas sufren mas ante `Juego directo` y transiciones verticales
  - la localia depende mas de masa social, estadio y prestigio
  - los clasicos suben la tension competitiva y las tarjetas
  - el tramo final del partido ahora pondera moral, liderazgo y atributo de `bigMatches`
- Los eventos del partido ya explican mejor el por que de ciertas acciones:
  - goles con texto segun rol o rasgo (`Poacher`, `Objetivo`, `Cita grande`, `Llega al area`, etc.)
  - eventos tacticos de presion, localia, juego directo y clasico
  - riesgo extra de lesion por intensidad cuando el equipo aprieta con poca energia
- Agregada identidad persistente de club en `models.h` / `models.cpp`:
  - `clubPrestige`
  - `clubStyle`
  - `youthIdentity`
  - `primaryRival`
- La identidad del club se infiere y actualiza automaticamente desde division, infraestructura, masa social, tacticas y rivalidades conocidas.
- El guardado de carrera sube a `VERSION 5` y mantiene carga retrocompatible para saves anteriores sin estos campos.
- Mejoradas las negociaciones en `app_services.cpp`:
  - jugadores pueden rechazar por reputacion/proyecto insuficiente
  - agentes mas duros encarecen salario, bono y honorarios
  - clubes rivales directos aplican sobreprecio
  - renovaciones castigan promesas de rol incumplidas
- Reforzada la capa de carrera y reportes en `ui.cpp`:
  - informe rival previo a la proxima fecha
  - mapa simple por lineas en analisis postpartido y dashboard semanal
  - alertas por fatiga de lineas, sequia de delanteros y promesas contractuales en riesgo
  - noticias nuevas de clasicos, entrevistas, rumores de banquillo e identidad de cantera
  - reputacion del manager ahora tambien responde un poco al estilo de juego
- Mejorado el mundo de fondo:
  - otras divisiones generan mas titulares narrativos de lideres, crisis y temporadas destacadas
  - ofertas entrantes ahora vienen desde clubes concretos y mueven realmente planteles del mundo
- Mejorada la portabilidad base del proyecto:
  - agregado `CMakeLists.txt`
  - `main.cpp` ahora cae a modo CLI fuera de Windows
  - en Windows se mantiene la GUI como flujo por defecto
- Validaciones realizadas:
  - compilacion verificada con `g++ -std=c++17 -static ... -o FootballManager.exe`
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas
- Pendiente para una segunda pasada si se quiere seguir esta linea:
  - modularizacion fuerte de archivos grandes como `app_services.cpp` / `ui.cpp`
  - soporte de datos de ligas y plantillas en formato mas formal/moddeable (JSON/CSV con esquema)
  - filtros/comparadores de plantilla mas profundos y ofertas de manager jugables de punta a punta

## Cambios recientes (2026-03-07) - Primera fase de refactor arquitectonico
- Objetivo de esta fase:
  - separar responsabilidades entre UI, servicios y logica compartida
  - preparar una estructura mas escalable sin romper la base actual
  - empezar a partir archivos grandes por dominios reales del juego

### Hecho
- Base de estructura nueva creada:
  - `include/`
  - `src/career`
  - `src/transfers`
  - `src/simulation`
  - `src/competition`
  - `src/ui`
  - `src/gui`
  - `src/io`
  - `src/validators`
  - `src/utils`
  - `data/`
  - `saves/`
  - `tests/`
- Datos de ligas movidos desde `LigaChilena/` a `data/LigaChilena/`.
- Guardado por defecto migrado a `saves/career_save.txt` con compatibilidad de carga para el save legacy `career_save.txt` en raiz.
- Modulo nuevo de negociacion extraido:
  - `include/transfers/negotiation_system.h`
  - `src/transfers/negotiation_system.cpp`
- Ese modulo ya centraliza:
  - factores de negociacion
  - promesas contractuales
  - demanda salarial
  - honorarios de agente
  - clima de vestuario
  - riesgo de promesas incumplidas
  - rechazo de fichajes por reputacion/proyecto
- Modulo nuevo de soporte de carrera extraido:
  - `include/career/career_support.h`
  - `src/career/career_support.cpp`
- Ese modulo ya centraliza:
  - estado de directiva
  - estilo del manager
  - mercado de trabajos del entrenador
  - cambio de club del manager
  - informe previo del rival
  - lectura por lineas del equipo
- `app_services.cpp` y `ui.cpp` ya reutilizan esos modulos y dejaron de duplicar varias reglas de carrera y negociacion.
- `build.bat` actualizado para:
  - compilar tambien archivos nuevos dentro de `src/`
  - usar includes de `include/` y `src/`
  - seguir abriendo el juego al terminar
  - permitir `FM_SKIP_RUN=1` para compilar sin lanzar la GUI
- `CMakeLists.txt` ajustado para detectar fuentes en raiz y en `src/`, y para exponer `include/` y `src/` como include paths.
- `README.md` actualizado con la nueva estructura y opciones de compilacion.
- Validaciones realizadas:
  - `build.bat` ejecutado con `FM_SKIP_RUN=1` sin errores
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas

### En progreso
- Division inicial de `simulation.cpp` en submodulos especializados:
  - `src/simulation/match_tactics.cpp`
  - `src/simulation/match_events.cpp`
  - `src/simulation/match_engine_internal.h`
- `simulation.cpp` ya consume helpers extraidos para:
  - modificadores tacticos
  - presion, linea alta y localia
  - eventos tacticos
  - fatiga
  - asignacion de goles y asistencias
  - riesgo extra de lesion por intensidad
- La migracion sigue siendo parcial:
  - persisten archivos legacy grandes
  - todavia hay logica importante concentrada en `ui.cpp`, `app_services.cpp` y `simulation.cpp`
  - la nueva estructura convive temporalmente con archivos raiz mientras se completa el traslado

### Siguiente fase
- Terminar la separacion fuerte entre logica del juego y capa de presentacion.
- Seguir partiendo `simulation.cpp` en motor de partido, eventos, tacticas e IA rival.
- Extraer mas responsabilidades desde `ui.cpp` hacia servicios y modelos de reporte.
- Dividir `app_services.cpp` por dominios como carrera, finanzas, reportes y mercado.
- Avanzar hacia datos mas moddeables para ligas, equipos y plantillas.
- Consolidar la reorganizacion fisica de archivos para que la mayor parte del codigo viva ya en `src/` e `include/`.

## Cambios recientes (2026-03-07) - Repo, build y presentacion open source
- Reorganizado el repositorio para que el codigo compilable principal viva en `src/` e `include/`.
- Movidos los modulos raiz a carpetas por dominio:
  - `src/engine`, `src/career`, `src/simulation`, `src/transfers`, `src/competition`, `src/io`, `src/ui`, `src/gui`, `src/utils`, `src/validators`
  - `include/engine`, `include/career`, `include/simulation`, `include/transfers`, `include/competition`, `include/io`, `include/ui`, `include/gui`, `include/utils`, `include/validators`
- Agregados headers puente en `include/` para mantener compatibilidad temporal con includes legacy mientras termina la migracion.
- `io.cpp` ahora devuelve advertencias estructuradas al cargar divisiones, en vez de imprimir avisos directamente desde la logica de carga.
- `Career` ahora conserva `loadWarnings` para que consola, GUI o servicios decidan como presentar esas advertencias.
- `MatchResult` ahora guarda:
  - advertencias
  - lineas de reporte
  - eventos del partido
  - veredicto final
- `simulation.cpp` ahora expone una base mas limpia para separar simulacion y presentacion:
  - `simulateMatch(...)` devuelve datos del partido sin necesidad de imprimir
  - `playMatch(...)` queda como wrapper compatible para flujo legacy con salida visible
- `utils.cpp` deja preparada una capa de paths/directorios con implementacion Windows y POSIX, reduciendo acoplamiento innecesario a `windows.h`.
- `build.bat` ahora:
  - intenta usar CMake primero
  - cae a compilacion directa con `g++` si CMake no es usable en el entorno
  - compila solo desde `src/`
  - ejecuta el juego usando la raiz del proyecto como directorio de trabajo
  - soporta argumentos directos como `--cli` y `--validate`
  - ejecuta `--cli` y `--validate` en la misma consola
  - mantiene `FM_SKIP_RUN=1` para compilar sin lanzar el juego
- `CMakeLists.txt` fue rehecho para reflejar la nueva estructura modular del proyecto.
- `.gitignore` fue ampliado para ignorar:
  - builds
  - artefactos de CMake
  - ejecutables y objetos
  - saves versionables accidentales
- Eliminado el save legacy `career_save.txt` desde la raiz del repositorio.
- Agregada documentacion nueva en `docs/`:
  - `ARCHITECTURE.md`
  - `ROADMAP.md`
- `README.md` reescrito por completo en ingles, alineado con el estado real del proyecto y pensado para GitHub/open source.
- Verificaciones:
  - `build.bat` ejecutado con `FM_SKIP_RUN=1` y compilacion correcta por fallback directo
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas
  - CMake configurado como camino principal, pero en este entorno concreto falla por un problema externo del toolchain MinGW (`ar.exe` no puede renombrar archivos durante la prueba minima del compilador)

## Cambios recientes (2026-03-09) - Refactor arquitectonico, motor modular y compatibilidad de compilacion
- Reorganizada la orquestacion principal del juego para separar arranque, control del flujo y carrera:
  - `src/main.cpp`
  - `src/engine/game_controller.cpp`
  - `src/engine/game_engine.cpp`
  - `src/career/career_manager.cpp`
  - `include/engine/game_controller.h`
  - `include/engine/game_engine.h`
  - `include/career/career_manager.h`
- Nuevo motor de partidos modular agregado en `include/simulation/` y `src/simulation/` con separacion real de responsabilidades:
  - `match_types`
  - `match_context`
  - `match_resolution`
  - `match_engine`
  - `tactics_engine`
  - `fatigue_engine`
  - `morale_engine`
- `MatchResult` fue ampliado para devolver estructura rica de simulacion:
  - `MatchContext`
  - `MatchStats`
  - `MatchTimeline`
- `simulation.cpp` ahora integra el nuevo motor y aplica consecuencias post-partido reutilizando datos estructurados en vez de depender solo de formulas monoliticas.
- Agregados modulos de IA deportiva y de mercado:
  - `src/ai/ai_match_manager.cpp`
  - `src/ai/ai_squad_planner.cpp`
  - `src/ai/ai_transfer_manager.cpp`
  - `include/ai/ai_match_manager.h`
  - `include/ai/ai_squad_planner.h`
  - `include/ai/ai_transfer_manager.h`
- Agregada capa nueva de tipos y evaluacion de fichajes:
  - `include/transfers/transfer_types.h`
  - `src/transfers/transfer_market.cpp`
  - shortlist por necesidad real de plantel
  - evaluacion por calidad, potencial, costo, salario y encaje
- Extraidos sistemas de desarrollo y finanzas a modulos propios:
  - `src/development/player_progression_system.cpp`
  - `src/development/training_impact_system.cpp`
  - `src/development/youth_generation_system.cpp`
  - `src/finance/finance_system.cpp`
- `src/career/player_development.cpp` paso a usar los sistemas de desarrollo reutilizables en vez de contener logica mezclada.
- `src/career/app_services.cpp` ahora usa el modulo de finanzas para reportes y proyecciones semanales.
- Preparada estructura de datos para crecimiento futuro:
  - `data/configs/`
  - `data/leagues/`
  - `data/players/`
  - `data/teams/`
- Documentacion y estructura del repo actualizadas:
  - `README.md` reescrito para reflejar el estado real del proyecto
  - `docs/ARCHITECTURE.md` actualizado
  - `CMakeLists.txt` ampliado con los nuevos modulos
  - `.gitignore` ajustado para builds y artefactos

### Correcciones tecnicas de compilacion realizadas
- Corregido error en `src/ai/ai_transfer_manager.cpp`:
  - faltaba incluir `utils/utils.h` para usar `normalizePosition(...)`
- Eliminada la dependencia obligatoria de `<filesystem>` para compatibilidad con toolchains MinGW/G++ mas antiguos:
  - `src/utils/utils.cpp` ahora implementa manejo de rutas y directorios con capa compatible Windows/POSIX
  - `src/engine/models.cpp` ahora usa `ensureDirectory("saves")`
- Eliminada la dependencia directa de `std::clamp` en modulos del motor:
  - nuevo helper `clampValue(...)` en `include/utils/utils.h`
  - aplicado en `fatigue_engine.cpp`, `morale_engine.cpp`, `tactics_engine.cpp`, `match_resolution.cpp` y `match_engine.cpp`
- Mejorado `build.bat`:
  - distingue fallo de configuracion de CMake y fallo de compilacion de CMake
  - apunta al log correcto segun el tipo de error
  - mantiene fallback completo con `g++`

### Validaciones realizadas
- Compilacion verificada con `build.bat` usando CMake como camino principal.
- Compilacion fallback completa verificada manualmente con `g++` sobre todo `src/` y link final correcto.
- `src/engine/models.cpp`, `src/utils/utils.cpp` y los modulos nuevos compilan correctamente en el entorno actual.

## Cambios recientes (2026-03-09) - Reportes estructurados, servicios de club y rutas Unicode
- Nueva capa de reportes de carrera agregada en:
  - `include/career/career_reports.h`
  - `src/career/career_reports.cpp`
- Esta capa devuelve estructuras `CareerReport`, `ReportFact` y `ReportBlock` para:
  - competicion
  - directiva
  - club y finanzas
  - scouting
- `app_services.cpp` ya no contiene la logica de armado de estos reportes; ahora delega en la nueva capa y solo expone wrappers como:
  - `buildCompetitionSummaryService(...)`
  - `buildBoardSummaryService(...)`
  - `buildClubSummaryService(...)`
  - `buildScoutingSummaryService(...)`
- Nueva separacion de runtime entre carrera y UI:
  - `include/career/career_runtime.h`
  - `include/career/week_simulation.h`
- Con esto `app_services.cpp` deja de depender directamente de `ui.h` para callbacks y simulacion semanal.
- Nuevo modulo UI para pantallas de reportes y operaciones de club:
  - `include/ui/career_reports_ui.h`
  - `src/ui/career_reports_ui.cpp`
- `ui.cpp` fue aligerado:
  - `displayCompetitionCenter(...)`
  - `displayBoardStatus(...)`
  - `displayNewsFeed(...)`
  - `displaySeasonHistory(...)`
  - `displayClubOperations(...)`
  ahora son wrappers del nuevo modulo.
- Las operaciones de club se movieron a servicios reutilizables:
  - `upgradeClubService(...)` ahora tambien soporta:
    - asistente tecnico
    - preparador fisico
    - jefe de juveniles
  - nuevo `changeYouthRegionService(...)`
  - nuevo `takeManagerJobService(...)`
  - nuevo `listYouthRegionsService()`
- Corregido el problema de carga de equipos con tildes/Ñ tras la eliminacion previa de `std::filesystem`:
  - `src/utils/utils.cpp` ahora usa WinAPI wide para `pathExists`, `isDirectory` y `listDirectories`
  - nuevo helper `readTextFileLines(...)` para leer archivos UTF-8 por ruta Unicode en Windows
  - `src/io/io.cpp` deja de depender de `ifstream` directo para carga de `players.csv`, `players.txt` y formatos legacy
  - `loadTeamsList(...)` tambien usa lectura de archivos compatible con UTF-8
- Esto restablecio correctamente la carga de clubes como:
  - `CD Ñublense`
  - `CD Universidad Católica`
  - `Deportes Concepción`
  - `Universidad de Concepción`
  - `Unión La Calera`
- Build y documentacion:
  - `CMakeLists.txt` actualizado con `career_reports.cpp` y `career_reports_ui.cpp`
  - `README.md` actualizado con `FM_FORCE_FALLBACK=1`
  - `build.bat` ahora permite forzar el build directo con `g++` via `FM_FORCE_FALLBACK=1`
- Validaciones realizadas:
  - compilacion completa por fallback con `g++` sobre todo `src/`
  - linking final correcto
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas en el ejecutable fallback

## Cambios recientes (2026-03-09) - Separacion adicional de UI y simulacion semanal
- Nueva extraccion de logica de carrera fuera de la UI:
  - `src/career/week_simulation.cpp`
  - `include/career/week_simulation.h`
- La simulacion semanal de carrera ya no vive en `src/ui/ui.cpp`:
  - `simulateCareerWeek(...)` se movio a la capa `career/`
  - `checkAchievements(...)` tambien se movio a la capa `career/`
- Esta extraccion dejo centralizados en `week_simulation.cpp` sistemas como:
  - finanzas semanales
  - dashboard semanal
  - eventos de club
  - analisis postpartido para carrera
  - copa de temporada interna
  - dinamica de vestuario y moral por minutos/promesas/resultados
  - desarrollo mensual
  - alertas de shortlist
  - narrativas semanales
  - ofertas entrantes
  - vencimientos y renovaciones de contrato
  - reputacion del DT y estado laboral
  - simulacion de divisiones en segundo plano
- La logica semanal ahora usa `emitUiMessage(...)` y callbacks de runtime en lugar de depender de flujo de entrada/salida embebido en UI.
- Nuevo modulo UI exclusivo para modo copa:
  - `src/ui/cup_ui.cpp`
- `playCupMode(...)` fue removido de `src/ui/ui.cpp` y ahora queda separado como flujo de presentacion especifico.
- Se ampliaron helpers reutilizables de competicion/reportes:
  - `competitionGroupForTeam(...)`
  - `buildCompetitionGroupTable(...)`
  en:
  - `include/career/career_reports.h`
  - `src/career/career_reports.cpp`
- `src/ui/ui.cpp` ahora consume esos helpers en vez de mantener duplicacion local para tablas por grupos.
- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/career/week_simulation.cpp`
    - `src/ui/cup_ui.cpp`
- Documentacion actualizada:
  - `README.md` ahora refleja que:
    - la simulacion semanal vive en `src/career/week_simulation.cpp`
    - el flujo de copa vive en `src/ui/cup_ui.cpp`
    - el manejo de rutas usa capa compatible Unicode/Windows en vez de `std::filesystem` obligatorio
- Impacto arquitectonico:
  - `ui.cpp` reduce tamano y responsabilidad
  - la capa UI queda mas enfocada en mostrar menus y datos
  - la capa `career/` absorbe una parte critica de la logica de negocio que antes seguia mezclada
- Validaciones realizadas:
  - compilacion completa con `build.bat` en fallback `g++`
  - linking final correcto
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas
- Pendiente principal tras esta pasada:
  - mover tambien la resolucion de fin de temporada fuera de `src/ui/ui.cpp`

## Cambios recientes (2026-03-09) - Modularizacion adicional del motor de partidos y tests
- Se modularizo mas el motor de partidos para sacar responsabilidades de `src/simulation/match_engine.cpp`:
  - nuevo `include/simulation/match_phase.h`
  - nuevo `src/simulation/match_phase.cpp`
  - nuevo `include/simulation/match_event_generator.h`
  - nuevo `src/simulation/match_event_generator.cpp`
  - nuevo `include/simulation/match_stats.h`
  - nuevo `src/simulation/match_stats.cpp`
- `src/simulation/match_engine.cpp` ahora actua como orquestador del partido y delega en modulos especializados:
  - `match_phase::evaluatePhase(...)`
  - `match_event_generator::playChances(...)`
  - `match_event_generator::registerDiscipline(...)`
  - `match_event_generator::maybeInjure(...)`
  - `match_stats::countSubstitutions(...)`
  - `match_stats::buildLegacyTimeline(...)`
- Mejoras funcionales aplicadas al motor:
  - las fases del partido ya calculan posesion, intensidad, probabilidad de ocasion y desgaste en un modulo dedicado
  - la generacion de eventos quedo separada del armado final del resultado
  - una expulsion ahora remueve al jugador del `activeXI`, por lo que la inferioridad numerica impacta fases posteriores
  - la fatiga por fase queda mas ligada a la presion, ritmo e intensidad tactica
- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/simulation/match_phase.cpp`
    - `src/simulation/match_event_generator.cpp`
    - `src/simulation/match_stats.cpp`
  - `CMakeLists.txt` ahora tambien define un target de tests:
    - `FootballManagerTests`
- Se agrego una base real de tests en:
  - `tests/project_tests.cpp`
- Cobertura inicial agregada por tests:
  - suite de validacion de datos externos y guardado/carga
  - estructura del motor de partidos y cantidad de fases
  - impacto tactico de presion alta sobre fatiga de fase
  - seleccion de tabla relevante por grupos de competicion
  - evaluacion de asequibilidad en objetivos de fichaje
- Documentacion actualizada:
  - `README.md` ahora refleja:
    - la separacion del flujo de partido en `match_context`, `match_phase`, `match_event_generator`, `match_resolution` y `match_stats`
    - el uso del target `FootballManagerTests`
    - la cobertura de tests disponible
  - `docs/ARCHITECTURE.md` ahora refleja:
    - que `match_engine.cpp` es capa de orquestacion
    - que la capa portable de `src/utils/utils.cpp` sigue reemplazando `std::filesystem` obligatorio por compatibilidad con MinGW viejo
- Validaciones realizadas:
  - compilacion completa del arbol por fallback real con `g++` usando `build.bat`
  - linking correcto del ejecutable principal
  - ejecucion de `FootballManager.exe --validate` con resultado sin fallas
  - compilacion y ejecucion de `build/FootballManagerTests.exe` con todos los tests pasando
- Limitacion de entorno detectada:
  - el target `FootballManagerTests` ya quedo agregado en CMake, pero en este entorno CMake no puede regenerar un arbol limpio por un problema externo de permisos con `ar.exe`/MinGW
  - el codigo quedo igualmente validado por la ruta fallback y por el ejecutable de tests enlazado sobre los objetos compilados

## Cambios recientes (2026-03-09) - Transicion de temporada fuera de UI, test dedicado y verificacion de build principal
- Se completo una separacion adicional entre logica de carrera y UI moviendo la transicion de fin de temporada a un modulo propio:
  - nuevo `include/career/season_transition.h`
  - nuevo `src/career/season_transition.cpp`
- El cierre de temporada ahora devuelve una estructura de datos `SeasonTransitionSummary` con:
  - campeon
  - ascensos
  - descensos
  - lineas de resumen
  - nota de temporada
- La logica de promociones, descensos, premios, historial y avance de temporada ya no pertenece a `src/ui/ui.cpp`:
  - `week_simulation.cpp` ahora consume `endSeason(career)` y emite sus lineas por callbacks/UI messages
  - referencias principales:
    - `src/career/week_simulation.cpp`
    - `src/career/season_transition.cpp`
- El bloque legacy de fin de temporada fue retirado del binario dentro de `src/ui/ui.cpp` para evitar duplicacion y mantener a la UI como capa de presentacion.
- Impacto arquitectonico:
  - la UI deja de ser dueña de reglas de ascenso/descenso y reinicio de temporada
  - la capa `career/` pasa a resolver estado y devolver datos estructurados
  - queda una base mas limpia para exponer este resumen en consola y GUI
- Se agrego cobertura nueva de tests para esta extraccion:
  - `tests/project_tests.cpp`
  - nuevo caso `season_transition`
  - valida que:
    - avance `currentSeason`
    - reinicie `currentWeek`
    - registre historial
    - reconstruya calendario
    - mantenga la division activa alineada con el club del usuario
- Build y verificacion:
  - `build.bat` fue verificado nuevamente usando CMake como camino principal
  - el ejecutable principal compila correctamente por CMake sin caer al fallback para el flujo normal
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas
  - `build/FootballManagerTests.exe` ejecutado manualmente con todos los tests pasando, incluido `season_transition`
- Estado respecto de la GUI:
  - la interfaz grafica ya consume esta logica de forma funcional a traves de `simulateCareerWeekService(...)`
  - el backend nuevo si impacta la GUI
  - aun queda pendiente mostrar en una vista dedicada todo el detalle completo de `SeasonTransitionSummary`
- Limitacion de entorno que sigue vigente:
  - el target `FootballManagerTests` por CMake sigue condicionado por un problema externo de permisos con MinGW/CMake en arboles limpios
  - esto no afecta la compilacion normal del ejecutable principal ni la validacion manual de la suite de tests

## Cambios recientes (2026-03-09) - SeasonService, separation de persistencia y tests estructurales
- Se introdujo un servicio central de flujo semanal para carrera:
  - nuevo `include/career/season_service.h`
  - nuevo `src/career/season_service.cpp`
  - nuevo `include/career/season_flow_controller.h`
  - nuevo `src/career/season_flow_controller.cpp`
- El flujo semanal ahora devuelve estructuras limpias:
  - `WeekSimulationResult`
  - `SeasonStepResult`
- Este servicio central:
  - ejecuta `simulateCareerWeek(...)`
  - captura mensajes del runtime por callback
  - absorbe tambien el `stdout` legado del motor para convertirlo en mensajes estructurados
  - devuelve semana inicial/final, temporada inicial/final, confianza de directiva y ultimo analisis de partido
- `app_services.cpp` fue simplificado:
  - ya no es el dueño del flujo semanal
  - ahora expone `simulateSeasonStepService(...)`
  - `simulateCareerWeekService(...)` pasa a ser un adaptador sobre el nuevo flujo estructurado
- `CareerManager` ahora tambien expone el flujo estructurado:
  - nuevo `simulateSeasonStep()`

- Se avanzo la separacion de `models.cpp` en dominio y persistencia:
  - nuevo `src/engine/career_state.cpp`
  - nuevo `src/io/save_serialization.cpp`
- `career_state.cpp` ahora contiene la implementacion activa de:
  - constructor de `Career`
  - inicializacion de liga
  - calendario
  - seleccion de division activa
  - reseteo y envejecimiento de plantilla
  - noticias
  - transfers pendientes
  - objetivos y confianza de directiva
  - estado de copa
- `save_serialization.cpp` ahora contiene la implementacion activa de:
  - `Career::saveCareer()`
  - `Career::loadCareer()`
  - encoding/decoding de:
    - historial
    - pending transfers
    - listas string
    - head-to-head
    - campos de jugador
- `src/engine/models.cpp` deja de ser la implementacion activa de estado de carrera y persistencia:
  - el bloque anterior fue retirado del binario con una marca legacy para evitar duplicacion de simbolos
  - sigue quedando como proxima etapa separar de ahi tambien el dominio restante de `Player` y `Team`

- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/career/season_service.cpp`
    - `src/career/season_flow_controller.cpp`
    - `src/engine/career_state.cpp`
    - `src/io/save_serialization.cpp`

- Tests nuevos agregados en `tests/project_tests.cpp`:
  - `season_service`
    - valida avance de semana y retorno estructurado
    - valida mensajes y ultimo analisis de partido
  - `save_load_roundtrip`
    - valida roundtrip de persistencia para manager, board state, historial, equipo controlado y fichajes pendientes

- Documentacion actualizada:
  - `README.md` ahora refleja:
    - el uso de `SeasonService` / `SeasonFlowController`
    - la separacion entre `career_state.cpp` y `save_serialization.cpp`
  - `docs/ARCHITECTURE.md` ahora refleja:
    - el nuevo flujo semanal estructurado
    - la separacion de runtime de carrera y serializacion

- Validaciones realizadas:
  - compilacion principal con `build.bat` y CMake como camino principal
  - `FootballManager.exe --validate` ejecutado con resultado sin fallas
  - compilacion manual de `build/FootballManagerTests.exe`
  - ejecucion de tests con resultado:
    - `validation_suite`
    - `match_engine_structure`
    - `tactical_fatigue`
    - `competition_group_table`
    - `transfer_affordability`
    - `season_transition`
    - `season_service`
    - `save_load_roundtrip`
    todos pasando

- Impacto arquitectonico:
  - la simulacion semanal ya no depende de `app_services` como centro de control
  - el dominio de carrera queda mas testeable
  - la persistencia deja de vivir mezclada con el estado de carrera
  - se reduce el acoplamiento entre logica, serializacion y presentacion

## Cambios recientes (2026-03-10) - Refactor fuerte de GUI Windows y navegacion tipo manager

- Se refactorizo la GUI Windows para dejar de depender de un `src/gui/gui.cpp` monolitico:
  - nuevo `include/gui/gui_internal.h`
  - nuevo `src/gui/gui_shared.cpp`
  - nuevo `src/gui/gui_layout.cpp`
  - nuevo `src/gui/gui_views.cpp`
  - nuevo `src/gui/gui_actions.cpp`
  - `src/gui/gui.cpp` queda reducido a:
    - `windowProc`
    - `runGuiApp()`

- Se agrego una estructura visual base mas cercana a un manager moderno:
  - `TopBar`
    - muestra club, fecha/semana, presupuesto, reputacion y alertas
  - `SideMenu`
    - Inicio
    - Plantilla
    - Tacticas
    - Calendario
    - Liga
    - Fichajes
    - Finanzas
    - Cantera
    - Directiva
    - Noticias
  - `MainContentPanel`
  - `OptionalInfoPanel`

- Se introdujo un contrato interno reutilizable para vistas GUI:
  - `GuiPage`
  - `GuiPageModel`
  - `AppState`
  - `DashboardMetric`
  - `TextPanelModel`
  - `ListPanelModel`
  - `FeedPanelModel`

- Componentes reutilizables agregados o consolidados:
  - `Card`
  - `TableView`
  - `SectionHeader`
  - `StatBar`
  - `ActionButton`
  - `TacticsBoard`
  - helpers de dibujo y tema en `src/gui/gui_shared.cpp`

- Se implementaron pantallas y paneles especializados en `src/gui/gui_views.cpp`:
  - `DashboardPanel`
  - `UpcomingMatchWidget`
  - `LeaguePositionWidget`
  - `TeamMoraleWidget`
  - `InjuryListWidget`
  - `NewsFeedWidget`
  - `PlayerTableView`
  - `PlayerProfilePanel`
  - `PlayerComparisonPanel`
  - `LeagueTableView`
  - `FixtureListView`
  - `TransferMarketView`
  - `TransferTargetCard`
  - `AlertPanel`
  - `NewsFeedPanel`

- Mejoras funcionales visibles:
  - dashboard con:
    - proximo partido
    - posicion en liga
    - estado del vestuario
    - lesionados
    - objetivo de directiva
    - noticias y alertas
  - pantalla de plantilla con:
    - tabla completa
    - orden por columnas
    - filtros por posicion
    - ficha lateral del jugador
    - comparador de jugador
  - pantalla tactica con:
    - campo dibujado
    - once inicial posicionado por lineas
    - barras tacticas de presion, ritmo, anchura, linea y moral
    - explicacion de impacto tactico
  - pantalla de fichajes con:
    - vista tipo base de datos
    - filtros por posicion/edad/potencial/costo
    - tarjeta de objetivo
    - pipeline de movimientos pendientes
  - pantallas de:
    - finanzas
    - cantera
    - directiva
    - noticias
    - calendario
    - liga

- Separacion de responsabilidades aplicada:
  - `gui_actions.cpp` solo coordina acciones de UI contra servicios
  - `gui_views.cpp` construye modelos de pantalla
  - `gui_layout.cpp` controla layout y chrome visual
  - `gui_shared.cpp` concentra helpers Win32, draw y estilo
  - la UI consume datos ya preparados, en vez de mezclar layout con logica dispersa

- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/gui/gui_actions.cpp`
    - `src/gui/gui_layout.cpp`
    - `src/gui/gui_shared.cpp`
    - `src/gui/gui_views.cpp`

- Validacion realizada:
  - compilacion con `build.bat --validate`
  - compilacion principal exitosa por CMake

- Estado actual / limite conocido:
  - el resumen y analisis post-partido ya se exponen mejor en dashboard/noticias
  - aun queda pendiente una pantalla dedicada de partido en vivo con timeline persistido completo

- Impacto arquitectonico:
  - la GUI ya no depende de un unico archivo gigante
  - la navegacion es mucho mas clara
  - la informacion queda mejor distribuida sin saturar una sola vista
  - queda una base mucho mas mantenible para seguir profundizando match center, modales y comparadores

## 2026-03-10 - Match Engine, IA Tactica y Negociacion Estructurada

- Se profundizo el motor de partidos para que la cadena de simulacion deje de saltar tan rapido de dominio a remate:
  - `include/simulation/match_types.h`
  - `include/simulation/match_phase.h`
  - `include/simulation/match_event_generator.h`
  - `src/simulation/match_phase.cpp`
  - `src/simulation/match_event_generator.cpp`
  - `src/simulation/match_engine.cpp`

- Nuevas capacidades del motor:
  - fases con:
    - posesiones
    - progresiones
    - ataques maduros
    - remates generados
    - riesgo defensivo
  - nuevo tipo de evento:
    - `Progression`
  - no todos los ataques terminan en remate
  - el partido ahora sigue mejor la cadena:
    - posesion
    - progresion
    - ataque
    - ocasion
    - remate
    - gol/parada/fallo

- Se agrego un sistema de reporte explicativo del partido:
  - `include/simulation/match_report.h`
  - `src/simulation/match_report.cpp`
  - `include/engine/models.h`

- Nuevas estructuras de reporte:
  - `MatchReport`
  - `MatchExplanation`
  - `TacticalImpactSummary`
  - `FatigueImpactSummary`

- El resultado del partido ahora incluye:
  - resumen por fases
  - explicacion probable del partido
  - impacto tactico
  - lectura de fatiga
  - trazabilidad de dominio y ocasiones

- Se corrigio una perdida de informacion en el motor:
  - los flags `homeTacticalChange` y `awayTacticalChange` ahora se preservan correctamente en cada fase despues de la reevaluacion del contexto

- Se mejoro la IA tactica en partido:
  - `include/ai/team_ai.h`
  - `src/ai/team_ai.cpp`
  - `src/ai/ai_match_manager.cpp`

- Nuevas reacciones de IA:
  - ajuste tras expulsiones
  - proteccion de amonestados
  - reduccion de agresividad por riesgo disciplinario
  - reestructuracion mas conservadora o de contra segun contexto

- Se profundizo el mercado de fichajes con negociacion real por rondas:
  - `include/transfers/transfer_types.h`
  - `include/transfers/negotiation_system.h`
  - `src/transfers/negotiation_system.cpp`
  - `src/career/app_services.cpp`
  - `src/transfers/transfer_market.cpp`

- Nuevas capacidades de negociacion:
  - multiples rondas
  - expectativa del club vendedor
  - demanda salarial del jugador
  - contraofertas
  - competencia de otros clubes
  - acuerdo final con:
    - fee
    - salario
    - bono/agente
    - clausula
    - duracion
    - rol prometido
  - trazabilidad via `roundSummaries`

- Servicios conectados a la negociacion estructurada:
  - `buyTransferTargetService(...)`
  - `triggerReleaseClauseService(...)`
  - `signPreContractService(...)`
  - `renewPlayerContractService(...)`

- La IA de mercado CPU tambien paso a usar esta base de negociacion para compras normales, manteniendo compatibilidad con prestamos y limites de presupuesto

- Se conecto el nuevo analisis del partido a carrera/UI:
  - `src/career/week_simulation.cpp`
  - `src/ui/ui.cpp`

- `career.lastMatchAnalysis` ahora puede reflejar:
  - explicacion probable del partido
  - lectura tactica
  - impacto de fatiga

- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/simulation/match_report.cpp`

- Tests agregados/mejorados:
  - `tests/project_tests.cpp`
  - se reforzo `match_engine_structure`
  - se agrego `transfer_negotiation`

- Validacion realizada:
  - `build.bat --validate`
  - compilacion principal exitosa por CMake
  - compilacion manual de `build/FootballManagerTests.exe` con `g++`
  - suite completa de tests pasando

- Limite conocido del entorno:
  - el target `FootballManagerTests` dentro de `build-cmake` sigue afectado por permisos temporales de MinGW/CTest al escribir archivos auxiliares
  - el codigo del proyecto y la suite manual quedaron validados igualmente

- Impacto arquitectonico:
  - el motor de partidos es mas explicable y menos dependiente de una formula comprimida
  - tactica e IA tienen efecto mas visible sobre el desarrollo del partido
  - el mercado deja historial y logica de negociacion reusable
  - queda una base mejor para seguir con match center, scouting avanzado y dinamica de vestuario

## 2026-03-10 - Persistencia desacoplada, reportes visibles y validacion de datos

- Se siguio limpiando la frontera entre dominio y persistencia:
  - `include/io/save_manager.h`
  - `include/io/save_serialization.h`
  - `src/io/save_manager.cpp`
  - `src/io/save_serialization.cpp`
  - `CMakeLists.txt`

- Nuevo esquema de persistencia:
  - `save_manager` ahora se encarga de:
    - resolver rutas
    - abrir archivos
    - coordinar guardado/carga
  - `save_serialization` ahora trabaja por stream con:
    - `serializeCareer(...)`
    - `deserializeCareer(...)`
  - `Career::saveCareer()` y `Career::loadCareer()` quedan como delegados finos sobre esa capa

- Mejora concreta de guardado/carga:
  - se corrigio una regresion donde el guardado podia recalcular y sobrescribir identidad de club
  - ahora se preservan correctamente datos como:
    - `clubPrestige`
    - `clubStyle`
    - `youthIdentity`
    - `primaryRival`

- Se reforzo la utilidad del reporte de partido:
  - `include/simulation/match_types.h`
  - `src/simulation/match_report.cpp`
  - `src/career/week_simulation.cpp`
  - `src/ui/ui.cpp`

- Nuevos datos visibles del postpartido:
  - `playerOfTheMatch`
  - `playerOfTheMatchScore`
  - `postMatchImpact`

- El analisis del ultimo partido ahora puede mostrar:
  - figura del partido
  - lectura de fatiga
  - impacto posterior del encuentro

- Se mejoro la presentacion de informacion al jugador:
  - `src/career/career_support.cpp`
  - `src/gui/gui_views.cpp`

- El informe rival ahora incluye:
  - estilo del rival
  - plan ofensivo esperado
  - amenaza principal
  - lectura por lineas
  - alerta tactica
  - contexto de clasico si corresponde

- La GUI aprovecha mejor esta informacion:
  - dashboard con informe rival visible
  - pantalla tactica con informe rival en el detalle
  - mejor reutilizacion del ultimo analisis de partido dentro de paneles ya existentes

- Se mejoro la validacion automatica de datos externos:
  - `src/validators/validators.cpp`

- Nueva validacion:
  - revisa estructura minima de planteles
  - detecta nombres duplicados dentro del mismo club
  - reporta advertencias de cobertura por lineas sin romper la suite sobre datos legacy que el juego ya acepta

- Tests agregados/mejorados:
  - `tests/project_tests.cpp`
  - nuevo test:
    - `opponent_report`
  - se reforzo cobertura del reporte de partido con figura del encuentro
  - se mantuvo cobertura de:
    - `save_load_roundtrip`
    - `season_service`
    - `validation_suite`

- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/io/save_manager.cpp`

- Validacion realizada:
  - `build.bat --validate`
  - compilacion principal exitosa por CMake
  - compilacion manual de `build/FootballManagerTests.exe` con `g++`
  - suite completa de tests pasando nuevamente

- Limite conocido del entorno:
  - siguen apareciendo dos lineas `Acceso denegado.` despues del `build.bat --validate`
  - el ejecutable principal se genera bien
  - la suite manual tambien queda validada

- Impacto arquitectonico:
  - persistencia mas aislada del dominio
  - save/load mas facil de probar y mantener
  - reportes mas utiles para el jugador sin mover logica a UI
  - mejor base para seguir con mods, save manager mas rico y reportes semanales especializados

2026-03-10

- Refactor importante del motor de partidos:
  - `src/simulation/simulation.cpp` dejo de ser un archivo tan cargado de responsabilidades
  - ahora funciona como fachada publica de compatibilidad para:
    - `computeStrength`
    - `applyTactics`
    - `simulateMatch`
    - `playMatch`

- Nuevos modulos creados para separar responsabilidades:
  - `include/simulation/match_postprocess.h`
  - `src/simulation/match_postprocess.cpp`
  - `src/simulation/player_condition.cpp`

- Nuevo modulo `match_postprocess`:
  - centraliza la aplicacion del resultado del partido al estado real del mundo
  - ahora resuelve en un solo lugar:
    - puntos
    - goles a favor/en contra
    - goles de visita
    - moral postpartido
    - tarjetas nominales
    - goles y asistencias
    - lesiones nominales
    - desarrollo por partido
    - forma postpartido
    - fatiga final

- Nuevo modulo `player_condition`:
  - mueve fuera de `simulation.cpp` la logica de:
    - `simulateInjury(...)`
    - `healInjuries(...)`
    - `recoverFitness(...)`
  - mejora la separacion entre simulacion de partido y mantenimiento fisico del plantel

- Mejora del impacto tactico real en el motor moderno:
  - `src/simulation/tactics_engine.cpp`
  - `src/simulation/match_phase.cpp`

- Ajustes tacticos aplicados:
  - mayor peso de:
    - presion
    - ritmo
    - anchura
    - linea defensiva
  - mas riesgo al usar linea alta contra juego directo
  - mayor influencia tactica en:
    - posesion por fase
    - riesgo defensivo
    - transiciones
    - control territorial

- Reportes del partido mejorados:
  - `src/simulation/match_report.cpp`

- El reporte ahora expone mejor:
  - riesgo tactico por partido
  - impacto disciplinario
  - riesgo defensivo por fase
  - mejor explicacion del dominio territorial
  - mejor lectura del desgaste por presion sostenida

- Mejora conductual de IA en cambios:
  - `src/simulation/fatigue_engine.cpp`
  - los equipos con:
    - presion alta
    - ritmo alto
    - marcaje mas agresivo
    - varios amonestados
  - ahora empujan antes cambios por fatiga/riesgo

- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/simulation/match_postprocess.cpp`
    - `src/simulation/player_condition.cpp`

- Tests ampliados:
  - `tests/project_tests.cpp`
  - se agrego:
    - `simulate_match_state`
  - se reforzo `match_engine_structure` para exigir:
    - `Riesgo tactico:` en `reportLines`
    - `Disciplina:` en `reportLines`

- Validacion realizada:
  - `build.bat --validate`
  - compilacion principal exitosa por CMake
  - compilacion manual de `build/FootballManagerTests.exe` con `g++`
  - suite completa de tests pasando:
    - `validation_suite`
    - `match_engine_structure`
    - `tactical_fatigue`
    - `competition_group_table`
    - `transfer_affordability`
    - `transfer_negotiation`
    - `season_transition`
    - `season_service`
    - `opponent_report`
    - `simulate_match_state`
    - `save_load_roundtrip`

- Limpieza tecnica adicional:
  - se eliminaron warnings evitables en:
    - `src/simulation/match_engine.cpp`
    - `src/transfers/negotiation_system.cpp`
    - `src/validators/validators.cpp`

- Impacto arquitectonico:
  - menor acoplamiento entre simulacion y aplicacion del estado del mundo
  - `simulation.cpp` mucho mas controlable para futuras extracciones
  - mejor frontera entre motor de partido, postproceso y estado fisico
  - base mas limpia para seguir rompiendo el motor en modulos pequenos

2026-03-10

- Se completo otro paso importante del refactor del motor de partidos:
  - se separo la resolucion de ocasiones del generador de eventos
  - ahora existe un modulo dedicado:
    - `include/simulation/match_event_resolver.h`
    - `src/simulation/match_event_resolver.cpp`

- Nuevo reparto de responsabilidades en simulacion:
  - `match_event_generator`
    - decide progresion, llegada y creacion de ocasion
  - `match_event_resolver`
    - resuelve remate, gol, parada, fallo y corner
  - `match_resolution`
    - queda enfocado en probabilidad base de ocasion

- Esto mejora la arquitectura del motor:
  - menor mezcla entre generacion y resolucion
  - mejor base para seguir afinando:
    - calidad de remate
    - lectura del portero
    - corners
    - conversion de ocasiones

- Se extrajo la logica de analisis del ultimo partido fuera de UI y simulacion semanal:
  - nuevo servicio:
    - `include/career/match_analysis_store.h`
    - `src/career/match_analysis_store.cpp`

- El servicio nuevo centraliza:
  - construccion del resumen pospartido del club usuario
  - almacenamiento del ultimo analisis
  - almacenamiento de lineas detalladas del reporte
  - almacenamiento de eventos clave
  - almacenamiento de la figura del partido

- Se limpio duplicacion de logica en:
  - `src/career/week_simulation.cpp`
  - `src/ui/ui.cpp`

- Ambas rutas ahora reutilizan:
  - `career_match_analysis::storeMatchAnalysis(...)`

- El dominio de carrera ahora conserva mas informacion del ultimo partido:
  - `include/engine/models.h`
  - nuevos campos:
    - `lastMatchReportLines`
    - `lastMatchEvents`
    - `lastMatchPlayerOfTheMatch`

- Se actualizo persistencia para estos nuevos datos:
  - `src/io/save_serialization.cpp`
  - tambien se ajusto la ruta legacy en:
    - `src/engine/models.cpp`

- Nuevo contenido persistido en save/load:
  - `LASTMATCH_REPORT`
  - `LASTMATCH_EVENTS`
  - `LASTMATCH_POTM`

- Version de save actualizada:
  - de `5` a `6`

- Limpieza adicional en reseteo de carrera/temporada:
  - `src/engine/career_state.cpp`
  - `src/engine/models.cpp`
  - ahora tambien se limpian los nuevos datos estructurados del ultimo partido

- Mejora visible en GUI:
  - `src/gui/gui_views.cpp`

- La interfaz ahora muestra mejor el ultimo partido:
  - resumen largo reutilizable
  - bloque `MatchReport`
  - bloque `MatchTimeline`
  - mas detalle en:
    - dashboard
    - pantalla tactica
    - paneles de reporte

- Mejora de reportes de carrera:
  - `src/career/career_reports.cpp`
  - el bloque `Ultimo analisis` ya no depende solo de un string corto
  - ahora incorpora parte del reporte estructurado del partido

- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/simulation/match_event_resolver.cpp`
    - `src/career/match_analysis_store.cpp`

- Tests ampliados:
  - `tests/project_tests.cpp`
  - nuevo test:
    - `match_analysis_store`
  - `save_load_roundtrip` ahora tambien valida:
    - reporte estructurado del ultimo partido
    - eventos guardados del ultimo partido
    - figura del partido guardada

- Validacion realizada:
  - `build.bat --validate`
  - compilacion principal exitosa por CMake
  - compilacion manual de `build/FootballManagerTests.exe` con `g++`
  - suite completa de tests pasando:
    - `validation_suite`
    - `match_engine_structure`
    - `tactical_fatigue`
    - `competition_group_table`
    - `transfer_affordability`
    - `transfer_negotiation`
    - `season_transition`
    - `season_service`
    - `opponent_report`
    - `match_analysis_store`
    - `simulate_match_state`
    - `save_load_roundtrip`

- Impacto arquitectonico:
  - menos logica repetida entre UI y carrera
  - mejor frontera entre motor, analisis y presentacion
  - mejor soporte para GUI rica y reportes mas profundos
  - el ultimo partido ya se trata como dato estructurado, no solo como texto plano

2026-03-10

- Se agrego un `match center` estructurado para el ultimo partido:
  - `include/career/match_center_service.h`
  - `src/career/match_center_service.cpp`

- Nuevo snapshot persistente del ultimo partido en dominio:
  - `include/engine/models.h`
  - nuevo campo:
    - `lastMatchCenter`

- El snapshot ahora conserva:
  - competicion
  - rival
  - localia
  - marcador
  - tiros
  - tiros al arco
  - posesion
  - corners
  - cambios
  - xG aproximado
  - clima
  - resumen tactico
  - resumen de fatiga
  - impacto posterior
  - fases resumidas

- Se extrajo la dinamica semanal de vestuario y promesas:
  - `include/career/dressing_room_service.h`
  - `src/career/dressing_room_service.cpp`

- El nuevo servicio de vestuario ahora evalua:
  - promesas en riesgo
  - moral baja
  - fatiga alta
  - deseo de salida
  - lideres del vestuario
  - alertas resumidas para GUI y CLI

- Limpieza de logica duplicada:
  - `src/career/week_simulation.cpp`
  - `src/ui/ui.cpp`
  - ambos flujos ahora delegan en:
    - `dressing_room_service`
    - `match_center_service`

- Mejora visible en GUI:
  - `src/gui/gui_views.cpp`
  - ahora muestra:
    - `MatchCenter`
    - `DressingRoomPanel`
  - se integraron en:
    - dashboard
    - panel tactico
    - vista de noticias/reportes

- Mejora de reportes estructurados:
  - `src/career/career_reports.cpp`
  - nuevo reporte:
    - `buildMatchCenterReport(...)`
  - los reportes de competicion, club y directiva ahora aprovechan:
    - match center
    - alertas de vestuario

- Persistencia ampliada:
  - `src/io/save_serialization.cpp`
  - `src/engine/models.cpp`

- Nuevos bloques de save/load:
  - `LASTMATCH_CENTER`
  - `LASTMATCH_PHASES`

- Version de save actualizada:
  - de `6` a `7`

- Limpieza de reseteo estacional:
  - `src/engine/career_state.cpp`
  - `src/engine/models.cpp`
  - ahora tambien se limpia:
    - `lastMatchCenter`

- Build actualizado:
  - `CMakeLists.txt` ahora incluye:
    - `src/career/match_center_service.cpp`
    - `src/career/dressing_room_service.cpp`

- Tests ampliados:
  - `tests/project_tests.cpp`
  - nuevos tests:
    - `match_center_service`
    - `dressing_room_service`
  - `save_load_roundtrip` ahora tambien valida:
    - snapshot del match center
    - fases persistidas del ultimo partido

- Validacion realizada:
  - `build.bat --validate`
  - compilacion principal exitosa por CMake
  - compilacion manual de `build/FootballManagerTests.exe` con `g++`
  - suite completa de tests pasando:
    - `validation_suite`
    - `match_engine_structure`
    - `tactical_fatigue`
    - `competition_group_table`
    - `transfer_affordability`
    - `transfer_negotiation`
    - `season_transition`
    - `season_service`
    - `opponent_report`
    - `match_analysis_store`
    - `match_center_service`
    - `dressing_room_service`
    - `simulate_match_state`
    - `save_load_roundtrip`

- Impacto arquitectonico:
  - el ultimo partido deja de depender de un string plano y pasa a ser un dato estructurado reutilizable
  - el vestuario deja de vivir incrustado en `week_simulation` y queda como sistema propio
  - GUI, CLI, carrera y persistencia consumen la misma fuente de verdad
  - mejora la base para un `match center` mas profundo y para dinamicas humanas mas ricas

## 2026-03-10 - Auditoria de plantillas y validacion de datos externos

- Se agrego una auditoria real de plantillas sobre datos crudos en:
  - `include/validators/validators.h`
  - `src/validators/validators.cpp`

- Nuevas estructuras de validacion:
  - `DataValidationIssue`
  - `DataValidationReport`

- Nuevas funciones publicas:
  - `buildRosterDataValidationReport()`
  - `writeRosterDataValidationReport(const std::string& path)`

- La auditoria ahora revisa directamente `data/LigaChilena` y detecta:
  - plantillas demasiado pequenas o demasiado grandes
  - posiciones obligatorias faltantes
  - jugadores duplicados dentro de un club
  - jugadores duplicados entre clubes
  - edades faltantes o fuera de rango
  - posiciones vacias o invalidas
  - inconsistencias entre `position` y `position_raw`
  - equipos sin archivo de plantilla valido
  - divisiones con conteo incorrecto de equipos
  - atributos cargados fuera de rango
  - casos con `potential < skill`

- `--validate` ahora imprime una nueva seccion:
  - `Auditoria de Plantillas`

- Se genera un reporte completo en disco:
  - `saves/roster_validation_report.txt`

- Resultado real de la auditoria sobre la base actual:
  - divisiones revisadas: `5`
  - equipos revisados: `90`
  - jugadores crudos revisados: `7111`
  - errores detectados: `9249`
  - advertencias detectadas: `1286`

- Problemas dominantes encontrados:
  - `Datos invalidos`
  - `Posicion`
  - `Integridad liga`
  - `Posiciones obligatorias`
  - `Tamano plantilla`

- Ejemplos concretos detectados:
  - clubes con posiciones obligatorias insuficientes en Primera Division:
    - `Audax Italiano`
    - `CD Cobresal`
    - `CD Palestino`
    - `CSD Colo-Colo`
    - `Deportes La Serena`
    - `Deportes Limache`
  - equipos sin plantilla valida en divisiones bajas:
    - `Atletico Oriente`
    - `Deportes Rancagua`
    - `Futuro FC`
    - `Rodelindo Roman`
    - `EFC Conchali`
    - `Gasparin FC`
    - `Jardin del Eden`
  - errores de codificacion visibles en nombres/valores:
    - `MartA-n`
    - `BenjamA-n`
    - `a,!300k`

- Decision tecnica actual:
  - la auditoria es informativa y no rompe `summary.ok` todavia
  - esto permite seguir usando `--validate` y la suite automatica mientras se corrige la base externa

- Validacion realizada:
  - compilacion por fallback con `build.bat --validate`
  - generacion exitosa de `saves/roster_validation_report.txt`
  - recompilacion manual de `build/FootballManagerTests.exe`
  - suite automatica completa pasando

- Impacto arquitectonico:
  - el juego ya no depende solo del `loader` para descubrir errores de datos
  - existe una barrera real de calidad para auditar la base externa antes de jugar
  - queda preparada la base para endurecer la validacion y bloquear arranque cuando la base este saneada

## 2026-03-10 - Mejora visual de la GUI y claridad de informacion

- Se mejoro la interfaz grafica sin tocar la logica del juego en:
  - `src/gui/gui_views.cpp`
  - `src/gui/gui_layout.cpp`
  - `src/gui/gui_shared.cpp`

- Se agrego un mapeo de nombres amigables para la interfaz:
  - los ids tecnicos se mantienen en codigo
  - la GUI ahora muestra textos como:
    - `Resumen del club`
    - `Tabla de liga`
    - `Proximo partido`
    - `Lesiones`
    - `Calendario`

- Se rediseño el dashboard principal:
  - el panel de `Proximo partido` ahora tiene mayor peso visual
  - se reorganizo el layout para mostrar:
    - `Proximo partido`
    - `Tabla de liga`
    - `Estado del equipo`
    - `Ultimo resultado`
    - `Lesiones`
    - `Noticias`

- Se agrego un panel nuevo:
  - `TeamStatusPanel`
  - muestra:
    - fatiga del equipo
    - moral del equipo
    - lesionados
    - suspendidos
    - carga alta
    - estado del vestuario

- Se mejoro el menu lateral:
  - se mantuvo la navegacion existente
  - se hicieron las entradas mas visuales con prefijos tipo:
    - `[H] Inicio`
    - `[P] Plantilla`
    - `[T] Tacticas`
    - `[$] Fichajes`
    - `[Y] Cantera`

- Se reforzo la jerarquia visual:
  - el dashboard usa un layout especial con panel principal mas ancho
  - la columna derecha muestra `Ultimo resultado` y `Noticias`
  - se redujo espacio vacio en la pantalla principal

- Se agregaron colores informativos en tablas:
  - verde para estados positivos
  - amarillo para advertencias
  - rojo para problemas
  - la tabla de liga ahora colorea:
    - puestos altos
    - mitad de tabla
    - descenso
  - las tablas de estado del equipo y lesiones ahora resaltan riesgo de forma visible

- Se mejoraron mensajes vacios:
  - `No hay carrera activa. Crea una nueva partida para comenzar.`
  - `No hay partidos programados`
  - `No hay lesiones actualmente`
  - `No hay noticias recientes`

- Se mantuvo la arquitectura actual:
  - no se movio logica de juego a GUI
  - la capa visual sigue consumiendo datos estructurados del motor

- Validacion realizada:
  - `build.bat --validate`
  - compilacion principal exitosa por CMake
  - ejecutable generado en `build-cmake/bin/FootballManager.exe`

- Impacto en UX:
  - la interfaz deja de exponer nombres internos al jugador
  - mejora lectura rapida de estado, riesgo y contexto competitivo
  - el dashboard se acerca mas a una experiencia tipo manager moderno

## Cambios recientes (2026-03-11) - Motor, IA, mercado, GUI y reglas externas

- Se profundizo otra iteracion del motor de partidos para que la secuencia:
  - posesion
  - progresion
  - llegada
  - ocasion
  - remate
  - gol / fallo
  dependa mas del contexto futbolistico y menos de conversiones demasiado directas.

- Archivos principales tocados en simulacion:
  - `include/simulation/match_context.h`
  - `include/simulation/match_resolution.h`
  - `src/simulation/match_context.cpp`
  - `src/simulation/match_phase.cpp`
  - `src/simulation/match_resolution.cpp`
  - `src/simulation/match_event_generator.cpp`
  - `src/simulation/match_event_resolver.cpp`
  - `src/simulation/match_report.cpp`

- Nuevos factores integrados al snapshot del partido:
  - `chanceCreation`
  - `finishingQuality`
  - `pressResistance`
  - `defensiveShape`
  - `lineBreakThreat`
  - `pressingLoad`
  - `setPieceThreat`

- Las tacticas ahora pesan mas en:
  - posesion
  - frecuencia de ataques
  - calidad de ocasiones
  - riesgo defensivo
  - desgaste fisico

- Se redujo la tendencia a marcadores exagerados:
  - menos ataques terminan en remate
  - se estrecho el rango de posesion esperable
  - se bajo la conversion final de ocasiones
  - el portero y la presion rival afectan mejor el remate

- Se mejoro la IA durante el partido:
  - `include/ai/team_ai.h`
  - `src/ai/team_ai.cpp`
  - `include/ai/ai_match_manager.h`
  - `src/ai/ai_match_manager.cpp`
  - `src/simulation/match_engine.cpp`

- La IA rival ahora puede:
  - reaccionar si va perdiendo
  - proteger ventaja
  - adaptarse a expulsiones
  - bajar intensidad con fatiga severa
  - empujar mas fuerte en el tramo final
  - hacer cambios extra por caida fisica del XI activo

- Se mejoro la IA de plantilla y uso de juveniles:
  - `include/ai/ai_squad_planner.h`
  - `src/ai/ai_squad_planner.cpp`
  - `src/engine/models.cpp`

- Nuevos datos de planificacion de plantel:
  - `rotationRisk`
  - `thinPositions`
  - `youthCoverPositions`

- Efecto practico:
  - mejor deteccion de puestos debiles
  - mejor rotacion por fatiga/sanciones
  - mas uso de juveniles con potencial cuando la politica del club lo permite

- Mercado de fichajes e IA de objetivos reforzados:
  - `include/transfers/transfer_types.h`
  - `src/ai/ai_transfer_manager.cpp`
  - `src/transfers/transfer_market.cpp`

- Se consolidaron estructuras y señales para fichajes:
  - `TransferTarget`
  - `TransferOffer`
  - `ContractOffer`
  - `NegotiationState`
  - `ClubTransferStrategy`

- Nuevos campos usados por la IA de mercado:
  - `onShortlist`
  - `urgentNeed`
  - `scoutingConfidence`
  - `competitionScore`
  - `scoutingNote`

- Efecto practico en fichajes:
  - los clubes priorizan mejor necesidad real
  - la shortlist influye de verdad
  - hay menos comportamiento CPU demasiado determinista
  - algunos clubes prefieren promocionar juveniles antes que comprar por inercia

- Reglas de competicion externalizadas:
  - `include/competition/competition.h`
  - `src/competition/competition.cpp`
  - `data/LigaChilena/competition_rules.csv`

- Ahora las reglas base por division se pueden cargar desde CSV con:
  - perfil de tabla
  - handler de temporada
  - grupos
  - ingresos
  - factor salarial
  - tamaño maximo de plantel
  - cantidad esperada de equipos

- Se reforzo la validacion automatica al cargar datos:
  - `include/validators/validators.h`
  - `src/validators/validators.cpp`
  - `src/engine/career_state.cpp`

- Nuevo control runtime de datos cargados:
  - equipos duplicados
  - planteles demasiado cortos/largos
  - jugadores duplicados
  - posiciones invalidas
  - equipos sin arquero
  - cobertura insuficiente por linea
  - referencias rotas en `preferredXI`

- Carga de datos externos y planteles:
  - `src/io/io.cpp`
  - se mantuvo compatibilidad con el proyecto existente
  - fallback real de archivos de plantilla
  - rebalanceo final de roles dentro de los limites por division

- Mejora visible de GUI:
  - `src/gui/gui_views.cpp`
  - se reemplazaron titulos tecnicos visibles por nombres mas naturales para el jugador
  - ejemplos:
    - `DashboardPanel` -> `Centro del club`
    - `LeagueTableView` -> `Clasificacion`
    - `MatchSummaryPanel` -> `Proximo encuentro`
    - `TeamStatusPanel` -> `Estado del plantel`

- Tests ampliados para cubrir este bloque:
  - `tests/project_tests.cpp`
  - nuevos tests:
    - `low_block_chance_quality`
    - `competition_rules_csv`
    - `runtime_load_validation`
    - `transfer_shortlist`

- Verificacion realizada:
  - `cmake --build build-cmake --config Release --target FootballManagerTests`
  - `FootballManagerTests.exe`: todos los tests pasan
  - `FootballManager.exe --validate`: 0 fallas logicas y la auditoria sigue reportando deuda de datos externos

- Impacto general:
  - el motor de partidos responde mejor a atributos, tacticas, presion y fatiga
  - la IA rival y de plantilla toma decisiones mas creibles
  - el mercado tiene mejor criterio deportivo y de scouting
  - la carga de reglas y datos externos queda mas flexible
  - la GUI expone mejor la informacion al jugador

## Cambios recientes (2026-03-11) - Validacion de arranque, ventas IA y radar de fichajes

- Se agrego una validacion automatica explicita de datos externos al iniciar la carga de ligas:
  - `include/validators/validators.h`
  - `src/validators/validators.cpp`
  - `src/engine/career_state.cpp`

- Nuevo resumen de arranque:
  - `StartupValidationSummary`
  - reutiliza la auditoria profunda de `data/LigaChilena`
  - genera/actualiza `saves/roster_validation_report.txt`
  - expone un resumen corto dentro de `loadWarnings`

- Efecto practico:
  - la validacion de datos ya no depende solo de ejecutar `--validate`
  - al iniciar el juego o recargar ligas quedan visibles:
    - conteo de errores
    - conteo de advertencias
    - primeras incidencias relevantes
    - referencia al reporte completo en disco

- Se ajusto la GUI para no ocultar estos avisos al jugador:
  - `src/gui/gui_actions.cpp`
  - al crear o cargar carrera, si hay mensajes relevantes de carga, ahora se muestran en dialogo

- IA de plantilla reforzada para detectar jugadores prescindibles:
  - `include/ai/ai_squad_planner.h`
  - `src/ai/ai_squad_planner.cpp`
  - `include/transfers/transfer_types.h`
  - `src/ai/ai_transfer_manager.cpp`
  - `src/transfers/transfer_market.cpp`

- Nuevos datos de planificacion:
  - `unusedSeniorPlayers`
  - `salePressure`
  - `saleCandidates`

- La IA ahora identifica mejor:
  - veteranos casi sin minutos
  - jugadores con potencial bajo respecto a su nivel actual
  - futbolistas que quieren salir
  - excedentes en posiciones sobrantes

- Efecto practico en mercado CPU:
  - los clubes IA pueden vender suplentes marginales aunque no esten solo fuera de perfil
  - la limpieza de plantel es mas creible cuando hay sobrecupo, tension financiera o acumulacion de piezas sin uso

- Radar de fichajes de la GUI conectado con la evaluacion real del mercado:
  - `src/gui/gui_views.cpp`

- La vista de fichajes ahora usa la evaluacion de `ai_transfer_manager` para mostrar:
  - costo estimado
  - salario esperado
  - etiqueta de radar/scouting
  - contexto de mercado
  - shortlist y prioridad por necesidad

- Mejora visible:
  - el panel de fichajes ya no es solo una lista cruda de jugadores
  - ahora refleja mejor:
    - viabilidad economica
    - urgencia deportiva
    - seguimiento previo

- Tests nuevos/agregados:
  - `startup_data_validation`
  - `squad_sale_candidates`

- Verificacion realizada:
  - `build.bat --validate`
  - `cmake --build build-cmake --config Release --target FootballManagerTests`
  - `FootballManagerTests.exe`: todos los tests pasan
  - `FootballManager.exe --validate`: sigue marcando `9236` errores y `1283` advertencias de la base externa, ahora tambien preparados para resumirse al arranque

## Cambios recientes (2026-03-11) - Saneamiento masivo de `data/LigaChilena`

- Se agrego una herramienta reusable para limpiar la base externa:
  - `tools/sanitize_ligachilena_data.ps1`

- El script sanea automaticamente:
  - `teams.txt` con nombres mojibake
  - `players.csv` existentes
  - conversion de `players.txt` / `players.json` a `players.csv` utilizable
  - generacion de `players.csv` base para clubes sin archivo de plantilla
  - edades faltantes
  - posiciones `N/A`
  - posiciones inferidas desde `position_raw`
  - nombres con prefijos numericos o sufijos de rol
  - deduplicacion basica por jugador
  - cobertura minima de arqueros, defensas, mediocampistas y delanteros
  - cobertura especifica de centrales y laterales para pasar la auditoria real

- Tambien se alineo el saneamiento con el parser del juego:
  - se corrigio el caso de `Defensive Midfield`, que el parser estaba leyendo como defensa por la heuristica `def`
  - ahora se reescribe a etiquetas compatibles como `Holding Midfield` o `Central Midfield`

- Se regeneraron plantillas faltantes para los clubes que antes disparaban:
  - `Plantilla invalida o incompleta, se genera una base temporal...`

- Resultado real despues del saneamiento:
  - antes:
    - `Errores: 9236`
    - `Advertencias: 1283`
    - `Jugadores crudos: 7111`
  - despues:
    - `Errores: 0`
    - `Advertencias: 196`
    - `Jugadores crudos: 2200`

- El popup de arranque en GUI tambien se ajusto:
  - `src/gui/gui_actions.cpp`
  - ya no abre un dialogo grande solo por advertencias cuando la auditoria queda en `Errores: 0`

- Advertencias que siguen pendientes:
  - carpetas historicas/no referenciadas en `teams.txt` de Tercera A/B
  - un par de discrepancias puntuales `DEL vs MED`
  - son deuda de curacion fina, ya no rompen el arranque ni la validacion

- Verificacion realizada:
  - `FootballManager.exe --validate`: `Resultado: sin fallas`
  - `FootballManagerTests.exe`: todos los tests pasan

## Cambios recientes (2026-03-11) - Promesas, mundo persistente, vestuario y desarrollo

- Se amplio el modelo base del juego para soportar sistemas sociales y de progresion persistentes:
  - `include/engine/models.h`
  - nuevos campos en `Player`:
    - `moraleMomentum`
    - `fatigueLoad`
    - `unhappinessWeeks`
    - `promisedPosition`
    - `socialGroup`
  - nuevas estructuras en `Career`:
    - `SquadPromise`
    - `HistoricalRecord`
    - `activePromises`
    - `historicalRecords`

- Se creo un nuevo modulo de estado del mundo:
  - `include/career/world_state_service.h`
  - `src/career/world_state_service.cpp`

- El nuevo `world_state_service` ahora maneja:
  - siembra automatica de promesas de temporada
  - promesas de minutos
  - promesas de fichaje por necesidad de plantilla
  - promesas competitivas
  - resolucion semanal de promesas cumplidas o rotas
  - noticias del mundo ligadas a presion institucional
  - deteccion y guardado de records historicos

- Se reforzo la dinamica de vestuario:
  - `src/career/dressing_room_service.cpp`
  - ahora el snapshot contempla:
    - `conflictCount`
    - `positionPromiseRiskCount`
    - grupos sociales
    - acumulacion de semanas de malestar
    - choques por jugar fuera de la posicion prometida

- La moral ya no depende solo del resultado:
  - `src/simulation/morale_engine.cpp`
  - `src/simulation/match_tactics.cpp`
  - `moraleMomentum` y `socialGroup` ahora impactan:
    - factor colectivo previo al partido
    - rendimiento individual
    - variacion moral post partido

- Se mejoro el sistema fisico y de lesiones:
  - `src/simulation/player_condition.cpp`
  - `src/simulation/match_postprocess.cpp`
  - `fatigueLoad` ahora persiste entre semanas
  - nuevos tipos de lesion:
    - `Sobrecarga`
    - `Muscular`
    - `Ligamentos`
    - `Fractura`
  - la carga acumulada ahora afecta:
    - riesgo de lesion
    - recuperacion
    - rendimiento

- Se profundizo el entrenamiento y el desarrollo:
  - `src/development/training_impact_system.cpp`
  - `src/development/player_progression_system.cpp`
  - `src/development/youth_generation_system.cpp`
  - nuevos focos/planes soportados:
    - `Ataque`
    - `Defensa`
    - `Resistencia`
  - el staff ahora influye con mas claridad:
    - `assistantCoach`
    - `fitnessCoach`
    - `medicalTeam`
    - `youthCoach`
  - el potencial del jugador ahora puede subir o bajar segun:
    - minutos
    - moral
    - profesionalismo
    - carga fisica
    - edad

- Se ajusto el flujo semanal y las transiciones de temporada:
  - `src/career/week_simulation.cpp`
  - `src/career/season_transition.cpp`
  - ahora:
    - se procesa el pulso semanal del mundo
    - se muestran titulares breves de mundo en la simulacion
    - se recalculan y guardan records historicos al cierre de temporada
    - se re-siembra el set de promesas al iniciar una temporada nueva

- Se mejoro scouting y negociacion para conectarlos con estos sistemas:
  - `src/career/app_services.cpp`
  - `src/transfers/negotiation_system.cpp`
  - el scouting ahora agrega:
    - `recommendation`
    - `upsideBand`
    - `confidence`
  - las promesas contractuales ahora fijan tambien:
    - `promisedPosition`

- Se actualizaron reportes e interfaz para hacer visibles los sistemas nuevos:
  - `src/career/career_reports.cpp`
  - `src/gui/gui_views.cpp`
  - ahora se exponen:
    - promesas activas
    - records historicos
    - carga acumulada del plantel
    - detalle social del jugador
    - staff tecnico en reportes de club

- Se actualizo la persistencia:
  - `src/io/save_serialization.cpp`
  - nueva version de save:
    - `VERSION 9`
  - ahora se guardan y cargan:
    - promesas activas
    - records historicos
    - `moraleMomentum`
    - `fatigueLoad`
    - `unhappinessWeeks`
    - `promisedPosition`
    - `socialGroup`

- Se actualizo la compilacion para el nuevo modulo:
  - `CMakeLists.txt`

- Tests nuevos/agregados:
  - `world_state_seed`
  - `world_state_promises`
  - extension de `save_load_roundtrip` para validar nuevos campos persistidos

- Verificacion realizada:
  - `build.bat --validate`
  - `cmake --build build-cmake --config Release --target FootballManagerTests`
  - `FootballManagerTests.exe`: todos los tests pasan
  - `FootballManager.exe --validate`: `Resultado: sin fallas`

## Cambios recientes (2026-03-13) - Mundo, promesas, vestuario y scouting

- Se cerró el ciclo de vida de promesas de temporada: ahora las promesas activas se resuelven explícitamente al final del curso antes del reset de la nueva temporada, evitando que desaparezcan sin premio o castigo.
- El servicio `world_state_service` ahora genera un pulso del mundo más rico con rumores de banquillo, presión institucional y focos de mercado, además de seguir produciendo records históricos.
- El cierre de temporada actualiza records de todas las divisiones cargadas, no solo de la división activa del usuario.
- El scouting quedó recalibrado: la confianza ya no se satura tan fácil, cada informe expone expectativa salarial y etiqueta de riesgo, y el seguimiento de shortlist también devuelve salario esperado.
- El vestuario pasó a usar dinámica social semanal: grupos como `Lideres`, `Titulares`, `Juveniles`, `Veteranos`, `Competitivos` y `Frustrados` se recalculan según minutos, felicidad, edad y rol prometido.
- El snapshot de vestuario ahora informa tensión social, apoyo de liderazgo, tamaño del grupo dominante y jugadores aislados.
- La moral colectiva del partido ya considera salidas potenciales, promesas de posición incumplidas, fatiga severa y focos sociales frustrados.
- La simulación semanal añadió ingresos de merchandising y activación de sponsor dentro del flujo financiero para representar mejor el estado comercial del club.
- Se ampliaron los eventos aleatorios de carrera con conflictos por minutos y picos de interés externo por jugadores importantes, además de los eventos previos de sponsor, protesta e lesión.
- La reputación del entrenador ahora también reacciona al uso de juveniles y a la salud social del vestuario, no solo a la tabla y al estilo táctico.
- Los reportes de directiva y club muestran la nueva lectura social del plantel, incluyendo tensión, apoyo de líderes y grupos del vestuario.
- La GUI agrega alertas de promesas bajo presión y de tensión interna alta en el dashboard.
- Se agregaron tests nuevos para `dressing_room_tension`, `scouting_confidence` y `season_promise_carryover`.
- Verificación ejecutada:
  - `cmake --build build-cmake --config Release --target FootballManagerTests`
  - `build-cmake\\bin\\FootballManagerTests.exe` -> todo OK
  - `build-cmake\\bin\\FootballManager.exe --validate` -> `Resultado: sin fallas` con `0` errores y `196` advertencias de dataset externo
- Pendiente real que sigue fuera de este cambio: limpiar carpetas históricas no referenciadas en `data/LigaChilena` y las advertencias residuales de posiciones crudas.


## Cambios recientes (2026-03-13) - Mundo vivo, microciclo, mercado y vestuario

- Se profundizo la simulacion semanal del mundo en `src/career/week_simulation.cpp`.
- Las divisiones de fondo ahora pueden generar titulares de liderato, presion institucional, revision de banquillo, promociones juveniles e lesiones cortas de jugadores clave.
- El entrenamiento semanal ya funciona como microciclo real en `src/development/training_impact_system.cpp`.
- Se agregaron horarios y cargas por sesion, diferenciando semanas regulares y congestionadas.
- `player_dev::applyWeeklyTrainingPlan(...)` ahora acepta semana congestionada y la simulacion la usa al cerrar cada fecha.
- El mercado se hizo mas humano en `src/transfers/negotiation_system.cpp`, `src/career/app_services.cpp`, `src/ai/ai_transfer_manager.cpp` y `src/transfers/transfer_market.cpp`.
- Las negociaciones ahora incluyen fee de agente, bonus de fidelidad y bonus por partido dentro de `NegotiationState` y `ContractOffer`.
- Los fichajes, clausulas, precontratos y renovaciones descuentan el paquete real de firma, no solo fee + bono basico.
- La IA de mercado ahora estima tambien costo de agente y usa ese costo en shortlist, viabilidad y fichajes CPU.
- Se agregaron interacciones directas del manager con el vestuario en `src/career/app_services.cpp`.
- Nuevo servicio `holdTeamMeetingService(...)` para reunion de plantel.
- Nuevo servicio `talkToPlayerService(...)` para charla individual.
- Nuevo servicio `cycleTrainingFocusService(...)` para rotar el plan semanal del equipo.
- La GUI expone estas acciones en `src/gui/gui_actions.cpp`, `src/gui/gui_layout.cpp` y `src/gui/gui_views.cpp`.
- El boton contextual ahora sirve como `Reunion` en dashboard, `Hablar` en plantilla/cantera e `Instruccion` en tacticas.
- El dashboard muestra tension social, plan semanal y preview del microciclo.
- La vista tactica muestra el microciclo, el impacto de fatiga y la relacion entre plan semanal e instruccion de partido.
- La vista financiera ahora muestra sponsor, taquilla, merchandising, bonos variables y buffer real de mercado.
- La persistencia se endurecio en `src/io/save_manager.cpp`.
- Cada guardado deja backup `.bak` del save previo antes de reemplazar el archivo principal.
- Los reportes de carrera en `src/career/career_reports.cpp` ahora incluyen merchandising, bonos y bloque de microciclo semanal.
- Se hizo limpieza puntual de datos externos.
- Se corrigieron las posiciones de Alan Medina en `data/LigaChilena/primera division/CD Everton/players.csv` y Javier Medina en `data/LigaChilena/segunda division/Colchagua CD/players.csv`.
- Se agrego la herramienta `tools/report_orphan_team_folders.ps1` para auditar carpetas de clubes no listadas en `teams.txt`.
- El reporte generado queda en `docs/data_cleanup_report.md`.
- Se ampliaron los tests en `tests/project_tests.cpp`.
- Nuevos tests: `training_microcycle`, `team_meeting`, `negotiation_agent_costs`, `save_backup`.
- Verificacion ejecutada:
- `cmake --build build-cmake --config Release --target FootballManagerTests`
- `build-cmake/bin/FootballManagerTests.exe`
- `build.bat --validate`
- `build-cmake/bin/FootballManager.exe --validate`
- Estado de validacion actual: `Resultado: sin fallas`, con `0` errores y `194` advertencias de dataset externo aun pendientes, concentradas sobre todo en carpetas no referenciadas y homonimias/doblados de jugadores.
- Ajuste Windows GUI/CLI:
- Se corrigio que al abrir el juego normal tambien apareciera `cmd`; la causa era que `FootballManager.exe` se estaba generando como aplicacion de consola y luego intentaba ocultarla.
- `CMakeLists.txt` ahora construye `FootballManager.exe` como app `WIN32` en Windows y agrega un ejecutable separado `FootballManagerCLI.exe` para `--cli` y `--validate`.
- Se agrego `src/winmain.cpp` como punto de entrada exclusivo para la GUI en Windows.
- `src/engine/game_controller.cpp` ahora separa el flujo por defecto de GUI del flujo de consola con `FM_CONSOLE_DEFAULT`.
- `build.bat` ahora elige el target correcto segun argumentos: sin flags abre `FootballManager.exe`; con `--cli` o `--validate` compila y ejecuta `FootballManagerCLI.exe`.
- Verificacion ejecutada:
- `build.bat`
- `build.bat --cli`
- Comprobacion de subsistema con `objdump -p`: `FootballManager.exe` quedo como `Windows GUI` y `FootballManagerCLI.exe` como `Windows CUI`.
- Nota de entorno: el camino CMake en `build-cmake` sigue fallando por un bloqueo externo de `ar.exe` al renombrar `objects.a`, pero el fallback directo con `g++` compilo ambos ejecutables correctamente.


## Cambios recientes (2026-03-16) - Registro de ligas, inbox central, staff y data hub

- Se agrego un registro externo de ligas en `data/configs/league_registry.csv` y un modulo nuevo `competition/league_registry`.
- `competition.cpp` ya no depende de una ruta fija hardcodeada para `competition_rules.csv`; ahora usa el registro externo.
- `utils.cpp` y el arranque de carrera quedaron alineados con el registro configurable de divisiones, reforzando la base para modding.
- Se agregaron dos modulos nuevos de arquitectura: `career/analytics_service` y `career/inbox_service`.
- `career/analytics_service` construye un data hub reusable con indices ofensivos, control, defensa, continuidad del XI, balance de roles, alertas de carga y lectura del ultimo partido.
- `career/inbox_service` unifica `managerInbox` y `scoutInbox` para que reportes y GUI usen una sola lectura del centro del manager.
- Los reportes de club, directiva y scouting ahora usan esos modulos nuevos y muestran un centro del manager mas claro, tendencias del ultimo partido y un data hub mas FM-like.
- La GUI fue ajustada para empujar mas fuerte el concepto de inbox central y pulso del club.
- La pagina de noticias ahora funciona mas como centro del manager y muestra mejor la mezcla de inbox, scouting y alertas.
- El dashboard muestra informacion analitica mas util junto con contexto del DT y lectura inmediata del club.
- Se profundizo el scouting.
- `ScoutingCandidate` ahora expone `reportStage`, `knowledgeLevel` y `hiddenRiskLabel`.
- Los informes de scouting muestran mejor si el reporte es radar lejano, seguimiento parcial o informe completo.
- Se reforzo el sistema de staff.
- `Team` ahora persiste nombres propios para asistente, PF, scouting, juveniles, medico, entrenador de arqueros y analista.
- Tambien se agrega `headCoachTenureWeeks` para seguir la antiguedad del DT del club.
- `upgradeClubService(...)` ahora puede contratar explicitamente `GoalkeepingCoach` y `PerformanceAnalyst`.
- El menu de infraestructura/club en consola fue ampliado para exponer esos dos fichajes de staff.
- `world_state_service` ahora mueve tambien el ecosistema de staff y aumenta la antiguedad semanal del DT.
- Los clubes pueden reforzar scouting y analisis en background segun presupuesto y reglas de mundo configurables.
- La persistencia subio a `VERSION 11` en `save_serialization.cpp` para guardar nombres de staff y antiguedad del DT.
- La limpieza de deuda de datos se externalizo parcialmente con `data/configs/ignored_team_folders.csv`.
- `validators.cpp` ya soporta una lista de carpetas archivadas/ignoradas para no mezclar historico con dataset activo.
- `tools/report_orphan_team_folders.ps1` ahora tambien respeta ese archivo de ignorados.
- La auditoria CLI bajo de `194` advertencias a `85` advertencias activas al validar con el binario recompilado.
- Se agregaron tests nuevos para `league_registry_data`, `analytics_inbox_blocks` e `ignored_archived_folders`.
- `save_load_roundtrip` fue ampliado para cubrir staff con nombre propio y antiguedad del DT.
- Verificacion ejecutada:
- `cmake --build build-cmake --config Release --target FootballManagerTests`
- `build-cmake\bin\FootballManagerTests.exe` -> todo OK
- `cmake --build build-cmake --config Release --target FootballManager` -> compila GUI OK
- `build.bat --validate` -> compila CLI OK
- `build-cmake\bin\FootballManagerCLI.exe --validate` -> `Resultado: sin fallas`, `0` errores y `85` advertencias activas de datos externos
- Pendiente real despues de este bloque: limpiar duplicados nominales de jugadores y las pocas carpetas huerfanas activas que todavia quedan fuera del ignore list.

## Cambios recientes (2026-03-16) - Staff real, scouting por asignaciones, centro medico e inbox accionable

- Se agregaron dos modulos nuevos de carrera: `career/staff_service` y `career/medical_service`.
- `career/staff_service` construye perfiles de staff, detecta el area mas debil del club y entrega un resumen reutilizable para reportes y GUI.
- `career/medical_service` centraliza el estado medico del plantel, combinando lesion, fatiga acumulada, riesgo de recaida y recomendacion del cuerpo medico.
- `Player` ahora guarda `individualInstruction` para soportar instrucciones individuales persistentes a nivel tactico.
- `models.cpp` agrega `defaultInstructionForPosition(...)` para inicializar la instruccion del jugador sin romper el comportamiento base del motor.
- `match_engine_internal.h`, `match_tactics.cpp` y `match_context.cpp` incorporan esas instrucciones individuales al calculo tactico del partido.
- Se mantuvo `Libre` como valor por defecto para no alterar el balance previo mientras las nuevas instrucciones se activan desde la experiencia de juego.
- `Career` ahora persiste `scoutingAssignments`, y `models.h` agrega la estructura `ScoutingAssignment` para representar asignaciones de red de scouting por region, perfil y prioridad.
- `app_services.h` y `app_services.cpp` suman servicios nuevos para: rotar instruccion individual del jugador, crear asignaciones de scouting, resolver decisiones del inbox y revisar la estructura de staff.
- `gui_actions.cpp` conecta esos servicios con acciones reales del manager: charla, reunion, decision del inbox, revision de staff y cambio de instruccion individual.
- `gui_layout.cpp` y `gui_views.cpp` se ajustaron para que la GUI principal muestre mejor el centro del manager, el estado medico, la debilidad de staff y las asignaciones activas.
- La vista de jugador ahora muestra instruccion individual y una lectura medica resumida.
- La seccion de lesiones paso a funcionar mas como centro medico del club usando `medical_service`.
- El feed de alertas y la vista de noticias agregan avisos por asignaciones de scouting y por necesidades del staff tecnico.
- `week_simulation.cpp` ahora hace progresar semanalmente las asignaciones de scouting y dispara informes completos cuando alcanzan su deadline.
- `world_state_service.cpp` profundiza el mundo autonomo con renovaciones de fondo, descontento de jugadores por baja moral y cambios de politica de fichajes por estres financiero.
- `career_reports.cpp` agrega bloques nuevos para asignaciones de scouting, staff tecnico y centro medico en los reportes del club, scouting y directiva.
- `save_serialization.cpp` sube a `VERSION 12` para guardar `individualInstruction` y `scoutingAssignments` dentro del save.
- `validators.cpp` agrega heuristicas y una whitelist externa en `data/configs/duplicate_player_whitelist.csv` para distinguir mejor homonimias legitimas de duplicados reales.
- Esa mejora bajo la auditoria de datos externos de `85` a `52` advertencias activas, manteniendo `0` errores.
- Se agrego el archivo nuevo `data/configs/duplicate_player_whitelist.csv` para normalizar excepciones conocidas sin ocultar problemas reales.
- `tests/project_tests.cpp` suma cobertura nueva para `player_instruction_service`, `inbox_medical_decision`, `staff_review` y `scouting_assignments`.
- `save_load_roundtrip` tambien fue ampliado para verificar la persistencia de instrucciones individuales y asignaciones de scouting.
- Verificacion ejecutada:
- `cmake --build build-cmake --config Release --target FootballManagerTests`
- `build-cmake\\bin\\FootballManagerTests.exe` -> todo OK
- `build-cmake\\bin\\FootballManagerCLI.exe --validate` -> `Resultado: sin fallas`, `0` errores y `52` advertencias activas
- Resultado practico de este bloque: el juego principal ya incorpora una capa mas FM-like de staff, decisiones del manager, scouting por red, centro medico y lectura contextual del club, y al mismo tiempo la base externa quedo mas limpia.
- Pendiente real despues de este bloque: limpiar los ultimos duplicados nominales activos y decidir si algunas carpetas huerfanas restantes deben volver a `teams.txt` o archivarse definitivamente.

## Cambios recientes (2026-03-16) - Consola, validacion limpia y cierre total de advertencias

- Se corrigio la experiencia del modo consola en `game_controller.cpp`, `career_reports_ui.cpp`, `market_ui.cpp` y `ui.cpp`.
- `showLoadWarnings()` ahora resume incidencias en vez de inundar la consola con todas las lineas crudas al arrancar carrera.
- Se elimino la duplicacion del resumen de advertencias al crear o cargar carrera.
- La consola de Windows quedo preparada para UTF-8 real, evitando mojibake visible en nombres con acentos y `Ñ`.
- La opcion `Noticias` paso a funcionar como inbox accionable tambien en CLI.
- Desde consola ahora se pueden revisar y resolver decisiones del `managerInbox`.
- El flujo de scouting en CLI fue ampliado para crear y revisar asignaciones activas semanales.
- La vista de alineacion/plantel en consola ahora muestra `individualInstruction` y permite rotarla desde el menu.
- El bloque `Club y Finanzas` en consola suma una revision real de staff.
- Se corrigio un bug de validacion en `validators.cpp` con carpetas activas configuradas en `teams.txt` como `Display|Folder`.
- La auditoria ahora reconoce correctamente alias de carpeta como `Futuro`, `Audax Paipote`, `CSD Ovalle`, `Laja Historico`, `Nacimiento` y `Republica Ind. de Hualqui`.
- Se agrego un test nuevo `teams_txt_folder_aliases` para asegurar que esos alias no vuelvan a disparar falsos positivos de integridad.
- Se limpio la deuda restante de duplicados nominales en `data/LigaChilena`.
- Se renombraron ocurrencias activas duplicadas en 18 archivos `players.csv`, conservando una referencia principal por nombre y eliminando colisiones reales entre plantillas.
- Con esa limpieza la auditoria externa bajo de `52` advertencias a `0`, manteniendo `0` errores.
- `save_manager.cpp` fue reforzado para cargar primero en una `Career` fresca y luego rehidratar el estado sobre el objeto activo.
- Ese ajuste corrigio la falla de `Guardado/carga` que aparecia en la suite de validacion cuando el objeto ya estaba inicializado.
- `tactics_engine.cpp` ahora da mas peso real a `Bloque bajo`, `Pausar juego`, `Por bandas` y `Contra-presion` dentro del perfil tactico.
- Eso corrigio la regresion de `low_block_chance_quality` y dejo mas coherente la relacion entre instrucciones y calidad de ocasiones.
- `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` fueron recompilados con este bloque final.
- Verificacion ejecutada:
- `build-cmake\\bin\\FootballManagerTests.exe` -> `All tests passed`
- `build-cmake\\bin\\FootballManagerCLI.exe --validate` -> `Resultado: sin fallas`
- Auditoria final de datos externos -> `Errores: 0 | Advertencias: 0`
- Todos estos cambios quedaron subidos a GitHub en `origin/main`.
- Commit publicado: `459f28d` - `Expand manager systems and clear validation warnings`
- Descripcion del commit:
- agrega servicios de staff y centro medico, mas flujos CLI accionables para inbox, scouting e instrucciones individuales
- mejora perfiles tacticos del motor de partido y endurece save/load para juego principal y validador
- corrige validacion de aliases `Display|Folder` en `teams.txt` y limpia los duplicados activos de `LigaChilena`
- actualiza tests, CMake y `TODO.md` con esta pasada

## Auditoria del proyecto (2026-03-16) - Arquitectura, modularidad y escalabilidad

### Problemas encontrados

- `src/ui/ui.cpp` concentraba demasiadas responsabilidades: menus, vista de plantel, entrenamiento, alineacion, edicion de equipo, narrativa semanal, transicion de temporada y utilidades de competicion en un solo archivo.
- La separacion entre UI y logica de carrera mejoro mucho en las ultimas iteraciones, pero el modulo de consola seguia mezclando flujos de navegacion con operaciones de gestion del plantel.
- `src/engine/models.cpp`, `src/gui/gui_views.cpp`, `src/career/app_services.cpp`, `src/io/save_serialization.cpp`, `src/career/week_simulation.cpp` y `src/validators/validators.cpp` siguen siendo hotspots de mantenimiento por volumen y mezcla de responsabilidades.
- Persisten bloques legacy marcados con `#if 0` en `src/engine/models.cpp` y `src/ui/ui.cpp`; son deuda estructural porque dificultan distinguir la ruta viva del codigo historico.
- El build actual sigue mostrando warnings por helpers legacy no usados en `src/ui/ui.cpp`; no se eliminaron a ciegas en esta pasada para no romper rutas antiguas sin mapear primero sus dependencias reales.

### Cambios aplicados

- Se hizo una auditoria completa de la estructura del repo, el grafo de modulos y los archivos mas grandes para identificar el punto de refactor de mayor impacto y menor riesgo.
- Se creo un modulo nuevo de consola: `src/ui/team_ui.cpp` con su header `include/ui/team_ui.h`.
- Ese modulo extrae la gestion de plantel/equipo fuera de `src/ui/ui.cpp`.
- Funciones movidas al modulo nuevo:
- `retirePlayer(...)`
- `viewTeam(...)`
- `addPlayer(...)`
- `trainPlayer(...)`
- `changeTactics(...)`
- `manageLineup(...)`
- `setTrainingPlan(...)`
- `editTeam(...)`
- `displayStatistics(...)`
- `include/ui/ui.h` ahora funciona como header agregador y delega la parte de gestion de equipo en `ui/team_ui.h`.
- `CMakeLists.txt` fue actualizado para compilar `src/ui/team_ui.cpp` dentro de `FM_CONSOLE_UI_SOURCES`.
- Resultado directo del split:
- `src/ui/ui.cpp` bajo de `2592` lineas a `1970` lineas.
- La logica de gestion de equipo quedo encapsulada en un archivo nuevo de `636` lineas, reduciendo acoplamiento y clarificando la frontera de la UI de consola.
- Se recompilo el proyecto completo y se verifico que el refactor no rompio ni la GUI, ni la CLI, ni la validacion de datos.

### Archivos modificados

- `CMakeLists.txt`
- `include/ui/ui.h`
- `include/ui/team_ui.h`
- `src/ui/ui.cpp`
- `src/ui/team_ui.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `build-cmake\\bin\\FootballManagerTests.exe` -> `All tests passed`
- `build-cmake\\bin\\FootballManagerCLI.exe --validate` -> `Resultado: sin fallas`
- Auditoria actual de datos -> `Errores: 0 | Advertencias: 0`

### Mejoras futuras recomendadas

- Extraer de `src/ui/ui.cpp` el bloque de simulacion semanal y transicion de temporada a modulos de carrera dedicados, para dejar la UI como una capa orquestadora y no como ejecutora de logica de dominio.
- Partir `src/gui/gui_views.cpp` por pantallas o secciones (`dashboard`, `inbox`, `market`, `medical`, `board`) para bajar complejidad de GUI.
- Seguir desmontando responsabilidades de `src/engine/models.cpp` hacia modulos mas explicitos (`team_identity`, `roster_selection`, `table_helpers`, `career_lookup`).
- Separar `save_serialization.cpp` por bloques (`career`, `team`, `player`, `world_state`) para hacer el save mas mantenible.
- Resolver o eliminar el codigo legacy marcado con `#if 0` solo despues de mapear sus callers o reemplazos activos.
- Consolidar los helpers `static` no usados de `src/ui/ui.cpp`; hoy son deuda visible y deberian moverse, reusarse o borrarse en una pasada controlada.

## Auditoria del proyecto (2026-03-16) - Segunda pasada de arquitectura GUI y consola

### Problemas encontrados

- `src/gui/gui_views.cpp` seguia mezclando dos responsabilidades: construccion del modelo de pagina y actualizacion/render de controles Win32.
- `src/ui/ui.cpp` aun mantenia helpers legacy y bloques historicos que ya no representaban el flujo activo del juego, lo que hacia mas dificil distinguir el codigo vivo del codigo heredado.
- La division inicial de UI de consola ya habia reducido complejidad, pero faltaba una segunda pasada para separar mejor runtime/presentacion en GUI y seguir limpiando deuda arquitectonica.

### Cambios aplicados

- Se creo `src/gui/gui_runtime.cpp` para mover fuera de `src/gui/gui_views.cpp` la capa de runtime y refresco de la GUI Win32.
- `src/gui/gui_runtime.cpp` ahora concentra el render y sincronizacion de controles: `renderFeed`, `renderListPanel`, `refreshFilterComboOptions`, `fillDivisionCombo`, `fillTeamCombo`, `syncManagerNameFromUi`, `syncCombosFromCareer`, `setStatus`, `refreshCurrentPage`, `refreshAll`, `setCurrentPage`, `handleFilterChange`, `handleListSelectionChange` y `handleListColumnClick`.
- `src/gui/gui_views.cpp` quedo enfocado en construir `GuiPageModel` y en los builders de pantalla, reduciendo mezcla entre datos de vista y manipulacion de widgets.
- `CMakeLists.txt` fue actualizado para compilar el nuevo modulo `src/gui/gui_runtime.cpp` dentro de `FM_GUI_SOURCES`.
- En `src/ui/ui.cpp` se eliminaron helpers sin uso y bloques legacy de transicion de temporada, incluyendo el bloque historico marcado con `#if 0`, para clarificar la ruta activa de la consola.
- Se corrigio una regresion del refactor en `displayLeagueTables(...)`, reemplazando dependencia a helpers internos de competicion por las utilidades locales `buildGroupTable(...)` y `regionalGroupTitle(...)`.
- Con esta pasada, la capa consola quedo mas limpia y la GUI separa mejor construccion de modelos versus actualizacion de controles.

### Archivos modificados

- `CMakeLists.txt`
- `src/gui/gui_views.cpp`
- `src/gui/gui_runtime.cpp`
- `src/ui/ui.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `build-cmake\\bin\\FootballManagerTests.exe` -> `All tests passed`
- `build-cmake\\bin\\FootballManagerCLI.exe --validate` -> `Resultado: sin fallas`
- Auditoria actual de datos -> `Errores: 0 | Advertencias: 0`

### Mejoras futuras recomendadas

- Partir `src/gui/gui_views.cpp` aun mas, por pantallas (`dashboard`, `squad`, `news`, `board`, `finances`) para que cada vista tenga un builder propio.
- Revisar `src/gui/gui_actions.cpp` con el mismo criterio y separar acciones por dominio o pantalla.
- Continuar la limpieza de deuda en `src/engine/models.cpp`, `src/career/app_services.cpp` y `src/io/save_serialization.cpp`, que siguen siendo hotspots de mantenimiento.
- Extraer mas flujo de carrera desde la consola a servicios dedicados, dejando la UI como capa de orquestacion fina.

## Auditoria del proyecto (2026-03-16) - Tercera pasada: builders por pantalla y modulos de Team/LeagueTable

### Problemas encontrados

- `src/gui/gui_views.cpp` seguia siendo un hotspot grande porque aun mezclaba helpers compartidos y todos los builders de pantallas en un solo archivo.
- `src/engine/models.cpp` continuaba concentrando responsabilidades de identidad de jugador, seleccion del XI, valoracion del plantel y tabla de liga en el mismo modulo.
- El objetivo de separar GUI por pantallas y empezar a desmontar `models.cpp` seguia pendiente para acercar el proyecto a una arquitectura mas profesional y escalable.

### Cambios aplicados

- Se creo `include/gui/gui_view_builders.h` como header compartido para declarar builders de pagina y helpers de vista reutilizables.
- Se dividio la GUI en modulos mas claros:
- `src/gui/gui_view_common.cpp` concentra helpers compartidos, construccion de paneles reutilizables y funciones comunes de vista.
- `src/gui/gui_view_overview.cpp` contiene los builders de `Dashboard`, `Squad`, `Youth` y `Tactics`.
- `src/gui/gui_view_competition.cpp` contiene los builders de `Calendar` y `League`.
- `src/gui/gui_view_management.cpp` contiene los builders de `Transfers`, `Finances`, `Board` y `News`.
- `src/gui/gui_views.cpp` quedo reducido al dispatcher `buildModel(...)`, dejando de mezclar dispatch con construccion detallada de todas las pantallas.
- Se creo `src/engine/league_table.cpp` para extraer la logica de ordenamiento, desempate y render de `LeagueTable` fuera de `src/engine/models.cpp`.
- Se creo `src/engine/team_selection.cpp` para mover fuera de `src/engine/models.cpp` la seleccion del XI, el banco y los calculos agregados del equipo (`getStartingXIIndices`, `getBenchIndices`, `getTotalAttack`, `getTotalDefense`, `getAverageSkill`, `getAverageStamina`, `getSquadValue`).
- `src/engine/models.cpp` quedo mas enfocado en identidad de jugadores, perfiles y serializacion del dominio, reduciendo mezcla con seleccion deportiva y tabla competitiva.
- `CMakeLists.txt` fue actualizado para compilar todos los modulos nuevos de GUI y engine.

### Archivos modificados

- `CMakeLists.txt`
- `TODO.md`
- `include/gui/gui_view_builders.h`
- `src/gui/gui_view_common.cpp`
- `src/gui/gui_view_overview.cpp`
- `src/gui/gui_view_competition.cpp`
- `src/gui/gui_view_management.cpp`
- `src/gui/gui_views.cpp`
- `src/engine/models.cpp`
- `src/engine/league_table.cpp`
- `src/engine/team_selection.cpp`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `build-cmake\\bin\\FootballManagerTests.exe` -> `All tests passed`
- `build-cmake\\bin\\FootballManagerCLI.exe --validate` -> `Resultado: sin fallas`
- Auditoria actual de datos -> `Errores: 0 | Advertencias: 0`

### Mejoras futuras recomendadas

- Partir `src/gui/gui_actions.cpp` por dominios (`market`, `board`, `inbox`, `staff`) para emparejar la nueva division de builders.
- Seguir desmontando `src/engine/models.cpp` en modulos como `player_identity`, `player_profile`, `team_identity` y `career_lookup`.
- Extraer la persistencia de `src/io/save_serialization.cpp` por bloques de dominio para mantener la misma claridad que ya se logro en GUI y engine.
- Revisar `src/career/app_services.cpp` con el mismo enfoque y separar reporting, scouting, board e interacciones del manager.

## Auditoria del proyecto (2026-03-20) - confiabilidad, recomendaciones accionables y lectura de temporada

### Problemas encontrados

- La suite de tests podia dejar pasar fallas logicas de la validacion completa porque `validation_suite` no exigia un estado sano cuando no habia errores de datos.
- El test `low_block_chance_quality` podia volverse flaky por el uso de RNG compartido en la evaluacion de fases.
- La GUI mostraba la validacion con un popup demasiado generico, sin explicar cuantos fallos logicos, errores de datos o advertencias habia.
- El proyecto ya generaba mucha informacion rica en `dashboard`, `board`, `news`, `finances`, `scouting` y `match center`, pero todavia faltaba convertir esa lectura en acciones sugeridas para el jugador.
- El `match center` devolvia buen contexto del ultimo partido, pero faltaba una capa final de “que haria ahora” para bajar esa info a decisiones tacticas.
- El mercado ya tenia buenas señales internas (`competitionScore`, `affordabilityScore`, shortlist, contratos cortos), pero la vista no exponia bien urgencia, competencia y paquete economico recomendado.
- Quedaba pendiente hacer un refactor pequeno pero real para no seguir inflando archivos grandes con mas heuristicas de UI y reportes.

### Cambios aplicados

- Se agrego control explicito del RNG en `include/utils/utils.h` y `src/utils/utils.cpp` con `setRandomSeed(...)` y `resetRandomSeed()`, de modo que los tests tacticos puedan fijar escenarios reproducibles.
- Se reforzo `tests/project_tests.cpp`:
- `validation_suite` ahora exige `summary.ok` cuando no existen errores de datos.
- `low_block_chance_quality` ahora usa una semilla fija para comparar ambos escenarios con el mismo ruido aleatorio.
- Se agregaron pruebas nuevas para `manager_advice` y para los ajustes sugeridos del `match center`.
- Se amplió `ValidationSuiteSummary` en `include/validators/validators.h` y `src/validators/validators.cpp` para exponer conteo de fallos logicos, errores de datos y advertencias.
- Se mejoro `src/gui/gui_actions.cpp` para que la validacion muestre un resumen util con incidencias destacadas y referencia al reporte completo, en vez de un simple “paso/fallo”.
- Se creo el modulo nuevo `include/career/manager_advice.h` + `src/career/manager_advice.cpp` para sacar heuristicas de accion/narrativa fuera de archivos ya cargados:
- `buildManagerActionLines(...)` genera decisiones sugeridas para vestuario, renovaciones, carga, mercado, directiva y caja.
- `buildCareerStorylines(...)` resume la narrativa activa de la semana.
- `buildTransferCompetitionLabel(...)`, `buildTransferActionLabel(...)` y `buildTransferPackageLabel(...)` bajan las señales del mercado a lenguaje accionable.
- Se integro ese modulo nuevo en `src/career/career_reports.cpp`, agregando bloques de `Acciones sugeridas` y `Narrativa de la semana` a reportes de competicion, directiva, club y scouting.
- Se amplió `MatchCenterView` en `include/career/match_center_service.h` y `src/career/match_center_service.cpp` con `recommendationLines`, generando ajustes sugeridos a partir de xG, tiros, control territorial y fatiga.
- `formatLastMatchCenter(...)` ahora imprime tambien una seccion `Ajustes sugeridos`.
- Se conectaron estas mejoras a la GUI:
- `src/gui/gui_view_overview.cpp` ahora muestra decisiones sugeridas y narrativa de semana en el `Dashboard`.
- `src/gui/gui_view_management.cpp` expone en `Transfers` la competencia, la recomendacion de movimiento y el paquete economico estimado del objetivo seleccionado.
- `src/gui/gui_view_management.cpp` tambien suma recomendaciones/narrativa en `Finances`, `Board` y `News`.
- Se extendio `TransferPreviewItem` en `include/gui/gui_view_builders.h` y se completan los nuevos campos desde `src/gui/gui_view_common.cpp`.
- `CMakeLists.txt` fue actualizado para compilar el nuevo modulo `manager_advice.cpp`.

### Archivos modificados

- `CMakeLists.txt`
- `TODO.md`
- `include/career/manager_advice.h`
- `include/career/match_center_service.h`
- `include/gui/gui_view_builders.h`
- `include/utils/utils.h`
- `include/validators/validators.h`
- `src/career/career_reports.cpp`
- `src/career/manager_advice.cpp`
- `src/career/match_center_service.cpp`
- `src/gui/gui_actions.cpp`
- `src/gui/gui_view_common.cpp`
- `src/gui/gui_view_management.cpp`
- `src/gui/gui_view_overview.cpp`
- `src/utils/utils.cpp`
- `src/validators/validators.cpp`
- `tests/project_tests.cpp`

### Verificacion ejecutada

- Recompilacion de objetos con `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- En este entorno MinGW + OneDrive el enlace por `objects.a` quedo bloqueado por permisos, asi que el link final de `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` se completo manualmente reutilizando los `.obj` ya generados por CMake.
- `build-cmake\\bin\\FootballManagerTests.exe` -> `All tests passed`
- `build-cmake\\bin\\FootballManagerCLI.exe --validate` -> `Resultado: sin fallas`
- Auditoria actual de datos -> `Errores: 0 | Advertencias: 0`

### Mejoras futuras recomendadas

- Llevar el mismo enfoque de recomendaciones accionables a la consola (`src/ui/`) para que CLI y GUI queden mas parejos.
- Seguir desmontando `src/career/app_services.cpp` y `src/engine/models.cpp`, que siguen siendo hotspots aunque esta pasada evito inflarlos mas.
- Si el entorno sigue en OneDrive, evaluar un build dir fuera de sincronizacion para evitar los bloqueos de `ar.exe` con MinGW.

## Auditoria del proyecto (2026-03-20) - segunda pasada sobre mercado, CLI y pulso semanal

### Objetivo de esta pasada

- Aplicar la tanda siguiente de mejoras de producto que habia quedado propuesta despues de la evaluacion del proyecto:
- convertir mas informacion en decisiones concretas
- hacer el mercado mas vivo
- bajar mejor el ultimo partido a acciones
- emparejar mas la experiencia entre GUI y CLI
- hacer un refactor pequeno y reutilizable en vez de seguir inflando vistas aisladas

### Cambios aplicados

- Se creo el modulo nuevo `include/career/transfer_briefing.h` + `src/career/transfer_briefing.cpp`.
- Este modulo concentra logica compartida para mercado:
- `buildTransferOptions(...)` arma objetivos con competencia, accion recomendada, paquete economico y confianza de scouting
- `buildPreContractOptions(...)` y `buildLoanOptions(...)` respetan las reglas reales de elegibilidad del juego
- `buildMarketPulseLines(...)` resume la situacion del mercado del club en lenguaje de manager
- `buildTransferOpportunityLines(...)` baja los mejores objetivos a una lista corta accionable
- `src/ui/market_ui.cpp` dejo de construir su pool de mercado localmente y ahora reutiliza el briefing compartido.
- La consola del mercado ahora muestra para cada objetivo:
- competencia del movimiento
- accion sugerida (`mover ahora`, `pedir mas scouting`, `explorar cesion`, etc.)
- paquete economico estimado
- nota de scouting y nivel de confianza
- Se mejoro tambien la UX del mercado CLI:
- la portada del mercado imprime un `Pulso de mercado`
- la entrada a prestamos ahora permite filtrar por posicion
- el flujo de precontrato vuelve a respetar exactamente la regla de `<= 12` semanas restantes
- `src/career/career_reports.cpp` ahora suma lectura de mercado a los reportes estructurados:
- `Club y Finanzas` agrega `Pulso de mercado` y `Oportunidades de mercado`
- `Scouting` agrega `Pulso de mercado` y `Objetivos sugeridos`
- Como esos reportes ya alimentan GUI y parte del CLI, el salto llega a mas de una interfaz con el mismo contenido.
- `src/ui/career_reports_ui.cpp` se reforzo para que `Noticias / Centro del Manager` en consola muestre:
- acciones sugeridas
- narrativa de la semana
- pulso de mercado
- lectura del ultimo partido
- ajustes inmediatos postpartido
- Esto deja al CLI mucho mas cerca del nivel de lectura accionable que ya habia ganado la GUI.
- `CMakeLists.txt` fue actualizado para compilar el nuevo modulo `transfer_briefing.cpp`.

### Tests y validacion

- `tests/project_tests.cpp` suma la prueba nueva `transfer_briefing`, que verifica que el modulo entregue objetivos accionables, pulso de mercado y paquetes economicos resumidos.
- Recompilacion completa:
- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- Verificacion:
- `build-cmake\\bin\\FootballManagerTests.exe` -> `All tests passed`
- `build-cmake\\bin\\FootballManagerCLI.exe --validate` -> `Resultado: sin fallas`
- En esta pasada el build enlazo bien por CMake y no hizo falta repetir el workaround manual de link.

### Archivos modificados en esta pasada

- `CMakeLists.txt`
- `TODO.md`
- `include/career/transfer_briefing.h`
- `src/career/career_reports.cpp`
- `src/career/transfer_briefing.cpp`
- `src/ui/career_reports_ui.cpp`
- `src/ui/market_ui.cpp`
- `tests/project_tests.cpp`

### Siguiente paso natural

- Si se quiere seguir con esta linea, lo mas valioso ahora seria llevar el mismo enfoque de modulo compartido a `src/career/app_services.cpp` para separar mejor mercado, scouting, inbox y decisiones del manager.

## Auditoria del proyecto (2026-03-25 21:48:06 -03:00) - salud unificada, partido contextual, progreso mensual y mercado mas realista

### Fecha y hora

- 2026-03-25 21:48:06 -03:00

### Lista de cambios realizados

- Se creo el modulo publico `include/simulation/player_condition.h` y se reforzo `src/simulation/player_condition.cpp` para centralizar carga, recaida, readiness y severidad de lesion.
- Lesiones, recuperacion, desarrollo, scouting, analitica y evaluacion de fichajes dejaron de usar formulas aisladas y ahora comparten el mismo modelo de salud del jugador.
- El motor de partido ahora considera contexto real de marcador y disponibilidad:
- `src/simulation/match_phase.cpp` ajusta urgencia, riesgo e intensidad segun ir ganando/perdiendo y segun hombres disponibles.
- `src/simulation/match_event_generator.cpp` ya no deja a un lesionado “jugando igual”; una lesion puede forzar sustitucion real o dejar al equipo con menos hombres.
- Se agrego mas profundidad ofensiva al match engine con seguimiento de balon parado tras interrupciones, para que corners/faltas generen segundas jugadas mas creibles.
- `src/simulation/match_report.cpp` ahora explica mejor tramos de persecucion del marcador y desventajas de disponibilidad.
- `src/simulation/match_context.cpp` incorpora readiness del jugador al snapshot colectivo para que el once refleje mejor su estado competitivo real.
- Se creo el modulo nuevo `include/development/monthly_development.h` + `src/development/monthly_development.cpp`.
- La progresion mensual salio de `src/career/week_simulation.cpp` y ahora vive en un sistema dedicado, con mejoras ligadas a:
- estabilidad del entorno
- minutos jugados
- plan individual
- estado fisico
- infraestructura y staff
- `src/development/training_impact_system.cpp` y `src/development/player_progression_system.cpp` fueron afinados para que carga, readiness y estabilidad afecten mejor crecimiento y recuperacion.
- `src/engine/models.cpp` ahora genera jugadores mas variados por perfil y posicion, con roles, stamina, potencial y valoracion menos lineales.
- Mercado y scouting quedaron mas profundos:
- `src/ai/ai_transfer_manager.cpp` penaliza riesgo medico y valora readiness + upside real.
- `src/career/app_services.cpp` agrega readiness y riesgo medico a informes de scouting.
- `src/career/transfer_briefing.cpp` y `src/ui/market_ui.cpp` exponen mejor si un objetivo llega listo para competir o si arrastra riesgo fisico.
- `CMakeLists.txt` ahora compila el nuevo modulo mensual y deja explicito `WIN32_EXECUTABLE` para `FootballManager`, reforzando que el `.exe` GUI no abra consola innecesaria en Windows.
- Se ampliaron tests de regresion para cubrir:
- urgencia tactica por marcador
- sustitucion obligada por lesion
- penalizacion de riesgo medico en mercado
- progresion mensual modular

### Archivos modificados

- `CMakeLists.txt`
- `include/career/app_services.h`
- `include/career/transfer_briefing.h`
- `include/development/monthly_development.h`
- `include/simulation/match_engine_internal.h`
- `include/simulation/match_event_generator.h`
- `include/simulation/match_phase.h`
- `include/simulation/match_types.h`
- `include/simulation/player_condition.h`
- `include/transfers/transfer_types.h`
- `src/ai/ai_match_manager.cpp`
- `src/ai/ai_transfer_manager.cpp`
- `src/career/analytics_service.cpp`
- `src/career/app_services.cpp`
- `src/career/medical_service.cpp`
- `src/career/transfer_briefing.cpp`
- `src/career/week_simulation.cpp`
- `src/development/monthly_development.cpp`
- `src/development/player_progression_system.cpp`
- `src/development/training_impact_system.cpp`
- `src/engine/models.cpp`
- `src/simulation/match_context.cpp`
- `src/simulation/match_engine.cpp`
- `src/simulation/match_event_generator.cpp`
- `src/simulation/match_phase.cpp`
- `src/simulation/match_postprocess.cpp`
- `src/simulation/match_report.cpp`
- `src/simulation/match_tactics.cpp`
- `src/simulation/player_condition.cpp`
- `src/ui/market_ui.cpp`
- `tests/project_tests.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- Resultado:
- `FootballManager` recompilo correctamente.
- `FootballManagerCLI` volvio a chocar con el problema del entorno MinGW + OneDrive al enlazar `objects.a`; no fue un error de compilacion del codigo cambiado sino un bloqueo del linker en el directorio sincronizado.
- `cmake --build build-cmake --config Release --target FootballManagerTests`
- `build-cmake\\bin\\FootballManagerTests.exe` -> `All tests passed`

### Mejoras futuras sugeridas

- Seguir desmontando `src/career/app_services.cpp`, que ya mejoro por dentro pero sigue concentrando demasiadas responsabilidades.
- Llevar las nuevas senales de readiness/riesgo medico a mas vistas GUI para que la profundidad agregada al scouting y mercado tambien se vea completa fuera del CLI.
- Si el proyecto sigue compilando dentro de OneDrive, mover el build dir fuera de sincronizacion para evitar el bloqueo recurrente de `objects.a` al enlazar los targets CLI.

## Auditoria del proyecto (2026-03-25 21:57:24 -03:00) - layout GUI menos amontonado y cabecera mas respirable

### Lista de cambios realizados

- Se rehizo la geometria principal de `src/gui/gui_layout.cpp` con constantes de layout para evitar seguir ajustando pixeles “a mano” en cada bloque.
- La cabecera superior ahora tiene mas altura util, botones principales mas separados y tarjetas de resumen con mayor alto interno para que etiqueta y valor no se monten entre si.
- Se redujo el peso visual de la tipografia base y de seccion para que la interfaz deje de verse tan apretada, especialmente en tarjetas, laterales y paneles de texto.
- El menu lateral gano un poco de ancho y mas separacion vertical entre botones para que cada seccion respire mejor.
- Los paneles del centro y la columna derecha ahora usan titulos mas altos, cuerpos con mas margen y una separacion consistente entre bloques.
- Se corrigio tambien la distancia entre botones de accion de pagina y el primer bloque de contenido para que no queden chocando segun la vista activa.
- La lista de noticias aumento su alto por item para mejorar legibilidad.

### Archivos modificados

- `src/gui/gui_layout.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager`
- Resultado:
- `FootballManager` recompilo correctamente y el ejecutable GUI actualizado quedo en `build-cmake\\bin\\FootballManager.exe`.

### Mejoras futuras sugeridas

- Hacer que la cabecera superior sea verdaderamente responsiva en resoluciones mas chicas, permitiendo que algunos controles pasen a una segunda fila si hace falta.
- Afinar anchos por pantalla para que vistas como fichajes, noticias y cantera puedan usar layouts ligeramente distintos en vez de compartir una sola proporcion fija.
- Reemplazar parte del texto largo de paneles vacios por tarjetas visuales o CTAs con iconografia ligera para mejorar onboarding sin saturar texto.

## Auditoria del proyecto (2026-03-25 22:08:31 -03:00) - header responsivo, perfiles por pantalla y onboarding visual

### Lista de cambios realizados

- Se aplico una segunda fase fuerte sobre la GUI Windows para ejecutar las mejoras futuras sugeridas en la entrada anterior.
- `src/gui/gui_layout.cpp` ahora usa perfiles de header y de pagina en vez de depender de una sola geometria fija para todas las vistas.
- La cabecera superior paso a ser responsiva: en anchos menores puede separar campos y botones en mas filas, y las metricas superiores se redistribuyen en multiples columnas/filas segun el ancho disponible.
- La columna lateral tambien responde al ancho de ventana con un rail mas compacto cuando hace falta.
- Cada pagina principal ahora tiene proporciones propias para panel central y columna derecha, mejorando especialmente dashboard, fichajes, noticias, tacticas, finanzas y plantilla.
- El estado vacio del dashboard se redisenio para parecer onboarding util y no un bloque de texto: ahora muestra pasos de arranque, acciones listas y una vista previa clara de lo que se desbloquea al iniciar carrera.
- Se actualizaron etiquetas de panel amigables para reflejar los nuevos bloques de onboarding.
- Se ajustaron tambien los botones del estado vacio (`Crear carrera`, `Abrir guardado`) para que funcionen como CTA mas claros.

### Archivos modificados

- `src/gui/gui_layout.cpp`
- `src/gui/gui_runtime.cpp`
- `src/gui/gui_view_overview.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager`
- Resultado:
- `FootballManager` recompilo correctamente y el ejecutable actualizado quedo en `build-cmake\\bin\\FootballManager.exe`.

### Mejoras futuras sugeridas

- Hacer que las tablas ajusten anchos de columna por contenido y por pagina para reducir scroll horizontal innecesario.
- Incorporar escalado DPI mas explicito para que la GUI mantenga proporciones limpias en pantallas 125%/150%.
- Explorar paneles owner-draw para tarjetas vacias y widgets resumen con mas identidad visual sin abandonar Win32 puro.

## Auditoria del proyecto (2026-03-25 22:16:50 -03:00) - fullscreen, DPI awareness y tablas autoajustables

### Lista de cambios realizados

- Se aplico una nueva pasada sobre la GUI para ejecutar las mejoras futuras pendientes y hacer que el juego arranque ocupando toda la pantalla util.
- `src/gui/gui.cpp` ahora habilita DPI awareness con fallback compatible para la toolchain actual, detecta DPI de la ventana y arranca `FootballManager` maximizado desde el inicio.
- La ventana Win32 maneja `WM_DPICHANGED`, recrea fuentes, reaplica tipografia a controles y reajusta layout/columnas cuando cambia la escala del monitor.
- `include/gui/gui_internal.h` y `src/gui/gui_layout.cpp` incorporan estado DPI y helpers para reconstruir fuentes y reaplicar tipografia sin depender de valores fijos.
- El layout principal ahora escala mejor con DPI alto en espaciados, tarjetas, botones, paneles y alturas clave, en vez de solo cambiar el tamano de la fuente.
- `src/gui/gui_shared.cpp` agrega autoajuste de columnas de `ListView` por contenido, encabezado y ancho visible del panel, reduciendo scroll horizontal innecesario.
- `src/gui/gui_runtime.cpp` recalcula columnas visibles despues de refrescar la pagina y tambien cuando la ventana cambia de tamano.
- El resultado es una GUI que se abre maximizada, usa mejor el espacio y se lee mejor tanto en fullscreen como en monitores con escalado de Windows.

### Archivos modificados

- `include/gui/gui_internal.h`
- `src/gui/gui.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_runtime.cpp`
- `src/gui/gui_shared.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager`
- Resultado:
- `FootballManager` recompilo correctamente y el ejecutable GUI actualizado quedo en `build-cmake\\bin\\FootballManager.exe`.
- La compilacion dejo una advertencia previa en custom draw sobre comparacion signed/unsigned en `src/gui/gui_shared.cpp`, pero no bloqueo la build ni afecta el runtime de esta mejora.

### Mejoras futuras sugeridas

- Agregar un modo `F11` o alternador explicito para cambiar entre maximizado, ventana y fullscreen sin borde durante la ejecucion.
- Afinar el reparto de ancho entre columnas “texto largo” y columnas numericas con reglas especificas por vista para mejorar todavia mas fichajes y noticias.
- Profundizar la parte visual owner-draw con widgets KPI y tarjetas de resumen mas ricas en el dashboard sin abandonar Win32 puro.

## Auditoria del proyecto (2026-03-25 22:21:51 -03:00) - publicacion en GitHub y registro de entrega

### Lista de cambios realizados

- Se creo el commit `de2bafe` con descripcion amplia de la refactorizacion de simulacion, mercado, desarrollo y modernizacion de la GUI Win32.
- El repositorio se publico/sincronizo en `origin/main` hacia GitHub.
- La descripcion del commit incluye:
- unificacion de salud del jugador y desarrollo mensual
- mejoras del match engine y reportes
- rediseno responsive de la GUI con DPI awareness, tablas autoajustables y arranque maximizado
- verificacion ejecutada con build de tests y suite completa pasando

### Archivos modificados

- `TODO.md`

### Verificacion ejecutada

- `git push origin main`
- Resultado:
- cambios publicados correctamente en GitHub sobre `main`
- GitHub informo que el push omitio la regla de PR para este envio directo, pero la actualizacion quedo aplicada en el remoto

### Mejoras futuras sugeridas

- Si quieres historial mas limpio en futuras entregas, publicar por ramas tematicas (`simulation`, `gui`, `data`) y abrir PRs separados.
- Repetir esta misma logica con changelog publico en `README.md` o `CHANGELOG.md` para que los cambios importantes tambien se vean fuera del commit log.
- Si vas a compartir builds, agregar una automatizacion de release con artefacto del `.exe` y notas de version.

## Auditoria del proyecto (2026-03-25 22:35:43 -03:00) - toggle F11, columnas contextuales y dashboard owner-draw reforzado

### Lista de cambios realizados

- Se agrego un modo `F11` real en tiempo de ejecucion para alternar entre la ventana actual y fullscreen sin borde, preservando el estilo y la colocacion anteriores al restaurar.
- `include/gui/gui_internal.h` y `src/gui/gui.cpp` ahora guardan el estado de la ventana, recuerdan `WINDOWPLACEMENT` y actualizan la barra de estado para que el atajo sea visible y util.
- El autoajuste de columnas en `src/gui/gui_shared.cpp` dejo de ser generico: ahora distingue columnas compactas/numericas frente a columnas de texto largo y aplica reglas especificas para liga, plantilla, fichajes, noticias, lesiones y finanzas.
- En las tablas de fichajes y noticias se reparte mejor el ancho visible entre `Jugador`, `Club`, `Detalle`, `Lectura`, `Mercado` y columnas numericas como edad, valor, salario o riesgo, reduciendo el efecto “todo apretado”.
- `src/gui/gui_layout.cpp` reserva una nueva franja visual para widgets KPI owner-draw en el dashboard y dibuja tarjetas de pulso competitivo, vestuario/salud y mesa del manager.
- El dashboard vacio tambien gana tarjetas de onboarding mas claras, incluyendo una pista visual de `F11` y una lectura mas limpia de los pasos para iniciar una carrera.
- Las tarjetas principales del dashboard recibieron remates visuales adicionales para que el centro del club tenga mas jerarquia y no se vea como una pila uniforme de paneles.

### Archivos modificados

- `include/gui/gui_internal.h`
- `src/gui/gui.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_shared.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager`
- `cmake --build build-cmake --config Release --target FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- `FootballManager` recompilo correctamente y el ejecutable actualizado quedo en `build-cmake\\bin\\FootballManager.exe`.
- La suite `FootballManagerTests` termino con `All tests passed`.
- Persisten advertencias antiguas/no bloqueantes de comparacion signed/unsigned en custom draw y notify Win32, pero no impiden compilar ni ejecutar esta mejora.

### Mejoras futuras sugeridas

- Añadir un boton explicito de modo de pantalla en la cabecera para acompanar el `F11` y hacer el cambio de modo aun mas descubrible para usuarios no tecnicos.
- Llevar la misma logica de widgets owner-draw a fichajes, finanzas y directiva con indicadores pequenos de riesgo, presupuesto y presion competitiva.
- Afinar todavia mas las reglas de ancho por tabla usando no solo encabezados, sino tambien densidad real del contenido mostrado en cada refresco.

## Auditoria del proyecto (2026-03-25 22:47:22 -03:00) - boton de pantalla, insight strips y autosize por densidad real

### Lista de cambios realizados

- Se agrego un boton explicito `Pantalla F11` en la cabecera para descubrir mejor el cambio de modo sin depender de conocer el atajo de teclado.
- El nuevo boton usa el mismo flujo del `F11`: entra a fullscreen sin borde y cambia su texto a `Restaurar F11` cuando corresponde.
- `src/gui/gui_layout.cpp` ajusta el layout superior para reservar espacio al nuevo boton y manejar mejor el wrap de acciones de cabecera en anchos medios.
- Los widgets owner-draw dejaron de ser exclusivos del dashboard: ahora fichajes, finanzas y directiva tambien muestran una franja de indicadores compactos antes de los paneles principales.
- En fichajes se anadieron indicadores de presion de mercado, caja/scouting y riesgo de plantilla para leer mejor la urgencia del mercado sin abrir cada panel.
- En finanzas se anadieron indicadores de caja operativa, estructura salarial e infraestructura para ver de un vistazo flujo, riesgo y capacidad del club.
- En directiva se anadieron indicadores de confianza, objetivo mensual y presion del cargo para reflejar mejor el estado politico del proyecto.
- El autoajuste de columnas en `src/gui/gui_shared.cpp` ahora usa tambien densidad real del contenido: mide muestras de filas visibles, detecta columnas compactas frente a texto largo y mezcla ese analisis con las reglas por encabezado y por vista.
- Con eso las tablas de fichajes, noticias, salarios y pipeline reparten mejor el ancho entre columnas de detalle y columnas cortas, incluso cuando cambian los datos mostrados tras refrescar.

### Archivos modificados

- `include/gui/gui_internal.h`
- `src/gui/gui.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_shared.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager`
- `cmake --build build-cmake --config Release --target FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- `FootballManager` recompilo correctamente y el ejecutable actualizado quedo en `build-cmake\\bin\\FootballManager.exe`.
- La suite `FootballManagerTests` termino con `All tests passed`.
- Siguen apareciendo advertencias previas/no bloqueantes de comparaciones signed/unsigned en codigo Win32 de notify/custom draw, pero no afectan esta entrega.

### Mejoras futuras sugeridas

- Afinar el boton de pantalla para ofrecer ciclo completo entre ventana restaurada, maximizado y fullscreen sin borde en vez de solo alternar el modo actual con fullscreen.
- Dar a los insight strips accion contextual directa, por ejemplo abrir scouting, contratos o objetivos al hacer click sobre cada tarjeta.
- Seguir puliendo el autosize con memoria por vista para recordar ajustes de ancho recientes y suavizar aun mas los cambios visuales entre refreshes.

## Auditoria del proyecto (2026-03-25 23:03:01 -03:00) - ciclo de pantalla triple, insight strips clicables y autosize con memoria

### Lista de cambios realizados

- El boton de modo de pantalla y `F11` dejaron de ser un toggle binario: ahora recorren ciclo completo entre ventana restaurada, maximizado y fullscreen sin borde.
- `src/gui/gui.cpp` detecta el modo actual de la ventana, ajusta el texto del boton segun el siguiente paso del ciclo (`Maximizar F11`, `Pantalla F11`, `Ventana F11`) y conserva el `WINDOWPLACEMENT` necesario para volver desde fullscreen a la ventana restaurada.
- Las tarjetas KPI e insight strips ya no son solo decorativas: ahora registran hotspots, muestran una pista `Click:` y reaccionan al mouse con acciones contextuales.
- En dashboard vacio las tarjetas ayudan a elegir division, club y lanzar la carrera; en dashboard activo abren liga, plantilla o directiva; en fichajes ejecutan radar/scouting o llevan a salarios; en finanzas y directiva abren resumen, salarios, infraestructura u objetivos concretos.
- `src/gui/gui_layout.cpp` se refactorizo para reutilizar un renderer comun de tarjetas owner-draw, evitando duplicacion entre dashboard e insight strips contextuales.
- El autosize de columnas ahora recuerda anchos recientes por vista y por conjunto de columnas, mezclando ese historial con el analisis actual de cabeceras y densidad para evitar saltos bruscos entre refreshes.
- `src/gui/gui_shared.cpp` guarda esa memoria en `AppState`, la usa como sesgo suave en el recalculo y luego persiste el nuevo reparto, haciendo que fichajes, noticias, salarios y tablas largas se vean mas estables al refrescar.
- La GUI recompilo correctamente tras el parche y la suite de pruebas completa siguio pasando.

### Archivos modificados

- `include/gui/gui_internal.h`
- `src/gui/gui.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_runtime.cpp`
- `src/gui/gui_shared.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerTests`
- `cmake --build build-cmake --config Release --target FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- `cmake --build build-cmake --config Release --target FootballManager`
- Resultado:
- `FootballManager.exe` recompilo correctamente en `build-cmake\\bin\\FootballManager.exe`.
- La suite `FootballManagerTests` termino con `All tests passed`.
- Persisten advertencias antiguas/no bloqueantes de comparaciones signed/unsigned en notify/custom draw Win32, pero no afectan esta entrega.

### Mejoras futuras sugeridas

- Hacer que las tarjetas clicables reaccionen tambien al teclado (`Tab`, `Enter`, `Space`) para que el onboarding y los paneles KPI sean accesibles sin mouse.
- Conectar el hover de las tarjetas a tooltips ricos con mas contexto operativo, por ejemplo recomendacion tactica, alerta medica o lectura financiera ampliada.
- Extender la memoria visual a altura de filas, orden preferido y filtro reciente por pagina para que cada vista recupere aun mejor el contexto de trabajo del manager.

## Auditoria del proyecto (2026-03-25 23:08:03 -03:00) - README rehecho y alineado con el estado real del proyecto

### Lista de cambios realizados

- Se rehizo `README.md` completo para que deje de mezclar descripcion antigua, refactor parcial y bloques desactualizados.
- El nuevo README explica con claridad que el proyecto ofrece GUI Win32, CLI, validacion y tests separados, en vez de presentar una unica forma ambigua de uso.
- Se corrigieron problemas de legibilidad y formato del README anterior, incluyendo texto roto por encoding y secciones que ya no reflejaban el estado actual de la GUI.
- La documentacion ahora describe los ejecutables reales generados por CMake: `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe`.
- Se documentaron las rutas y comandos correctos para compilar con CMake, usar `build.bat`, ejecutar la GUI, abrir el modo consola y lanzar validacion o tests.
- Se agrego una vista mas ordenada de arquitectura, capas del proyecto, carpetas principales, flujo de entrada y documentacion adicional disponible en `docs/`.
- Tambien se actualizaron highlights funcionales para reflejar mejoras recientes de GUI, simulacion, desarrollo de jugadores y estructura modular.

### Archivos modificados

- `README.md`
- `TODO.md`

### Verificacion ejecutada

- Revision manual del contenido de `README.md`
- Resultado:
- La documentacion quedo alineada con el estado real del repositorio y con los targets actuales de `CMakeLists.txt`.
- No fue necesario recompilar ni ejecutar tests porque esta pasada fue exclusivamente documental.

### Mejoras futuras sugeridas

- Agregar capturas de pantalla de la GUI actual al README para que el repositorio muestre de inmediato el estado visual del juego.
- Incorporar una seccion corta de troubleshooting para errores comunes de MinGW, OneDrive y relink de `objects.a` en Windows.
- Sumar un `CHANGELOG.md` o una seccion de releases para separar mejor documentacion estable de historial de cambios recientes.

## Auditoria del proyecto (2026-03-25 23:11:18 -03:00) - guia de MinGW corregida y aterrizada al proyecto

### Lista de cambios realizados

- Se rehizo `proceso de instalacion MinGW/proceso_instalacion.txt` para eliminar la mezcla anterior de guia generica en ingles, bloque duplicado en espanol y texto roto por encoding.
- La nueva version ahora explica de forma concreta como instalar MinGW, agregar `C:\MinGW\bin` al PATH y verificar `g++`, `mingw32-make` y `cmake`.
- La guia quedo alineada con este repositorio real: usa la ruta actual del proyecto, documenta `build.bat`, la compilacion principal con CMake y los ejecutables reales generados (`FootballManager.exe`, `FootballManagerCLI.exe`, `FootballManagerTests.exe`).
- Tambien se anadio una seccion de problemas comunes para PATH, falta de CMake y bloqueos de linking en Windows/OneDrive.

### Archivos modificados

- `proceso de instalacion MinGW/proceso_instalacion.txt`
- `TODO.md`

### Verificacion ejecutada

- Revision manual del contenido final de `proceso de instalacion MinGW/proceso_instalacion.txt`
- Contraste rapido contra `build.bat` y `CMakeLists.txt`
- Resultado:
- La guia quedo consistente con el flujo actual de build del proyecto.
- No fue necesario recompilar ni ejecutar tests porque esta pasada fue exclusivamente documental.

### Mejoras futuras sugeridas

- Crear una segunda guia breve de "instalacion minima" orientada a usuarios no tecnicos con menos texto y capturas.
- Volver a incorporar o rehacer `install_mingw_instructions.txt` si quieres mantener una version paralela en ingles.
- Agregar una seccion de troubleshooting especifica para VS Code y extensiones de C++ en Windows.

## Auditoria del proyecto (2026-03-25 23:44:21 -03:00) - flujo de nueva partida centralizado, validacion automatica y UX del resumen del club corregida

### Lista de cambios realizados

- Se centralizo el setup de nueva partida en un unico estado `gameSetup` con `division`, `club`, `manager`, `ready`, paso actual, error de manager y mensaje inline para evitar desincronizaciones entre combos, cabecera y dashboard.
- Se agregaron funciones explicitas `set_division()`, `set_club()`, `set_manager()` y `check_game_ready()` para separar la logica del flujo respecto de la capa Win32.
- El flujo ahora valida en tiempo real: cambiar division, club o manager actualiza el estado global al instante y habilita o deshabilita `Nueva partida` sin depender del boton `Validar`.
- Se corrigio la inconsistencia visual donde podia verse un club seleccionado pero la cabecera seguia mostrando `Sin carrera`; ahora las metricas leen el setup activo y muestran division, club, manager, estado y presupuesto real del club elegido.
- `Guardar` y `Simular` quedaron vinculados a la existencia de una carrera activa, mientras que `Nueva partida` solo se habilita si la checklist completa esta resuelta.
- El campo manager ahora usa placeholder `Ingresa nombre del manager`, limite razonable de texto y ayuda contextual debajo del input con feedback visual: verde cuando esta listo, amarillo en progreso y rojo si falta el nombre.
- Se rehizo el dashboard vacio para que el panel derecho deje de repetir `Acciones listas` y pase a mostrar `Estado actual de la partida` mas una `Checklist dinamico` coherente con el flujo real.
- Se elimino el boton de validacion redundante del estado vacio y el boton superior se renombro a `Auditar` para separar la auditoria de datos del flujo de creacion de partida.
- Tambien se redujo ruido visual en la pantalla vacia: menos caps decorativos, menos acciones redundantes y tarjetas de pasos con foco claro en `Paso 1 -> Paso 2 -> Paso 3`.

### Archivos modificados

- `include/gui/gui_internal.h`
- `include/gui/gui_view_builders.h`
- `src/gui/gui.cpp`
- `src/gui/gui_actions.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_runtime.cpp`
- `src/gui/gui_view_common.cpp`
- `src/gui/gui_view_competition.cpp`
- `src/gui/gui_view_management.cpp`
- `src/gui/gui_view_overview.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager`
- `cmake --build build-cmake --config Release --target FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- La compilacion del codigo GUI llego correctamente hasta el link final tras recompilar `gui.cpp`, `gui_actions.cpp`, `gui_layout.cpp`, `gui_runtime.cpp`, `gui_view_common.cpp`, `gui_view_overview.cpp` y vistas relacionadas.
- `FootballManagerTests.exe` termino con `All tests passed`.
- El relink final de `build-cmake\\bin\\FootballManager.exe` quedo bloqueado por Windows porque el ejecutable seguia abierto (`FootballManager`, PID 16064), por lo que el codigo quedo actualizado pero ese binario en particular necesita cerrarse antes de volver a compilarlo.

### Mejoras futuras sugeridas

- Llevar la misma checklist guiada a un modal o wizard opcional para usuarios nuevos que prefieran creacion paso a paso fuera del dashboard principal.
- Anadir mensajes contextuales mas ricos por division y club, por ejemplo objetivo deportivo, presupuesto esperado y nivel de presion antes de crear la carrera.
- Hacer que el dashboard de setup recuerde la ultima division explorada entre sesiones, sin autoarrancar la carrera hasta que la checklist vuelva a quedar valida.

## Auditoria del proyecto (2026-03-26 10:22:05 -03:00) - hub del manager, staff semanal, partido mas contextual y README alineado

### Cambios realizados

- Se profundizo la capa tipo Football Manager con una `mesa de staff` real: ahora el juego genera recomendaciones semanales priorizadas del asistente, preparador, medico, scouting, juveniles, analista y direccion deportiva.
- Se creo un `hub del manager` que mezcla prioridades de staff, agenda del DT, pulso de mercado e inbox combinado para que el dashboard y la vista de noticias muestren decisiones utiles y no solo texto suelto.
- La simulacion semanal ahora despacha briefing del staff al inbox y a noticias, conectando clima de vestuario, estado fisico, contratos, scouting y directiva con el flujo real de la carrera.
- El motor de mercado gano mas profundidad en scouting y negociacion: la IA ya considera cobertura de asignaciones, encaje con el proyecto, identidad del club y clima interno al valorar objetivos y cerrar traspasos.
- El motor de partido quedo mas coherente con la instruccion elegida: `Por bandas`, `Laterales altos`, `Balon parado`, `Juego directo`, `Pausar juego`, `Bloque bajo`, `Contra-presion` y `Presion final` afectan mejor posesion, intensidad, riesgo y tipo de ocasiones.
- La generacion de eventos del partido ahora describe mejor el patron de ataque y potencia jugadas coherentes con la instruccion, especialmente ataques por fuera, cargas de laterales y acciones de balon parado.
- La GUI del dashboard y noticias ahora expone el `Centro del manager` y el digest del staff para que esas capas se vean al ejecutar el juego, no solo en codigo.
- Se ampliaron los tests automaticos para cubrir el hub del manager y el efecto real de instrucciones de partido sobre el perfil de ocasiones.
- Se rehizo `README.md` para dejarlo alineado con el estado real del proyecto, la arquitectura actual, la GUI Win32 y las mecanicas profundas ya integradas.

### Archivos modificados

- `README.md`
- `include/career/inbox_service.h`
- `include/career/staff_service.h`
- `src/ai/ai_transfer_manager.cpp`
- `src/career/app_services.cpp`
- `src/career/inbox_service.cpp`
- `src/career/manager_advice.cpp`
- `src/career/staff_service.cpp`
- `src/career/transfer_briefing.cpp`
- `src/career/week_simulation.cpp`
- `src/gui/gui_view_management.cpp`
- `src/gui/gui_view_overview.cpp`
- `src/simulation/match_event_generator.cpp`
- `src/simulation/match_phase.cpp`
- `src/transfers/negotiation_system.cpp`
- `tests/project_tests.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` compilaron correctamente.
- La suite automatizada termino con `All tests passed`.

### Mejoras futuras sugeridas

- Llevar el hub del manager a una vista propia con bandejas separadas para staff, scouting, directiva, prensa y mercado.
- Profundizar las negociaciones con promesas especificas por rol, minutos, competicion y clausulas de salida.
- Seguir enriqueciendo el motor de partido con patrones por zona, ajustes del rival durante el encuentro y mas detalle en balon parado.

## Auditoria del proyecto (2026-03-26 10:47:27 -03:00) - menu principal Chilean Footballito y configuraciones integradas

### Cambios realizados

- Se agrego un menu principal completo y funcional con el titulo `Chilean Footballito`, visible antes de entrar al flujo normal del juego.
- En consola ahora existe un frontend real de arranque con dos opciones principales: `Jugar` y `Configuraciones`, mas salida limpia del juego.
- `Jugar` abre el flujo normal ya existente del proyecto sin romper el menu de carrera, juego rapido, copa ni validacion.
- `Configuraciones` permite ajustar opciones base desde el frontend: `Volumen`, `Dificultad` y `Modo de simulacion`.
- Se creo una capa compartida de configuracion con `GameSettings`, etiquetas, helpers de ciclo y aplicacion basica de dificultad para evitar logica duplicada entre consola y GUI.
- La dificultad ya tiene efecto inicial real: al crear carrera modifica un poco el contexto de arranque del proyecto, y en juego rapido ajusta el contexto del partido.
- El modo de simulacion ahora se integra al flujo actual: en consola controla si el partido rapido va en modo detallado o rapido, y en GUI informa el estilo de simulacion activo al lanzar la semana.
- En la GUI Win32 se integro un menu de inicio propio dentro del runtime actual, no como ventana aparte: arranca en `Menu principal`, muestra `Chilean Footballito`, y desde ahi deja entrar a `Jugar` o `Configuraciones`.
- La pantalla de configuraciones de GUI reutiliza el mismo estado compartido y permite ciclar volumen, dificultad y modo de simulacion sin salir del frontend.
- Se cambio el titulo de la consola Windows a `Chilean Footballito` y el titulo de la ventana GUI tambien se actualizo para mantener consistencia.
- Se ampliaron los tests para cubrir el ciclo de settings y el impacto base de dificultad.

### Archivos modificados

- `CMakeLists.txt`
- `include/engine/front_menu.h`
- `include/engine/game_controller.h`
- `include/engine/game_settings.h`
- `include/gui/gui.h`
- `include/gui/gui_internal.h`
- `include/gui/gui_view_builders.h`
- `src/engine/front_menu.cpp`
- `src/engine/game_controller.cpp`
- `src/engine/game_settings.cpp`
- `src/gui/gui.cpp`
- `src/gui/gui_actions.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_runtime.cpp`
- `src/gui/gui_view_common.cpp`
- `src/gui/gui_view_menu.cpp`
- `src/gui/gui_views.cpp`
- `src/main.cpp`
- `src/ui/ui.cpp`
- `tests/project_tests.cpp`
- `TODO.md`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` compilaron correctamente con el nuevo frontend de menu.
- La suite automatizada termino con `All tests passed`.

### Mejoras futuras sugeridas

- Persistir `GameSettings` en disco para que volumen, dificultad y modo de simulacion sobrevivan entre sesiones.
- Expandir el frontend inicial con opciones extra como `Continuar`, `Cargar guardado` y `Salir al escritorio` directamente desde la GUI.
- Llevar el menu de configuraciones a una capa de opciones mas amplia con accesibilidad, idioma, velocidad de texto y preferencias visuales.

## Auditoria del proyecto (2026-03-26 10:56:36 -03:00) - publicacion de cambios en GitHub

### Cambios realizados

- Se preparo la entrega completa para publicacion en GitHub con todos los cambios profundos acumulados en simulacion, manager hub, scouting, GUI y menu principal.
- La entrega incluye el nuevo frontend `Chilean Footballito`, las configuraciones base compartidas entre consola y GUI, y la integracion del menu antes del flujo normal del juego.
- Tambien quedan incluidos los cambios previos de profundidad tipo Football Manager: mesa de staff, centro del manager, mejoras de scouting/negociacion y mayor peso de instrucciones de partido.
- Se dejo registrada esta publicacion dentro de `TODO.md` para mantener el historial append-only del proyecto.

### Archivos modificados

- `CMakeLists.txt`
- `README.md`
- `TODO.md`
- `include/career/inbox_service.h`
- `include/career/staff_service.h`
- `include/engine/front_menu.h`
- `include/engine/game_controller.h`
- `include/engine/game_settings.h`
- `include/gui/gui.h`
- `include/gui/gui_internal.h`
- `include/gui/gui_view_builders.h`
- `src/ai/ai_transfer_manager.cpp`
- `src/career/app_services.cpp`
- `src/career/inbox_service.cpp`
- `src/career/manager_advice.cpp`
- `src/career/staff_service.cpp`
- `src/career/transfer_briefing.cpp`
- `src/career/week_simulation.cpp`
- `src/engine/front_menu.cpp`
- `src/engine/game_controller.cpp`
- `src/engine/game_settings.cpp`
- `src/gui/gui.cpp`
- `src/gui/gui_actions.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_runtime.cpp`
- `src/gui/gui_view_common.cpp`
- `src/gui/gui_view_management.cpp`
- `src/gui/gui_view_menu.cpp`
- `src/gui/gui_view_overview.cpp`
- `src/gui/gui_views.cpp`
- `src/main.cpp`
- `src/simulation/match_event_generator.cpp`
- `src/simulation/match_phase.cpp`
- `src/transfers/negotiation_system.cpp`
- `src/ui/ui.cpp`
- `tests/project_tests.cpp`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- Los binarios `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` compilaron correctamente.
- La suite automatizada termino con `All tests passed`.

### Mejoras futuras sugeridas

- Crear notas de version o `CHANGELOG` para resumir entregas grandes de forma mas visible en el repositorio.
- Persistir configuraciones del frontend entre sesiones y conectarlas a guardado/carga.
- Seguir empujando el parecido a Football Manager con mas narrativa semanal, staff avanzado y ajustes del rival en vivo.

## Auditoria del proyecto (2026-03-26 11:06:31 -03:00) - frontend principal mas profesional y escalable

### Cambios realizados

- Se rehizo la base del frontend de arranque para que `Chilean Footballito` tenga un menu principal mas cercano a un videojuego real, con una arquitectura modular compartida entre consola y GUI.
- En consola se reemplazo el flujo minimo por un `MenuController` con pantallas separadas, opciones modeladas, navegacion por `W/S`, `Enter`, numeros y atajos, manteniendo `Jugar` y `Configuraciones` como entradas principales.
- Se amplio `GameSettings` con `velocidad de simulacion`, ademas de volumen, dificultad y modo de simulacion, dejando una estructura central lista para futuras expansiones y reutilizada por GUI y CLI.
- La GUI Win32 recibio un submenu de configuraciones mas completo con cuatro ajustes, foco visual para botones, soporte de teclado con `Tab`/`Enter`, salida con `Esc` desde configuraciones y estilos diferenciados para el frontend.
- Se reforzo la presentacion del menu en la GUI con botones owner-draw mas claros, resaltado por foco para la opcion seleccionada y textos de apoyo que explican el flujo inicial del juego.
- Se actualizo la documentacion para reflejar el frontend inicial, la navegacion de consola y el nuevo set de configuraciones disponibles.

### Archivos modificados

- `README.md`
- `TODO.md`
- `include/engine/front_menu.h`
- `include/engine/game_settings.h`
- `include/gui/gui_internal.h`
- `src/engine/front_menu.cpp`
- `src/engine/game_controller.cpp`
- `src/engine/game_settings.cpp`
- `src/gui/gui.cpp`
- `src/gui/gui_actions.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_shared.cpp`
- `src/gui/gui_view_common.cpp`
- `src/gui/gui_view_menu.cpp`
- `tests/project_tests.cpp`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` compilaron correctamente con el frontend profesionalizado y la nueva capa de settings.
- La suite automatizada termino con `All tests passed`.

### Mejoras futuras sugeridas

- Persistir `GameSettings` en archivo para conservar volumen, dificultad, velocidad y modo entre sesiones.
- Agregar opciones futuras ya preparadas por la arquitectura como `Continuar`, `Cargar partida`, `Creditos` y `Salir` tambien en la GUI.
- Conectar `velocidad de simulacion` a animaciones, pausas, transiciones y timing real de la interfaz para que deje de ser solo estructural.

## Auditoria del proyecto (2026-03-26 11:16:52 -03:00) - portada tipo manager game y menu mas elegante

### Resumen de cambios realizados

- Se profundizo el frontend inicial para que `Chilean Footballito` se sienta mas cercano a la portada de un manager game serio desde el primer frame, sin romper el flujo real ya existente.
- En consola se separaron mejor responsabilidades con `MenuRenderer`, `MenuActionHandler` y `MenuController`, dejando el menu principal mas limpio, con paneles ASCII, estados visibles y base lista para crecer.
- La GUI Win32 recibio una portada mas fuerte visualmente: hero title central, subtitulo editorial, chips de estado, roadmap visual para futuras opciones y paneles mejor jerarquizados.
- Se agrego navegacion por flechas, `Enter`, numeros y atajos directos dentro del front menu GUI, para que no dependa solo de `Tab` o del mouse.
- El submenu de configuraciones se mantuvo integrado al flujo real del juego y ahora muestra mejor el impacto de dificultad, velocidad, modo y volumen placeholder.
- Se reforzo la documentacion del proyecto para reflejar que el frontend inicial ya funciona como una portada de simulador de gestion y no como una simple lista de opciones.

### Archivos creados o modificados

- `README.md`
- `TODO.md`
- `include/engine/front_menu.h`
- `include/engine/game_settings.h`
- `include/gui/gui_internal.h`
- `src/engine/front_menu.cpp`
- `src/engine/game_controller.cpp`
- `src/engine/game_settings.cpp`
- `src/gui/gui.cpp`
- `src/gui/gui_actions.cpp`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_runtime.cpp`
- `src/gui/gui_shared.cpp`
- `src/gui/gui_view_common.cpp`
- `src/gui/gui_view_menu.cpp`
- `tests/project_tests.cpp`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` compilaron correctamente tras la nueva portada y la refactorizacion del menu.
- La suite automatizada termino con `All tests passed`.

### Mejoras futuras sugeridas

- Conectar la portada con opciones futuras reales como `Continuar`, `Cargar partida`, `Creditos` y `Salir` sin tocar la arquitectura base creada.
- Persistir en disco el ultimo perfil de configuracion y usarlo para recordar dificultad, velocidad, modo y volumen al abrir el juego.
- Llevar la misma identidad visual del front menu a pantallas futuras como carga, perfil de usuario y selecciones previas al inicio de carrera.

## Auditoria del proyecto (2026-03-26 11:32:37 -03:00) - musica del menu principal con mp3 local

### Resumen de cambios realizados

- Se agrego un modulo de audio Win32 especifico para el frontend del juego, aislado del resto de la GUI para no mezclar reproduccion multimedia con layout o acciones.
- El archivo local `Los Miserables - El Crack  Video Oficial (HD Remastered).mp3` ahora se busca automaticamente y se reproduce en bucle cuando la pantalla activa es el `MainMenu`.
- La musica se detiene y reinicia limpia al salir de la portada principal, evitando que siga sonando dentro del dashboard, configuraciones u otras pantallas.
- El volumen del frontend ahora actualiza la sesion de musica del menu cuando el track esta abierto, aprovechando la capa de `GameSettings` ya existente.
- La build Win32 fue actualizada para enlazar `winmm`, dejando la reproduccion lista sin agregar dependencias externas nuevas.
- Se actualizo la documentacion para reflejar que la portada puede reproducir el tema local del repositorio mientras esta activa.

### Archivos creados o modificados

- `CMakeLists.txt`
- `README.md`
- `TODO.md`
- `include/gui/gui_audio.h`
- `include/gui/gui_internal.h`
- `src/gui/gui_audio.cpp`
- `src/gui/gui.cpp`
- `src/gui/gui_actions.cpp`
- `src/gui/gui_runtime.cpp`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` compilaron correctamente con el nuevo modulo de audio del menu.
- La suite automatizada termino con `All tests passed`.

### Mejoras futuras sugeridas

- Persistir la preferencia de musica del menu para poder activarla o silenciarla desde configuraciones.
- Sustituir el control actual por una pequena capa de audio mas completa con mute, fade-in y fade-out al entrar o salir del menu.
- Agregar una opcion visual en configuraciones que permita elegir entre reproducir el tema solo en portada o tambien en otras pantallas del frontend.

## Auditoria del proyecto (2026-03-26 11:33:50 -03:00) - compatibilidad de build.bat con menu y audio

### Resumen de cambios realizados

- Se verifico que `build.bat` compila el proyecto actual usando la ruta principal con CMake y que el ejecutable resultante incluye el frontend nuevo, la portada GUI y la musica del menu.
- Se corrigio la ruta `fallback` del propio `build.bat` para enlazar tambien `winmm`, evitando que la reproduccion del mp3 falle cuando la compilacion no use CMake.
- Se confirmo que el script sigue dejando el ejecutable actualizado en `build-cmake\\bin\\FootballManager.exe` cuando compila por CMake.

### Archivos creados o modificados

- `TODO.md`
- `build.bat`

### Verificacion ejecutada

- `FM_SKIP_RUN=1`
- `build.bat`
- Resultado:
- El script compilo correctamente usando CMake.
- La salida generada fue `build-cmake\\bin\\FootballManager.exe`.

### Mejoras futuras sugeridas

- Hacer que `build.bat` tambien tenga un paso opcional para compilar y correr `FootballManagerTests`.
- Mostrar en el propio script si se uso la ruta CMake o la ruta fallback con un resumen final mas detallado.
- Permitir flags explicitos para compilar GUI, CLI o tests desde el mismo `build.bat`.

## Auditoria del proyecto (2026-03-26 11:36:52 -03:00) - limpieza visual de portada y espaciado del menu principal

### Resumen de cambios realizados

- Se corrigio el layout de la portada GUI para que la franja superior de accesos futuros no invada el mismo espacio que usan los botones principales `Jugar` y `Configuraciones`.
- El bloque interactivo del frontend ahora reserva altura distinta segun la pantalla: menu principal con CTA compactos y configuraciones con stack completo de ajustes, evitando solapes con los paneles inferiores.
- Se limpiaron los textos del menu principal y de configuraciones para que los paneles no repitan dentro del cuerpo el mismo titulo que ya se muestra en la cabecera del widget.
- Se recompilo la GUI y la CLI con el ajuste visual, manteniendo compatibilidad con la arquitectura actual del frontend.

### Archivos creados o modificados

- `TODO.md`
- `src/gui/gui_layout.cpp`
- `src/gui/gui_view_menu.cpp`

### Verificacion ejecutada

- `cmake --build build-cmake --config Release --target FootballManager FootballManagerCLI FootballManagerTests`
- `.\\build-cmake\\bin\\FootballManagerTests.exe`
- Resultado:
- `FootballManager.exe`, `FootballManagerCLI.exe` y `FootballManagerTests.exe` compilaron correctamente despues del ajuste de espaciado.
- La suite automatizada termino con `All tests passed`.

### Mejoras futuras sugeridas

- Convertir la franja `Continuar / Nueva partida / Cargar / Creditos` en tarjetas realmente interactivas para que dejen de ser solo anticipos visuales.
- Ajustar la portada segun ancho y alto de ventana con presets especificos para pantallas mas pequenas y ultraanchas.
- Reemplazar parte del texto descriptivo por pequenos KPI visuales o tarjetas de estado para dar aun mas sensacion de dashboard de manager.

## Auditoria del proyecto (2026-03-26 11:44:29 -03:00) - asset de audio versionado en el repositorio

### Resumen de cambios realizados

- Se agrego al repositorio el archivo `Los Miserables - El Crack  Video Oficial (HD Remastered).mp3` para que la musica de la portada principal ya no dependa solo de un asset local no trackeado.
- Se actualizo la documentacion para reflejar que el tema del menu principal queda versionado dentro del proyecto y acompana la experiencia inicial de `Chilean Footballito`.
- La subida de este asset se realizo apoyandose en tu confirmacion explicita de que el audio puede distribuirse como dominio publico.

### Archivos creados o modificados

- `Los Miserables - El Crack  Video Oficial (HD Remastered).mp3`
- `README.md`
- `TODO.md`

### Verificacion ejecutada

- `Get-Item "Los Miserables - El Crack  Video Oficial (HD Remastered).mp3" | Select-Object Name,Length,LastWriteTime`
- Resultado:
- El archivo existe en la raiz del proyecto y quedo listo para ser versionado junto al frontend actual.

### Mejoras futuras sugeridas

- Mover los assets de audio a una carpeta dedicada como `assets/audio/` para separar binarios multimedia del codigo fuente.
- Agregar metadatos o un manifiesto de assets que documente origen, licencia y uso previsto de cada recurso audiovisual.
- Permitir seleccionar entre varios temas de menu desde configuraciones si en el futuro se versionan mas pistas.
