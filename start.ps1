param (
        [string] $Toolchain = "arm-eabi-aarch",
        [string] $Configuration = "Debug"
     )

$Name = (Get-Item "$PSScriptRoot").Name
$Modules = Get-ChildItem "$PSScriptRoot/tools/pwsh/modules"

foreach($Module in $Modules)
{
    Import-Module $Module.FullName -Force
}

# Call the environment setup with input 
Set-RepoEnv -Toolchain $Toolchain -Configuration $Configuration

# Dump help block for SVP and FPGA Debug
Get-SvpHelp
Get-FPGAHelp
