#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

param (
    [string] $Toolchain = "arm-eabi-aarch",
    [string] $Configuration = "Debug",
    [switch] $SkipEnv = $false,
    [switch] $MSCP_vUART = $false,
    [switch] $Localtest = $false,
    [switch] $Fast = $false
)

$Name = (Get-Item "$PSScriptRoot").Name
$Modules = Get-ChildItem "$PSScriptRoot/tools/pwsh/modules"

foreach ($Module in $Modules) {
    Import-Module $Module.FullName -Force -DisableNameChecking
}

# Call the environment setup with input
if (-not $SkipEnv) {
    Set-RepoEnv -Toolchain $Toolchain -Configuration $Configuration -MSCP_vUART $MSCP_vUART -Fast:$Fast
}

if ($Localtest) {
    Setup-LocalPythia -RepoRootPath $PSScriptRoot
}

# Dump help block for SVP and FPGA Debug
Get-SvpHelp
Get-SvpGuiHelp
Get-FPGAHelp
Get-SocHelp 