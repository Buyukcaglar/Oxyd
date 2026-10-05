param(
    [string]$OutputDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'build'),
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$taskOutput = [IO.Path]::GetFullPath($OutputDirectory)

# Use an existing x86 developer environment, or discover the latest installed
# Visual Studio/Build Tools installation. No developer-specific paths are used.
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x86' -or -not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $taskVswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $taskVswhere)) {
        throw 'Install Visual Studio 2022 or newer C++ Build Tools and a Windows SDK, or use an x86 Native Tools prompt.'
    }
    $taskVs = & $taskVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $taskVs) { throw 'No Visual Studio x86/x64 C++ toolset found.' }
    $taskDevShell = Join-Path $taskVs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
    Import-Module $taskDevShell
    Enter-VsDevShell -VsInstallPath $taskVs -SkipAutomaticLocation -DevCmdArguments '-arch=x86 -host_arch=x64' | Out-Null
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x86') { throw 'The compiler must target x86.' }
$taskCompiler = (Get-Command cl.exe -ErrorAction Stop).Source
$taskCommon = @('/nologo','/W4','/EHsc','/std:c++17','/O2','/MT','/DUNICODE','/D_UNICODE')
$taskGraphics = @('user32.lib','gdi32.lib','msimg32.lib')
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
Push-Location $taskOutput
try {
    & $taskCompiler @taskCommon /LD (Join-Path $taskRoot 'src\oxscale.cpp') /Fo:oxscale.obj /link "/DEF:$taskRoot\src\oxscale.def" /OUT:oxscale.dll @taskGraphics
    if ($LASTEXITCODE -ne 0) { throw 'DLL build failed.' }
    & $taskCompiler @taskCommon /DOXSCALE_TEST (Join-Path $taskRoot 'src\oxscale.cpp') /Fe:verify-scale.exe /Fo:verify-scale.obj /link @taskGraphics
    if ($LASTEXITCODE -ne 0) { throw 'Scaling test build failed.' }
    foreach ($taskProbe in @('verify_loader','verify_window','probe_input','inspect_typelib')) {
        & $taskCompiler @taskCommon (Join-Path $taskRoot "tests\$taskProbe.cpp") "/Fe:$taskProbe.exe" "/Fo:$taskProbe.obj" /link @taskGraphics ole32.lib oleaut32.lib
        if ($LASTEXITCODE -ne 0) { throw "$taskProbe build failed." }
    }
    Copy-Item -LiteralPath (Join-Path $taskRoot 'portable\dx7vb.dll'),(Join-Path $taskRoot 'portable\OxydEx.exe.manifest') -Destination $taskOutput
    if (-not $SkipTests) {
        foreach ($taskCheck in @('verify-scale.exe','verify_window.exe')) {
            & (Join-Path $taskOutput $taskCheck)
            if ($LASTEXITCODE -ne 0) { throw "$taskCheck failed: exit $LASTEXITCODE." }
        }
        & (Join-Path $taskOutput 'probe_input.exe') (Join-Path $taskOutput 'OxydEx.exe.manifest')
        if ($LASTEXITCODE -ne 0) { throw "Local DirectX input probe failed: exit $LASTEXITCODE." }
    }
    Write-Host "Built x86 patch and probes in $taskOutput"
} finally { Pop-Location }
