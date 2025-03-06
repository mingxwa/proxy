[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string]$OutputPath
)

$compiler = ((cl 2>&1 | Out-String) -split '\r?\n')[0] -replace '^cl\s:\s',''
$jsonObject = [ordered]@{
    OSName = (Get-CimInstance Win32_OperatingSystem).Caption
    KernelVersion = (Get-CimInstance Win32_OperatingSystem).Version
    Architecture = $env:PROCESSOR_ARCHITECTURE
    Compiler = $compiler
} | ConvertTo-Json
Set-Content -Path $OutputPath -Value $jsonObject
