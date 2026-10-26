param(
    [ValidateSet('Debug', 'Release', 'Both')]
    [string]$Configuration = 'Both'
)

$ErrorActionPreference = 'Stop'
$repoPath = $PSScriptRoot
$cmakePath = (Get-Command cmake -ErrorAction Stop).Source
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswherePath)) {
    throw 'Install Visual Studio 2026 with Desktop development with C++ first.'
}

$installations = @(& $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json)
if ($installations.Count -eq 0) {
    throw 'No Visual Studio installation with the C++ toolchain was found.'
}
$visualStudioMajor = ([version]$installations[0].installationVersion).Major
if ($visualStudioMajor -ne 18) {
    throw 'This solution is configured for Visual Studio 2026 (v145, x64).'
}

$dependencyBuildPath = Join-Path $repoPath 'build\dependencies'
& $cmakePath -S (Join-Path $repoPath 'dependencies') -B $dependencyBuildPath -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'Dependency configuration failed.' }

$configurations = @($Configuration)
if ($Configuration -eq 'Both') { $configurations = @('Debug', 'Release') }
foreach ($buildConfiguration in $configurations) {
    & $cmakePath --build $dependencyBuildPath --config $buildConfiguration --target sfml-graphics --parallel 4
    if ($LASTEXITCODE -ne 0) { throw "Dependency build failed: $buildConfiguration" }
}

Write-Output 'Dependencies are ready. Open NXRP.slnx, choose Debug/x64 and run Maze.Graphics with F5.'
