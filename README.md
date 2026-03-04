# Juego
Aqui estan los archivos del juego
Nota: valores monetarios usan enteros de 64 bits; entrada manual hasta 1e12.
Plantillas canonicas: players.csv.
Equipos por division: ver LigaChilena/<division>/teams.txt.
Compilacion (ejemplo):
g++ -std=c++17 -O2 main.cpp utils.cpp models.cpp io.cpp simulation.cpp ui.cpp -o FootballManagerGame.exe

## Ultimos cambios
- Moral de equipo y potencial/desarrollo de jugadores.
- Ascensos/descensos con playoffs al final de temporada.
- Finanzas semanales (ingresos/sueldos) y mercado con ofertas/CPU.
- Lesiones con tipo/duracion y tacticas nuevas (Pressing/Counter).
- Estadisticas post-partido y editor rapido de equipo/jugadores.
