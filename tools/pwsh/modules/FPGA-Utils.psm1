#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

# Define the RDU Prefix globally
$Global:RDUPrefix = "RDU-120015"

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
    [Parameter(Mandatory=$false)] [string] $dest = "/var/wcs/home"
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
        Write-Host -ForegroundColor Blue "Downloading Primary Flash Image via BMC . . ."
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "System: $Global:RDUPrefix-$system"
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
Check-UserLogin -pc DH7 -user atiru
Check-UserLogin
#>
Function Check-UserLogin(
    [Parameter(Mandatory=$false)] [string] $user = "all",
    [Parameter(Mandatory=$false)] [string] $pc = "all"
)
{
    if ($user -ne "all") {
        if ($pc -ne "all") {
            $systemExists = $SystemList | Where-Object { $_."PC Name" -eq $pc }
            if (-not $systemExists) {
                Write-Host -ForegroundColor Red "System $pc not found in the system list."
                return
            }
            Write-Host -ForegroundColor Blue "Checking user $user on system: $Global:RDUPrefix-$pc"
            query user /SERVER:$Global:RDUPrefix-$pc | Select-String -Pattern $user
        } else {
            Write-Host -ForegroundColor Blue "Checking if user $user is logged in on any FPGA system"
            Write-Host ""
            
            foreach($System in $SystemList) {
                $PCName = $System."PC Name"
                Write-Host -ForegroundColor Blue "Checking user $user on system: $Global:RDUPrefix-$PCName"
                query user /SERVER:$Global:RDUPrefix-$PCName | Select-String -Pattern $user
            }
        }
    } else {
        if ($pc -ne "all") {
            $systemExists = $SystemList | Where-Object { $_."PC Name" -eq $pc }
            if (-not $systemExists) {
                Write-Host -ForegroundColor Red "System $pc not found in the system list."
                return
            }

            Write-Host -ForegroundColor Blue "Checking all users logged in on system: $Global:RDUPrefix-$pc"
            query user /SERVER:$Global:RDUPrefix-$pc
        } else {
            Write-Host -ForegroundColor Blue "Checking users logged in on all FPGA systems"
            Write-Host ""
            foreach($System in $SystemList) {
                $PCName = $System."PC Name"
                Write-Host "Querying users on system: $Global:RDUPrefix-$PCName"
                query user /SERVER:$Global:RDUPrefix-$PCName
            }
        }
    }

    Write-Host  -ForegroundColor Blue "If the command returned nothing, please check your VPN. . ."
    Write-Host ""
}

<#
.SYNOPSIS
Logs out a user from a remote FPGA system.

.EXAMPLE
1. Log out all users from a specific PC :- kickuser -pc 'DH3' -user 'all'
2. Log out a specific user (v-ybertuskag) from a specific PC (DH3):- kickuser -pc 'DH3' -user 'v-ybertuskag'
3. Log out a specific user from all PCs : kickuser -pc 'all' -user 'v-ybertuskag'
#>
Function Logout-User(
    [Parameter(Mandatory=$false)] [string] $pc = "all",
    [Parameter(Mandatory=$true)] [string] $user
)
{
    if ($user -eq "all") {
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "Logging out all users from system(s)"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        
        if ($pc -eq "all") {
            foreach($System in $SystemList) {
                $PCName = $System."PC Name"
                Write-Host -ForegroundColor Blue "Logging out users from system $Global:RDUPrefix-$PCName"
                $users = query user /SERVER:$Global:RDUPrefix-$PCName
                
                # Capture both "Active" and "Disc" users (Disc refers to Disconnected)
                $userSessions = $users | Where-Object { $_ -match 'Active|Disc' }

                foreach ($userSession in $userSessions) {
                    # Extracting the user name and session ID properly
                    $fields = $userSession -split '\s+'
                    $sessionId = $fields[2]  # Session ID is the third field

                    if ($sessionId -ne $null -and $sessionId -ne "") {
                        Write-Host -ForegroundColor Blue "Logging out user with Session ID: $sessionId from system $Global:RDUPrefix-$PCName"
                        logoff $sessionId /SERVER:$Global:RDUPrefix-$PCName
                        Write-Host -ForegroundColor Blue "User with Session ID: $sessionId logged out from system $Global:RDUPrefix-$PCName"
                    } else {
                        Write-Host -ForegroundColor Red "Failed to identify  Session ID on system $PCName"
                    }
                }
            }
        } else {
            # If a specific PC is selected
            Write-Host -ForegroundColor Blue "Logging out all users from system $Global:RDUPrefix-$pc"
            $users = query user /SERVER:$Global:RDUPrefix-$pc
            # Capture both "Active" and "Disc" users (Correctly parse the output)
            $userSessions = $users | Where-Object { $_ -match 'Active|Disc' }

            foreach ($userSession in $userSessions) {
                # Extracting the user name and session ID properly
                $fields = $userSession -split '\s+'
                $sessionId = $fields[2]  # Session ID is the third field

                if ($sessionId -ne $null -and $sessionId -ne "") {
                    Write-Host -ForegroundColor Blue "Logging out Session ID: $sessionId from system $Global:RDUPrefix-$pc"
                    logoff $sessionId /SERVER:$Global:RDUPrefix-$pc
                    Write-Host -ForegroundColor Blue "User with Session ID: $sessionId is logged out from system $Global:RDUPrefix-$pc"
                } else {
                    Write-Host -ForegroundColor Red "Failed to identify session ID on system $pc"
                }
            }
        }
    } else {
        # If a specific user is passed, log out that user on the specific or all systems
        if ($pc -ne "all") {
            Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
            Write-Host -ForegroundColor Blue "Logging out user $user from system $Global:RDUPrefix-$pc"
            Write-Host -ForegroundColor Blue "------------------------------------------------------------------"

            $sessionId = ((query user /SERVER:$Global:RDUPrefix-$pc | Where-Object { $_ -match $user }) -split ' +')[2]
            if ($sessionId -eq $null -or $sessionId -eq "") {
                Write-Host -ForegroundColor Red "User $user is not logged in on system $Global:RDUPrefix-$pc"
                return
            }

            logoff $sessionId /SERVER:$Global:RDUPrefix-$pc
            Write-Host -ForegroundColor Blue "User $user logged out from system $Global:RDUPrefix-$pc"
        } else {
            Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
            Write-Host -ForegroundColor Blue "Logging out user $user from all systems"
            Write-Host -ForegroundColor Blue "------------------------------------------------------------------"

            foreach($System in $SystemList) {
                $PCName = $System."PC Name"
                Write-Host -ForegroundColor Blue "Logging out user $user from system $Global:RDUPrefix-$PCName"
                $sessionId = ((query user /SERVER:$Global:RDUPrefix-$PCName | Where-Object { $_ -match $user }) -split ' +')[2]
                if ($sessionId -eq $null -or $sessionId -eq "") {
                    Write-Host -ForegroundColor Red "User $user is not logged in on system $Global:RDUPrefix-$PCName"
                } else { 	
                    logoff $sessionId /SERVER:$Global:RDUPrefix-$PCName
                    Write-Host -ForegroundColor Blue "User $user logged out from system $Global:RDUPrefix-$PCName"
                }
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
Runs the R:\Kingsgate\Kingsgate_TRACE32\Start\TRACE32_USB_KILL.bat script to kill all trace32 sessions

.EXAMPLE
Kill-Trace32
#>
Function Kill-Trace32()
{
    Write-Host ""  
    Write-Host "Running TRACE32_USB_KILL.bat"
    & \\lakshmi.svceng.com\rdu_lab\Kingsgate\Kingsgate_TRACE32\Start\TRACE32_USB_KILL.bat
}

<#
.SYNOPSIS
Kills all running Putty processes.
.EXAMPLE
Kill-Putty
#>
# Kill all Putty and Plink processes (including UART)
function Kill-Putty {
    foreach ($proc in Get-Process) {
        if ($proc.Name -like "*putty*") {
            Write-Host "Killing Putty process with PID $($proc.Id)"
            Stop-Process -Id $proc.Id -Force
        }
    }
}

<#
.SYNOPSIS
Runs an OOB reset on a remote PC

.EXAMPLE
Run-OOBReset <PC_Name>: Run-OOBReset 'DH3'
#>
Function Start-OOBReset(
    [Parameter(Mandatory=$false)] [string] $pc = "none"
)
{

    if ($pc -eq "none") {
        Write-Host "Running OOB Reset for the current system"
        $fpga_system_name = (hostname).ToLower()
        $fpga_system_name = $fpga_system_name -replace 'rdu-120015-', ''
    }
    else
    {
        $fpga_system_name = $pc.ToLower()
        Write-Host "Running OOB Reset for the system: $fpga_system_name"
    }

    # Create new file in the shared directory for the reset monitor listener
    Write-Output "Creating reset monitor listener file to trigger OOB reset..."

    $reset_monitor_file_path = "\\lakshmi.svceng.com\rdu_lab\haps_runtime\kingsgate_big_fpga\reset_tools\force_reset_$fpga_system_name"
    $reset_monitor_file = Join-Path $reset_monitor_file_path "reset"
    $new_monitor_file = Join-Path $reset_monitor_file_path ".newer"

    # Check if a file called .newer exists in the reset request directory, the
    # modification time of this file is used by the watcher script to compare and
    # issue a reset if there is anything "newer" found in the dir. If it doesn't
    # exist, create one here.
    if (-Not (Test-Path $new_monitor_file)) {
        New-Item -ItemType File -Path $new_monitor_file
        Write-Output "Newer file created at: $new_monitor_file"
    }

    # Finally, remove any stale reset notification and create a new notification
    # file. Sleep for 5 seconds after a file operation as that is the current 
    # worst case delay for the adapters to go through a reset.
    if (Test-Path $reset_monitor_file) {
        Remove-Item $reset_monitor_file
        Write-Output "Reset file deleted at: $reset_monitor_file"
    }

    Start-Sleep -Seconds 5

    # Create the reset file
    New-Item -ItemType File -Path $reset_monitor_file
    Write-Output "Reset file created at: $reset_monitor_file"

    Start-Sleep -Seconds 5

    # Delete the reset file
    Remove-Item $reset_monitor_file
    Write-Output "Reset file deleted at: $reset_monitor_file"
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
New-Alias -Name killt32 -Value Kill-Trace32 -Force
New-Alias -Name killputty -Value Kill-Putty -Force
New-Alias -Name oobreset -Value Start-OOBReset -Force

Export-ModuleMember -Alias * -Function *