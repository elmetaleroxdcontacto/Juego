param(
    [string]$BasePath = ".\data\LigaChilena",
    [string]$OutputPath = ".\docs\data_cleanup_report.md"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Normalize-Id([string]$Value) {
    $value = $Value.Trim().ToLowerInvariant()
    $value = [regex]::Replace($value, "[^a-z0-9]+", " ").Trim()
    return $value
}

function Read-TeamList([string]$TeamsFile) {
    $ids = @{}
    if (-not (Test-Path $TeamsFile)) { return $ids }
    foreach ($line in Get-Content $TeamsFile) {
        $trimmed = $line.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed) -or $trimmed.StartsWith("#")) { continue }
        $parts = $trimmed.Split('|')
        $display = $parts[0].Trim()
        $folder = if ($parts.Length -gt 1 -and -not [string]::IsNullOrWhiteSpace($parts[1])) { $parts[1].Trim() } else { $display }
        $ids[(Normalize-Id $folder)] = $display
    }
    return $ids
}

$divisions = Get-ChildItem $BasePath -Directory | Sort-Object Name
$lines = @("# Reporte de Carpetas Huerfanas", "", "Generado automaticamente para revisar carpetas de clubes que existen en disco pero no estan listadas en teams.txt.", "")

foreach ($division in $divisions) {
    $teamsFile = Join-Path $division.FullName "teams.txt"
    if (-not (Test-Path $teamsFile)) { continue }
    $listed = Read-TeamList $teamsFile
    $folders = Get-ChildItem $division.FullName -Directory | Where-Object { $_.Name -ne ".git" }
    $orphans = @()
    foreach ($folder in $folders) {
        $id = Normalize-Id $folder.Name
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
