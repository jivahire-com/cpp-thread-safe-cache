
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
Invoke-Pythia -test .\tests\functional\pythia\test_suites\heart_beat\
#>
Function Invoke-Pythia(
    [Parameter(Mandatory=$true)] [string] $test
)
{

    Write-Host ""
    Write-Host "`tInvoking Pythia Functional Test"
    Write-Host ""

    $test = Join-Path -Path $env:REPO_APP_ROOT -ChildPath $test

    if (-not (Test-Path -Path $test))
    {
        throw "Path not found: $test"
    }

    # Create test run paths based on timestamp
    $time_stamp = (Get-Date -Format "yyyy_MM_dd_hhmmss")
    $pythia_test_dir = Join-Path -Path $env:REPO_APP_TEST_LOG_DIR -ChildPath "pythia"
    $test_results_dir = Join-Path -Path $pythia_test_dir -ChildPath $time_stamp

    # Create the test results directory
    New-Item -ItemType Directory -Path $test_results_dir | Out-Null

    # Setup the paths for the necessary Pythia configurations
    # In\Out for pre/post processed json files (for variable expansion)

    $workspace_config_in = Join-Path -Path $env:REPO_APP_ROOT -ChildPath "tests\functional\pythia\configs\svp_workspace.json"
    $workspace_config_out = Join-Path -Path $test_results_dir -ChildPath "workspace.json"

    # Create new json files with expanded string values, expanding environment variables
    Expand-File -in_file $workspace_config_in -out_file $workspace_config_out

    $host_configs_path = Join-Path -Path $env:REPO_APP_ROOT -ChildPath "tests\functional\pythia\hosts\"

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
    $recipe_payload_dir = Join-Path -Path $env:REPO_APP_ROOT -ChildPath "tests\functional\pythia\payloads\payload_svp"

    Write-Host ""
    Write-Host "`tStoring Test Results: $test_results_dir "
    Write-Host ""

    Write-Host ""
    Write-Host "`t`tUsing Test: $test"
    Write-Host "`t`tUsing Workspace Config: $workspace_config_out"
    Write-Host "`t`tUsing Payload: $recipe_payload_dir"
    Write-Host ""

    # This can also be set via the host configuration json for pythia, however that requires absolute paths.
    # Resolving the paths allows us to update the version and not have to update the paths.
    $env:SNPS_VS_VDK_SEARCH_PATHS=(Resolve-Path ${env:REPO_APP_PATH_microsoft.internal.virtualized.kingsgate.svp}\win\release\*\*\*\).toString()

    # Move to the test directory. Pythia will use relative paths once executed, moving into the test
    # dir will put any any folders that get created there (for things we can't configure atm, ex: SVP output dir).
    Push-Location -Path $test_results_dir

    & ${env:REPO_APP_PATH_python.win64}\tools\python.exe -m robot `
    --variable WORKSPACE_CONFIG:"$workspace_config_out" `
    --variable LOG_DIR:"$test_results_dir" `
    --variable PAYLOAD_DIR:"$recipe_payload_dir" `
    --variable HOST_CONFIG_DIR:"$test_results_dir" `
    --debugfile rlog.txt  `
    --outputdir $test_results_dir `
    -K on `
    -L TRACE `
    -W 120 `
    -x Results.xml `
    $test

    # Move back to wherever the function was invoked from
    Pop-Location

    Write-Host ""
    Write-Host "`tPythia Tests Completed. Please see tests results: $test_results_dir"
    Write-Host ""

    Write-Host "Removing .svp_simulator dir"
    Remove-Item $test_results_dir/.svp_simulator -Recurse -Force

}
