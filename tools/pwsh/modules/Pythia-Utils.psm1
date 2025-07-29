<# Copyright Microsoft Corporation #>

<#
.SYNOPSIS
Read a file and output the same contents of the file to a file, with all strings expanded. This expands environement variables.

.PARAMETER in_file
Path to the file to read the raw content from.

.PARAMETER out_file
Path to create a file at with expanded variables.

.EXAMPLE
Expand-File -in_file <file_with_env_vars> -out_file <file_with_env_vars>.expanded
#>
Function Expand-File(
    [Parameter(Mandatory=$true)] [string] $in_file,
    [Parameter(Mandatory=$true)] [string] $out_file
){

    # Read the contents of the input file
    $content = Get-Content $in_file -Raw

    # If the input is a string, expand any environment variables in it
    if ($content -is [string]) {
        $content = $ExecutionContext.InvokeCommand.ExpandString($content)
    }
    else {
        Throw "Unable to parse file. Raw content not a string! : $in_file"
    }

    # Send the content to the outfile
    $content | Set-Content $out_file
}

<#
.SYNOPSIS
Starts a local Pythia Test Run, using the SVP Recipe.

.PARAMETER test
Path to the test to run. Can be a robot file, or a directory containing robot files.

.EXAMPLE
Invoke-Pythia -test .\tests\functional\test_suites\heart_beat\
#>
Function Invoke-Pythia(
    [Parameter(Mandatory=$true)] [string] $test,
    [Parameter(Mandatory=$true)] [ValidateSet('svp', 'fpga', 'soc')] [string] $platform = "svp",
    [Parameter(Mandatory=$false)] [string] $ifwi = "$env:REPO_APP_BUILD_DIR/Debug/arm-eabi-aarch/bin/flash/kingsgate.ifwi.svp.debug.custom.dat",
    [Parameter(Mandatory=$false)] [string] $hostjson = "hsp_scp_bl_embed_fw.json"
)
{

    Write-Host ""
    Write-Host "`tInvoking Pythia Functional Test"
    Write-Host ""
    
    # Ensure REPO_APP_ROOT is set
    if (-not $env:REPO_APP_ROOT -or $env:REPO_APP_ROOT -eq "") {
        throw "ERROR: REPO_APP_ROOT is not set. Please set it before running tests."
    }

    $env:REPO_TEST_DIR = (Join-Path $env:REPO_APP_ROOT "tests/functional").replace("\", "/")
    # Ensure test path is absolute but avoid duplicating root path
    if (-not $test -or $test -eq "") {
        $test = Join-Path -Path $env:REPO_TEST_DIR -ChildPath "test_suites"
    } elseif (-not [System.IO.Path]::IsPathRooted($test)) {
        $test = Join-Path -Path $env:REPO_APP_ROOT -ChildPath $test
    }

    if (-not (Test-Path -Path $test)) {
        throw "ERROR: Test path not found: $test"
    }

    if ($platform -eq "svp") {
        # IFWI dat file to load in the simulation - not used in FPGA
        if ($ifwi.StartsWith(".")) {
            $ifwi = $ifwi.Substring(1)
            $env:IFWI_TEST_FILE = (Join-Path -Path $env:REPO_APP_ROOT -ChildPath $ifwi).replace("\", "/")
        }
        else 
        {
            $env:IFWI_TEST_FILE = $ifwi.replace("\", "/")
        }
    } elseif ($platform -eq "fpga") {
        $env:IFWI_TEST_FILE = "Applicable for SVP only. For FPGA, the IFWI is pre-programmed."
    }

    # Read include/exclude tags from JSON if CLI parameter is empty
    $env:RESOURCES = $env:REPO_TEST_DIR
    $env:LIBRARIES = Join-Path -Path $env:REPO_TEST_DIR -ChildPath "library"

    # Create test run paths based on timestamp
    $time_stamp = (Get-Date -Format "yyyy_MM_dd_hhmmss")
    $pythia_test_dir = Join-Path -Path $env:REPO_APP_TEST_LOG_DIR -ChildPath "pythia"
    $test_results_dir = Join-Path -Path $pythia_test_dir -ChildPath $time_stamp

    # Create the test results directory
    New-Item -ItemType Directory -Path $test_results_dir -Force | Out-Null

    # Setup the paths for the necessary Pythia configurations
    $workspace_config_in = Join-Path -Path $env:REPO_TEST_DIR -ChildPath "configs/workspace.json"
    $workspace_config_out = Join-Path -Path $test_results_dir -ChildPath "workspace.json"
    $env:REPO_TEST_SUITES_PATH = Join-Path -Path $env:REPO_TEST_DIR -ChildPath "test_suites"

    # Expand environment variables in JSON configuration files
    Expand-File -in_file $workspace_config_in -out_file $workspace_config_out

    if ($platform -eq "svp") {
        $host_configs_path = Join-Path -Path $env:RESOURCES -ChildPath "hosts/svp_hosts"
    } elseif ($platform -eq "fpga") {
        $host_configs_path = Join-Path -Path $env:RESOURCES -ChildPath "hosts/fpga_hosts"
    } elseif ($platform -eq "soc") {
        $host_configs_path = Join-Path -Path $env:RESOURCES -ChildPath "hosts/soc_hosts"
    }

    $iteration = 1
    Get-ChildItem $host_configs_path |
    Foreach-Object {
        $host_config_in = Join-Path $host_configs_path -ChildPath $_.Name
        $host_config_out = Join-Path -Path $test_results_dir -ChildPath $_.Name

        # Create new json files with expanded string values, expanding environment variables
        Expand-File -in_file $host_config_in -out_file $host_config_out
        Write-Host "`t` Host Config $iteration : $host_config_out"
        $iteration++
    }

    # Set the payload to the pre-made SVP Payload
    $recipe_payload_dir = Join-Path -Path $env:RESOURCES -ChildPath "payloads/payload_svp"

    Write-Host ""
    Write-Host "`tStoring Test Results: $test_results_dir"
    Write-Host ""

    Write-Host ""
    Write-Host "`t`tUsing Test: $test"
    Write-Host "`t`tUsing Workspace Config: $workspace_config_out"
    Write-Host "`t`tUsing Payload: $recipe_payload_dir"
    Write-Host "`t`tUsing IFWI: $env:IFWI_TEST_FILE"
    Write-Host "`t`tAt Present, this IFWI file is only configurable for SVP. For FPGA, the IFWI is pre-programmed."
    Write-Host ""

    # Set SNPS search paths if platform is SVP
    if ($platform -eq "svp") {
    	# This can also be set via the host configuration json for pythia, however that requires absolute paths.
    	# Resolving the paths allows us to update the version and not have to update the paths.
        $env:SNPS_VS_VDK_SEARCH_PATHS=(Resolve-Path ${env:REPO_APP_PATH_microsoft.internal.virtualized.kingsgate.svp}\win\release\KingsgateSVP\*\*\).toString()
    }

    # Move to the test directory. Pythia will use relative paths once executed, moving into the test
    # dir will put any any folders that get created there (for things we can't configure atm, ex: SVP output dir).
    # Move to the test directory
    Push-Location -Path $test_results_dir

    $destinationPath = "$env:REPO_APP_BUILD_DIR"
    # since we are using soc dat file for fpga platform, the binaries will be in .build/soc folder
    $pctoolFilePath = "$destinationPath/postcodes_and_traces.json"
    if([System.IO.File]::Exists($pctoolFilePath))
    {
        Write-Host "Set pctool file path to: $pctoolFilePath"
        $env:PCTOOL_PATH = $pctoolFilePath
    } else {
        Write-Host "$pctoolFilePath not found"
    }

    $extension = [System.IO.Path]::GetExtension($test)
    if ($extension -eq ".args") {
        
        # If using .args as a test plan, expland the environment variables in the test plan
        Write-Host "Running Pythia test plans from $test"
        Write-Host "Expanding any environment variables, output: $test_results_dir/test_plan.args"
        Expand-File -in_file $test -out_file $test_results_dir/test_plan.args

        & ${env:REPO_APP_PATH_python.win64}\tools\python.exe -m robot `
        --variable WORKSPACE_CONFIG:"$workspace_config_out" `
        --variable LIBRARIES:"$env:LIBRARIES" `
        --variable RESOURCES:"$env:RESOURCES" `
        --variable LOG_DIR:"$test_results_dir" `
        --variable PAYLOAD_DIR:"$recipe_payload_dir" `
        --variable HOST_CONFIG_DIR:"$test_results_dir" `
        --debugfile rlog.txt  `
        --outputdir $test_results_dir `
        --listener "${env:REPO_APP_PATH_python.win64}\tools\Lib\site-packages\pythia\tdk\rrm\listener\listener_verbosity.py;Debug"  `
        -K on `
        -L TRACE `
        -W 120 `
        -x Results.xml `
        -A $test_results_dir/test_plan.args
    } else {
        & ${env:REPO_APP_PATH_python.win64}\tools\python.exe -m robot `
        --variable HOST_JSON:"$hostjson" `
        --variable WORKSPACE_CONFIG:"$workspace_config_out" `
        --variable LIBRARIES:"$env:LIBRARIES" `
        --variable RESOURCES:"$env:RESOURCES" `
        --variable LOG_DIR:"$test_results_dir" `
        --variable PAYLOAD_DIR:"$recipe_payload_dir" `
        --variable HOST_CONFIG_DIR:"$test_results_dir" `
        --debugfile rlog.txt  `
        --outputdir $test_results_dir `
        --listener "${env:REPO_APP_PATH_python.win64}\tools\Lib\site-packages\pythia\tdk\rrm\listener\listener_verbosity.py;Debug"  `
        -K on `
        -L TRACE `
        -W 120 `
        -x Results.xml `
        $test
    }

    # Move back to original location
    Pop-Location

    Write-Host ""
    Write-Host "`tPythia Tests Completed. Please see test results: $test_results_dir"
    Write-Host ""

    # Cleanup for SVP platform
    if ($platform -eq "svp") {
        Write-Host "Terminating all SVP instances and removing .svp_simulator directory"
        Stop-Virtualizer

        Start-Sleep -Seconds 20
        Remove-Item $test_results_dir/.svp_simulator -Recurse -Force -ErrorAction SilentlyContinue
    }

}