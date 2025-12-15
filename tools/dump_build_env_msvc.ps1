[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string]$OutputPath
)

function Convert-BytesToReadableString {
    param([Nullable[double]]$Bytes)
    if ($null -eq $Bytes -or $Bytes -le 0) {
        return 'Unknown'
    }
    $units = 'B','KB','MB','GB','TB','PB'
    $value = [double]$Bytes
    $index = 0
    while ($value -ge 1024 -and $index -lt $units.Length - 1) {
        $value /= 1024
        $index++
    }
    return ('{0:N2} {1}' -f $value, $units[$index])
}

function Convert-KilobytesToReadableString {
    param([Nullable[double]]$Value)
    if ($null -eq $Value -or $Value -le 0) {
        return 'Unknown'
    }
    if ($Value -ge 1024) {
        return ('{0:N0} MB' -f ($Value / 1024))
    }
    return ('{0:N0} KB' -f $Value)
}

function Get-RamSpeedString {
    param([CimInstance[]]$Modules)
    if (-not $Modules) {
        return 'Unknown'
    }
    $speeds = $Modules |
        ForEach-Object {
            $value = $null
            if ($_.ConfiguredClockSpeed -and $_.ConfiguredClockSpeed -gt 0) {
                $value = $_.ConfiguredClockSpeed
            } elseif ($_.Speed -and $_.Speed -gt 0) {
                $value = $_.Speed
            }
            if ($null -ne $value) {
                $value
            }
        } |
        Where-Object { $_ } |
        Sort-Object -Unique
    if (-not $speeds) {
        return 'Unknown'
    }
    return (($speeds | ForEach-Object { "$_ MHz" }) -join ', ')
}

function Get-CacheSizeString {
    param(
        [CimInstance[]]$CacheInventory,
        [CimInstance]$Processor,
        [Parameter(Mandatory=$true)][int]$Level,
        [string]$FallbackProperty
    )

    $sizeKb = $null
    if ($CacheInventory) {
        $entry = $CacheInventory |
            Where-Object { $_.Level -eq $Level -and $_.InstalledSize -gt 0 } |
            Select-Object -First 1
        if ($entry) {
            $sizeKb = $entry.InstalledSize
        }
    }

    if (-not $sizeKb -and $Processor -and $FallbackProperty -and $Processor.PSObject.Properties[$FallbackProperty]) {
        $sizeKb = $Processor.$FallbackProperty
    }

    if (-not $sizeKb -or $sizeKb -le 0) {
        return 'Unknown'
    }

    return (Convert-KilobytesToReadableString -Value $sizeKb)
}

$osInfo = Get-CimInstance Win32_OperatingSystem
$computerSystem = Get-CimInstance Win32_ComputerSystem
$memoryModules = Get-CimInstance Win32_PhysicalMemory
$processors = Get-CimInstance Win32_Processor
$cacheInventory = Get-CimInstance Win32_CacheMemory -ErrorAction SilentlyContinue
$primaryProcessor = $processors | Select-Object -First 1

$compiler = ((cl /? 2>&1 | Out-String) -split '\r')[0] -replace '^cl\s:\s',''

$ramTotal = Convert-BytesToReadableString -Bytes $computerSystem.TotalPhysicalMemory
$ramSpeed = Get-RamSpeedString -Modules $memoryModules

$physicalCoresValue = ($processors | Measure-Object -Property NumberOfCores -Sum).Sum
if (-not $physicalCoresValue -or $physicalCoresValue -le 0) {
    $physicalCoresString = 'Unknown'
} else {
    $physicalCoresString = $physicalCoresValue.ToString()
}

$logicalProcessorsValue = ($processors | Measure-Object -Property NumberOfLogicalProcessors -Sum).Sum
if (-not $logicalProcessorsValue -or $logicalProcessorsValue -le 0) {
    $logicalProcessorsString = 'Unknown'
} else {
    $logicalProcessorsString = $logicalProcessorsValue.ToString()
}

$cpuModel = if ($primaryProcessor.Name) { $primaryProcessor.Name.Trim() } else { 'Unknown' }
$baseSpeed = if ($primaryProcessor.MaxClockSpeed -and $primaryProcessor.MaxClockSpeed -gt 0) {
    ('{0:N0} MHz' -f $primaryProcessor.MaxClockSpeed)
} else {
    'Unknown'
}

$cacheInfo = [ordered]@{
    L1 = Get-CacheSizeString -CacheInventory $cacheInventory -Processor $primaryProcessor -Level 3 -FallbackProperty 'L1CacheSize'
    L2 = Get-CacheSizeString -CacheInventory $cacheInventory -Processor $primaryProcessor -Level 4 -FallbackProperty 'L2CacheSize'
    L3 = Get-CacheSizeString -CacheInventory $cacheInventory -Processor $primaryProcessor -Level 5 -FallbackProperty 'L3CacheSize'
}

$hardware = [ordered]@{
    Memory = [ordered]@{
        Total = $ramTotal
        Speed = $ramSpeed
    }
    CPU = [ordered]@{
        Model = $cpuModel
        BaseSpeed = $baseSpeed
        PhysicalCores = $physicalCoresString
        LogicalProcessors = $logicalProcessorsString
        Caches = $cacheInfo
    }
}

$jsonObject = [ordered]@{
    OS = $osInfo.Caption
    KernelVersion = $osInfo.Version
    Architecture = $env:PROCESSOR_ARCHITECTURE
    Compiler = $compiler
    Hardware = $hardware
} | ConvertTo-Json -Depth 6

Set-Content -Path $OutputPath -Value $jsonObject -Encoding UTF8
