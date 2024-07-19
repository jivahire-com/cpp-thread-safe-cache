#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

<#
.SYNOPSIS
Copies the elf files to the R drive. Synchronizes the source to git to fetch on the FPGA system.

.EXAMPLE
Sync-FPGADebug
#>
Function Export-FPGAbins(
    [Parameter(Mandatory=$false)] [string] $ShareDriveLoc = "\\lakshmi.svceng.com\rdu_lab\Users",
    [Parameter(Mandatory=$false)] [string] $User = "mscp_shared_debug",
    [Parameter(Mandatory=$false)] [string] $Repo = "Kingsgate.MSCP",
    [Parameter(Mandatory=$false)] [string] $BinLoc = ".build\Debug\arm-eabi-aarch\bin",
    [Parameter(Mandatory=$false)] [ValidateSet('git_commit', 'src_copy')] [string] $TransferMethod = "git_commit"
)
{

    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

    # # Create the directory structure if not present on R drive
    Write-Host "Creating binary directory structure on R drive and copying elfs..."
    New-Item -ItemType Directory -Force -Path "$ShareDriveLoc\$User\$Repo\$BinLoc"
    Get-ChildItem -Path $BinLoc -Recurse -Filter *.elf | ForEach-Object { Copy-Item -Path $_.FullName -Destination $ShareDriveLoc\$User\$Repo\$BinLoc\ -Force }

    if ($TransferMethod -eq "git_commit") {
        # Push all changes to git
        Write-Host  "Pushing all changes to git... Fetch changes on FPGA system"
        git add --all
        git commit -m "FPGA Debug Source Sync - $timestamp"
        git push -f origin HEAD

    } elseif ($TransferMethod -eq "src_copy") {
        # Copy src folder to R Drive
        Write-Host  "Copying src folder to R:\$User\$Repo"
        Copy-Item src $ShareDriveLoc\$User\$Repo -Force
    }
}

<#
.SYNOPSIS
Gets the help information of the commands available for debugging on FPGA.

.EXAMPLE
Get-FPGAHelp
#>
Function Get-FPGAHelp()
{
    Write-Host "Available Commands for FPGA Debugging:"
    $Commands = (Get-Module FPGA-Utils).ExportedAliases.Values.GetEnumerator()

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

New-Alias -Name expbins -Value Export-FPGAbins -Force
New-Alias -Name helpfpga -Value Get-FPGAHelp -Force

Export-ModuleMember -Alias * -Function *