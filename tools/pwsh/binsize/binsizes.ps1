#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

param (
    [Parameter(Mandatory=$true)]$FileName
)

# Define the elf file and temp file paths
$toolchain = $(Get-ChildItem -Path "Env:REPO_APP_PATH_gcc.arm.eabi.aarch-win64").Value
$sizeTool = Join-Path -Path $toolchain -ChildPath 'bin\arm-none-eabi-size.exe'
$currentDirectory = Get-Location
$FullFileName = $FileName.FullName

# Takes in FileName as a parameter if given - or defaults to scp_fw.elf
# Test parameter given for FileName (elf file)
if (-not $FullFileName) {
    Write-Host "No filename given, defaulting to scp_fw.elf in the build path"
    $elfFile = Join-Path -Path $(Get-ChildItem -Path "Env:REPO_APP_TARGET_BUILD_DIR").Value -ChildPath 'bin\scp\scp_fw.elf'
    $tempFile = "$currentDirectory/scp_fw.sizes"
}
elseif (-not (Test-Path $FullFileName)) {
    Write-Host "elf file not found at path provided.  Exiting."
    exit
}
else {
    Write-Host "Binsize Processing: $FullFileName"
    $elfFile = $FullFileName
    $tempFile = Join-Path -Path $currentDirectory -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($FullFileName)).sizes"
}

try {
    # Initialize last build sizes to 0 if temp file doesn't exist
    if (-not (Test-Path $tempFile)) {
        Write-Host "Temp file not found. Initializing last build sizes to 0."
        $lastBuildSizes = @{
            "text" = 0
            "data" = 0
            "bss"  = 0
            "dec"  = 0
        }
    } else {
        # Read the last build sizes
        $lastBuildSizes = Get-Content $tempFile | ConvertFrom-Json
        if (-not $lastBuildSizes) {
            Write-Host "Last build sizes are null. Initializing to 0."
            $lastBuildSizes = @{
                "text" = 0
                "data" = 0
                "bss"  = 0
                "dec"  = 0
            }
        }
    }

    # Get the section sizes of the current build
    $sizeOutput = & $sizeTool --format=berkeley $elfFile
    $sizeValues = $sizeOutput -split '\s+' | Where-Object { $_ -match "\d+" }
    $currentBuildSizes = @{
        "text" = $sizeValues[0]
        "data" = $sizeValues[1]
        "bss"  = $sizeValues[2]
        "dec"  = $sizeValues[3]
    }

    # Calculate the differences between current and previous build sizes
    $difference = @{
        "text" = $currentBuildSizes["text"] - $lastBuildSizes.text
        "data" = $currentBuildSizes["data"] - $lastBuildSizes.data
        "bss"  = $currentBuildSizes["bss"]  - $lastBuildSizes.bss
        "dec"  = $currentBuildSizes["dec"]  - $lastBuildSizes.dec
    }

    # Create an array of custom objects
    $sizes = @(
        [PSCustomObject]@{
            'Type' = 'text'
            'Current' = $currentBuildSizes['text']
            'Previous' = $lastBuildSizes.text
            'Difference' = $difference['text']
        },
        [PSCustomObject]@{
            'Type' = 'data'
            'Current' = $currentBuildSizes['data']
            'Previous' = $lastBuildSizes.data
            'Difference' = $difference['data']
        },
        [PSCustomObject]@{
            'Type' = 'bss'
            'Current' = $currentBuildSizes['bss']
            'Previous' = $lastBuildSizes.bss
            'Difference' = $difference['bss']
        },
        [PSCustomObject]@{
            'Type' = 'TOTAL'
            'Current' = $currentBuildSizes['dec']
            'Previous' = $lastBuildSizes.dec
            'Difference' = $difference['dec']
        }
    )

    foreach ($size in $sizes) {
        if ($size.Current -ne 0) {
            $size.Current = "{0,10} B ({1,10} KB)" -f $size.Current, [math]::Round($size.Current / 1KB, 2)
        }
        if ($size.Previous -ne 0) {
            $size.Previous = "{0,10} B ({1,10} KB)" -f $size.Previous, [math]::Round($size.Previous / 1KB, 2)
        }
        if ($size.Difference -ne 0) {
            $size.Difference = "{0,10} B ({1,10} KB)" -f $size.Difference, [math]::Round($size.Difference / 1KB, 2)
        }
    }
    # Output the differences in a tabular format
    
    $sizes | Format-Table -Property `
             @{Label="Type";Expression={$_.Type};Align="Right"}, `
             @{Label="Current";Expression={$_.Current};Align="Right"}, `
             @{Label="Previous";Expression={$_.Previous};Align="Right"}, `
             @{Label="Difference";Expression={$_.Difference};Align="Right"} `
             -AutoSize

    # Store the current build sizes for the next run
    $currentBuildSizes | ConvertTo-Json | Set-Content $tempFile
} catch {
    Write-Host "Error occurred: $_"
}
