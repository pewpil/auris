<#
.SYNOPSIS
Display live Auris sensor data in Windows Terminal.

.DESCRIPTION
Poll the compute unit's read-only /state endpoint, display current IMU, ToF,
camera, placement and link status, and draw fixed-scale distance bars.

.PARAMETER Url
Compute-unit /state endpoint, defaulting to the Pi's current Wi-Fi address.

.PARAMETER IntervalSeconds
Seconds between HTTP snapshots while the display is live.

.PARAMETER TimeoutSeconds
Maximum HTTP request duration, in seconds.

.PARAMETER SourceIp
Optional local IPv4 address to bind, such as 192.168.77.1 for direct Ethernet.

.PARAMETER BarWidth
Character-cell width of each distance bar.

.PARAMETER Once
Print one snapshot and exit.

.EXAMPLE
.\3d\terminal_view.ps1

.EXAMPLE
.\3d\terminal_view.ps1 -Url http://192.168.77.2:8081/state -SourceIp 192.168.77.1 -Once
#>

[CmdletBinding()]
param(
    [string]$Url = "http://192.168.68.59:8081/state",
    [ValidateRange(0.05, 60.0)]
    [double]$IntervalSeconds = 0.25,
    [ValidateRange(0.25, 60.0)]
    [double]$TimeoutSeconds = 2.0,
    [string]$SourceIp,
    [ValidateRange(8, 100)]
    [int]$BarWidth = 32,
    [switch]$Once
)

$InvariantCulture = [System.Globalization.CultureInfo]::InvariantCulture

function Format-Number {
    <#
    .SYNOPSIS
    Format a finite sensor measurement in its caller-specified units.

    .PARAMETER Value
    Numeric sensor value; units and coordinate frame follow the field being displayed.

    .PARAMETER Digits
    Number of decimal places to show.

    .OUTPUTS
    System.String. Formatted value or n/a for a missing or invalid value.
    #>
    param(
        [AllowNull()][object]$Value,
        [ValidateRange(0, 6)][int]$Digits = 2
    )

    if ($null -eq $Value) {
        return "n/a"
    }
    try {
        $number = [double]$Value
    }
    catch {
        return "n/a"
    }
    if ([double]::IsNaN($number) -or [double]::IsInfinity($number)) {
        return "n/a"
    }
    return $number.ToString("F$Digits", $InvariantCulture)
}

function Format-Pan {
    <#
    .SYNOPSIS
    Format unitless stereo balance as a left, centre or right percentage.

    .PARAMETER Value
    Normalized pan in -1..1, where -1 is full left and +1 is full right.

    .OUTPUTS
    System.String. Side and percentage, Center, or n/a when unavailable.
    #>
    param([AllowNull()][object]$Value)

    if ($null -eq $Value) {
        return "n/a"
    }
    try {
        $pan = [double]$Value
    }
    catch {
        return "n/a"
    }
    if ([double]::IsNaN($pan) -or [double]::IsInfinity($pan)) {
        return "n/a"
    }
    $pan = [Math]::Max(-1.0, [Math]::Min(1.0, $pan))
    if ([Math]::Abs($pan) -lt 0.005) {
        return "Center"
    }
    $side = if ($pan -lt 0) { "L" } else { "R" }
    $percentage = [Math]::Round([Math]::Abs($pan) * 100)
    return "$side $percentage%"
}

function Format-Angles {
    <#
    .SYNOPSIS
    Format roll, pitch and yaw in degrees in the supplied body frame.

    .PARAMETER Values
    Three angles ordered roll, pitch, yaw, in degrees.

    .OUTPUTS
    System.String. Signed degree values or n/a for an invalid sample.
    #>
    param([AllowNull()][object[]]$Values)

    if ($null -eq $Values -or $Values.Count -ne 3) {
        return "n/a"
    }
    $parts = foreach ($value in $Values) {
        try {
            $number = [double]$value
        }
        catch {
            return "n/a"
        }
        if ([double]::IsNaN($number) -or [double]::IsInfinity($number)) {
            return "n/a"
        }
        $number.ToString("+0.0;-0.0;0.0", $InvariantCulture) + " deg"
    }
    return ($parts -join " / ")
}

function Format-OrientationMatrix {
    <#
    .SYNOPSIS
    Convert a row-major body-to-head rotation matrix into roll, pitch and yaw.

    .PARAMETER Values
    Nine matrix entries mapping pointer body axes into the head body frame.

    .OUTPUTS
    System.String. Roll, pitch and yaw in degrees, or n/a for invalid data.
    #>
    param([AllowNull()][object[]]$Values)

    if ($null -eq $Values -or $Values.Count -ne 9) {
        return "n/a"
    }
    try {
        $matrix = @($Values | ForEach-Object { [double]$_ })
    }
    catch {
        return "n/a"
    }
    foreach ($value in $matrix) {
        if ([double]::IsNaN($value) -or [double]::IsInfinity($value)) {
            return "n/a"
        }
    }

    $scale = 180.0 / [Math]::PI
    $roll = [Math]::Atan2($matrix[7], $matrix[8]) * $scale
    $pitchInput = [Math]::Max(-1.0, [Math]::Min(1.0, -$matrix[6]))
    $pitch = [Math]::Asin($pitchInput) * $scale
    $yaw = [Math]::Atan2($matrix[3], $matrix[0]) * $scale
    return Format-Angles -Values @($roll, $pitch, $yaw)
}

function New-DistanceBar {
    <#
    .SYNOPSIS
    Render a metre distance against a fixed maximum range.

    .PARAMETER DistanceM
    Distance in metres in the head frame for the hit, or pointer-local frame for ToF.

    .PARAMETER MaximumM
    Full-scale distance in metres.

    .PARAMETER Width
    Number of character cells in the complete bar.

    .OUTPUTS
    System.String. A filled and empty bar, or dashes for unavailable data.
    #>
    param(
        [AllowNull()][object]$DistanceM,
        [double]$MaximumM,
        [int]$Width
    )

    if ($null -eq $DistanceM -or $MaximumM -le 0) {
        return "-" * $Width
    }
    try {
        $distance = [double]$DistanceM
    }
    catch {
        return "-" * $Width
    }
    if ([double]::IsNaN($distance) -or [double]::IsInfinity($distance)) {
        return "-" * $Width
    }
    $filled = [int][Math]::Round([Math]::Max(0.0, [Math]::Min($Width, $distance / $MaximumM * $Width)))
    return ("#" * $filled) + ("-" * ($Width - $filled))
}

function Get-SourceBoundSnapshot {
    <#
    .SYNOPSIS
    Fetch one JSON snapshot over HTTP from a selected local IPv4 address.

    .PARAMETER Url
    Compute-unit /state endpoint using plain HTTP.

    .PARAMETER SourceIp
    Local IPv4 address to bind, such as 192.168.77.1 on this PC's Ethernet link.

    .PARAMETER TimeoutSeconds
    Maximum connect, send or receive duration in seconds.

    .OUTPUTS
    System.Management.Automation.PSCustomObject returned by ConvertFrom-Json.

    .NOTES
    Throws when the source address is invalid, the endpoint is unreachable, or
    the server returns a non-2xx response or invalid JSON.
    #>
    param(
        [Parameter(Mandatory)][string]$Url,
        [Parameter(Mandatory)][string]$SourceIp,
        [Parameter(Mandatory)][double]$TimeoutSeconds
    )

    $uri = [Uri]$Url
    if ($uri.Scheme -ne "http") {
        throw "-SourceIp supports the Pi's plain HTTP endpoint; omit it for HTTPS."
    }
    $timeoutMilliseconds = [int][Math]::Ceiling($TimeoutSeconds * 1000.0)
    $localAddress = [System.Net.IPAddress]::Parse($SourceIp)
    $client = [System.Net.Sockets.TcpClient]::new()
    $client.SendTimeout = $timeoutMilliseconds
    $client.ReceiveTimeout = $timeoutMilliseconds
    try {
        $client.Client.Bind([System.Net.IPEndPoint]::new($localAddress, 0))
        $connect = $client.BeginConnect($uri.Host, $uri.Port, $null, $null)
        $connectWaitHandle = $connect.AsyncWaitHandle
        try {
            if (-not $connectWaitHandle.WaitOne($timeoutMilliseconds)) {
                throw "Connection to $($uri.Host):$($uri.Port) timed out."
            }
            $client.EndConnect($connect)
        }
        finally {
            $connectWaitHandle.Close()
        }

        $stream = $client.GetStream()
        $stream.ReadTimeout = $timeoutMilliseconds
        $stream.WriteTimeout = $timeoutMilliseconds
        $hostHeader = $uri.Host
        if (-not $uri.IsDefaultPort) {
            $hostHeader += ":$($uri.Port)"
        }
        $requestPath = $uri.PathAndQuery
        if ([string]::IsNullOrEmpty($requestPath)) {
            $requestPath = "/"
        }
        $requestText = "GET $requestPath HTTP/1.0`r`nHost: $hostHeader`r`nAccept: application/json`r`nCache-Control: no-cache`r`nConnection: close`r`n`r`n"
        $requestBytes = [System.Text.Encoding]::ASCII.GetBytes($requestText)
        $stream.Write($requestBytes, 0, $requestBytes.Length)
        $reader = [System.IO.StreamReader]::new($stream, [System.Text.Encoding]::UTF8)
        $responseText = $reader.ReadToEnd()
        $reader.Dispose()

        $headerEnd = $responseText.IndexOf("`r`n`r`n")
        if ($headerEnd -lt 0) {
            throw "The Pi returned an incomplete HTTP response."
        }
        $statusEnd = $responseText.IndexOf("`r`n")
        $statusLine = $responseText.Substring(0, $statusEnd)
        if ($statusLine -notmatch '^HTTP/\d\.\d\s+2\d\d(?:\s|$)') {
            throw "The Pi returned $statusLine."
        }
        $json = $responseText.Substring($headerEnd + 4)
        return ConvertFrom-Json -InputObject $json -ErrorAction Stop
    }
    finally {
        $client.Dispose()
    }
}

function Format-Snapshot {
    <#
    .SYNOPSIS
    Render one compute-unit snapshot as a compact text dashboard.

    .PARAMETER State
    JSON object returned by the Pi's /state endpoint.

    .PARAMETER FetchMilliseconds
    HTTP request duration in milliseconds.

    .PARAMETER Width
    Character-cell width for each distance bar.

    .OUTPUTS
    System.String. Human-readable current sensor and tracking status.
    #>
    param(
        [Parameter(Mandatory)][object]$State,
        [double]$FetchMilliseconds,
        [int]$Width
    )

    $head = $State.head
    $pointer = $State.pointer
    $placement = $State.placement
    $camera = $State.camera
    $config = $State.config
    if ($null -eq $head) { $head = [pscustomobject]@{} }
    if ($null -eq $pointer) { $pointer = [pscustomobject]@{} }
    if ($null -eq $placement) { $placement = [pscustomobject]@{} }
    if ($null -eq $camera) { $camera = [pscustomobject]@{} }
    if ($null -eq $config) { $config = [pscustomobject]@{} }

    $maximum = 4.0
    if ($null -ne $config.max_range_m -and [double]$config.max_range_m -gt 0) {
        $maximum = [double]$config.max_range_m
    }

    $mock = $State.mock -eq $true
    $headFresh = $head.fresh -eq $true -and $null -ne $head.euler_deg
    $pointerPresent = $null -ne $State.pointer
    $cameraFresh = $camera.position_fresh -eq $true
    if ($mock) {
        $link = "SYNTHETIC DATA"
    }
    elseif ($headFresh -and $pointerPresent -and $State.aligned -ne $false) {
        $link = "LIVE"
    }
    elseif ($headFresh -or $pointerPresent -or $cameraFresh) {
        $link = "PARTIAL"
    }
    else {
        $link = "NO SENSORS"
    }

    $orientation = "n/a"
    $orientationSource = "unavailable"
    if ($cameraFresh -and $null -ne $camera.rotation_head) {
        $orientation = Format-OrientationMatrix -Values $camera.rotation_head
        $orientationSource = "ArUco cube"
    }
    elseif ($null -ne $pointer.matrix_head) {
        $orientation = Format-OrientationMatrix -Values $pointer.matrix_head
        $orientationSource = "pointer IMU"
    }
    elseif ($null -ne $camera.rotation_head) {
        $orientation = Format-OrientationMatrix -Values $camera.rotation_head
        $orientationSource = "last camera pose"
    }

    $button = "n/a"
    if ($pointer.button -eq $true -or $pointer.button -eq 1) { $button = "pressed" }
    elseif ($pointer.button -eq $false -or $pointer.button -eq 0) { $button = "released" }

    $cameraPosition = "n/a"
    if ($null -ne $camera.position_head_m -and $camera.position_head_m.Count -eq 3) {
        $cameraPosition = "{0}, {1}, {2}" -f `
            (Format-Number $camera.position_head_m[0] 3), `
            (Format-Number $camera.position_head_m[1] 3), `
            (Format-Number $camera.position_head_m[2] 3)
    }
    $markerIds = "n/a"
    if ($null -ne $camera.marker_ids -and $camera.marker_ids.Count -gt 0) {
        $markerIds = $camera.marker_ids -join ","
    }
    $headHealth = if ($headFresh) { "fresh" } else { "stale / unavailable" }
    $alignment = if ($State.aligned -eq $true) { "aligned" }
        elseif ($State.aligned -eq $false) { "needs alignment" }
        else { "unknown" }
    $cameraHasFrames = $null -ne $camera.frame_count -and [double]$camera.frame_count -gt 0
    $cameraHealth = if ($cameraFresh) { "tracking" }
        elseif ($null -ne $camera.distance_m) { "stale fix" }
        elseif ($cameraHasFrames) { "searching tags" }
        else { "no frames" }

    $lines = @(
        "AURIS LIVE SENSOR VIEW  |  Ctrl+C to stop",
        "Sensors: $link | yaw $alignment | head $headHealth | camera $cameraHealth | fetch $([Math]::Round($FetchMilliseconds, 1)) ms",
        "",
        "HEAD r/p/y       $(Format-Angles -Values $head.euler_deg)",
        "RAYGUN r/p/y     $orientation | $orientationSource",
        "TOF $(Format-Number $pointer.range_m 3) / $($maximum.ToString('F1', $InvariantCulture)) m",
        "  $(New-DistanceBar -DistanceM $pointer.range_m -MaximumM $maximum -Width $Width)",
        "HIT $(Format-Number $placement.distance_m 2) / $($maximum.ToString('F1', $InvariantCulture)) m  gain $(Format-Number $placement.gain 3)  pan $(Format-Pan $placement.pan)",
        "  $(New-DistanceBar -DistanceM $placement.distance_m -MaximumM $maximum -Width $Width)",
        "CAMERA cube XYZ $cameraPosition m | range $(Format-Number $camera.distance_m 3) m | IDs $markerIds | age $(Format-Number $camera.age_s 3) s",
        "CAM frames $(Format-Number $camera.frame_count 0) | poses $(Format-Number $camera.detection_count 0) | fails $(Format-Number $camera.read_failures 0) | read/detect $(Format-Number $camera.read_ms 1)/$(Format-Number $camera.detection_ms 1) ms",
        "BEAM az/el $(Format-Number $placement.azimuth_deg 1) / $(Format-Number $placement.elevation_deg 1) deg | trigger $button"
    )
    return ($lines -join [Environment]::NewLine)
}

while ($true) {
    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    try {
        if ([string]::IsNullOrWhiteSpace($SourceIp)) {
            $state = Invoke-RestMethod -Uri $Url -Method Get -TimeoutSec ([int][Math]::Ceiling($TimeoutSeconds)) -Headers @{ "Cache-Control" = "no-cache" }
        }
        else {
            $state = Get-SourceBoundSnapshot -Url $Url -SourceIp $SourceIp -TimeoutSeconds $TimeoutSeconds
        }
        $timer.Stop()
        $frame = Format-Snapshot -State $state -FetchMilliseconds $timer.Elapsed.TotalMilliseconds -Width $BarWidth
    }
    catch {
        $timer.Stop()
        $frame = "AURIS SENSOR VIEW  |  server unavailable`n$Url`n$($_.Exception.Message)"
    }

    if ($Once) {
        Write-Output $frame
        break
    }
    Clear-Host
    Write-Output $frame
    Start-Sleep -Milliseconds ([int][Math]::Max(50, $IntervalSeconds * 1000))
}
