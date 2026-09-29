$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$taskDeps = Join-Path $taskRoot '.deps'
$taskWdk = Join-Path $taskDeps 'wdk'
if (-not (Test-Path (Join-Path $taskWdk 'c/Include/10.0.26100.0/km/ntddk.h'))) {
    $taskZip = Join-Path $taskDeps 'wdk.zip'
    New-Item -ItemType Directory -Path $taskDeps -Force | Out-Null
    Invoke-WebRequest 'https://api.nuget.org/v3-flatcontainer/microsoft.windows.wdk.x64/10.0.26100.1/microsoft.windows.wdk.x64.10.0.26100.1.nupkg' -OutFile $taskZip
    if ((Get-FileHash $taskZip -Algorithm SHA256).Hash -ne '247B2919AE451F65BA5F1CD51C7C39730FB0FC383D607F3E8AB317FDDC8A8239') { throw 'WDK checksum mismatch.' }
    Expand-Archive -LiteralPath $taskZip -DestinationPath $taskWdk -Force
}
$taskVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$taskVs = & $taskVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $taskVs) { throw 'Visual Studio C++ tools not found.' }
$taskVc = Get-ChildItem (Join-Path $taskVs 'VC/Tools/MSVC') -Directory | Sort-Object Name -Descending | Select-Object -First 1
$taskCompiler = Join-Path $taskVc.FullName 'bin/Hostx64/x64/cl.exe'
$taskLinker = Join-Path $taskVc.FullName 'bin/Hostx64/x64/link.exe'
$taskSdk = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/Include/10.0.26100.0'
$taskLib = Join-Path $taskWdk 'c/Lib/10.0.26100.0/km/x64'
$taskOut = Join-Path $taskRoot 'dist/offline'
New-Item -ItemType Directory -Path $taskOut -Force | Out-Null
$taskObj = Join-Path $taskOut 'Driver.obj'
& $taskCompiler /nologo /c /TC /kernel /GS /Zl /W4 /WX /O2 /D_AMD64_ /DAMD64 /D_WIN64 /D_WIN32 /D_WIN32_WINNT=0x0A00 /DNTDDI_VERSION=0x0A000000 "/I$(Join-Path $taskWdk 'c/Include/10.0.26100.0/km')" "/I$(Join-Path $taskSdk 'shared')" "/I$(Join-Path $taskSdk 'ucrt')" "/I$(Join-Path $taskVc.FullName 'include')" "/Fo$taskObj" (Join-Path $PSScriptRoot 'Driver.c')
if ($LASTEXITCODE -ne 0) { throw 'Driver compile failed.' }
& $taskLinker /nologo /driver /subsystem:native,10.00 /machine:x64 /nodefaultlib /entry:GsDriverEntry /integritycheck /nxcompat /dynamicbase /opt:ref "/out:$(Join-Path $taskOut 'H1OfflineLab.sys')" $taskObj (Join-Path $taskLib 'ntoskrnl.lib') (Join-Path $taskLib 'hal.lib') (Join-Path $taskLib 'wdmsec.lib') (Join-Path $taskLib 'BufferOverflowK.lib')
if ($LASTEXITCODE -ne 0) { throw 'Driver link failed.' }
Copy-Item -LiteralPath (Join-Path $taskWdk 'LICENSE.txt') -Destination (Join-Path $taskOut 'WDK-LICENSE.txt') -Force
Write-Host 'Built unsigned H1OfflineLab.sys. No driver installed or loaded.'

