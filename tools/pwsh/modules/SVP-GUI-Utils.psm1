#
# Copyright (c) Microsoft Corporation. All rights reserved.
#

<#
SVP-Gui-Utils.psm1

Description ->
    These utilities are meant to be used for quick developer iteration and sanity checks.
    These are NOT meant to validate firmware requirements and should not be invoked in pipelines
    Please use the Pythia functional testing framework to ensure a uniform simulation configuration for validation.
#>

<#
.SYNOPSIS
Read all the config files in the tools\vpcfg directory and return a hashtable of the contents.

#>
Function Read-ConfigFiles()
{
    $config_dir = Join-Path -Path $env:REPO_APP_ROOT -ChildPath "tools\vpcfg"
    $ConfigParams = @{}
    Get-ChildItem -Path $config_dir -Filter "*.txt" | ForEach-Object {
        $content = Get-Content $_.FullName
        $configName = $_.BaseName -replace '^svpcfg-', ''
        $ConfigParams[$configName] =  $content
    }
    return $ConfigParams
}

<#
.SYNOPSIS
Modify the vpcfg file to enable or disable autoContinueInitialCrunch and traceSimOutput

.PARAMETER $ConfigXml
The path to the vpcfg file

#>
Function Set-VpCfg( [Parameter(Mandatory=$true)] [string] $ConfigXml)
{
    if (Test-Path $ConfigXml)
    {
        $xml = New-Object System.Xml.XmlDocument
        $xml.Load($ConfigXml)
        $debugInfo = $xml.SelectSingleNode("//debugInfo")
        if ($null -eq $debugInfo)
        {
            $debugInfo = $xml.CreateElement("debugInfo")
            $xml.DocumentElement.AppendChild($debugInfo)
        }
        $debugInfo.SetAttribute("autoContinueInitialCrunch", "true")
        $debugInfo.SetAttribute("traceSimOutput", "true")
        $xml.Save($ConfigXml)
    }
}

<#
.SYNOPSIS
Stop any running simulations

#>
Function Stop-Sims()
{
    # check if sim is running
    $processes = Get-Process -Name 'sim' -ErrorAction SilentlyContinue

     # Stop the executables if they are there (implies GUI mode if there after stop job)
     Stop-Process -Name "sim" -Force -ErrorAction SilentlyContinue
     Stop-Process -Name "_ui_vs" -Force -ErrorAction SilentlyContinue
     Stop-Process -Name "vs" -Force -ErrorAction SilentlyContinue

    # if the sim was running, delay for processes to stop prior to restarting.
     if ($processes) {
        Start-Sleep -Seconds 3
     }
}

<#
.SYNOPSIS
Initialize the simulation environment

#>
Function Initialize-SimEnv()
{
    $svp_runtime_dir = ${env:REPO_APP_PATH_microsoft.internal.windows.virtualizerruntime}

    # Set License Servers and Search Paths
    $env:ARMLMD_LICENSE_FILE="40002@wanlic.svceng.com"
    $env:SNPSLMD_LICENSE_FILE = "40003@gem-lic-01.svceng.com;40003@wanlic.svceng.com"
    $env:SNPS_VPX_START_SIMULATION_TIMEOUT = 600
    $env:SNPS_VPSESSION_LAUNCH_TIMEOUT_SEC = 600
    $env:SNPS_VPX_DEFAULT_TIMEOUT = 600
    $env:SNPS_VS_VDK_SEARCH_PATHS=(Resolve-Path ${env:REPO_APP_PATH_microsoft.internal.virtualized.kingsgate.svp}\win\release\KingsgateSVP\*\*\).toString()

    # Clear any app data for synopsys, prevents stale data from causing simulation conflicts.
    $cur_user = get-content env:username
    $cur_user_synopsys_appdata = "C:\users\" + $cur_user + "\AppData\Roaming\Synopsys"

    if (Test-Path -Path $cur_user_synopsys_appdata) {
        Remove-Item ($cur_user_synopsys_appdata) -Recurse -Force
    }

    # The difference between running with `tve` and `tvrb` is that the license server used changes. When using `tvrb` the regression
    # licenses are used, which we have a lot more of. This pool however does not allow for the GUI to attach to a simulation running
    # the regression license. Since this is a GUI simulation, we use the `tve` license server.
    Invoke-CmdScript "$svp_runtime_dir\SLS\windows\setup.bat" tve
}

<#
.SYNOPSIS
Check SVP workspace version against package xml SVP version and delete workspace if versions mismatch.
If deleted, a new workspace will be generated. Workspace generation takes time so it is skipped if the versions match.

.PARAMETER $workspaceJsonFilePath
location of the workspace json file

.PARAMETER $svpPackageFilePath
location of the package xml file

#>
Function Test-And-Delete-Workspace(
    [Parameter(Mandatory=$true)] [string] $workspaceJsonFilePath,
    [Parameter(Mandatory=$true)] [string] $svpPackageFilePath
)
{
    # Ensure that both files exist
    if (-not (Test-Path $svpPackageFilePath)) {
        Write-Error "The Package XML file does not exist: $svpPackageFilePath"
        return
    }

    if (-not (Test-Path $workspaceJsonFilePath)) {
        Write-Error "The Workspace JSON file does not exist: $workspaceJsonFilePath"
        return
    }

    # Extract the version from a packages xml
    [xml]$xmlContent = Get-Content -Path $svpPackageFilePath
    $packageNode = $xmlContent.SelectSingleNode("//package[@name='microsoft.internal.virtualized.kingsgate.svp']")

    # Check if the packageNode or version is null
    if (-not $packageNode) {
        Write-Error "Package name 'microsoft.internal.virtualized.kingsgate.svp' not found in the XML file."
        return
    }

    $xmlPackageVersion = $packageNode.version
    Write-Host "== Xml Package: $svpPackageFilePath  Version: $xmlPackageVersion =="



    Write-Host "== Workspace: $workspaceJsonFilePath =="
    # Extract the version from the JSON config file in workspace directory
    # Read the content of the JSON file
    $jsonContent = Get-Content -Path $workspaceJsonFilePath -Raw
    $jsonObject = $jsonContent | ConvertFrom-Json

    $templatePath = $jsonObject.template_path

    # Using regex to search for virtualized\.kingsgate\.svp to extract the version number
    $versionMatch = $templatePath -match 'virtualized\.kingsgate\.svp\.(\d+(?:\.\d+)+)'

    if ($versionMatch) {
        $workspaceVersion = $matches[1]
        Write-Host "== Workspace version: $workspaceVersion =="
    } else {
        Write-Error "Version pattern not found in template_path"
        return
    }

    # Compare the versions
    if ($xmlPackageVersion -ne $workspaceVersion) {
        Write-Host "SVP Versions differ: Package XML version ($xmlPackageVersion), Workspace JSON version ($workspaceVersion)"

        # Delete the workspace directory
        $WorkspaceDir = "$env:REPO_APP_ROOT.svp_simulator\workspace_gui"
        Remove-Item -Path $workspaceDir -Recurse -Force
        Write-Host "Deleted the workspace directory: $workspaceDir"
    } else {
        Write-Host "== Versions match: $xmlPackageVersion =="
    }
}

Function New-VPconfig(
    [Parameter(Mandatory=$true)] [string] $SimConfig
)
{
    $ConfigParams = Read-ConfigFiles

    if (-not $ConfigParams.ContainsKey($SimConfig))
    {
        Throw "config $SimConfig does not exist"
    }

    $CfgDir = "$env:REPO_APP_ROOT.svp_simulator\workspace_gui\KingsgateSVP\vpconfigs\$SimConfig"

    if (Test-Path $CfgDir) {
        Remove-Item -Path $CfgDir -Recurse -Force
    }

    $WorkspaceDir = "$env:REPO_APP_ROOT.svp_simulator\workspace_gui"
    $OutputDir = "$env:REPO_APP_ROOT.svp_simulator\gui_output"

    $SimDir = ${env:REPO_APP_PATH_microsoft.internal.virtualized.kingsgate.svp}

    $ProbeDir = $env:REPO_APP_ROOT + "tools/svp_probes"
    
    $SimArgs = @(
       "-d", "$WorkspaceDir",
        "-s", "$SimDir\win\release\run_fixed_vdk.py",
        "--pyargs",
        "--output_dir", "$OutputDir",
        "--template_name", "KingsgateSVP",
        "--norun",
        "--sim_probe", "$ProbeDir/mother_probe.py"
    )

    $SimArgs += @("--vpconfig", "$SimConfig")

    Write-Host "== Create VPconfig $SimConfig =="
    Write-Host "_> vssh.exe $SimArgs ... --pyargs_end"

    # Was part of conflict, Adding ExpandEnvironmentVariables was added as part of running Simulation using same config in pythia and also locally
    $SimArgs += $ConfigParams["$SimConfig"] | ForEach-Object { @("--parameter", [System.Environment]::ExpandEnvironmentVariables($_)) }
    $SimArgs += "--pyargs_end"

    & "vssh.exe" $SimArgs
}

<#
.SYNOPSIS
Run a gui simulation

.PARAMETER $SimConfig
The simulation configuration to run

#>
Function Start-SimGui(
    [Parameter(Mandatory=$false)] [ValidateSet('sideloaded_chie_bins', 'chie_bins_single_die_dat', 'chie_bins_dual_die_dat', 'ap_baremetal_dual_die_dat', 'ap_baremetal_single_die_dat')] [string] $SimConfig = "chie_bins_dual_die_dat"
)
{
    Write-Host "== Stop Running Simulations =="
    Stop-Sims

    Write-Host ""
    Write-Host "== Check for newer SVP Release =="
    Write-Host ""

    # path example: .svp_simulator\workspace_gui\KingsgateSVP\KingsgateSVP.vdkfxd
    $WorkspaceDir = "$env:REPO_APP_ROOT.svp_simulator\workspace_gui"
    $WorkspaceDirJsonPath = $WorkspaceDir + "\KingsgateSVP\KingsgateSVP.vdkfxd"
    $svpPackageFilePath = "$env:REPO_APP_ROOT" + "tools\packages.arm-eabi-aarch.xml"

    # Calling the Test-And-Delete-Workspace function before SimEnv initialization as workspace dir will be recreated
    # with New-VPconfig if deleted as part of mismatch check
    Test-And-Delete-Workspace -workspaceJsonFilePath $WorkspaceDirJsonPath -svpPackageFilePath $svpPackageFilePath

    Initialize-SimEnv

    $CfgDir = "$env:REPO_APP_ROOT.svp_simulator\workspace_gui\KingsgateSVP\vpconfigs\$SimConfig"
    if (-not (Test-Path $CfgDir))
    {
        # gen vpcfg
        New-VPconfig -SimConfig $SimConfig
    }

    $ConfigXml = Join-Path -Path $CfgDir -ChildPath "$SimConfig.vpcfg"
    $WorkspaceDir = "$env:REPO_APP_ROOT.svp_simulator\workspace_gui"
    $SimArgs = @("-d", $WorkspaceDir, "-r", $ConfigXml)

    Write-Host ""
    Write-Host "== Launch $SimConfig =="

    Set-VpCfg $ConfigXml
    Write-Host "Cmd Line: vs.exe $SimArgs"

    & "vs.exe" $SimArgs
}

<#
.SYNOPSIS
Gets the help information of the commands available for running svp in GUI mode.

.EXAMPLE
Get-SvpGuiHelp
#>
Function Get-SvpGuiHelp()
{
    Write-Host "Available Commands for running SVP with GUI:"
    $Commands = (Get-Module SVP-GUI-Utils).ExportedAliases.Values.GetEnumerator()

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

New-Alias -Name runsvpgui -Value Start-SimGui -Force
New-Alias -Name stopsvpgui -Value Stop-Sims -Force
New-Alias -Name helpsvpgui -Value Get-SvpGuiHelp -Force

Export-ModuleMember -Alias * -Function *