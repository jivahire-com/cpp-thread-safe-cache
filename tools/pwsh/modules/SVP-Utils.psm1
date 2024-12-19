#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

<#
.SYNOPSIS
Stops a SVP Virtualizer Simulation if one exists.

.EXAMPLE
Stop-Virtualizer
#>
Function Stop-Virtualizer()
{
    if ($null -ne $env:SVP_SIM_JOB_ID)
    {
        # set to $true to print out simulator output for debugging
        $DebugMode = $false;

        if($DebugMode) {
            $retrievedJob = Get-Job -Id $env:SVP_SIM_JOB_ID

            # Retrieve and print the job output, including checking for errors
            $output = $retrievedJob | Receive-Job -Keep

            # Check if output contains any errors
            foreach ($item in $output) {
                if ($item -is [System.Management.Automation.ErrorRecord]) {
                    Write-Host "Error: $($item.Exception.Message)" -ForegroundColor Red
                }
                else {
                    Write-Host "Output: $item"
                }
            }

            # Check detailed job information if needed
            $retrievedJob | Get-Job | Format-List *
        }

        # Stopping the Job will stop headless mode
        Write-Host "`tStopping Simulator background job: ID == $env:SVP_SIM_JOB_ID"
        Stop-Job -Id $env:SVP_SIM_JOB_ID
        $env:SVP_SIM_JOB_ID = $null
    }
    else {
        Write-Host "`tNo Simulator background job to stop."
    }

    # Stop the executables if they are there (implies GUI mode if there after stop job)
    Stop-Process -Name "sim" -Force -ErrorAction SilentlyContinue
    Stop-Process -Name "_ui_vs" -Force -ErrorAction SilentlyContinue
    Stop-Process -Name "vs" -Force -ErrorAction SilentlyContinue
}

<#
.SYNOPSIS
Starts a SVP Virtualizer Simulation, running in a background job.

.PARAMETER SimConfig
What default simulation parameters to launch with. Default: scp_mcp_chie_bins


.EXAMPLE
Invoke-Virtualizer -SimConfig chie_bins_single_die_dat
#>
Function Invoke-Virtualizer(
    [Parameter(Mandatory=$false)] [ValidateSet('sideloaded_chie_bins', 'chie_bins_single_die_dat', 'chie_bins_dual_die_dat', 'ap_baremetal_dual_die_dat')] [string] $SimConfig = "ap_baremetal_dual_die_dat"
)
{
    # Setup the workspace for the virtualizer
    $workspace_dir = "$env:REPO_APP_ROOT.svp_simulator\workspace"
    $workspace_dir = "$workspace_dir".replace("/", "\")

    # Grab the paths to the runtime and the simulation vdk from the SVP Team
    $svp_sim_dir = ${env:REPO_APP_PATH_microsoft.internal.virtualized.kingsgate.svp}
    $svp_runtime_dir = ${env:REPO_APP_PATH_microsoft.internal.windows.virtualizerruntime}

    # Set License Servers and Search Paths
    $env:ARMLMD_LICENSE_FILE="40002@wanlic.svceng.com"
    $env:SNPSLMD_LICENSE_FILE="40003@gem-lic-01.svceng.com;40003@wanlic.svceng.com"
    $env:SNPS_VPX_START_SIMULATION_TIMEOUT = 600
    $env:SNPS_VPSESSION_LAUNCH_TIMEOUT_SEC = 600
    $env:SNPS_VPX_DEFAULT_TIMEOUT = 600

    $env:SNPS_VS_VDK_SEARCH_PATHS=(Resolve-Path ${env:REPO_APP_PATH_microsoft.internal.virtualized.kingsgate.svp}\win\release\KingsgateSVP\*\*\).toString()

    # Cleanup any SVP Setup that may be running. Do this before cleaning anything else up.
    Write-Host ""
    Write-Host "Stopping the current simulation (if there is one)."
    Write-Host ""

    Stop-Virtualizer

    # Clear any app data for synopsys, prevents stale data from causing simulation conflicts.
    $cur_user = get-content env:username
    $cur_user_synopsys_appdata = "C:\users\" + $cur_user + "\AppData\Roaming\Synopsys"

    if (Test-Path -Path $cur_user_synopsys_appdata) {
        Remove-Item ($cur_user_synopsys_appdata) -Recurse -Force
    }

    # Rename the existing workspace if it exists
    if (Test-Path -Path $workspace_dir) {
        $old_workspace_dir = $workspace_dir + "_" + (Get-Date -Format "yyyyMMddhhmmss")
        Rename-Item -Path $workspace_dir -NewName $old_workspace_dir -Force
    }

    # Dump the parameters used for invoking SVP
    Write-Host ""
    Write-Host "Invoking SVP - Using Config:"
    Write-Host "`t-svp_sim_dir    : $svp_sim_dir"
    Write-Host "`t-svp_runtime_dir: $svp_runtime_dir"
    Write-Host ""
    Write-Host "Using Settings:"
    Write-Host "`t-ARMLMD_LICENSE_FILE               : $env:ARMLMD_LICENSE_FILE"
    Write-Host "`t-SNPSLMD_LICENSE_FILE              : $env:SNPSLMD_LICENSE_FILE"
    Write-Host "`t-SNPS_VPX_START_SIMULATION_TIMEOUT : $env:SNPS_VPX_START_SIMULATION_TIMEOUT"
    Write-Host "`t-SNPS_VPSESSION_LAUNCH_TIMEOUT_SEC : $env:SNPS_VPSESSION_LAUNCH_TIMEOUT_SEC"
    Write-Host "`t-SNPS_VPX_DEFAULT_TIMEOUT          : $env:SNPS_VPX_DEFAULT_TIMEOUT"
    Write-Host "`t-SNPS_VS_VDK_SEARCH_PATHS          : $env:SNPS_VS_VDK_SEARCH_PATHS"
    Write-Host ""
    Write-Host "Using Parameters:"
    Write-Host "`t-SimConfig    : $SimConfig"
    Write-Host "`t-WorkspaceDir : $workspace_dir"
    Write-Host "`t-MSCP vUART   : $env:REPO_APP_MSCP_VUART"

    # Select the config file based on SimConfig. These configurations are stored under tools/vpcfg.
    # Additionally perform any any config specific setup as needed
    $svpcfg_param_file = "$env:REPO_APP_ROOT\tools\vpcfg\svpcfg-$SimConfig.txt"

    # Build the additional simulation parameters to pass to the `run_fixed_vdk.py`, as a array of arguements.
    # This is uncurled in the call to the executable.
    $input_parameters = (Get-Content -Path $svpcfg_param_file | ForEach-Object { @("--parameter", "$($_.TrimEnd())") })

    # The difference between running with `tve` and `tvrb` is that the license server used changes. When using `tvrb` the regression
    # licenses are used, which we have a lot more of. This pool however does not allow for the GUI to attach to a simulation running
    # the regression license. Since this is run in headless mode, we will run with `tvrb` to use the regression licenses.
    Invoke-CmdScript "$svp_runtime_dir\SLS\windows\setup.bat" tvrb

    $job = $null

    Write-Host ""
    Write-Host "Starting simulation [$SimConfig] as a background job. Job information displayed on creation."
    Write-Host ""

    $job = Start-job -ScriptBlock {
        try {
            vssh.exe `
            -d $using:workspace_dir `
            -s $using:svp_sim_dir\win\release\run_fixed_vdk.py `
            --pyargs `
            --debug `
            --template_name KingsgateSVP `
            --vpconfig $using:SimConfig `
            --output_dir $using:env:REPO_APP_ROOT\.svp_simulator\ `
            $using:input_parameters `
            --pyargs_end
        }
        catch {
            Write-Error $_.Exception.Message
        }
    }


    # Both cases launch sim.exe, so we can validate that it started to ensure setup is correct.
    if ($null -ne $job)
    {
        $env:SVP_SIM_JOB_ID = $job.Id

        Write-Host ""
        Write-Host "Please wait for the 'sim.exe' to launch via the below job:"
        Write-Host "`tJob Details:"
        Write-Host "`t`tId    :" $job.Id
        Write-Host "`t`tName  :" $job.Name
        Write-Host "`t`tState :" $job.State
        Write-Host ""

        # Wait 300 seconds, checking every 5 seconds for the `sim.exe` to finish launching
        $time_waited_s = 0
        $timeout_s = 300
        while ($time_waited_s -lt $timeout_s) {
            Start-Sleep 5
            $time_waited_s += 5
            if ($null -ne (Get-Process | Where-Object {$_.Name -eq "sim"}))
            {
                Write-Host "`tSimulation Started. Time to Start: $time_waited_s seconds"
                Write-Host "`tThe Simulation can be attached to with the GUI: vs.exe -d $workspace_dir"
                break;
            }
        }

        if ($time_waited_s -eq $timeout_s)
        {
            Stop-Virtualizer
            Throw "Failed to start sim.exe within: $timeout_s seconds"
        }

        Write-Host ""
        Write-Host "Expected Simulation Connectivity: If connection issues, check port config in SVP log."
        Write-Host "`tMCP UART telnet : localhost:4256"
        Write-Host "`tSCP UART telnet : localhost:4257"

        Write-Host "`tMCP DIE 0 GDB   : localhost:12372"
        Write-Host "`tSCP DIE 0 GDB   : localhost:12373"
        Write-Host "`tMCP DIE 1 GDB   : localhost:12402"
        Write-Host "`tSCP DIE 1 GDB   : localhost:12403"
        Write-Host "`tEnsure configuration matches symbols used!"
        Write-Host ""

        Write-Host ""
        Write-Host "Run the following to stop the simulation background job: stopsvp"
        Write-Host ""
    }
}

<#
.SYNOPSIS
Gets the help information of the commands available for running svp.

.EXAMPLE
Get-SvpHelp
#>
Function Get-SvpHelp()
{
    Write-Host "Available Commands for SVP Virtualizer:"
    $Commands = (Get-Module SVP-Utils).ExportedAliases.Values.GetEnumerator()

    Write-Host ""
    foreach($Command in $Commands)
    {
        $Message = $Command.Name
        $Message = $Message + (" " * (20 - $Message.Length))
        $Help = Get-Help $Command
        Write-Host $Message -NoNewLine -ForegroundColor Yellow
        Write-Host $Help.Synopsis
    }
    Write-Host ""
}

New-Alias -Name runsvp -Value Invoke-Virtualizer -Force
New-Alias -Name stopsvp -Value Stop-Virtualizer -Force
New-Alias -Name helpsvp -Value Get-SvpHelp -Force

Export-ModuleMember -Alias * -Function *
