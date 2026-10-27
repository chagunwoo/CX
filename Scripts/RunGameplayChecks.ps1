param(
    [ValidateSet('Rules', 'Network', 'All')][string]$Mode = 'All',
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $projectRoot 'CX.uproject'
$logs = Join-Path $projectRoot 'Saved\Logs'
$reports = Join-Path $projectRoot 'Saved\Tests'
New-Item -ItemType Directory -Force $reports | Out-Null
$ownedProcesses = @()
function Launch-Check([string[]]$Arguments) {
    $process = Start-Process -FilePath $Editor -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    $script:ownedProcesses += $process
    return $process
}
try {
    if ($Mode -in @('Rules', 'All')) {
        $rules = Launch-Check @($project, '-unattended', '-nullrhi', '-nosound', '-nop4', '-nosplash', '-ExecCmds="Automation RunTests CX.Gameplay"', '-TestExit="Automation Test Queue Empty"', "-ReportExportPath=$reports\Automation", "-abslog=$logs\GameplayTests.log")
        if (!$rules.WaitForExit(600000)) { throw 'Server-rules test timed out.' }
        if ($rules.ExitCode -ne 0) { throw "Server-rules test failed; inspect $logs\GameplayTests.log" }
        if (!(Select-String -Path "$logs\GameplayTests.log" -Pattern 'Result=\{Success\}.*CX.Gameplay.ServerRules' -Quiet)) { throw 'Missing successful test result.' }
        Write-Output 'CX server-rules test PASS'
    }
    if ($Mode -in @('Network', 'All')) {
        $serverLog = Join-Path $logs 'NetworkServer.log'
        $launchTime = Get-Date
        $server = Launch-Check @($project, '/Game/VehicleTemplate/Maps/VehicleAdvExampleMap', '-server', '-port=17777', '-CXNetworkSmoke', '-unattended', '-nullrhi', '-nosound', '-nop4', "-abslog=$serverLog")
        $deadline = (Get-Date).AddSeconds(180)
        $ready = $false
        while ((Get-Date) -lt $deadline -and !$server.HasExited) {
            if ((Test-Path $serverLog) -and (Get-Item $serverLog).LastWriteTime -ge $launchTime -and (Select-String -Path $serverLog -Pattern 'listening on port 17777' -Quiet)) { $ready = $true; break }
            Start-Sleep -Milliseconds 500
        }
        if (!$ready) { throw 'Network test server did not become ready.' }
        $clients = @()
        foreach ($index in 1..2) {
            $clients += Launch-Check @($project, "127.0.0.1:17777?Name=CXTest$index", '-game', '-CXNetworkSmoke', '-unattended', '-nullrhi', '-nosound', '-nop4', '-nosplash', '-windowed', '-ResX=960', '-ResY=540', '-ExecCmds="t.MaxFPS 30"', "-abslog=$logs\NetworkClient$index.log")
        }
        $deadline = (Get-Date).AddSeconds(300)
        do {
            $running = $false
            foreach ($process in $clients + @($server)) {
                if (!$process.HasExited) { $running = $true }
                elseif ($process.ExitCode -ne 0) { throw 'Network smoke test process failed; inspect Network*.log.' }
            }
            if ((Get-Date) -ge $deadline) { throw 'Network smoke test timed out.' }
            if ($running) { Start-Sleep -Milliseconds 500 }
        } while ($running)
        foreach ($name in @('NetworkServer', 'NetworkClient1', 'NetworkClient2')) {
            if (!(Select-String -Path "$logs\$name.log" -Pattern 'CX_NETWORK_SMOKE PASS' -Quiet)) { throw "Missing PASS in $name" }
        }
        Write-Output 'CX two-client network smoke test PASS'
    }
}
finally {
    foreach ($process in $ownedProcesses) {
        if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
    }
}
