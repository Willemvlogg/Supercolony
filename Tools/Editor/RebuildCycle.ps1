<#
.SYNOPSIS
    Quit the editor cleanly, build, run all Supercolony automation tests, relaunch the editor on M1_Swarm,
    and wait for the MCP server. Save dirty assets through MCP before running this (see CLAUDE.md, Rebuild cycle).

.PARAMETER SkipRelaunch
    Stop after the tests instead of reopening the editor.

Exit codes: 0 all good, 1 build failed, 2 tests failed, 3 MCP did not answer within the timeout.
The editor is not relaunched after a failed build, so it never opens with stale binaries.
#>
param(
    [switch]$SkipRelaunch
)

$ErrorActionPreference = 'Stop'

$EngineRoot   = 'C:\Program Files\Epic Games\UE_5.8\Engine'
$Project      = 'C:\Dev\Supercolony\Supercolony.uproject'
$StartupMap   = '/Game/Maps/M1_Swarm'
$McpUrl       = 'http://127.0.0.1:8000/mcp'
$QuitTimeout  = 60
$McpTimeout   = 120
$TestLog      = 'C:\Dev\Supercolony\Saved\Logs\AutomationTests.log'
$ScriptDir    = Split-Path -Parent $MyInvocation.MyCommand.Path

function Write-Step([string]$Message) { Write-Host "`n=== $Message ===" }

# b. Quit the editor through Python and wait for the process to exit.
Write-Step 'Quit editor'
$Editor = Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue
if ($Editor) {
    & python -I (Join-Path $ScriptDir 'editor_python.py') --expect-exit 'unreal.SystemLibrary.quit_editor()'
    $Editor | Wait-Process -Timeout $QuitTimeout -ErrorAction SilentlyContinue
    if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) {
        Write-Host "FORCE-KILLED: the editor was still running after $QuitTimeout s."
        Get-Process -Name UnrealEditor | Stop-Process -Force
        Start-Sleep -Seconds 2
    } else {
        Write-Host 'Editor exited cleanly.'
    }
} else {
    Write-Host 'Editor was not running.'
}

# c. Build, then run every Supercolony automation test headless.
Write-Step 'Build'
& "$EngineRoot\Build\BatchFiles\Build.bat" SupercolonyEditor Win64 Development "-Project=$Project" -WaitMutex 2>&1 |
    Select-String -Pattern 'error|warning|Result:' | ForEach-Object { $_.Line }
if ($LASTEXITCODE -ne 0) {
    Write-Host "BUILD FAILED (exit $LASTEXITCODE). Editor not relaunched."
    exit 1
}

Write-Step 'Automation tests'
& "$EngineRoot\Binaries\Win64\UnrealEditor-Cmd.exe" $Project '-ExecCmds=Automation RunTests Supercolony; Quit' `
    -unattended -nopause -nullrhi -nosplash -NoSound "-abslog=$TestLog" 2>&1 | Out-Null
$Results = Select-String -Path $TestLog -Pattern 'Test Completed\. Result=\{(\w+)\}.*Path=\{([^}]+)\}'
$Failed  = $Results | Where-Object { $_.Matches[0].Groups[1].Value -ne 'Success' }
$Results | ForEach-Object { '{0,-8} {1}' -f $_.Matches[0].Groups[1].Value, $_.Matches[0].Groups[2].Value }
if ($Results.Count -eq 0 -or $Failed) {
    Write-Host "TESTS FAILED: $($Failed.Count) failed of $($Results.Count). Log: $TestLog"
    exit 2
}
Write-Host "All $($Results.Count) tests passed."

if ($SkipRelaunch) { exit 0 }

# d. Relaunch the editor on the M1 map.
Write-Step 'Relaunch editor'
Start-Process -FilePath "$EngineRoot\Binaries\Win64\UnrealEditor.exe" -ArgumentList "`"$Project`"", $StartupMap

# e. Wait for the MCP server to answer an initialize request.
Write-Step 'Wait for MCP'
$Body = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"rebuild-cycle","version":"1"}}}'
$Headers = @{ Accept = 'application/json, text/event-stream' }
$Deadline = (Get-Date).AddSeconds($McpTimeout)
while ((Get-Date) -lt $Deadline) {
    try {
        $Response = Invoke-WebRequest -Uri $McpUrl -Method Post -Body $Body -ContentType 'application/json' -Headers $Headers -TimeoutSec 5 -UseBasicParsing
        if ($Response.StatusCode -eq 200) {
            Write-Host "MCP server answering at $McpUrl."
            exit 0
        }
    } catch { }
    Start-Sleep -Seconds 3
}
Write-Host "MCP server did not answer within $McpTimeout s."
exit 3
