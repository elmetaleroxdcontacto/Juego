#!/usr/bin/env python3
"""
Liga Chilena (target: 2026) – Carpetas por división + equipos + planteles (con edad) + posiciones normalizadas

Estructura generada:
  LigaChilena/
    primera division/
    primera b/
    segunda division/
    tercera division a/
    tercera division b/

Fuentes:
- Primera / Primera B / Segunda: Transfermarkt (planteles)
- Tercera División A / B: EnElCamarin (JoomSport) (planteles)

Mejoras (pedido):
- Normaliza posiciones a: ARQ / DEF / MED / DEL
- Agrega edad (age) en TODAS las divisiones:
    - Transfermarkt: usa edad de la tabla del plantel
    - EnElCamarin: intenta obtener fecha de nacimiento/edad desde la página del jugador (joomsport_player)
      y calcula la edad (si se encuentra la fecha). Si no, deja N/A.

Uso:
  python scrape_transfermarkt.py --outdir LigaChilena

Opcional (si cambian temporadas):
  python scrape_transfermarkt.py --tercera-a-url "<URL>" --tercera-b-url "<URL>"

Opcional (formatos extra):
  python scrape_transfermarkt.py --outdir LigaChilena --write-json --write-txt

Requisitos:
  pip install requests beautifulsoup4
"""
from __future__ import annotations
import argparse
import csv
import json
import os
import random
import re
import time
from dataclasses import dataclass
from datetime import date, datetime
import sys
from typing import Dict, List, Optional, Tuple
from urllib.parse import urljoin, urlparse

import requests
from bs4 import BeautifulSoup

HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/121.0.0.0 Safari/537.36"
    ),
    "Accept-Language": "es,en;q=0.9",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    "Connection": "keep-alive",
}

# -----------------
# Utilidades
# -----------------
def slugify(name: str) -> str:
    name = re.sub(r"[\\/:*?\"<>|]+", "-", name.strip())  # inválidos Windows
    return re.sub(r"\s+", " ", name).strip()

def ensure_dir(path: str) -> None:
    os.makedirs(path, exist_ok=True)

def decode_response(r: requests.Response) -> str:
    """
    Decodifica HTML con la codificación correcta para evitar mojibake (Ã/â).
    Requests suele caer en ISO-8859-1 cuando no hay header de charset.
    """
    enc = (r.encoding or "").lower()
    if not enc or enc in {"iso-8859-1", "latin1", "cp1252", "windows-1252"}:
        enc = (r.apparent_encoding or "utf-8").lower()
    try:
        return r.content.decode(enc, errors="replace")
    except Exception:
        return r.text

def get(url: str, session: requests.Session, retries: int = 4, timeout: int = 35) -> Optional[str]:
    last_err = None
    for i in range(retries):
        try:
            r = session.get(url, headers=HEADERS, timeout=timeout)
            if r.status_code == 200:
                return decode_response(r)
            last_err = f"HTTP {r.status_code}"
        except Exception as e:
            last_err = str(e)
        time.sleep(1.1 + i * 1.4 + random.random())
    print(f"[WARN] No se pudo obtener: {url} ({last_err})")
    return None

def normalize_position(raw: str) -> str:
    """
    Normaliza a 4 categorías: ARQ/DEF/MED/DEL.
    """
    if not raw:
        return "N/A"
    s = raw.strip().lower()

    # Portero
    if any(k in s for k in ["goalkeeper", "keeper", "gk", "portero", "arquero", "guardameta", "arq", "por"]):
        return "ARQ"

    # Defensa
    if any(k in s for k in [
        "defender", "defensa", "defensor", "centre-back", "center-back",
        "full-back", "fullback", "wing-back", "lateral", "carrilero", "central",
        "cb", "rb", "lb", "rwb", "lwb", "wb", "def"
    ]):
        return "DEF"

    # Mediocampo
    if any(k in s for k in [
        "midfield", "midfielder", "medioc", "mediocentro", "centrocampista",
        "volante", "pivote", "enganche", "interior", "medio", "cm", "dm", "am", "lm", "rm", "med"
    ]):
        return "MED"

    # Delantero
    if any(k in s for k in [
        "forward", "striker", "delantero", "atacante", "extremo", "punta", "nueve",
        "winger", "centre-forward", "center-forward", "second striker",
        "fw", "st", "cf", "ss", "rw", "lw", "del"
    ]):
        return "DEL"

    return "N/A"

def _strip_leading_number(text: str) -> str:
    if not text:
        return ""
    return re.sub(r"^\s*\d{1,3}\s*[\.\-)\]]\s*", "", text)

def infer_position_from_text(text: str) -> Tuple[str, str]:
    """
    Intenta inferir la posicion desde texto libre (ej: nombre con 'Portero').
    Retorna (pos_normalizada, token_encontrado).
    """
    if not text:
        return "N/A", ""
    s = _strip_leading_number(text)
    patterns = [
        ("ARQ", [
            r"\b(portero|arquero|guardameta|goalkeeper|keeper)\b",
            r"\b(gk|arq|por)\b",
        ]),
        ("DEF", [
            r"\b(defensa|defensor|zaguero|central|lateral|carrilero)\b",
            r"\b(full-?back|wing-?back|centre-?back|center-?back)\b",
            r"\b(cb|rb|lb|rwb|lwb|wb|def)\b",
        ]),
        ("MED", [
            r"\b(mediocampista|mediocentro|centrocampista|volante|pivote|enganche|interior|medio)\b",
            r"\b(midfield(?:er)?)\b",
            r"\b(cm|dm|am|lm|rm|med)\b",
        ]),
        ("DEL", [
            r"\b(delantero|atacante|extremo|punta|nueve)\b",
            r"\b(winger|striker|forward|centre-?forward|center-?forward|second striker)\b",
            r"\b(cf|ss|rw|lw|fw|st|del)\b",
        ]),
    ]
    for pos, pats in patterns:
        for pat in pats:
            m = re.search(pat, s, re.IGNORECASE)
            if m:
                return pos, m.group(0)
    return "N/A", ""

def finalize_position(name: str, pos_raw: str) -> Tuple[str, str]:
    pos_raw = pos_raw or ""
    pos = normalize_position(pos_raw)
    if pos == "N/A":
        inferred_pos, inferred_raw = infer_position_from_text(name or "")
        if inferred_pos != "N/A":
            pos = inferred_pos
            if not pos_raw or pos_raw == "N/A":
                pos_raw = inferred_raw or inferred_pos
    return pos, pos_raw

def safe_int_from_text(t: str) -> Optional[int]:
    if not t:
        return None
    m = re.search(r"\b(\d{1,2})\b", t)
    return int(m.group(1)) if m else None

def calc_age_from_birthdate(birth: date, today: Optional[date] = None) -> int:
    today = today or date.today()
    years = today.year - birth.year
    if (today.month, today.day) < (birth.month, birth.day):
        years -= 1
    return years

def parse_date_any(s: str) -> Optional[date]:
    """
    Intenta parsear fechas comunes (es/en):
    - 2001-12-31
    - 31/12/2001
    - 31-12-2001
    - 31 Dec 2001 / Dec 31, 2001
    - 31 de diciembre de 2001
    """
    if not s:
        return None
    s = s.strip()

    # ISO
    for fmt in ("%Y-%m-%d", "%d/%m/%Y", "%d-%m-%Y", "%d.%m.%Y"):
        try:
            return datetime.strptime(s, fmt).date()
        except Exception:
            pass

    # Inglés con mes abreviado/completo
    for fmt in ("%d %b %Y", "%d %B %Y", "%b %d, %Y", "%B %d, %Y"):
        try:
            return datetime.strptime(s, fmt).date()
        except Exception:
            pass

    # Español: "31 de diciembre de 2001"
    m = re.search(r"\b(\d{1,2})\s+de\s+([a-zA-Záéíóúñ]+)\s+de\s+(\d{4})\b", s.lower())
    if m:
        day = int(m.group(1))
        mon_name = m.group(2)
        year = int(m.group(3))
        months = {
            "enero": 1, "febrero": 2, "marzo": 3, "abril": 4, "mayo": 5, "junio": 6,
            "julio": 7, "agosto": 8, "septiembre": 9, "setiembre": 9, "octubre": 10,
            "noviembre": 11, "diciembre": 12,
        }
        if mon_name in months:
            try:
                return date(year, months[mon_name], day)
            except Exception:
                return None
    return None

# -----------------
# Transfermarkt
# -----------------
BASE_TM = "https://www.transfermarkt.com"

@dataclass
class TeamTM:
    name: str
    startseite_url: str
    verein_id: str
    saison_id: str

def discover_tm(code: str, path: str, session: requests.Session) -> List[TeamTM]:
    html = get(f"{BASE_TM}/{path}/startseite/wettbewerb/{code}", session, timeout=25)
    if not html:
        return []
    soup = BeautifulSoup(html, "html.parser")
    teams: Dict[str, TeamTM] = {}

    for a in soup.select('a[href*="/startseite/verein/"][href*="/saison_id/"]'):
        href = a.get("href", "")
        m = re.search(r"/verein/(\d+)/saison_id/(\d+)", href)
        if not m:
            continue
        vid, sid = m.groups()
        name = a.get("title") or a.get_text(" ", strip=True)
        if not name:
            continue
        teams.setdefault(vid, TeamTM(name=name, startseite_url=urljoin(BASE_TM, href), verein_id=vid, saison_id=sid))
    return list(teams.values())

def squad_url_tm(t: TeamTM) -> str:
    return t.startseite_url.replace("/startseite/", "/kader/")

def scrape_tm_players(team_squad_url: str, session: requests.Session) -> List[dict]:
    html = get(team_squad_url, session, timeout=25)
    if not html:
        return []
    soup = BeautifulSoup(html, "html.parser")
    players: List[dict] = []

    for r in soup.select("table.items tbody tr"):
        a = r.select_one("td.hauptlink a") or r.select_one('td.posrela a.spielprofil_tooltip')
        if not a:
            continue
        name = a.get_text(" ", strip=True)

        pos_td = r.select_one("td.posrela")
        pos_raw = pos_td.get_text(" ", strip=True) if pos_td else ""

        # Edad: en muchas tablas es una columna 'zentriert' (heurística)
        age_td = r.select_one("td.zentriert:nth-of-type(3)") or r.select_one("td.zentriert:nth-of-type(6)")
        age = age_td.get_text(" ", strip=True) if age_td else "N/A"

        # Market value: suele estar a la derecha
        val_td = r.select_one("td.rechts") or r.select_one("td.rechts.hauptlink")
        market_value = val_td.get_text(" ", strip=True) if val_td else "N/A"

        pos, pos_raw = finalize_position(name, pos_raw)
        players.append({
            "name": name,
            "position_raw": pos_raw if pos_raw else "N/A",
            "position": pos,
            "age": age if age else "N/A",
            "number": "N/A",
            "market_value": market_value if market_value else "N/A",
            "source": team_squad_url,
        })

    # dedup: prefer rows with more data
    def score(p: dict) -> int:
        return int(p.get("position") != "N/A") + int(p.get("age") != "N/A") + int(p.get("market_value") != "N/A")

    by_name: Dict[str, dict] = {}
    for p in players:
        name = (p.get("name") or "").strip()
        if not name:
            continue
        if score(p) == 0:
            continue
        key = name.lower()
        if key not in by_name or score(p) > score(by_name[key]):
            by_name[key] = p

    return list(by_name.values())

# -----------------
# EnElCamarin / JoomSport
# -----------------
def norm_url(url: str) -> str:
    p = urlparse(url)
    b = f"{p.scheme}://{p.netloc}{p.path}"
    return b if b.endswith("/") else b + "/"

def discover_eec(season_url: str, session: requests.Session) -> List[Tuple[str, str]]:
    html = get(norm_url(season_url), session, retries=6, timeout=55)
    if not html:
        return []
    soup = BeautifulSoup(html, "html.parser")
    teams: Dict[str, str] = {}

    for a in soup.select('a[href*="/joomsport_team/"]'):
        href = (a.get("href") or "").strip()
        if not href:
            continue
        name = (a.get_text(" ", strip=True) or a.get("title") or "").strip()
        if not name or len(name) < 3:
            continue
        full = urljoin(season_url, href)
        teams.setdefault(full, name)

    return [(name, url) for url, name in teams.items()]

def extract_player_links_and_rows(soup: BeautifulSoup, base_url: str) -> List[Tuple[str, str, List[str]]]:
    """
    Devuelve lista de (player_name, player_url, row_cols) encontradas en tablas.
    """
    found = []
    for tr in soup.select("table tr"):
        a = tr.select_one('a[href*="/joomsport_player/"]')
        if not a:
            continue
        href = (a.get("href") or "").strip()
        if not href:
            continue
        name = a.get_text(" ", strip=True)
        if not name:
            continue
        cols = [td.get_text(" ", strip=True) for td in tr.select("td")]
        found.append((name, urljoin(base_url, href), cols))
    return found

def scrape_eec_player_age(player_url: str, session: requests.Session, cache: Dict[str, str]) -> str:
    """
    Busca fecha de nacimiento/edad en página del jugador.
    Devuelve edad como string o "N/A".
    """
    if player_url in cache:
        return cache[player_url]

    html = get(norm_url(player_url), session, retries=4, timeout=45)
    if not html:
        cache[player_url] = "N/A"
        return "N/A"
    soup = BeautifulSoup(html, "html.parser")

    text = soup.get_text(" ", strip=True)

    # 1) Si aparece "Edad" o "Age"
    m = re.search(r"\b(Edad|Age)\s*[:\-]?\s*(\d{1,2})\b", text, re.IGNORECASE)
    if m:
        cache[player_url] = m.group(2)
        return m.group(2)

    # 2) Busca "Fecha de nacimiento" / "Born" con una fecha cerca
    patterns = [
        r"(Fecha\s+de\s+nacimiento|Nacim(?:iento)?|Born)\s*[:\-]?\s*([0-9]{1,2}[^0-9]{1,3}[0-9]{1,2}[^0-9]{1,3}[0-9]{2,4}|[0-9]{4}-[0-9]{2}-[0-9]{2}|[0-9]{1,2}\s+de\s+[A-Za-záéíóúñ]+\s+de\s+[0-9]{4}|[A-Za-z]{3,9}\s+[0-9]{1,2},\s+[0-9]{4})"
    ]
    birth_date: Optional[date] = None
    for pat in patterns:
        mm = re.search(pat, text, re.IGNORECASE)
        if mm:
            cand = mm.group(2).strip()
            birth_date = parse_date_any(cand)
            if birth_date:
                break

    # 3) Si no, intenta capturar cualquier fecha que parezca nacimiento con etiqueta "born" en HTML
    if not birth_date:
        for lab in ["Fecha de nacimiento", "Born", "Nacimiento", "Nacido"]:
            node = soup.find(string=re.compile(lab, re.IGNORECASE))
            if node:
                seg = node.parent.get_text(" ", strip=True)
                # intenta extraer fecha del segmento
                cand_date = parse_date_any(seg)
                if cand_date:
                    birth_date = cand_date
                    break

    if birth_date:
        age_int = calc_age_from_birthdate(birth_date)
        cache[player_url] = str(age_int)
        return str(age_int)

    cache[player_url] = "N/A"
    return "N/A"

def scrape_eec_players(team_url: str, session: requests.Session, player_age_cache: Dict[str, str]) -> List[dict]:
    html = get(norm_url(team_url), session, retries=6, timeout=55)
    if not html:
        return []
    soup = BeautifulSoup(html, "html.parser")

    rows = extract_player_links_and_rows(soup, base_url=team_url)
    if not rows:
        # fallback: solo nombres (sin urls)
        out = []
        for a in soup.select('a[href*="/joomsport_player/"]'):
            name = a.get_text(" ", strip=True)
            if name:
                pos, pos_raw = finalize_position(name, "")
                out.append({
                    "name": name,
                    "position_raw": pos_raw if pos_raw else "N/A",
                    "position": pos,
                    "age": "N/A",
                    "number": "N/A",
                    "market_value": "N/A",
                    "source": team_url,
                })
        # dedup
        seen=set()
        ded=[]
        for p in out:
            k=p["name"].lower()
            if k in seen: continue
            seen.add(k); ded.append(p)
        return ded

    players: List[dict] = []
    for name, purl, cols in rows:
        # Número: primera columna si es dígito
        number = cols[0] if cols and re.fullmatch(r"\d{1,2}", cols[0]) else "N/A"

        # Posición: busca tokens conocidos en columnas
        pos_raw = ""
        for c in cols:
            cu = c.upper()
            if cu in ("ARQ", "POR", "GK", "DEF", "MED", "DEL"):
                pos_raw = c
                break
            if any(k in cu for k in ["PORTER", "ARQUER", "DEFEN", "VOLAN", "MEDIO", "DELANT", "EXTREMO", "WINGER", "FORWARD", "STRIKER"]):
                pos_raw = c
                break

        age = scrape_eec_player_age(purl, session=session, cache=player_age_cache)
        pos, pos_raw = finalize_position(name, pos_raw)

        players.append({
            "name": name,
            "position_raw": pos_raw if pos_raw else "N/A",
            "position": pos,
            "age": age,
            "number": number,
            "market_value": "N/A",
            "source": purl,
        })

        # pausa corta por jugador para no sobrecargar
        time.sleep(0.35 + random.random() * 0.25)

    # dedup por nombre
    seen=set()
    out=[]
    for p in players:
        k=p["name"].lower()
        if k in seen: continue
        seen.add(k); out.append(p)
    return out

# -----------------
# Salidas
# -----------------
OUT_FIELDS = ["name", "position", "position_raw", "age", "number", "market_value", "source"]

def write_outputs(team_dir: str, players: List[dict], write_json: bool, write_txt: bool) -> None:
    ensure_dir(team_dir)
    # JSON (opcional)
    if write_json:
        with open(os.path.join(team_dir, "players.json"), "w", encoding="utf-8") as f:
            json.dump(players, f, ensure_ascii=False, indent=2)

    # CSV
    with open(os.path.join(team_dir, "players.csv"), "w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=OUT_FIELDS)
        w.writeheader()
        for p in players:
            w.writerow({k: p.get(k, "N/A") for k in OUT_FIELDS})

    # TXT (opcional)
    if write_txt:
        with open(os.path.join(team_dir, "players.txt"), "w", encoding="utf-8") as f:
            for p in players:
                f.write(
                    f"- {p.get('name')} | {p.get('position')} ({p.get('position_raw')}) | "
                    f"Edad: {p.get('age')} | Nº: {p.get('number')} | Valor: {p.get('market_value')}\n"
                )

# -----------------
# Main
# -----------------
LEAGUES_TM = [
    ("primera division", "CLPD", "liga-de-primera"),
    ("primera b", "CL2B", "liga-de-ascenso"),
    ("segunda division", "CHI3", "segunda-division-profesional"),
]

DEFAULT_TERCERA_A_URL = "https://enelcamarin.cl/joomsport_season/tercera-division-a-tercera-a-2025/"
DEFAULT_TERCERA_B_URL = "https://enelcamarin.cl/joomsport_season/tercera-division-b-tercera-b-2025/"

def main() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")

    ap = argparse.ArgumentParser()
    ap.add_argument("--outdir", default="LigaChilena")
    ap.add_argument("--sleep", type=float, default=1.2, help="Pausa entre requests de equipos.")
    ap.add_argument("--tercera-a-url", default=DEFAULT_TERCERA_A_URL)
    ap.add_argument("--tercera-b-url", default=DEFAULT_TERCERA_B_URL)
    ap.add_argument("--write-json", action="store_true", help="Generar players.json (opcional).")
    ap.add_argument("--write-txt", action="store_true", help="Generar players.txt (opcional).")
    args = ap.parse_args()

    ensure_dir(args.outdir)

    with requests.Session() as s:
        # Transfermarkt (profesionales)
        for folder, code, path in LEAGUES_TM:
            league_root = os.path.join(args.outdir, folder)
            ensure_dir(league_root)
            print(f"\n=== {folder.upper()} (Transfermarkt) ===")

            teams = discover_tm(code, path, s)
            print(f"[INFO] Equipos detectados: {len(teams)}")
            for t in sorted(teams, key=lambda x: x.name.lower()):
                team_dir = os.path.join(league_root, slugify(t.name))
                url = squad_url_tm(t)
                print(f"-> {t.name}: {url}")
                players = scrape_tm_players(url, s)
                if players:
                    write_outputs(team_dir, players, args.write_json, args.write_txt)
                else:
                    print("   [WARN] Sin jugadores (o cambió el HTML).")
                time.sleep(args.sleep + random.random() * 0.6)

        # EnElCamarin (Tercera A/B)
        age_cache: Dict[str, str] = {}

        for div_name, season_url in [
            ("tercera division a", args.tercera_a_url),
            ("tercera division b", args.tercera_b_url),
        ]:
            league_root = os.path.join(args.outdir, div_name)
            ensure_dir(league_root)
            print(f"\n=== {div_name.upper()} (EnElCamarin) ===")
            print(f"Fuente: {season_url}")

            teams = discover_eec(season_url, s)
            print(f"[INFO] Equipos detectados: {len(teams)}")
            for name, url in sorted(teams, key=lambda x: x[0].lower()):
                team_dir = os.path.join(league_root, slugify(name))
                print(f"-> {name}: {url}")
                players = scrape_eec_players(url, s, player_age_cache=age_cache)
                if players:
                    write_outputs(team_dir, players, args.write_json, args.write_txt)
                else:
                    print("   [WARN] Sin jugadores (o cambió el HTML).")
                time.sleep(args.sleep + random.random() * 0.8)

    print("\n[FIN] Generado.")

if __name__ == "__main__":
    main()
