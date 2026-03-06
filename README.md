# Juego
Aqui estan los archivos del juego
Nota: valores monetarios usan enteros de 64 bits; entrada manual hasta 1e12.
Plantillas canonicas: players.csv.
Equipos por division: ver LigaChilena/<division>/teams.txt.
Compilacion (ejemplo):
g++ -std=c++17 -O2 main.cpp utils.cpp models.cpp io.cpp competition.cpp app_services.cpp gui.cpp simulation.cpp ui.cpp validators.cpp -o FootballManager.exe -lcomctl32 -lgdi32

Ejecucion:
- `FootballManager.exe` abre la interfaz grafica nativa en Windows.
- `FootballManager.exe --cli` mantiene la interfaz de consola original.
- `FootballManager.exe --validate` ejecuta la suite de validacion.

Arquitectura:
- `app_services.cpp` desacopla la GUI de decisiones y mensajes de terminal para iniciar/cargar/guardar/simular y validar.
- La simulacion semanal en GUI ya no depende de `readInt` para renovaciones, ofertas entrantes o despidos.

## Ultimos cambios
- Moral de equipo y potencial/desarrollo de jugadores.
- Ascensos/descensos con playoffs al final de temporada.
- Finanzas semanales (ingresos/sueldos) y mercado con ofertas/CPU.
- Lesiones con tipo/duracion y tacticas nuevas (Pressing/Counter).
- Estadisticas post-partido y editor rapido de equipo/jugadores.
- Scouting con incertidumbre, contratos y salarios.\n- Roles por jugador, plan de entrenamiento semanal y cantera.\n- Eventos de club, dashboard semanal y tacticas CPU adaptativas.\n- Modo Copa (eliminacion directa).
