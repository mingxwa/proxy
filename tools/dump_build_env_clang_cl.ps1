[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$osInfo = Get-CimInstance Win32_OperatingSystem
$compiler = (& clang-cl --version 2>&1 | Select-Object -First 1).Trim()
[ordered]@{
    OS = $osInfo.Caption
    KernelVersion = $osInfo.Version
    Architecture = $env:PROCESSOR_ARCHITECTURE
    Compiler = $compiler
} | ConvertTo-Json | Set-Content -Path $OutputPath
