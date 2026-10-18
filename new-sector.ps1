param(
    [string]$Name = '',
    [string]$From = 'example:relay',
    [string]$Campaign = 'example:two_sectors'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Join-Path $PSScriptRoot 'content'
$validator = Join-Path $PSScriptRoot 'starfall-content.exe'
if (!(Test-Path -LiteralPath $validator)) { throw 'Build first and run this script from the generated package folder.' }
if (!$Name) { $Name = Read-Host 'New sector ID suffix (example: my_sector)' }
if ($Name -cnotmatch '^[a-z0-9_-]{1,32}$') { throw 'Use 1-32 lowercase ASCII letters, digits, underscores or hyphens.' }
if ($Campaign -cnotmatch '^[a-z0-9_-]{1,48}:[a-z0-9_-]{1,48}$') { throw 'Invalid campaign ID.' }
$pack = $Campaign.Split(':')[0]
$id = "$($pack):$Name"
& $validator '--content-root' $root '--validate-content'
if ($LASTEXITCODE -ne 0) { throw 'Existing content is invalid. Nothing was changed.' }
$config = Get-Content -LiteralPath (Join-Path $root 'game.json') -Raw -Encoding UTF8 | ConvertFrom-Json
function Find-Definition([string]$Category, [string]$Id) {
    $matches = @()
    foreach ($enabled in $config.packs) {
        $folder = Join-Path $root "$enabled\$Category"
        if (!(Test-Path -LiteralPath $folder)) { continue }
        foreach ($file in Get-ChildItem -LiteralPath $folder -Filter '*.json' -File) {
            $definition = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8 | ConvertFrom-Json
            if ($definition.id -ceq $Id) { $matches += [PSCustomObject]@{ File = $file.FullName; Data = $definition } }
        }
    }
    if ($matches.Count -gt 1) { throw "Duplicate ID: $Id" }
    if ($matches.Count -eq 1) { return $matches[0] }
    return $null
}
$source = Find-Definition 'sectors' $From
$route = Find-Definition 'campaigns' $Campaign
if ($null -eq $source) { throw "Unknown source sector: $From" }
if ($null -eq $route) { throw "Unknown campaign: $Campaign" }
if ($null -ne (Find-Definition 'sectors' $id)) { throw "Sector already exists: $id. Nothing was changed." }
$folder = Join-Path $root "$pack\sectors"
[IO.Directory]::CreateDirectory($folder) | Out-Null
$destination = Join-Path $folder "$Name.json"
if (Test-Path -LiteralPath $destination) { throw "File already exists: $destination" }
$source.Data.id = $id
$source.Data.name = $Name.Replace('_', ' ').ToUpperInvariant()
$route.Data.sectors = @($route.Data.sectors) + @($id)
$utf8 = New-Object Text.UTF8Encoding($false)
$sectorBytes = $utf8.GetBytes(($source.Data | ConvertTo-Json -Depth 32) + "`n")
$campaignBytes = $utf8.GetBytes(($route.Data | ConvertTo-Json -Depth 32) + "`n")
$oldCampaign = [IO.File]::ReadAllBytes($route.File)
$tag = [Guid]::NewGuid().ToString('N')
$backup = "$($route.File).before-$tag.bak"
$temp = "$($route.File).$tag.tmp"
[IO.File]::WriteAllBytes($backup, $oldCampaign)
$sectorCreated = $false
$campaignChanged = $false
try {
    # CreateNew refuses to overwrite any file that appeared since our existence check.
    $stream = [IO.File]::Open($destination, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write)
    try { $stream.Write($sectorBytes, 0, $sectorBytes.Length) } finally { $stream.Dispose() }
    $sectorCreated = $true
    [IO.File]::WriteAllBytes($temp, $campaignBytes)
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes($route.File)) -cne [Convert]::ToBase64String($oldCampaign)) {
        throw 'Campaign changed during creation. Stop editing and retry with a new sector ID.'
    }
    Move-Item -LiteralPath $temp -Destination $route.File -Force
    $campaignChanged = $true
    & $validator '--content-root' $root '--validate-content'
    if ($LASTEXITCODE -ne 0) { throw 'Validation of the new content failed.' }
    Write-Host "CREATED: $id"
    Write-Host "CAMPAIGN: $Campaign"
    Write-Host "EDIT: $destination"
    Write-Host "BACKUP: $backup"
    Write-Host "TEST: .\starfall.exe --sector $id"
} catch {
    # Restore only files whose bytes are still exactly what this script wrote.
    if ($campaignChanged -and [Convert]::ToBase64String([IO.File]::ReadAllBytes($route.File)) -ceq [Convert]::ToBase64String($campaignBytes)) {
        [IO.File]::WriteAllBytes($route.File, $oldCampaign)
    }
    if ($sectorCreated -and (Test-Path -LiteralPath $destination) -and [Convert]::ToBase64String([IO.File]::ReadAllBytes($destination)) -ceq [Convert]::ToBase64String($sectorBytes)) {
        Remove-Item -LiteralPath $destination
    }
    throw
} finally {
    if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp }
}
