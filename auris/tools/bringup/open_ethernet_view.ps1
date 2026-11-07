<#
Purpose: Open the Auris dashboard through the PC's direct Ethernet route to the Pi.
Inputs: Optional -NoBrowser switch; uses PC Ethernet address 192.168.77.1 and Pi address 192.168.77.2.
Outputs: Starts a hidden loopback proxy if needed; opens the default browser unless -NoBrowser is set.
#>

[CmdletBinding()]
param(
    [switch]$NoBrowser
)

$ErrorActionPreference = "Stop"
$proxyPort = 8781
$proxyAddress = "127.0.0.1"
$dashboardUrl = "http://$proxyAddress`:$proxyPort/"
$healthUrl = "http://$proxyAddress`:$proxyPort/health"
$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$repositoryRoot = Split-Path -Parent $scriptDirectory
$proxyScript = Join-Path $scriptDirectory "ethernet_proxy.py"
$python = Get-Command python -ErrorAction SilentlyContinue

if ($null -eq $python) {
    throw "Python was not found. Install Python 3, then rerun this launcher."
}

$healthy = $false
try {
    $health = Invoke-WebRequest -Uri $healthUrl -TimeoutSec 2 -UseBasicParsing
    $healthy = $health.StatusCode -eq 200 -and $health.Content.Trim() -eq "ok"
}
catch {
    $healthy = $false
}

if (-not $healthy) {
    $listener = Get-NetTCPConnection -LocalPort $proxyPort -State Listen -ErrorAction SilentlyContinue |
        Where-Object { $_.LocalAddress -in @($proxyAddress, "0.0.0.0", "::", "::1") }
    if ($null -ne $listener) {
        throw "Port $proxyPort is occupied, but it is not serving the Auris proxy health endpoint."
    }

    $arguments = '"{0}" --source-ip 192.168.77.1' -f $proxyScript
    Start-Process -FilePath $python.Source -ArgumentList $arguments `
        -WorkingDirectory $repositoryRoot -WindowStyle Hidden | Out-Null

    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    while ($timer.Elapsed.TotalSeconds -lt 10 -and -not $healthy) {
        Start-Sleep -Milliseconds 250
        try {
            $health = Invoke-WebRequest -Uri $healthUrl -TimeoutSec 2 -UseBasicParsing
            $healthy = $health.StatusCode -eq 200 -and $health.Content.Trim() -eq "ok"
        }
        catch {
            $healthy = $false
        }
    }
}

if (-not $healthy) {
    throw "The Pi dashboard did not answer through Ethernet. Check the cable and Pi service, then rerun this launcher."
}

if ($NoBrowser) {
    Write-Host "Auris Ethernet proxy is healthy at $dashboardUrl"
}
else {
    Write-Host "Opening the live Auris Ethernet dashboard at $dashboardUrl"
    Start-Process -FilePath $dashboardUrl
}
