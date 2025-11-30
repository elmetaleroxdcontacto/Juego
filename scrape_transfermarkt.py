import requests
from bs4 import BeautifulSoup
import re

def scrape_team_players(team_url):
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36'
    }
    response = requests.get(team_url, headers=headers)
    if response.status_code != 200:
        print(f"Failed to retrieve page: {team_url}")
        return []

    soup = BeautifulSoup(response.text, 'html.parser')

    players = []
    # Try to find player rows in different possible table structures
    player_rows = soup.select('table.items tbody tr')
    if not player_rows:
        # Alternative selector if page structure changed
        player_rows = soup.select('div.responsive-table tbody tr')

    for row in player_rows:
        # Skip rows that are just separators or empty
        if 'class' in row.attrs and ('odd' not in row['class'] and 'even' not in row['class']):
            continue

        # Try different selectors for player name
        name_tag = row.select_one('td.posrela a.spielprofil_tooltip')
        if not name_tag:
            name_tag = row.select_one('td.hauptlink a')
        if not name_tag:
            continue
        name = name_tag.text.strip()

        # Try different selectors for position
        position_tag = row.select_one('td.posrela')
        if not position_tag:
            position_tag = row.select_one('td.zentriert:nth-of-type(2)')
        position = position_tag.text.strip() if position_tag else 'N/A'

        # Extract age
        age_tag = row.select_one('td.zentriert:nth-of-type(6)')
        if not age_tag:
            age_tag = row.select_one('td.zentriert:nth-of-type(3)')
        age = age_tag.text.strip() if age_tag else 'N/A'

        # Extract value
        value_tag = row.select_one('td.rechts.hauptlink')
        if not value_tag:
            value_tag = row.select_one('td.rechts')
        value = value_tag.text.strip() if value_tag else 'N/A'

        # For Attack, Defense, Stamina, Skill, we will set as 0 or N/A since not available
        player = {
            'Name': name,
            'Position': position,
            'Attack': 0,
            'Defense': 0,
            'Stamina': 0,
            'Skill': 0,
            'Age': age,
            'Value': value
        }
        players.append(player)

    return players

def format_player_data(players):
    lines = []
    for p in players:
        line = f"- Name: {p['Name']}, Position: {p['Position']}, Attack: {p['Attack']}, Defense: {p['Defense']}, Stamina: {p['Stamina']}, Skill: {p['Skill']}, Age: {p['Age']}, Value: {p['Value']}"
        lines.append(line)
    return lines

def main():
    # Example team URL for Audax Italiano 2025 squad on Transfermarkt
    team_url = input("Enter Transfermarkt team URL for 2025 squad: ").strip()
    if not team_url.startswith("http"):
        print("Invalid URL. Please enter a full URL starting with http or https.")
        return
    players = scrape_team_players(team_url)
    if not players:
        print("No players found or failed to scrape.")
        return

    print("Team: Extracted Team")
    print("Players:")
    for line in format_player_data(players):
        print(line)

if __name__ == "__main__":
    main()
