param([ValidateSet('Debug','Release')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$bundlePath = Join-Path $projectRoot "build\msvc-$Configuration\ReverseReverb_artefacts\$Configuration\VST3\ReverseReverb.vst3"
$binaryPath = Join-Path $bundlePath 'Contents\x86_64-win\ReverseReverb.vst3'
if (-not (Test-Path -LiteralPath $binaryPath -PathType Leaf)) { throw 'Build the plugin with scripts/build.ps1 first.' }
$destination = [IO.Path]::GetFullPath((Join-Path $env:CommonProgramFiles 'VST3\ReverseReverb.vst3'))
Write-Output "Installing this project's VST3 bundle to: $destination"
if (Test-Path -LiteralPath $destination -PathType Leaf) { throw 'A file occupies the destination. No changes made.' }
$installedBinary = Join-Path $destination 'Contents\x86_64-win\ReverseReverb.vst3'
if (Test-Path -LiteralPath $installedBinary -PathType Leaf) {
    try {
        $fileProbe = [IO.File]::Open($installedBinary, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
        $fileProbe.Dispose()
    } catch {
        throw 'Cannot replace the installed plugin. Save your project, close FL Studio (and any plugin scanner), then run this installer as administrator.'
    }
}
New-Item -ItemType Directory -Path $destination -Force | Out-Null
foreach ($file in Get-ChildItem -LiteralPath $bundlePath -File -Recurse) {
    $relativePath = [IO.Path]::GetRelativePath($bundlePath, $file.FullName)
    $targetFile = [IO.Path]::GetFullPath((Join-Path $destination $relativePath))
    if (-not $targetFile.StartsWith($destination + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Invalid file path in bundle. Installation stopped.'
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $targetFile) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $targetFile -Force
    if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $targetFile).Hash) {
        throw "Installed file verification failed: $relativePath"
    }
}
Write-Output 'Installed and verified. In FL Plugin Manager, enable Verify plugins and click Find installed plugins.'
