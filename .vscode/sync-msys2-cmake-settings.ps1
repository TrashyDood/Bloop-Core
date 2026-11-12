param(
    [Parameter(Mandatory = $true)]
    [string]$Msys2Root
)

$ErrorActionPreference = "Stop"

$Msys2Root = $Msys2Root.TrimEnd('\', '/')
$cmakePath = Join-Path $Msys2Root "ucrt64\bin\cmake.exe"
$ucrt64Bin = Join-Path $Msys2Root "ucrt64\bin"

if (-not (Test-Path -LiteralPath $cmakePath -PathType Leaf)) {
    throw "CMake was not found at '$cmakePath'. Check the msys2.root VS Code setting."
}

$settingsPath = Join-Path $PSScriptRoot "settings.json"
$settings = [System.IO.File]::ReadAllText($settingsPath)
$cmakePathPattern = '(?m)^(\s*"cmake\.cmakePath"\s*:\s*)"(?:\\.|[^"\\])*"'
$environmentPathPattern = '(?ms)("cmake\.environment"\s*:\s*\{.*?^\s*"PATH"\s*:\s*)"(?:\\.|[^"\\])*"'

$cmakePathMatch = [regex]::Match($settings, $cmakePathPattern)
$environmentPathMatch = [regex]::Match($settings, $environmentPathPattern)

if (-not $cmakePathMatch.Success -or -not $environmentPathMatch.Success) {
    throw "Could not find cmake.cmakePath and cmake.environment.PATH in '$settingsPath'."
}

$newCmakePath = ConvertTo-Json -InputObject $cmakePath -Compress
$newEnvironmentPath = ConvertTo-Json -InputObject "$ucrt64Bin;`${env:PATH}" -Compress
$updatedSettings = [regex]::Replace(
    $settings,
    $cmakePathPattern,
    { param($match) $match.Groups[1].Value + $newCmakePath },
    1
)
$updatedSettings = [regex]::Replace(
    $updatedSettings,
    $environmentPathPattern,
    { param($match) $match.Groups[1].Value + $newEnvironmentPath },
    1
)

if ($updatedSettings -ne $settings) {
    $utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($settingsPath, $updatedSettings, $utf8WithoutBom)
    Write-Output "Updated CMake Tools paths in '$settingsPath'."
} else {
    Write-Output "CMake Tools paths already match msys2.root."
}
