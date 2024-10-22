#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

# Define a global array of hashtables with PC Name and DC-SCM IP
$Global:SystemList = @(
    @{ "PC Name" = "DE3";  "DC-SCM IP" = "172.29.232.21" },
    @{ "PC Name" = "DE4";  "DC-SCM IP" = "172.29.233.220"},
    @{ "PC Name" = "DE6";  "DC-SCM IP" = "172.29.232.25" },
    @{ "PC Name" = "DH3";  "DC-SCM IP" = "172.29.232.28" },
    @{ "PC Name" = "DH4";  "DC-SCM IP" = "172.29.232.22" },
    @{ "PC Name" = "DH5";  "DC-SCM IP" = "172.29.233.221"},
    @{ "PC Name" = "DH6";  "DC-SCM IP" = "172.29.232.27" },
    @{ "PC Name" = "DH7";  "DC-SCM IP" = "172.29.232.24" },
    @{ "PC Name" = "DH10"; "DC-SCM IP" = "172.29.232.23" },
    @{ "PC Name" = "DH11"; "DC-SCM IP" = "172.29.232.29" },
    @{ "PC Name" = "DH12"; "DC-SCM IP" = "172.29.232.13" },
    @{ "PC Name" = "DH13"; "DC-SCM IP" = "172.29.232.15" },
    @{ "PC Name" = "DH14"; "DC-SCM IP" = "172.29.233.222"},
    @{ "PC Name" = "DF2";  "DC-SCM IP" = "172.29.232.11" },
    @{ "PC Name" = "DF5";  "DC-SCM IP" = "172.29.232.26" },
    @{ "PC Name" = "DF8";  "DC-SCM IP" = "172.29.232.14" },
    @{ "PC Name" = "DF11"; "DC-SCM IP" = "172.29.232.12" },
    @{ "PC Name" = "DF14"; "DC-SCM IP" = "172.29.232.8"  }
)  

<#
.SYNOPSIS
Fetches the DC-SCM IP from a predefines list of PC names and their corresponding DC-SCP IP addresses

.EXAMPLE
Get-DCSCPIP 'DH6'
#>
function Get-DCSCPIP {
    param (
        [string]$PCName
    )

    # Find the matching PC Name and return the DC-SCM IP
    $index = $SystemList | Where-Object { $_."PC Name" -eq $PCName }

    if ($index) {
        return $index."DC-SCM IP"
    } else {
        return "na"
    }
}


<#
.SYNOPSIS
Clears the flash on the FPGA (Run only on an FPGA system)

.EXAMPLE
Clear-FPGAFlash $ip <dc-scm IP> $loc <location of the image> $file <image file> $user <username> $pw <password>
#>
Function Clear-FPGAFlash(
    [Parameter(Mandatory=$true)] [string] $system,
    [Parameter(Mandatory=$false)] [string] $loc = "//lakshmi.svceng.com/rdu_lab/Kingsgate/images/DC-SCM_V1_2/BIOS0",
    [Parameter(Mandatory=$false)] [string] $file = "BIOS0_sp1_while_one.img",
    [Parameter(Mandatory=$false)] [string] $user = "admin",
    [Parameter(Mandatory=$false)] [string] $pw = "admin"
)
{
    Write-FPGAFlash -system $system -loc $loc -file $file -user $user -pw $pw
}

<#
.SYNOPSIS
Programs the flash on the FPGA (Run only on an FPGA system)

.EXAMPLE
Write-FPGAFlash -ip <dc-scm IP> -loc <location of the image> -file <image file> -user <username> -pw <password>
#>
Function Write-FPGAFlash(
    [Parameter(Mandatory=$true)] [string] $system,
    [Parameter(Mandatory=$false)] [string] $loc = ".build/Debug/arm-eabi-aarch/bin/flash",
    [Parameter(Mandatory=$false)] [string] $file = "kingsgate.ifwi.fpga.debug.custom.dat",
    [Parameter(Mandatory=$false)] [string] $user = "admin",
    [Parameter(Mandatory=$false)] [string] $pw = "admin",
    [Parameter(Mandatory=$false)] [string] $dest = "/tmp"
)
{

    $filepath = "$loc/$file"
    $ip = Get-DCSCPIP "$system"

    # Program flash if FPGA system found
    if ($ip -ne "na")
    {
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Red "                  ,,,,,                     "
        Write-Host -ForegroundColor Red "            ,;) .'     ',                   "
        Write-Host -ForegroundColor Red ";;,,_,-.-.,;;'_,||\   /;!,_                 "
        Write-Host -ForegroundColor Red "   ;;/:|:);{ ;;;|| \./ ;|;;\__              "
        Write-Host -ForegroundColor Red "     L;/-';/ \;;\',/ \//;;.') \             "
        Write-Host -ForegroundColor Red "      :'''' - \;;'.___/;;;/  . _'-._        "
        Write-Host -ForegroundColor Red "           \     \;\;;/;/.'_7:.  '). \_     "
        Write-Host -ForegroundColor Red "            | '._ );}{;//.'    '-:_\'.,\    "
        Write-Host -ForegroundColor Red "             \  ( |/;;/_/           \./;\   "
        Write-Host -ForegroundColor Red "             |\ ( /;;/_/             /;;;\  "
        Write-Host -ForegroundColor Red "             )__(/;;/_/              \;;;/  "
        Write-Host -ForegroundColor Red "           _;:':;;;;:';-._                  "
        Write-Host -ForegroundColor Red "          /   \''''''/  -.'-._              "
        Write-Host -ForegroundColor Red "        .'     '.  ,'         '-.           "
        Write-Host -ForegroundColor Red "       /    /   r--,..__       '.\          "
        Write-Host -ForegroundColor Red "     .'    '  .'        '--._     ]         "
        Write-Host -ForegroundColor Red "     (     :.(;>        _ .' '- ;/          "
        Write-Host -ForegroundColor Red "     |      /:;(    ._.';(   __.'  Your friendly neighbourhood web-slinger says: "
        Write-Host -ForegroundColor Red "      '- -''|;:/    (;;;;-'--'        - With Great Power comes Great Responsibility!!"
        Write-Host -ForegroundColor Red "            |;/     |;;(              - Please only program your test FPGA!!!"
        Write-Host -ForegroundColor Red "            ''      /;;|              - Do not flash someone else's FPGA!!!"
        Write-Host -ForegroundColor Red "                    \;;|              - Use the right system name!!!"
        Write-Host -ForegroundColor Red "                     \/                     "
    

        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "Downloading Flash Image via BMC . . ."
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "System: RDU-120015-$system"
        Write-Host -ForegroundColor Blue "IP    : $ip"
        Write-Host -ForegroundColor Blue "File  : $filepath"
        Write-Host -ForegroundColor Blue "Dest  : ${ip}:${dest}"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        echo "y" | pscp -scp -pw $pw $filepath $user@${ip}:$dest
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"

        Write-host ""

        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "Programming SoC Flash . . ."
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        echo "n" | plink -ssh $user@${ip} -pw $pw bios-updater -mode fwupdate -file $dest/$file -index 0
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
    }
    # Skip if unknown config or SVP
    else 
    {
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host "SVP/Unknown config detected. Skipping SoC Flash Programming"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
    }
}

<#
.SYNOPSIS
Queries the users logged in on a remote FPGA machine

.EXAMPLE
Check-UserLogin -user atiru
Check-UserLogin
#>
Function Check-UserLogin(
    [Parameter(Mandatory=$true)] [string] $user = "all"
)
{
    if ($user -ne "all") {
        Write-Host "Checking if user $user is logged in on FPGA systems. If this command returns nothing, please check that you are on VPN!!!"
        Write-Host ""
        foreach($System in $SystemList) {
            $PCName = $System."PC Name"
            Write-Host "Querying user $user on system: RDU-120015-$PCName"
            query user /SERVER:RDU-120015-$PCName | Select-String -Pattern $user
        }
    } else {
        Write-Host "Checking users logged in on FPGA systems. If this command returns nothing, please check that you are on VPN!!!"
        Write-Host ""
        foreach($System in $SystemList) {
            $PCName = $System."PC Name"
            Write-Host "Querying users on system: RDU-120015-$PCName"
            query user /SERVER:RDU-120015-$PCName
        }
    }
    Write-Host ""
}

<#
.SYNOPSIS
Logs out a user from a remote FPGA system.

.EXAMPLE
Logout-User -pc 'DH6' -user 'atiru'
#>
Function Logout-User(
    [Parameter(Mandatory=$false)] [string] $pc = "all",
    [Parameter(Mandatory=$true)] [string] $user
)
{
    if ($pc -ne "all") {

        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "Logging out user $user from system RDU-120015-$pc"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"

        $sessionId = ((query user /SERVER:RDU-120015-$pc | Where-Object { $_ -match $user }) -split ' +')[2]
        if ($sessionId -eq $null) {
            Write-Host -ForegroundColor Red "User $user is not logged in on system RDU-120015-$pc"
            return
        }

        logoff $sessionId /SERVER:RDU-120015-$pc
        Write-Host -ForegroundColor Blue "User $user logged out from system RDU-120015-$pc"

    } else {
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "Logging out user $user from all systems"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        
        foreach($System in $SystemList) {
            $PCName = $System."PC Name"
            Write-Host -ForegroundColor Blue "Logging out user $user from system RDU-120015-$PCName"
            $sessionId = ((query user /SERVER:RDU-120015-$PCName | Where-Object { $_ -match $user }) -split ' +')[2]
            if ($sessionId -eq $null) {
                Write-Host -ForegroundColor Red "User $user is not logged in on system RDU-120015-$PCName"
            } else {
                logoff $sessionId /SERVER:RDU-120015-$PCName
                Write-Host -ForegroundColor Blue "User $user logged out from system RDU-120015-$PCName"
            }
        }
        
    }
    Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
}

<#
.SYNOPSIS
Exports all .dat files from a specified local directory to a remote PC.

.EXAMPLE
Copy-DatFiles -sourceDir ".build/Debug/arm-eabi-aarch/bin/flash" -destDir "R:/Users/atiru/datfiles" -file "kingsgate.ifwi.fpga.debug.ap.baremetal.dat"
#>
Function Copy-DatFiles(
    [Parameter(Mandatory=$false)] [string] $file = 'all',
    [Parameter(Mandatory=$false)] [string] $sourceDir = ".build/Debug/arm-eabi-aarch/bin/flash",
    [Parameter(Mandatory=$true)] [string] $destDir
)
{
    Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
    Write-Host -ForegroundColor Blue "Copying .dat files to remote PC"
    Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
    Write-Host -ForegroundColor Blue "Source Directory      : $sourceDir"
    Write-Host -ForegroundColor Blue "Destination Directory : $destDir"
    Write-Host -ForegroundColor Blue "File(s)               : $file"
    Write-Host -ForegroundColor Blue "------------------------------------------------------------------"

    if ($file -ne 'all')
    {
        Write-Host -ForegroundColor Blue "Copying file: $sourceDir/$file"
        Copy-Item -Path $sourceDir/$file -Destination $destDir -Force
    } else {
        $datFiles = Get-ChildItem -Path $sourceDir -Filter *.dat
        foreach ($datFile in $datFiles)
        {
            $filePath = $datFile.FullName
            Write-Host -ForegroundColor Blue "Copying file: $filePath"
            Copy-Item -Path $filePath -Destination $destDir -Force
        }
    }

    Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
    Write-Host -ForegroundColor Blue "Completed copying .dat files to remote PC"
    Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
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

New-Alias -Name clearscpfw -Value Clear-FPGAFlash -Force
New-Alias -Name copydat -Value Copy-DatFiles -Force
New-Alias -Name flashscpfw -Value Write-FPGAFlash -Force
New-Alias -Name helpfpga -Value Get-FPGAHelp -Force
New-Alias -Name checkuser -Value Check-UserLogin -Force
New-Alias -Name kickuser -Value Logout-User -Force

Export-ModuleMember -Alias * -Function *