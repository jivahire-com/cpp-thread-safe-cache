# Copyright (c) Microsoft Corporation. All rights reserved.

# =============================================================================
# Run-PythiaTestLocally.ps1
# =============================================================================
#
# PURPOSE:
#   All-in-one script for running Pythia functional tests locally on SoC hardware.
#   Handles everything end-to-end:
#     1. SSH key setup (passphrase-free) and ssh-agent configuration
#     2. Windows keyring credential population from Azure Key Vault
#     3. creds.yaml creation via Setup-LocalPythia
#     4. Iterative test execution with watchdog timeout and result parsing
#
# USAGE:
#   # First time setup + run (full credential/SSH setup + test):
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1
#
#   # Subsequent runs (skip setup, credentials already in keyring):
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup
#
#   # Multiple iterations (stops on first failure):
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -Iterations 5 -SkipSetup
#
#   # Different agent:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n2 -SkipSetup
#
# EXAMPLES - Power Test Suites (SoC):
#   # Power PLDM service (default):
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup
#
#   # Power control loop health:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup -TestSuite .\tests\functional\test_suites\power_tests\mod_power_control_loop_health_soc.robot
#
#   # Power telemetry loop health:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup -TestSuite .\tests\functional\test_suites\power_tests\mod_power_telemetry_loop_health_soc.robot
#
#   # Power P-state transition:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup -TestSuite .\tests\functional\test_suites\power_tests\mod_power_pstate_transition_soc.robot
#
#   # Power FT (functional test):
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup -TestSuite .\tests\functional\test_suites\power_tests\mod_power_ft_soc.robot
#
# EXAMPLES - Other Test Suites (SoC):
#   # Warm start tests:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup -TestSuite .\tests\functional\test_suites\warm_start_tests\warm_start_test_soc.robot
#
#   # Variable service tests:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup -TestSuite .\tests\functional\test_suites\varserv_tests\varserv_cli_test_soc.robot
#
#   # Sensor FIFO MCP/SCP sync:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup -TestSuite .\tests\functional\test_suites\sensor_fifo_tests\sensor_fifo_mcp_scp_sync_soc.robot
#
#   # Run an entire test directory (all .robot files in a folder):
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -SkipSetup -TestSuite .\tests\functional\test_suites\power_tests\
#
# EXAMPLES - SVP/FPGA Platforms:
#   # SVP platform (no agent name needed):
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -Platform svp -TestSuite .\tests\functional\test_suites\power_tests\mod_power_ft.robot
#
#   # FPGA platform:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -Platform fpga -TestSuite .\tests\functional\test_suites\warm_start_tests\warm_start_test.robot
#
# EXAMPLES - Advanced Options:
#   # Custom timeout (45 min) and cooldown (60s):
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -Iterations 10 -TimeoutMinutes 45 -CooldownSeconds 60 -SkipSetup
#
#   # Disable timeout entirely:
#   .\tools\pwsh\Run-PythiaTestLocally.ps1 -TrimmedAgentName s14n1 -TimeoutMinutes 0 -SkipSetup
#
# PREREQUISITES:
#   1. Run start.ps1 first (sets $env:REPO_APP_ROOT and Python paths)
#   2. Azure CLI installed and logged in (az login) -- only needed without -SkipSetup
#   3. Access to the "chie-1pfw" Azure Key Vault -- only needed without -SkipSetup
#   4. VPN connected to reach the rack manager / BMC
#
# WHEN TO RUN:
#   - FIRST TIME on a machine: run WITHOUT -SkipSetup (sets up SSH key + keyring)
#   - EVERY TIME after: run WITH -SkipSetup (keyring persists across reboots)
#   - AGAIN without -SkipSetup if: Key Vault passwords rotated, SSH key regenerated,
#     or Windows Credential Manager was reset
#   - Credentials are SHARED across all agents -- no need to re-run per agent
#   - Each iteration does a full SoC power cycle in setup(), so a failed teardown
#     is automatically recovered by the next iteration
#
# CREDENTIAL FLOW:
#   Azure Key Vault (chie-1pfw)
#     |-- rack-manager-host-pwd  --> Windows Keyring: account/rack_manager_host_pwd
#     |-- bmc-admin-password     --> Windows Keyring: account/bmc_admin_password
#     |-- bmc-privmode-password  --> Windows Keyring: account/bmc_privmode_password
#     |-- rack-manager-host-pwd  --> creds.yaml: RM_PASSWORD
#     |-- bmc-privmode-password  --> creds.yaml: BMC_PASSWORD
#     v
#   Pythia TDK reads from keyring at test init
#   RscmHelperLibrary reads from creds.yaml via $env:SECURE_FILE_PATH
#
# =============================================================================

<#
.SYNOPSIS
All-in-one local Pythia test runner: sets up credentials, SSH, and runs test iterations.

.PARAMETER TrimmedAgentName
Agent name for SoC host config lookup (e.g., s14n1, s14n2). Required for SoC platform.

.PARAMETER TestSuite
Path to the .robot file or test directory. Defaults to mod_power_pldm_service.robot.

.PARAMETER Iterations
Number of times to run the suite (stops on first failure). Defaults to 1.

.PARAMETER Platform
Target platform: soc, svp, or fpga. Defaults to 'soc'.

.PARAMETER TimeoutMinutes
Max minutes per iteration before killing hung processes. Defaults to 20.

.PARAMETER CooldownSeconds
Seconds between iterations for SSH cleanup. Defaults to 30.

.PARAMETER HostJson
Host JSON file name for non-SoC platforms.

.PARAMETER SkipSetup
Skip the credential/SSH setup step (use when already configured this session).

.PARAMETER VaultName
Azure Key Vault name for credentials. Defaults to "chie-1pfw".

.PARAMETER SshKeyPath
Path to SSH private key. Defaults to ~/.ssh/id_rsa.
#>
param (
    [Parameter(Mandatory = $false)]
    [string] $TrimmedAgentName,

    [Parameter(Mandatory = $false)]
    [string] $TestSuite = ".\tests\functional\test_suites\power_tests\mod_power_pldm_service.robot",

    [Parameter(Mandatory = $false)]
    [ValidateRange(1, 1000)]
    [int] $Iterations = 1,

    [Parameter(Mandatory = $false)]
    [ValidateSet('svp', 'fpga', 'soc')]
    [string] $Platform = "soc",

    [Parameter(Mandatory = $false)]
    [ValidateRange(0, 1440)]
    [int] $TimeoutMinutes = 60,

    [Parameter(Mandatory = $false)]
    [ValidateRange(0, 300)]
    [int] $CooldownSeconds = 30,

    [Parameter(Mandatory = $false)]
    [string] $HostJson = "hsp_scp_bl_embed_fw.json",

    [Parameter(Mandatory = $false)]
    [switch] $SkipSetup,

    [Parameter(Mandatory = $false)]
    [string] $VaultName = "chie-1pfw",

    [Parameter(Mandatory = $false)]
    [string] $SshKeyPath = "$env:USERPROFILE\.ssh\id_rsa"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# =============================================================================
# Helper Functions
# =============================================================================

function Invoke-ProcessCleanup {
    param(
        [string] $Reason = "cleanup",
        [int] $WaitSeconds = 10,
        [switch] $Quiet
    )
    if (-not $Quiet) {
        Write-Host "  [$Reason] Cleaning up stale processes..." -ForegroundColor DarkGray
    }
    # Kill Robot Framework Python processes
    Get-CimInstance Win32_Process -Filter "Name LIKE 'python%'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -match "robot" } |
        ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
    # Kill SSH tunnel Python processes
    Get-CimInstance Win32_Process -Filter "Name LIKE 'python%'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -match "sshtunnel|paramiko|ssh_forward" } |
        ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
    # Kill orphaned ssh.exe
    Get-Process ssh -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    if ($WaitSeconds -gt 0) {
        if (-not $Quiet) { Write-Host "    Waiting $WaitSeconds seconds for BMC to release sessions..." -ForegroundColor DarkGray }
        Start-Sleep -Seconds $WaitSeconds
    }
    if (-not $Quiet) { Write-Host "  [$Reason] Cleanup complete." -ForegroundColor DarkGray }
}

function Get-LatestTestResultsDir {
    $pythiaDir = Join-Path $env:REPO_APP_TEST_LOG_DIR "pythia"
    if (-not (Test-Path $pythiaDir)) { return $null }
    $latest = Get-ChildItem -Path $pythiaDir -Directory | Sort-Object CreationTime -Descending | Select-Object -First 1
    return $latest.FullName
}

function Test-SuiteResults {
    param([Parameter(Mandatory)] [string] $ResultsDir)
    $details = @(); $passCount = 0; $failCount = 0; $skipCount = 0

    # Try xUnit Results.xml first
    $resultsFile = Join-Path $ResultsDir "Results.xml"
    if (Test-Path $resultsFile) {
        [xml]$xml = Get-Content $resultsFile -Encoding UTF8
        $testcases = $xml.SelectNodes("//testcase")
        if ($testcases -and $testcases.Count -gt 0) {
            foreach ($tc in $testcases) {
                $name = $tc.GetAttribute("name")
                $failNode = $tc.SelectSingleNode("failure")
                $skipNode = $tc.SelectSingleNode("skipped")
                if ($failNode) { $status = "FAIL"; $failCount++ }
                elseif ($skipNode) { $status = "SKIP"; $skipCount++ }
                else { $status = "PASS"; $passCount++ }
                $details += [PSCustomObject]@{ Name = $name; Status = $status }
            }
            $total = $passCount + $failCount + $skipCount
            $allPassed = ($failCount -eq 0) -and ($skipCount -eq 0) -and ($total -gt 0)
            return @{ Success = $allPassed; Total = $total; Pass = $passCount; Fail = $failCount; Skip = $skipCount
                      Message = $(if ($allPassed) { "All $total tests passed." } else { "Results: $passCount passed, $failCount failed, $skipCount skipped out of $total." })
                      Details = $details }
        }
    }
    # Fallback to output.xml
    $outputFile = Join-Path $ResultsDir "output.xml"
    if (Test-Path $outputFile) {
        [xml]$xml = Get-Content $outputFile -Encoding UTF8
        $tests = $xml.SelectNodes("//test")
        if ($tests -and $tests.Count -gt 0) {
            foreach ($t in $tests) {
                $name = $t.GetAttribute("name")
                $statusNode = $t.SelectSingleNode("status")
                $status = if ($statusNode) { $statusNode.GetAttribute("status") } else { "UNKNOWN" }
                $details += [PSCustomObject]@{ Name = $name; Status = $status }
                switch ($status) { "PASS" { $passCount++ } "FAIL" { $failCount++ } "SKIP" { $skipCount++ } default { $failCount++ } }
            }
            $total = $passCount + $failCount + $skipCount
            $allPassed = ($failCount -eq 0) -and ($skipCount -eq 0) -and ($total -gt 0)
            return @{ Success = $allPassed; Total = $total; Pass = $passCount; Fail = $failCount; Skip = $skipCount
                      Message = $(if ($allPassed) { "All $total tests passed." } else { "Results: $passCount passed, $failCount failed, $skipCount skipped out of $total." })
                      Details = $details }
        }
    }
    return @{ Success = $false; Total = 0; Pass = 0; Fail = 0; Skip = 0; Message = "No Results.xml or output.xml found in: $ResultsDir"; Details = @() }
}

function Start-Watchdog {
    param([int] $TimeoutMinutes)
    if ($TimeoutMinutes -le 0) { return $null }
    return Start-Job -ScriptBlock {
        param($timeout)
        Start-Sleep -Seconds $timeout
        Get-CimInstance Win32_Process -Filter "Name LIKE 'python%'" |
            Where-Object { $_.CommandLine -match "robot" } |
            ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
    } -ArgumentList ($TimeoutMinutes * 60)
}

function Stop-Watchdog {
    param($WatchdogJob)
    if ($WatchdogJob) {
        $WatchdogJob | Stop-Job -ErrorAction SilentlyContinue
        $WatchdogJob | Remove-Job -Force -ErrorAction SilentlyContinue
    }
}

function Show-Summary {
    param([array] $Results, [datetime] $StartTime)
    $totalDuration = ((Get-Date) - $StartTime).ToString("hh\:mm\:ss")
    $passedIterations = @($Results | Where-Object { $_.Status -eq "PASS" }).Count
    Write-Host ""
    Write-Host "----------------------------------------------------------------------" -ForegroundColor Cyan
    Write-Host "  SUMMARY" -ForegroundColor Cyan
    Write-Host "----------------------------------------------------------------------" -ForegroundColor Cyan
    Write-Host "  Iterations Passed : $passedIterations / $($Results.Count)" -ForegroundColor Cyan
    Write-Host "  Total Duration    : $totalDuration" -ForegroundColor Cyan
    Write-Host "----------------------------------------------------------------------" -ForegroundColor Cyan
    foreach ($r in $Results) {
        $color = if ($r.Status -eq "PASS") { "Green" } else { "Red" }
        Write-Host "  Iteration $($r.Iteration.ToString().PadLeft(3)): [$($r.Status)]  Duration: $($r.Duration)  $($r.Message)" -ForegroundColor $color
    }
    Write-Host "----------------------------------------------------------------------" -ForegroundColor Cyan
    Write-Host ""
}

# =============================================================================
# Ctrl+C Trap
# =============================================================================

$script:interrupted = $false
[Console]::TreatControlCAsInput = $false
Register-EngineEvent -SourceIdentifier PowerShell.Exiting -Action {
    Invoke-ProcessCleanup -Reason "Script exit" -WaitSeconds 5
} -ErrorAction SilentlyContinue | Out-Null
trap {
    $script:interrupted = $true
    Write-Host "`n  INTERRUPTED: Cleaning up..." -ForegroundColor Red
    Invoke-ProcessCleanup -Reason "Ctrl+C" -WaitSeconds 10
    Write-Host "  Exiting." -ForegroundColor Red
    exit 130
}

# =============================================================================
# Step 0: Validate Prerequisites
# =============================================================================

Write-Host ""
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host "  Pythia Local Test Runner" -ForegroundColor Cyan
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host "  Test Suite : $TestSuite" -ForegroundColor Cyan
Write-Host "  Platform   : $Platform" -ForegroundColor Cyan
Write-Host "  Agent      : $(if ($TrimmedAgentName) { $TrimmedAgentName } else { '(default)' })" -ForegroundColor Cyan
Write-Host "  Iterations : $Iterations" -ForegroundColor Cyan
Write-Host "  Timeout    : $TimeoutMinutes min per iteration" -ForegroundColor Cyan
Write-Host "  Skip Setup : $SkipSetup" -ForegroundColor Cyan
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host ""

if (-not $env:REPO_APP_ROOT -or $env:REPO_APP_ROOT -eq "") {
    Write-Host "ERROR: REPO_APP_ROOT is not set. Run start.ps1 first." -ForegroundColor Red; exit 1
}
if (-not (Get-Command "Invoke-Pythia" -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: Invoke-Pythia not found. Run start.ps1 first." -ForegroundColor Red; exit 1
}
if ($Platform -eq "soc" -and -not $TrimmedAgentName) {
    Write-Host "ERROR: -TrimmedAgentName required for SoC. Example: -TrimmedAgentName s14n1" -ForegroundColor Red; exit 1
}

$python = "${env:REPO_APP_PATH_python.win64}\tools\python.exe"
if (-not (Test-Path $python)) {
    Write-Host "ERROR: Python not found at $python. Run start.ps1 first." -ForegroundColor Red; exit 1
}

# =============================================================================
# Step 1: Credential & SSH Setup (unless -SkipSetup)
# =============================================================================

if (-not $SkipSetup) {
    Write-Host "[1/3] Setting up credentials and SSH..." -ForegroundColor Yellow

    # -- 1a. SSH Key Setup --
    Write-Host "  Setting up SSH key..." -ForegroundColor Cyan
    $sshDir = Split-Path $SshKeyPath -Parent
    if (-not (Test-Path $sshDir)) { New-Item -ItemType Directory -Path $sshDir -Force | Out-Null }

    if (Test-Path $SshKeyPath) {
        $keyContent = Get-Content $SshKeyPath -Raw
        $hasPassphrase = $keyContent -match "ENCRYPTED"
        if (-not $hasPassphrase) {
            $testResult = ssh-keygen -y -P "" -f $SshKeyPath 2>&1
            $hasPassphrase = ($testResult | Select-String -Pattern "incorrect passphrase" -Quiet)
        }
        if ($hasPassphrase) {
            Write-Host "    Key is passphrase-protected. Backing up and regenerating..." -ForegroundColor Yellow
            $backupPath = "$SshKeyPath.bak_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
            Copy-Item $SshKeyPath $backupPath -Force
            if (Test-Path "$SshKeyPath.pub") { Copy-Item "$SshKeyPath.pub" "$backupPath.pub" -Force }
            Write-Host "    Backup: $backupPath" -ForegroundColor DarkGray
            Remove-Item $SshKeyPath -Force
            if (Test-Path "$SshKeyPath.pub") { Remove-Item "$SshKeyPath.pub" -Force }
            ssh-keygen -t rsa -b 4096 -f $SshKeyPath -N "" -q
            Write-Host "    New key generated (no passphrase)." -ForegroundColor Green
        } else {
            Write-Host "    SSH key OK (no passphrase)." -ForegroundColor Green
        }
    } else {
        Write-Host "    Generating new SSH key..." -ForegroundColor Yellow
        ssh-keygen -t rsa -b 4096 -f $SshKeyPath -N "" -q
        Write-Host "    Key generated at $SshKeyPath" -ForegroundColor Green
    }

    # -- 1b. SSH Agent Setup --
    $sshAgentService = Get-Service ssh-agent -ErrorAction SilentlyContinue
    if ($sshAgentService) {
        $needsAdmin = ($sshAgentService.StartType -ne 'Automatic') -or ($sshAgentService.Status -ne 'Running')
        if ($needsAdmin) {
            $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
            if ($isAdmin) {
                if ($sshAgentService.StartType -ne 'Automatic') { Set-Service ssh-agent -StartupType Automatic }
                if ($sshAgentService.Status -ne 'Running') { Start-Service ssh-agent }
            } else {
                Write-Host "    Elevating to start ssh-agent (UAC prompt)..." -ForegroundColor Yellow
                try {
                    Start-Process powershell.exe -ArgumentList "-NoProfile","-ExecutionPolicy","Bypass","-Command","Set-Service ssh-agent -StartupType Automatic; Start-Service ssh-agent" -Verb RunAs -Wait -WindowStyle Hidden
                    Write-Host "    ssh-agent started." -ForegroundColor Green
                } catch { Write-Host "    WARNING: Could not start ssh-agent." -ForegroundColor Yellow }
            }
        } else {
            Write-Host "    ssh-agent already running." -ForegroundColor Green
        }
        $sshAgentService = Get-Service ssh-agent -ErrorAction SilentlyContinue
        if ($sshAgentService.Status -eq 'Running' -and (Test-Path $SshKeyPath)) {
            ssh-add $SshKeyPath 2>&1 | Out-Null
        }
    }

    # -- 1c. Azure CLI Check --
    Write-Host "  Verifying Azure CLI login..." -ForegroundColor Cyan
    try { az account show 2>&1 | Out-Null }
    catch { Write-Host "ERROR: Not logged into Azure CLI. Run 'az login' first." -ForegroundColor Red; exit 1 }

    # -- 1d. Windows Keyring Credentials --
    Write-Host "  Populating Windows keyring from Key Vault '$VaultName'..." -ForegroundColor Cyan
    $credentials = @(
        @{ KeyringName = "rack_manager_host_pwd";  KeyVaultSecret = "rack-manager-host-pwd";  Description = "Rack Manager password" }
        @{ KeyringName = "bmc_admin_password";     KeyVaultSecret = "bmc-admin-password";      Description = "BMC admin password" }
        @{ KeyringName = "bmc_privmode_password";  KeyVaultSecret = "bmc-privmode-password";   Description = "BMC privmode password" }
    )
    $service = "account"
    foreach ($cred in $credentials) {
        Write-Host "    $($cred.Description)..." -NoNewline
        $secretValue = az keyvault secret show --vault-name $VaultName --name $cred.KeyVaultSecret --query "value" -o tsv
        if (-not $secretValue) { Write-Host " FAILED" -ForegroundColor Red; throw "Failed to get $($cred.KeyVaultSecret) from Key Vault" }
        & $python -c "import keyring; keyring.set_password('$service', '$($cred.KeyringName)', r'''$secretValue''')"
        if ($LASTEXITCODE -ne 0) { Write-Host " FAILED" -ForegroundColor Red; throw "Failed to store $($cred.KeyringName) in keyring" }
        Write-Host " OK" -ForegroundColor Green
    }

    # -- 1e. Verify Keyring --
    Write-Host "  Verifying keyring..." -NoNewline
    $allGood = $true
    foreach ($cred in $credentials) {
        $v = & $python -c "import keyring; v = keyring.get_password('$service', '$($cred.KeyringName)'); print('OK' if v else 'MISSING')"
        if ($v -ne "OK") { $allGood = $false }
    }
    if ($allGood) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " MISSING entries!" -ForegroundColor Red; exit 1 }

    # -- 1f. Create creds.yaml --
    Write-Host "  Creating creds.yaml..." -ForegroundColor Cyan
    Setup-LocalPythia -RepoRootPath $env:REPO_APP_ROOT
    Write-Host "    SECURE_FILE_PATH = $env:SECURE_FILE_PATH" -ForegroundColor DarkGray

    Write-Host "  Setup complete." -ForegroundColor Green
    Write-Host ""
} else {
    Write-Host "[1/3] Skipping setup (-SkipSetup)." -ForegroundColor DarkGray
    if (-not $env:SECURE_FILE_PATH) {
        try { Setup-LocalPythia -RepoRootPath $env:REPO_APP_ROOT } catch { }
    }
    Write-Host ""
}

# =============================================================================
# Step 2: Startup Cleanup
# =============================================================================

Write-Host "[2/3] Cleaning up stale processes..." -ForegroundColor Yellow
Invoke-ProcessCleanup -Reason "Startup" -WaitSeconds 15
Write-Host ""

# =============================================================================
# Step 3: Run Test Iterations
# =============================================================================

Write-Host "[3/3] Running test suite ($Iterations iteration(s))..." -ForegroundColor Yellow
Write-Host ""

$overallStartTime = Get-Date
$iterationResults = @()

for ($i = 1; $i -le $Iterations; $i++) {

    if ($i -gt 1) {
        Write-Host "  Cooldown before iteration $i..." -ForegroundColor DarkGray
        Invoke-ProcessCleanup -Reason "Between iterations" -WaitSeconds $CooldownSeconds
    }

    $iterStart = Get-Date
    Write-Host "----------------------------------------------------------------------" -ForegroundColor Yellow
    Write-Host "  ITERATION $i / $Iterations   ($($iterStart.ToString('yyyy-MM-dd HH:mm:ss')))" -ForegroundColor Yellow
    Write-Host "----------------------------------------------------------------------" -ForegroundColor Yellow

    $pythiaArgs = @{ test = $TestSuite; platform = $Platform }
    if ($TrimmedAgentName) { $pythiaArgs["TrimmedAgentName"] = $TrimmedAgentName }
    if ($Platform -ne "soc" -and $HostJson) { $pythiaArgs["hostjson"] = $HostJson }

    $watchdog = Start-Watchdog -TimeoutMinutes $TimeoutMinutes
    if ($TimeoutMinutes -gt 0) { Write-Host "  (Watchdog: $TimeoutMinutes min timeout)" -ForegroundColor DarkGray }

    $timedOut = $false
    try {
        Invoke-Pythia @pythiaArgs
    } catch {
        if ($watchdog -and $watchdog.State -eq "Completed") {
            $timedOut = $true
            Write-Host "`n  TIMEOUT: Iteration exceeded $TimeoutMinutes minutes." -ForegroundColor Red
        } else {
            Stop-Watchdog $watchdog
            Write-Host "`n  ERROR: $_ " -ForegroundColor Red
            $iterationResults += [PSCustomObject]@{ Iteration=$i; Status="ERROR"; Duration=((Get-Date)-$iterStart).ToString("hh\:mm\:ss"); Message="$_" }
            Show-Summary $iterationResults $overallStartTime; exit 1
        }
    }

    Stop-Watchdog $watchdog
    $iterDuration = ((Get-Date) - $iterStart).ToString("hh\:mm\:ss")

    if ($timedOut) {
        # Check logs for SSH gateway errors
        $sshGatewayError = $false
        $tDir = Get-LatestTestResultsDir
        if ($tDir) {
            $rlog = Join-Path $tDir "rlog.txt"
            if (Test-Path $rlog) {
                $sshGatewayError = [bool](Select-String -Path $rlog -Pattern "Could not establish session to SSH gateway|Error reading SSH protocol banner" -ErrorAction SilentlyContinue)
            }
        }
        $msg = if ($sshGatewayError) { "SSH gateway unreachable (rack manager lost connectivity)" } else { "Killed after $TimeoutMinutes min timeout" }
        $iterationResults += [PSCustomObject]@{ Iteration=$i; Status="TIMEOUT"; Duration=$iterDuration; Message=$msg }
        if ($sshGatewayError) {
            Write-Host "  ROOT CAUSE: Rack manager SSH gateway became unreachable." -ForegroundColor Red
            Write-Host "  Verify: Test-NetConnection <rack_manager_ip> -Port 22" -ForegroundColor Yellow
        }
        Show-Summary $iterationResults $overallStartTime; exit 1
    }

    $resultsDir = Get-LatestTestResultsDir
    if (-not $resultsDir) { Write-Host "  ERROR: No results directory found." -ForegroundColor Red; exit 1 }
    $results = Test-SuiteResults -ResultsDir $resultsDir

    if ($results.Success) {
        Write-Host "`n  ITERATION $i PASSED  ($($results.Message))  Duration: $iterDuration" -ForegroundColor Green
        Write-Host "    Results: $resultsDir" -ForegroundColor DarkGray
        $iterationResults += [PSCustomObject]@{ Iteration=$i; Status="PASS"; Duration=$iterDuration; Message=$results.Message }
    } else {
        Write-Host "`n  ITERATION $i FAILED  Duration: $iterDuration" -ForegroundColor Red
        Write-Host "    $($results.Message)" -ForegroundColor Red
        Write-Host "    Results: $resultsDir" -ForegroundColor DarkGray
        foreach ($d in $results.Details) {
            $c = switch ($d.Status) { "PASS" { "Green" } "FAIL" { "Red" } "SKIP" { "Yellow" } default { "Gray" } }
            Write-Host "      [$($d.Status)] $($d.Name)" -ForegroundColor $c
        }
        $iterationResults += [PSCustomObject]@{ Iteration=$i; Status="FAIL"; Duration=$iterDuration; Message=$results.Message }
        Show-Summary $iterationResults $overallStartTime; exit 1
    }
}

Write-Host "`n  SUCCESS: All $Iterations iterations passed!" -ForegroundColor Green
Show-Summary $iterationResults $overallStartTime
exit 0
