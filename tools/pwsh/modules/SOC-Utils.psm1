#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

<#
.SYNOPSIS
Fetches the DC-SCM IP from a predefines list of PC names and their corresponding DC-SCP IP addresses

.EXAMPLE
Get-SOCSshIp 'A15-R3'
#>
function Get-SOCSshIp {
    param (
        [string]$PCName
    )

    # Define an array of hashtables with PC Name and DC-SCM IP
    $IPList = @(
        @{ "PC Name" = "A15-R3";  "IsRM" = $false; "DC-SCM IP" = "M1120-SCM-445.svceng.com" },
        @{ "PC Name" = "B20-B1";  "IsRM" = $false; "DC-SCM IP" = "M1120-SCM-278.svceng.com" },
        @{ "PC Name" = "B20-B2";  "IsRM" = $false; "DC-SCM IP" = "M1120-SCM-393.svceng.com" },
        @{ "PC Name" = "G10-B2";  "IsRM" = $false; "DC-SCM IP" = "M1120-SCM-351.svceng.com" },
        @{ "PC Name" = "J10-R5";  "IsRM" = $false; "DC-SCM IP" = "M1120-SCM-392.svceng.com" },
        @{ "PC Name" = "E12-N1";  "IsRM" = $true; "DC-SCM IP" = "172.17.0.97";  "RM IP" = "172.29.89.33"; "Node ID" = "25"}
    )

    # Find the matching PC Name and return both IsRM and IP
    $entry = $IPList | Where-Object { $_."PC Name" -eq $PCName }

    if ($entry) {
        if ($entry."IsRM") {
            return @{
                IsRm  = $true
                RmIp  = $entry."RM IP"
                BmcIp = $entry."DC-SCM IP"
                NodeID = $entry."Node ID"
            }
        } else {
            return @{
                IsRM = $false
                RmIp  = "na"
                BmcIp = $entry."DC-SCM IP"
                NodeID = "na"
            }
        }
    } else {
        return @{
            IsRM  = $false
            RmIp  = "na"
            BmcIp = "na"
            NodeID = "na"
        }
    }
}

<#
.SYNOPSIS
Creates a Ifwi tar file for the specified platform and target

.EXAMPLE
createifwitar -version 1.2.3-4
#>
Function Invoke-CreateIfwiTar(
    [Parameter(Mandatory=$false)] [string] $dat_loc = "${env:REPO_APP_ROOT}/.build/Debug/arm-eabi-aarch/bin/flash",
    [Parameter(Mandatory=$false)] [string] $dat_file = "kingsgate.ifwi.soc.debug.custom.dat",
    [Parameter(Mandatory=$false)] [string] $tar_loc = "${env:REPO_APP_ROOT}/.build/Debug/arm-eabi-aarch/bin/flash/kingsgate.ifwi.soc.debug.custom.tar.gz",
    [Parameter(Mandatory=$false)] [string] $version = "0.0.0-0"
)
{

    Write-Title -Title "Creating Ifwi Tar" -Color Cyan

    # Remove old tar file
    Remove-Item -Path $tar_loc -Force -ErrorAction SilentlyContinue

    $env:IFWI_FILE_NAME = $dat_file

    $ifwi_config_in = Join-Path -Path $env:REPO_APP_ROOT -ChildPath "tools\gentar\input_files.json"
    $ifwi_config_out = Join-Path -Path $dat_loc -ChildPath "input_files.json"
    Expand-File -in_file $ifwi_config_in -out_file $ifwi_config_out

    Push-Location -Path $env:REPO_APP_ROOT/tools/gentar

    & ${env:REPO_APP_PATH_python.win64}\tools\python.exe $env:REPO_APP_ROOT/tools/gentar/gen-firmware-tar.py `
        -c BIOS -m m1120-C4143 -v $version -p None `
        -i $dat_loc `
        -o $tar_loc

    # Move back to wherever the function was invoked from
    Pop-Location

    if (-not (Test-Path $tar_loc)) {
        throw "Failed to create Ifwi tar at: $tar_loc"
    }

    Write-Host "Ifwi tar created successfully at: $tar_loc" -ForegroundColor Green
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
    [Parameter(Mandatory=$false)] [string] $loc = "${env:REPO_APP_ROOT}/.build/Debug/arm-eabi-aarch/bin/flash",
    [Parameter(Mandatory=$false)] [string] $file = "kingsgate.ifwi.soc.debug.custom.dat",
    [Parameter(Mandatory=$false)] [string] $securefile = "tools/robot-secure-txt.yml"
)
{
    # Load secure credentials from YAML file
    Import-Module powershell-yaml
    $cred = Get-Content -Path $securefile | ConvertFrom-Yaml

    $result = Get-SOCSshIp "$system"

    $is_rm = $result.IsRM
    $nodeid = $result.NodeID

    $bmcip = $result.BmcIp
    $bmcuser = $cred.BMC_USER
    $bmcpw = $cred.BMC_PASSWORD
    $sudobmcpw = $cred.SUDO_BMC_PASSWORD
    $bmcdest = "/var/wcs/home"

    $rmip = $result.RmIp
    $rmuser = $cred.RM_USER
    $rmpw = $cred.RM_PASSWORD
    $rmdest = "/usr/srvroot/shared"

	$localhost = "127.0.0.1"

    # Program flash if SOC system found
    if ($bmcip -ne "na")
    {
        if ($is_rm) {

            $FullSysName = $system
            
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
            Write-Host -ForegroundColor Red "            |;/     |;;(              - Please only program your test Node!!!"
            Write-Host -ForegroundColor Red "            ''      /;;|              - Do not flash someone else's Node!!!"
            Write-Host -ForegroundColor Red "                    \;;|              - Use the right system name!!!"
            Write-Host -ForegroundColor Red "                     \/                     "

            $tar_file = ($file -replace '\.dat$', '') + '.tar.gz'
            $tar_loc = Join-Path -Path $loc -ChildPath $tar_file
        
            Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
            Write-Host -ForegroundColor Blue "Generating tar ifwi . . ."
            Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
            Write-Host -ForegroundColor Blue "DAT file  : $loc/$file"
            Write-Host -ForegroundColor Blue "Tar File  : $tar_loc"
            Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
            Invoke-CreateIfwiTar -dat_file $file -tar_loc $tar_loc
            Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
            
            Write-host ""

            $srcfile = "$tar_loc"
            Write-Host "Updated source file path: $srcfile"

            Write-Title -Title "Copying Ifwi to RM" -Color Cyan
            Write-Host -ForegroundColor Blue "System: $system"
            Write-Host -ForegroundColor Blue "RM IP : $rmip"
			Write-Host -ForegroundColor Blue "Bmc IP: $bmcip"
            Write-Host -ForegroundColor Blue "File  : $srcfile"
            Write-Host -ForegroundColor Blue "Dest  : ${rmip}:$rmdest"
            Write-host ""

            # Check if Posh-SSH module is available
            if (-not (Get-InstalledModule -Name Posh-SSH -ErrorAction SilentlyContinue)) {
                Write-Host -ForegroundColor Yellow "Posh-SSH module not found. Installing..."
                try {
                    Install-Module -Name Posh-SSH -Force -Scope CurrentUser -AllowClobber
                    Write-Host -ForegroundColor Green "Posh-SSH module installed successfully."
                }
                catch {
                    Write-Error "Failed to install Posh-SSH module: $($_.Exception.Message)"
                    Write-Host -ForegroundColor Red "Please install Posh-SSH manually: Install-Module -Name Posh-SSH -Force -Scope CurrentUser"
                    return
                }
            }

            Import-Module Posh-SSH

            # Create the SFTP and SSH sessions to the RM
            $RmSecurePassword = ConvertTo-SecureString $rmpw -AsPlainText -Force
            $RmCredential = New-Object System.Management.Automation.PSCredential ($rmuser, $RmSecurePassword)
            $RmSFTPSession = New-SFTPSession -ComputerName $rmip -Credential $RmCredential -AcceptKey -Force -KeepAliveInterval 180
            $RmSSHSession = New-SSHSession -ComputerName $rmip -Credential $RmCredential -AcceptKey -Force -KeepAliveInterval 180

            # Copy the file to the RM
            #Set-SFTPItem -SessionId $RmSFTPSession.SessionId -Path $srcfile -Destination $rmdest -Force
            try {
                Set-SFTPItem -SessionId $RmSFTPSession.SessionId -Path $srcfile -Destination $rmdest -Force -Verbose
            } catch {
                Write-Host -ForegroundColor Red "File transfer to RM failed."
                Remove-SFTPSession -SFTPSession $RmSFTPSession | Out-Null
                Remove-SSHSession -SSHSession $RmSSHSession | Out-Null
                return
            }
            Write-Host -ForegroundColor Green "File transfer to RM successful."

            # Flash Ifwi tar file
			Write-Title -Title "Flash Ifwi tar file" -Color Cyan
            $Command = "set system bios update -i $nodeid -f kingsgate.ifwi.soc.debug.custom.tar.gz"
            $maxRetries = 4
            $retryCount = 0
            $success = $false

            while (-not $success -and $retryCount -lt $maxRetries) {
                try {
                    $output = Invoke-SSHCommand -SSHSession $RmSSHSession -Command $Command -ShowStandardOutputStream -Timeout 180
                    if ($output.ExitStatus -eq 0) {
                        $success = $true
                    } else {
                        Write-Host -ForegroundColor Yellow "Command failed (ExitStatus: $($output.ExitStatus)). Retrying..."
                        $retryCount++
                    }
                } catch {
                    Write-Host -ForegroundColor Yellow "Command execution error: $($_.Exception.Message). Retrying..."
                    $retryCount++
                }
            }

            if (-not $success) {
                Write-Host -ForegroundColor Red "Failed to flash Ifwi tar."
            }
            else {
                Write-Host -ForegroundColor Green "Ifwi tar successfully flashed."
            }

            Remove-SFTPSession -SFTPSession $RmSFTPSession | Out-Null
			Remove-SSHSession -SSHSession $RmSSHSession | Out-Null
            Write-host ""
        } else {
            $srcfile = "$loc/$file"

        Write-Title -Title "Copying Ifwi to BMC" -Color Cyan
        Write-Host -ForegroundColor Blue "System: $system"
        Write-Host -ForegroundColor Blue "IP    : $bmcip"
        Write-Host -ForegroundColor Blue "File  : $srcfile"
        Write-Host -ForegroundColor Blue "Dest  : ${bmcip}:${bmcdest}"

        pscp -hostkey "ssh-rsa 2048 SHA256:+vQIRLRXfSLV4m8RmtiVCjevzQTvHl3xp/VvojmwAFQ" -P 2200 -pw $bmcpw $srcfile "$bmcuser@${bmcip}:$bmcdest"
        Write-host ""

        Write-Title -Title "Programming SoC Flash" -Color Cyan
        echo "n" | plink -ssh -hostkey "ssh-rsa 2048 SHA256:+vQIRLRXfSLV4m8RmtiVCjevzQTvHl3xp/VvojmwAFQ" -P 2200 $bmcuser@$bmcip -pw $bmcpw "echo $sudobmcpw | sudo -S su -c 'bios-updater -mode fwupdate -file $bmcdest/$file -index 0'"
        Write-host ""

        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        #Write-Host -ForegroundColor Blue "Resetting Board via I2C for Silicon RVP. . ."
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        #echo "n" | plink -ssh -hostkey "ssh-rsa 2048 SHA256:+vQIRLRXfSLV4m8RmtiVCjevzQTvHl3xp/VvojmwAFQ" -P 2200 $user@$ip -pw $pw "echo $bmcpw | sudo -S su -c 'i2ctransfer -f -y 6 w1@0x52 0xd9p'"
        #plink -ssh -hostkey "ssh-rsa 2048 SHA256:+vQIRLRXfSLV4m8RmtiVCjevzQTvHl3xp/VvojmwAFQ" -P 2200 $user@$ip -pw $pw "echo $bmcpw | sudo -S su -c 'i2ctransfer -f -y 6 w1@0x52 0xd9p'"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        }
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
Programs the flash on a PBU Node

.EXAMPLE
Reset-NodeFirmware -ip <dc-scm IP> -loc <location of the image> -file <image file> -user <username> -pw <password>
#>
Function Reset-Node(
    [Parameter(Mandatory=$true)] [string] $system,
    [Parameter(Mandatory=$false)] [string] $rm_user = "root",
    [Parameter(Mandatory=$true)] [string] $pw 
)
{
    $result = Get-SOCSshIp "$system"
    $rmip = $result.RmIp
    $nodeid = $result.NodeID

    # Program flash if FPGA system found
    if ($ip -eq "NA")
    {
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host "Rack Manager IP Not Found - Unable to reset Node"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        return
    }
    else 
    {
        $FullSysName = $system

        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "Resetting Node. . ."
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        Write-Host -ForegroundColor Blue "System: $FullSysName"
        Write-Host -ForegroundColor Blue "IP    : $rmip"
        Write-Host -ForegroundColor Blue "Node  : $nodeid"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"
        echo "n" | plink -ssh -pw $rmpw $rm_user@$rmip "set sys reset -i $nodeid"
        Write-Host -ForegroundColor Blue "------------------------------------------------------------------"

        Write-host ""
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
    & \\shakti.svceng.com\lab\validation\projects\kingsgate\tools\Kingsgate_TRACE32\Start\JTAG\5_TRACE32_JTAG_KILL.bat
}

<#
.SYNOPSIS
Fetches the RM Password from the keyvault

.EXAMPLE
Fetch-RMPass
#>
Function Fetch-RMPass(
    [Parameter(Mandatory=$false)] [string] $keyvault = "wus3-dev-rm-kv-001"
)
{
    # Get the secret ID for a specific secret from the keyvault
    $secretName = "SCHIE-Lab-Rack-Manager" # Replace with the actual secret name
    $secretId = az keyvault secret show --vault-name $keyvault --name $secretName --query "id" -o tsv

    Write-Host "Fetching Key from KeyVault"
    if (-not $secretId) {
        Write-Host "Secret not found in KeyVault" -ForegroundColor Red
        return $null
    }

    # Get the secret value
    $rmpw = az keyvault secret show --id $secretId --query "value" -o tsv
    if (-not $rmpw) {
        Write-Host "Failed to fetch Key from KeyVault" -ForegroundColor Red
        return $null
    }

    return $rmpw
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