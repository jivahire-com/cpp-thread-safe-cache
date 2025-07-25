#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

<#
.SYNOPSIS
Fetches the DC-SCM IP from a predefines list of PC names and their corresponding DC-SCP IP addresses

.EXAMPLE
Get-DCSCPIP 'A15-R3'
#>
function Get-DCSCPIP {
    param (
        [string]$PCName
    )

    # Define an array of hashtables with PC Name and DC-SCM IP
    $IPList = @(
        @{ "PC Name" = "A15-R3";  "DC-SCM IP" = "M1120-SCM-445.svceng.com" },
        @{ "PC Name" = "B20-B1";  "DC-SCM IP" = "M1120-SCM-278.svceng.com" },
        @{ "PC Name" = "B20-B2";  "DC-SCM IP" = "M1120-SCM-393.svceng.com" },
        @{ "PC Name" = "J10-R5";  "DC-SCM IP" = "M1120-SCM-392.svceng.com" }
    )

    # Find the matching PC Name and return the DC-SCM IP
    $index = $IPList | Where-Object { $_."PC Name" -eq $PCName }

    if ($index) {
        return $index."DC-SCM IP"
    } else {
        return "na"
    }
}

<#
.SYNOPSIS
Programs the flash on the SOC (Run only on an SOC system)

.EXAMPLE
To flash soc dat file.
flashsoc -system A15-R3 -loc .build/soc -file kingsgate.ifwi.soc.local.dat
#>
Function Write-SOCFlash(
    [Parameter(Mandatory=$true)] [string] $system,
    [Parameter(Mandatory=$false)] [string] $loc = ".build/Debug/arm-eabi-aarch/bin/flash",
    [Parameter(Mandatory=$false)] [string] $file = "kingsgate.ifwi.soc.debug.custom.dat",
    [Parameter(Mandatory=$false)] [string] $user = "admin",
    [Parameter(Mandatory=$false)] [string] $pw = "admin",
    [Parameter(Mandatory=$false)] [string] $dest = "/var/wcs/home"
)
{
    $filepath = "$loc/$file"
    $ip = Get-DCSCPIP "$system"
    $bmcpw = "0penBmc"

    Get-ChildItem -Path $loc

    # Program flash if SOC system found
    if ($ip -ne "na")
    {
        Write-Title -Title "Copying Ifwi to BMC" -Color Cyan
        Write-Host -ForegroundColor Blue "System: $system"
        Write-Host -ForegroundColor Blue "IP    : $ip"
        Write-Host -ForegroundColor Blue "File  : $filepath"
        Write-Host -ForegroundColor Blue "Dest  : ${ip}:${dest}"

        pscp -hostkey "ssh-rsa 2048 SHA256:+vQIRLRXfSLV4m8RmtiVCjevzQTvHl3xp/VvojmwAFQ" -P 2200 -pw $pw $filepath "$user@${ip}:$dest"
        Write-host ""

        Write-Title -Title "Programming SoC Flash" -Color Cyan
        echo "n" | plink -ssh -hostkey "ssh-rsa 2048 SHA256:+vQIRLRXfSLV4m8RmtiVCjevzQTvHl3xp/VvojmwAFQ" -P 2200 $user@$ip -pw $pw "echo $bmcpw | sudo -S su -c 'bios-updater -mode fwupdate -file $dest/$file -index 0'"
        Write-host ""

        # I2C Reset required only for SoC systems
        Write-host ""
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "Resetting Board via I2C for Silicon RVP. . ."
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        #echo "n" | plink -ssh -hostkey "ssh-rsa 2048 SHA256:+vQIRLRXfSLV4m8RmtiVCjevzQTvHl3xp/VvojmwAFQ" -P 2200 $user@$ip -pw $pw "echo $bmcpw | sudo -S su -c 'i2ctransfer -f -y 6 w1@0x52 0xd9p'"
        #plink -ssh -hostkey "ssh-rsa 2048 SHA256:+vQIRLRXfSLV4m8RmtiVCjevzQTvHl3xp/VvojmwAFQ" -P 2200 $user@$ip -pw $pw "echo $bmcpw | sudo -S su -c 'i2ctransfer -f -y 6 w1@0x52 0xd9p'"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
    }
    # Skip if unknown config
    else
    {
        Write-Host -ForegroundColor Red "Unknown config detected. Skipping SoC Flash Programming"
    }
}

<#
.SYNOPSIS
Logs out all users from a remote SOC system.

.EXAMPLE
logoutuser -system 'A15-R3'
#>
Function Invoke-LogoutUser(
    [Parameter(Mandatory=$true)] [string] $system
)
{
    # If a specific system is selected
    Write-Title -Title "Logging out all users" -Color Cyan

    $users = query user /SERVER:$system
    # Capture both "Active" and "Disc" users (Correctly parse the output)
    $userSessions = $users | Where-Object { $_ -match 'Active|Disc' }

    foreach ($userSession in $userSessions) {
        # Extracting the user name and session ID properly
        $fields = $userSession -split '\s+'
        $sessionId = $fields[2]  # Session ID is the third field

        if ($sessionId -ne $null -and $sessionId -ne "") {
            Write-Host -ForegroundColor Blue "Logging out Session ID: $sessionId from system $system"
            logoff $sessionId /SERVER:$system
            Write-Host -ForegroundColor Blue "User with Session ID: $sessionId is logged out from system $system"
        } else {
            Write-Host -ForegroundColor Red "Failed to identify session ID on system $system"
        }
    }
}

<#
.SYNOPSIS
Kills all running T32 sessions.

.EXAMPLE
Kill-Trace32Soc
#>
Function Kill-Trace32Soc()
{
    Write-Host ""  
    Write-Host "Running SOC 5_TRACE32_JTAG_KILL.bat"
    & "\\shakti.svceng.com\validation\projects\kingsgate\tools\Kingsgate_TRACE32\Start\JTAG\5_TRACE32_JTAG_KILL.bat"
}

<#
.SYNOPSIS
Kills all running Putty processes.

.EXAMPLE
killputtysoc
#>
function Invoke-KillPutty {
    Write-Title -Title "Kill SOC Putty Sessions" -Color Cyan

    $puttyProcesses = Get-Process | Where-Object { $_.Name -like "*putty*" }
    if ($puttyProcesses) {
        foreach ($proc in $puttyProcesses) {
            try {
                Write-Host "Killing Putty process with PID $($proc.Id)"
                Stop-Process -Id $proc.Id -Force
                Write-Host "Successfully killed Putty process with PID $($proc.Id)"
            } catch {
                Write-Host "Failed to kill Putty process with PID $($proc.Id). Error: $_" -ForegroundColor Red
            }
        }
    } else {
        Write-Host "No Putty processes found."
    }
}

<#
.SYNOPSIS
Gets the help information of the commands available for debugging on SOC.

.EXAMPLE
Get-SOCHelp
#>
Function Get-SOCHelp()
{
    Write-Title -Title "Available Commands for SOC" -Color Cyan
    $Commands = (Get-Module SOC-Utils).ExportedAliases.Values.GetEnumerator()

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

New-Alias -Name helpsoc -Value Get-SOCHelp -Force
New-Alias -Name flashscpfwsoc -Value Write-SOCFlash -Force
New-Alias -Name kickusersoc -Value Invoke-LogoutUser -Force
New-Alias -Name killputtysoc -Value Invoke-KillPutty -Force
New-Alias -Name killt32soc -Value Kill-Trace32Soc -Force

Export-ModuleMember -Alias * -Function *