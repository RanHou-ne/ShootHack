param(
    [ValidateSet("Build", "Clean")]
    [string]$Target = "Build",

    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [string]$Platform = "x64",

    [switch]$MaxCpuCount
)

$pf86 = [Environment]::GetFolderPath("ProgramFilesX86")
$vswhere = Join-Path $pf86 "Microsoft Visual Studio\Installer\vswhere.exe"

if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found: $vswhere"
}

$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) {
    throw "MSBuild.exe not found by vswhere"
}

$arguments = @(
    "ShootHack.sln",
    "/t:$Target",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform"
)

if ($MaxCpuCount) {
    $arguments += "/m"
}

& $msbuild @arguments
exit $LASTEXITCODE
