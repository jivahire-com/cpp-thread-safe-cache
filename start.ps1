param (
    [string] $Toolchain = "arm-eabi-aarch",
    [string] $Configuration = "Debug",
    [string] $ElfBuildSizes = "one",
    [boolean] $SkipEnv = $false,
    [boolean] $MSCP_vUART = $false
)

$Name = (Get-Item "$PSScriptRoot").Name
$Modules = Get-ChildItem "$PSScriptRoot/tools/pwsh/modules"

foreach ($Module in $Modules) {
    Import-Module $Module.FullName -Force -DisableNameChecking
}

# Call the environment setup with input
if (-not $SkipEnv) {
    Set-RepoEnv -Toolchain $Toolchain -Configuration $Configuration -MSCP_vUART $MSCP_vUART
}

# Set environment variable to dictate if one/all .elf files are parsed for size
$env:ElfBuildSizes = $ElfBuildSizes

# Dump help block for SVP and FPGA Debug
Get-SvpHelp
Get-FPGAHelp
