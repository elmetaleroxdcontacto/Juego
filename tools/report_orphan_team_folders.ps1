param(
    [string]$BasePath = ".\data\LigaChilena",
    [string]$OutputPath = ".\docs\data_cleanup_report.md",
    [string]$IgnorePath = ".\data\configs\ignored_team_folders.csv"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Convert-Id([string]$Value) {
    $value = $Value.Trim().ToLowerInvariant()
    $value = [regex]::Replace($value, "[^a-z0-9]+", " ").Trim()
    return $value
}

function Read-IgnoredFolders([string]$Path) {
    $ids = @{}
    if (-not (Test-Path $Path)) { return $ids }
    foreach ($row in Import-Csv -Path $Path -Encoding UTF8) {
        if (-not $row.division -or -not $row.folder) { continue }
        $key = ((Convert-Id $row.division) + "|" + (Convert-Id $row.folder))
        $ids[$key] = $true
    }
    return $ids
}

function Read-TeamList([string]$TeamsFile) {
    $ids = @{}
    if (-not (Test-Path $TeamsFile)) { return $ids }
    foreach ($line in Get-Content -Path $TeamsFile -Encoding UTF8) {
        $trimmed = $line.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed) -or $trimmed.StartsWith("#")) { continue }
        $parts = $trimmed -split '\|', 2
        $display = $parts[0].Trim()
        $folder = if ($parts.Count -gt 1 -and -not [string]::IsNullOrWhiteSpace($parts[1])) { $parts[1].Trim() } else { $display }
        $ids[(Convert-Id $folder)] = $display
    }
    return $ids
}

$ignoredFolders = Read-IgnoredFolders $IgnorePath
$divisions = Get-ChildItem $BasePath -Directory | Sort-Object Name
$lines = @("# Reporte de Carpetas Huerfanas", "", "Generado automaticamente para revisar carpetas de clubes que existen en disco pero no estan listadas en teams.txt.", "")

foreach ($division in $divisions) {
    $teamsFile = Join-Path $division.FullName "teams.txt"
    if (-not (Test-Path $teamsFile)) { continue }
    $listed = Read-TeamList $teamsFile
    $folders = Get-ChildItem $division.FullName -Directory | Where-Object { $_.Name -ne ".git" }
    $orphans = @()
    foreach ($folder in $folders) {
        $id = Convert-Id $folder.Name
        $ignoreKey = (Convert-Id $division.Name) + "|" + $id
        if ($ignoredFolders.ContainsKey($ignoreKey)) { continue }
        if (-not $listed.ContainsKey($id)) { $orphans += $folder.Name }
    }
    $lines += "## $($division.Name)"
    if ($orphans.Count -eq 0) {
        $lines += "- Sin carpetas huerfanas detectadas."
    } else {
        $lines += "- Carpetas no listadas en teams.txt: $($orphans.Count)"
        foreach ($name in $orphans | Sort-Object) {
            $lines += "  - $name"
        }
    }
    $lines += ""
}

$directory = Split-Path $OutputPath -Parent
if ($directory -and -not (Test-Path $directory)) {
    New-Item -ItemType Directory -Path $directory | Out-Null
}
Set-Content -Path $OutputPath -Value $lines -Encoding UTF8
Write-Host "Reporte escrito en $OutputPath"
