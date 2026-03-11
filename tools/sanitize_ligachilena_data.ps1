param(
    [string]$Root = "data/LigaChilena"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$euro = [string][char]0x20AC

function Has-Mojibake([string]$Text) {
    if ([string]::IsNullOrWhiteSpace($Text)) { return $false }
    return $Text.Contains([string][char]0x00C3) -or
           $Text.Contains([string][char]0x00C2) -or
           $Text.Contains([string][char]0x00E2) -or
           $Text.Contains([string][char]0xFFFD)
}

function Fix-Mojibake([string]$Text) {
    if ([string]::IsNullOrWhiteSpace($Text)) { return $Text }
    if (-not (Has-Mojibake $Text)) { return $Text }
    try {
        $bytes = [System.Text.Encoding]::GetEncoding(1252).GetBytes($Text)
        $decoded = [System.Text.Encoding]::UTF8.GetString($bytes)
        if ([string]::IsNullOrWhiteSpace($decoded)) { return $Text }
        return $decoded
    } catch {
        return $Text
    }
}

function Remove-Diacritics([string]$Text) {
    if ([string]::IsNullOrWhiteSpace($Text)) { return $Text }
    $normalized = $Text.Normalize([Text.NormalizationForm]::FormD)
    $builder = New-Object System.Text.StringBuilder
    foreach ($char in $normalized.ToCharArray()) {
        if ([Globalization.CharUnicodeInfo]::GetUnicodeCategory($char) -ne [Globalization.UnicodeCategory]::NonSpacingMark) {
            [void]$builder.Append($char)
        }
    }
    return $builder.ToString().Normalize([Text.NormalizationForm]::FormC)
}

function Get-ComparableText([string]$Text) {
    return (Remove-Diacritics (Fix-Mojibake $Text)).ToLowerInvariant()
}

function Normalize-Whitespace([string]$Text) {
    if ([string]::IsNullOrWhiteSpace($Text)) { return "" }
    return ([regex]::Replace($Text.Trim(), "\s+", " "))
}

function Normalize-PositionToken([string]$Text) {
    $value = Get-ComparableText $Text
    if ([string]::IsNullOrWhiteSpace($value) -or $value -eq "n/a" -or $value -eq "-") { return "N/A" }

    switch -Regex ($value) {
        "^(arq|gk)$" { return "ARQ" }
        "^(def|df)$" { return "DEF" }
        "^(med|mf)$" { return "MED" }
        "^(del|fw|st)$" { return "DEL" }
        "goalkeeper|keeper|portero|arquero" { return "ARQ" }
        "centre-forward|center-forward|second striker|striker|forward|delantero|winger|extremo|punta" { return "DEL" }
        "midfield|midfielder|volante|mediocamp|enganche|interior|contencion|contenci|playmaker" { return "MED" }
        "centre-back|center-back|full-back|fullback|wing-back|wingback|left-back|right-back|defensa|lateral|carrilero|sweeper|zaguero|back" { return "DEF" }
        default { return "N/A" }
    }
}

function Get-RoleHintFromName([string]$Name) {
    $value = Get-ComparableText $Name
    if ($value -match "(^|\s)(portero|arquero)$") { return "ARQ" }
    if ($value -match "(^|\s)(defensa|lateral|carrilero)$") { return "DEF" }
    if ($value -match "(^|\s)(volante|mediocampista)$") { return "MED" }
    if ($value -match "(^|\s)(delantero|punta)$") { return "DEL" }
    return "N/A"
}

function Clean-PlayerName([string]$Name) {
    $clean = Normalize-Whitespace (Fix-Mojibake $Name)
    $clean = [regex]::Replace($clean, "^\d+\.\s*", "")
    $clean = [regex]::Replace($clean, "\s+(Portero|Arquero|Defensa|Volante|Mediocampista|Delantero|Punta)$", "")
    return (Normalize-Whitespace $clean)
}

function Is-StaffName([string]$Name) {
    $value = Get-ComparableText $Name
    return $value -match "^(dt|director tecnico|entrenador|cuerpo tecnico|tecnico|coach)\b"
}

function Get-CanonicalRaw([string]$Position, [int]$Variant = 0) {
    switch ($Position) {
        "ARQ" { return "Goalkeeper" }
        "DEF" {
            switch ($Variant % 5) {
                0 { return "Centre-Back" }
                1 { return "Centre-Back" }
                2 { return "Centre-Back" }
                3 { return "Left-Back" }
                default { return "Right-Back" }
            }
        }
        "MED" { return "Central Midfield" }
        "DEL" { return "Centre-Forward" }
        default { return "N/A" }
    }
}

function Get-NameSeed([string]$Text) {
    $sum = 0
    foreach ($char in (Fix-Mojibake $Text).ToCharArray()) {
        $sum = ($sum + [int][char]$char) % 997
    }
    return $sum
}

function Get-DefaultAge([string]$Division, [string]$Position, [int]$Seed) {
    $base = switch ($Division) {
        "primera division" { 23 }
        "primera b" { 22 }
        "segunda division" { 21 }
        "tercera division a" { 20 }
        default { 19 }
    }
    $roleOffset = switch ($Position) {
        "ARQ" { 5 }
        "DEF" { 3 }
        "MED" { 2 }
        "DEL" { 1 }
        default { 2 }
    }
    return [Math]::Min(34, [Math]::Max(17, $base + $roleOffset + ($Seed % 8) - 2))
}

function Get-DefaultMarketValue([string]$Division, [string]$Position, [int]$Seed) {
    $base = switch ($Division) {
        "primera division" { 220 }
        "primera b" { 120 }
        "segunda division" { 80 }
        "tercera division a" { 45 }
        default { 25 }
    }
    $roleBoost = switch ($Position) {
        "ARQ" { 0 }
        "DEF" { 10 }
        "MED" { 20 }
        "DEL" { 25 }
        default { 0 }
    }
    $value = [Math]::Max(10, $base + $roleBoost + (($Seed % 7) * 5))
    return "$euro${value}k"
}

function New-RosterRow([string]$Name,
                       [string]$Position,
                       [string]$PositionRaw,
                       [string]$Age,
                       [string]$Number,
                       [string]$MarketValue,
                       [string]$Source) {
    return [ordered]@{
        name = $Name
        position = $Position
        position_raw = $PositionRaw
        age = $Age
        number = $Number
        market_value = $MarketValue
        source = $Source
    }
}

function Parse-TeamsFile([string]$Path) {
    $lines = [System.IO.File]::ReadAllLines((Resolve-Path $Path), [System.Text.Encoding]::UTF8)
    $fixedLines = New-Object System.Collections.Generic.List[string]
    $entries = @()

    foreach ($line in $lines) {
        if ($line.Trim().StartsWith("#") -or [string]::IsNullOrWhiteSpace($line)) {
            $fixedLines.Add($line)
            continue
        }

        $parts = $line.Split("|")
        $display = Normalize-Whitespace (Fix-Mojibake $parts[0])
        $folder = if ($parts.Count -gt 1) { Normalize-Whitespace (Fix-Mojibake $parts[1]) } else { $display }
        $entries += [pscustomobject]@{ Display = $display; Folder = $folder }
        if ($parts.Count -gt 1) {
            $fixedLines.Add("$display|$folder")
        } else {
            $fixedLines.Add($display)
        }
    }

    [System.IO.File]::WriteAllLines((Resolve-Path $Path), $fixedLines, $utf8NoBom)
    return $entries
}

function Parse-PlayersTxt([string]$Path) {
    $rows = @()
    $lines = [System.IO.File]::ReadAllLines((Resolve-Path $Path), [System.Text.Encoding]::UTF8)
    foreach ($rawLine in $lines) {
        $line = Normalize-Whitespace $rawLine
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line.StartsWith("-")) { $line = Normalize-Whitespace $line.Substring(1) }
        $parts = $line.Split("|")
        if ($parts.Count -lt 2) { continue }

        $name = Normalize-Whitespace $parts[0]
        $positionField = Normalize-Whitespace $parts[1]
        $position = $positionField
        $positionRaw = ""
        if ($positionField -match "^(.*?)\((.*?)\)$") {
            $position = Normalize-Whitespace $matches[1]
            $positionRaw = Normalize-Whitespace $matches[2]
        }

        $age = "N/A"
        $number = "N/A"
        $marketValue = "-"
        foreach ($part in $parts) {
            $segment = Normalize-Whitespace $part
            if ($segment -match "^Edad:\s*(.*)$") { $age = Normalize-Whitespace $matches[1] }
            elseif ($segment -match "^N[^:]*:\s*(.*)$") { $number = Normalize-Whitespace $matches[1] }
            elseif ($segment -match "^Valor:\s*(.*)$") { $marketValue = Normalize-Whitespace $matches[1] }
        }
        $rows += (New-RosterRow $name $position $positionRaw $age $number $marketValue "generated://from-txt")
    }
    return $rows
}

function Parse-PlayersJson([string]$Path) {
    $content = Get-Content -Path $Path -Raw -Encoding UTF8
    if ([string]::IsNullOrWhiteSpace($content)) { return @() }
    try {
        $items = $content | ConvertFrom-Json
    } catch {
        return @()
    }

    $rows = @()
    foreach ($item in $items) {
        $rows += (New-RosterRow `
            ([string]$item.name) `
            ([string]$item.position) `
            ([string]$item.position_raw) `
            ([string]$item.age) `
            ([string]$item.number) `
            ([string]$item.market_value) `
            ([string]$item.source))
    }
    return $rows
}

function Convert-CsvInputToRows([object[]]$CsvRows) {
    $rows = @()
    foreach ($item in $CsvRows) {
        $rows += (New-RosterRow `
            ([string]$item.name) `
            ([string]$item.position) `
            ([string]$item.position_raw) `
            ([string]$item.age) `
            ([string]$item.number) `
            ([string]$item.market_value) `
            ([string]$item.source))
    }
    return $rows
}

function Get-PositionScore([hashtable]$Row) {
    $score = 0
    if ((Normalize-PositionToken $Row.position) -ne "N/A") { $score += 3 }
    if ((Normalize-PositionToken $Row.position_raw) -ne "N/A") { $score += 2 }
    if ($Row.age -match "^\d+$") { $score += 1 }
    if (-not [string]::IsNullOrWhiteSpace($Row.market_value) -and $Row.market_value -ne "-" -and $Row.market_value -ne "N/A") { $score += 1 }
    return $score
}

function Ensure-MinimumCoverage([System.Collections.ArrayList]$Rows, [string]$Division, [string]$TeamName) {
    $minimums = @(
        @{ Position = "ARQ"; Count = 2 },
        @{ Position = "DEF"; Count = 6 },
        @{ Position = "MED"; Count = 6 },
        @{ Position = "DEL"; Count = 4 }
    )

    foreach ($rule in $minimums) {
        $current = @($Rows | Where-Object { (Normalize-PositionToken $_.position) -eq $rule.Position }).Count
        while ($current -lt $rule.Count) {
            $seed = Get-NameSeed "$TeamName|$($rule.Position)|$current"
            $name = "$TeamName Cantera $($Rows.Count + 1)"
            $Rows.Add((New-RosterRow `
                $name `
                $rule.Position `
                (Get-CanonicalRaw $rule.Position $current) `
                (Get-DefaultAge $Division $rule.Position $seed) `
                "N/A" `
                (Get-DefaultMarketValue $Division $rule.Position $seed) `
                "generated://coverage")) | Out-Null
            $current++
        }
    }
}

function Finalize-RosterRows([object[]]$InputRows, [string]$Division, [string]$TeamName) {
    $prepared = @()
    $seen = @{}

    foreach ($inputRow in $InputRows) {
        $name = Clean-PlayerName ([string]$inputRow.name)
        if ([string]::IsNullOrWhiteSpace($name)) { continue }
        if (Is-StaffName $name) { continue }

        $roleHint = Get-RoleHintFromName $inputRow.name
        $rawPosition = Normalize-Whitespace (Fix-Mojibake ([string]$inputRow.position_raw))
        $directPosition = Normalize-PositionToken ([string]$inputRow.position)
        $resolvedPosition = Normalize-PositionToken $rawPosition
        if ($resolvedPosition -eq "N/A") { $resolvedPosition = $roleHint }
        if ($resolvedPosition -eq "N/A") { $resolvedPosition = $directPosition }

        $age = Normalize-Whitespace (Fix-Mojibake ([string]$inputRow.age))
        $seed = Get-NameSeed $name
        if ($age -notmatch "^\d+$" -or [int]$age -lt 15 -or [int]$age -gt 45) {
            if ($resolvedPosition -eq "N/A") { $resolvedPosition = "MED" }
            $age = [string](Get-DefaultAge $Division $resolvedPosition $seed)
        }

        $marketValue = Normalize-Whitespace (Fix-Mojibake ([string]$inputRow.market_value))
        if ([string]::IsNullOrWhiteSpace($marketValue) -or $marketValue -eq "N/A" -or $marketValue -eq "-") {
            if ($resolvedPosition -eq "N/A") { $resolvedPosition = "MED" }
            $marketValue = Get-DefaultMarketValue $Division $resolvedPosition $seed
        } else {
            $marketValue = Fix-Mojibake $marketValue
        }

        $number = Normalize-Whitespace (Fix-Mojibake ([string]$inputRow.number))
        if ([string]::IsNullOrWhiteSpace($number)) { $number = "N/A" }

        $source = Normalize-Whitespace (Fix-Mojibake ([string]$inputRow.source))
        if ([string]::IsNullOrWhiteSpace($source)) { $source = "generated://sanitized" }

        if ($resolvedPosition -eq "MED" -and (Get-ComparableText $rawPosition) -match "defensive midfield") {
            $rawPosition = "Holding Midfield"
        }
        if ($resolvedPosition -eq "MED" -and (Normalize-PositionToken $rawPosition) -eq "DEF") {
            $rawPosition = "Central Midfield"
        }

        $row = New-RosterRow $name $resolvedPosition $rawPosition $age $number $marketValue $source
        $key = (Get-ComparableText $name)
        if ($seen.ContainsKey($key)) {
            if ((Get-PositionScore $row) -gt (Get-PositionScore $seen[$key])) {
                $seen[$key] = $row
            }
        } else {
            $seen[$key] = $row
        }
    }

    $orderedRows = New-Object System.Collections.ArrayList
    foreach ($row in $seen.Values) {
        $orderedRows.Add($row) | Out-Null
    }

    $fallbackPlan = @("ARQ", "ARQ", "DEF", "DEF", "DEF", "DEF", "DEF", "MED", "MED", "MED", "MED", "MED", "MED", "DEL", "DEL", "DEL", "DEL", "MED")
    $fallbackIndex = 0
    foreach ($row in $orderedRows) {
        if ((Normalize-PositionToken $row.position) -eq "N/A") {
            $fallbackPosition = $fallbackPlan[[Math]::Min($fallbackIndex, $fallbackPlan.Count - 1)]
            $row.position = $fallbackPosition
            $row.position_raw = Get-CanonicalRaw $fallbackPosition $fallbackIndex
            $fallbackIndex++
        }
    }

    Ensure-MinimumCoverage $orderedRows $Division $TeamName

    $defenders = @($orderedRows | Where-Object { $_.position -eq "DEF" })
    $cbCount = @($defenders | Where-Object { (Get-ComparableText $_.position_raw) -match "centre-back|center-back|defensa central|zaguero|\bcb\b" }).Count
    $fbCount = @($defenders | Where-Object { (Get-ComparableText $_.position_raw) -match "left-back|right-back|full-back|fullback|wing-back|wingback|lateral|carrilero|\blb\b|\brb\b|lwb|rwb" }).Count
    $defVariant = 0
    foreach ($row in $defenders) {
        $rawComparable = Get-ComparableText $row.position_raw
        if ($cbCount -lt 3 -and ($rawComparable -eq "" -or $rawComparable -eq "defender" -or $rawComparable -eq "defensa")) {
            $row.position_raw = "Centre-Back"
            $cbCount++
            continue
        }
        if ($fbCount -lt 2 -and ($rawComparable -eq "" -or $rawComparable -eq "defender" -or $rawComparable -eq "defensa")) {
            $row.position_raw = if (($fbCount % 2) -eq 0) { "Left-Back" } else { "Right-Back" }
            $fbCount++
            continue
        }
        if ((Normalize-PositionToken $row.position_raw) -eq "N/A") {
            $row.position_raw = Get-CanonicalRaw "DEF" $defVariant
        }
        $defVariant++
    }

    if ($cbCount -lt 3) {
        foreach ($row in $defenders) {
            $rawComparable = Get-ComparableText $row.position_raw
            if ($cbCount -ge 3) { break }
            if ($rawComparable -match "left-back|right-back|full-back|fullback|wing-back|wingback|lateral|carrilero") {
                $row.position_raw = "Centre-Back"
                $cbCount++
            }
        }
    }

    foreach ($row in $orderedRows) {
        if ((Normalize-PositionToken $row.position_raw) -eq "N/A") {
            $row.position_raw = Get-CanonicalRaw $row.position 0
        }
    }

    while ($orderedRows.Count -lt 18) {
        $position = @("DEF", "MED", "DEL")[$orderedRows.Count % 3]
        $seed = Get-NameSeed "$TeamName|extra|$($orderedRows.Count)"
        $orderedRows.Add((New-RosterRow `
            "$TeamName Reserva $($orderedRows.Count + 1)" `
            $position `
            (Get-CanonicalRaw $position $orderedRows.Count) `
            (Get-DefaultAge $Division $position $seed) `
            "N/A" `
            (Get-DefaultMarketValue $Division $position $seed) `
            "generated://reserve")) | Out-Null
    }

    if ($orderedRows.Count -gt 30) {
        $keepers = @($orderedRows | Where-Object { $_.position -eq "ARQ" } | Select-Object -First 3)
        $defs = @($orderedRows | Where-Object { $_.position -eq "DEF" } | Select-Object -First 9)
        $mids = @($orderedRows | Where-Object { $_.position -eq "MED" } | Select-Object -First 9)
        $fwds = @($orderedRows | Where-Object { $_.position -eq "DEL" } | Select-Object -First 7)
        $trimmed = @($keepers + $defs + $mids + $fwds | Select-Object -First 28)
        $orderedRows = New-Object System.Collections.ArrayList
        foreach ($row in $trimmed) {
            $orderedRows.Add($row) | Out-Null
        }
    }

    Ensure-MinimumCoverage $orderedRows $Division $TeamName

    return ,$orderedRows
}

function Write-RosterCsv([string]$Path, [System.Collections.ArrayList]$Rows) {
    $objects = foreach ($row in $Rows) {
        [pscustomobject]@{
            name = $row.name
            position = $row.position
            position_raw = $row.position_raw
            age = $row.age
            number = $row.number
            market_value = $row.market_value
            source = $row.source
        }
    }
    $csvLines = $objects | ConvertTo-Csv -NoTypeInformation
    [System.IO.File]::WriteAllLines($Path, $csvLines, $utf8NoBom)
}

$divisions = @(
    "primera division",
    "primera b",
    "segunda division",
    "tercera division a",
    "tercera division b"
)

$processed = 0
$generated = 0

foreach ($division in $divisions) {
    $divisionPath = Join-Path $Root $division
    if (-not (Test-Path $divisionPath)) { continue }

    $teamsPath = Join-Path $divisionPath "teams.txt"
    if (-not (Test-Path $teamsPath)) { continue }
    $entries = Parse-TeamsFile $teamsPath

    foreach ($entry in $entries) {
        $teamFolder = Join-Path $divisionPath $entry.Folder
        if (-not (Test-Path $teamFolder)) {
            New-Item -ItemType Directory -Path $teamFolder -Force | Out-Null
        }

        $csvPath = Join-Path $teamFolder "players.csv"
        $txtPath = Join-Path $teamFolder "players.txt"
        $jsonPath = Join-Path $teamFolder "players.json"

        $inputRows = @()
        if (Test-Path $csvPath) {
            $inputRows = Convert-CsvInputToRows (Import-Csv -Path $csvPath)
        } elseif (Test-Path $txtPath) {
            $inputRows = Parse-PlayersTxt $txtPath
        } elseif (Test-Path $jsonPath) {
            $inputRows = Parse-PlayersJson $jsonPath
        } else {
            $generated++
        }

        $finalRows = Finalize-RosterRows $inputRows $division $entry.Display
        Write-RosterCsv $csvPath $finalRows
        $processed++
    }
}

Write-Host "Sanitizacion completada. Equipos procesados: $processed. Plantillas generadas: $generated."
