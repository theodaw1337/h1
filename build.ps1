param([ValidateSet('Release','Debug')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$taskRoot = $PSScriptRoot
$taskDeps = Join-Path $taskRoot '.deps'
$taskPackage = Join-Path $taskDeps 'd3dx.zip'
$taskExpectedHash = 'EAD0906AE8A26C18A7525DA7490127A2110F7C58F18293738283E30E97C6EA4B'
if (-not (Test-Path -LiteralPath (Join-Path $taskDeps 'd3dx/build/native/include/d3dx9.h'))) {
    New-Item -ItemType Directory -Path $taskDeps -Force | Out-Null
    Invoke-WebRequest -Uri 'https://api.nuget.org/v3-flatcontainer/microsoft.dxsdk.d3dx/9.29.952.8/microsoft.dxsdk.d3dx.9.29.952.8.nupkg' -OutFile $taskPackage
    if ((Get-FileHash -LiteralPath $taskPackage -Algorithm SHA256).Hash -ne $taskExpectedHash) {
        throw 'D3DX package checksum mismatch.'
    }
    Expand-Archive -LiteralPath $taskPackage -DestinationPath (Join-Path $taskDeps 'd3dx') -Force
}
$taskImgui = Join-Path $taskDeps 'imgui-1.91.9b'
if (-not (Test-Path -LiteralPath (Join-Path $taskImgui 'imgui.cpp'))) {
    $taskImguiArchive = Join-Path $taskDeps 'imgui-v1.91.9b.zip'
    Invoke-WebRequest -Uri 'https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9b.zip' -OutFile $taskImguiArchive
    if ((Get-FileHash -LiteralPath $taskImguiArchive -Algorithm SHA256).Hash -ne 'FD37507C8476A6D14CC7C4B352401F31BCBD0F0D995D35390811E968C466F46E') {
        throw 'Dear ImGui package checksum mismatch.'
    }
    Expand-Archive -LiteralPath $taskImguiArchive -DestinationPath $taskDeps -Force
}
& cmake -S $taskRoot -B (Join-Path $taskRoot 'build') -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& cmake --build (Join-Path $taskRoot 'build') --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
& ctest --test-dir (Join-Path $taskRoot 'build') -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
$taskOutput = Join-Path $taskRoot "dist/$Configuration"
if (-not (Test-Path -LiteralPath (Join-Path $taskOutput 'settings.ini'))) {
    Copy-Item -LiteralPath (Join-Path $taskRoot 'settings.ini') -Destination $taskOutput
}
Copy-Item -LiteralPath (Join-Path $taskDeps 'd3dx/LICENSE.txt') -Destination (Join-Path $taskOutput 'D3DX-LICENSE.txt') -Force
Copy-Item -LiteralPath (Join-Path $taskImgui 'LICENSE.txt') -Destination (Join-Path $taskOutput 'IMGUI-LICENSE.txt') -Force
Write-Host "Ready: $taskOutput/h1_updated.exe"
