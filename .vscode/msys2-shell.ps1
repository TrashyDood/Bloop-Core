param(
    [string]$Command,
    [switch]$Interactive
)

$ErrorActionPreference = "Stop"

$launcher = $null
foreach ($pathEntry in ($env:PATH -split [IO.Path]::PathSeparator)) {
    if ([string]::IsNullOrWhiteSpace($pathEntry)) {
        continue
    }

    $binDirectory = [IO.Path]::GetFullPath($pathEntry.Trim().Trim('"'))
    $ucrt64Directory = Split-Path -Parent $binDirectory
    if ((Split-Path -Leaf $binDirectory) -ne "bin" -or
        (Split-Path -Leaf $ucrt64Directory) -ne "ucrt64") {
        continue
    }

    $candidate = Join-Path (Split-Path -Parent $ucrt64Directory) "msys2_shell.cmd"
    if (Test-Path -LiteralPath $candidate -PathType Leaf) {
        $launcher = $candidate
        break
    }
}

if ($null -eq $launcher) {
    Write-Error "Could not locate MSYS2 from PATH. Add its ucrt64\bin directory to PATH."
    exit 1
}

$launcherArguments = @("-ucrt64", "-defterm", "-no-start", "-here")
if ($Interactive) {
    $launcherArguments += "-i"
}
elseif (-not [string]::IsNullOrWhiteSpace($Command)) {
    $launcherArguments += @("-c", $Command)
}
else {
    Write-Error "Provide a command or use -Interactive."
    exit 1
}

& $launcher @launcherArguments
exit $LASTEXITCODE
