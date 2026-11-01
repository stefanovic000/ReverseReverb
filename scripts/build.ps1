param([ValidateSet('Debug','Release')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$juceFolder = Join-Path $projectRoot '.dependencies\JUCE'
if (-not (Test-Path -LiteralPath (Join-Path $juceFolder 'CMakeLists.txt'))) {
    & git -c http.sslBackend=openssl clone --depth 1 --branch 8.0.16-security https://github.com/juce-framework/JUCE.git $juceFolder
    if ($LASTEXITCODE -ne 0) { throw 'Could not download JUCE.' }
}
$vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vsWhere)) { throw 'Visual Studio with Desktop development with C++ is required.' }
$vsInstall = & $vsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstall) { throw 'Install the Visual Studio Desktop development with C++ workload.' }
$cmakePath = Join-Path $vsInstall 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ninjaPath = Join-Path $vsInstall 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$vsDevCmd = Join-Path $vsInstall 'Common7\Tools\VsDevCmd.bat'
$buildPath = Join-Path $projectRoot "build\msvc-$Configuration"
# Import the official compiler environment. Paths are quoted, never executed from env output.
$envCommand = '"' + $vsDevCmd + '" -no_logo -arch=x64 >nul && set'
$envLines = & $env:ComSpec /d /s /c $envCommand
if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the Visual C++ compiler environment.' }
foreach ($line in $envLines) {
    if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
}
& $cmakePath -S $projectRoot -B $buildPath -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninjaPath" "-DCMAKE_BUILD_TYPE=$Configuration" -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmakePath --build $buildPath --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
& $cmakePath --build $buildPath --target test
if ($LASTEXITCODE -ne 0) { throw 'Checks failed.' }
Write-Output "Built plugin: $buildPath\ReverseReverb_artefacts\$Configuration\VST3\ReverseReverb.vst3"
Write-Output "Standalone: $buildPath\ReverseReverb_artefacts\$Configuration\Standalone\ReverseReverb.exe"
