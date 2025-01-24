#
# Copyright (C) Microsoft Corporation. All rights reserved.
#

# Invokes a Cmd.exe shell script and updates the environment.
Function Invoke-CmdScript {
    param(
        [String] $scriptName
    )
    $cmdLine = """$scriptName"" $args & set"
    & $Env:SystemRoot\system32\cmd.exe /c $cmdLine |
        Select-String '^([^=]*)=(.*)$' | ForEach-Object {
        $varName = $_.Matches[0].Groups[1].Value
        $varValue = $_.Matches[0].Groups[2].Value
        Set-Item Env:$varName $varValue
        }
}

Function Get-FileFromWeb(
    [Parameter(Mandatory=$true)] [string] $OutDir,
    [Parameter(Mandatory=$true)] [string] $DownloadUrl,
    [Parameter(Mandatory=$true)] [string] $DownloadName,
    [Parameter(Mandatory=$false)] [boolean] $Unzip = $false
)
{
    Add-Type -AssemblyName System.IO
    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $etagFile = Join-Path -Path $OutDir -ChildPath "$DownloadName.ETag"
    $downloadPath = Join-Path -Path $env:TEMP -ChildPath "$DownloadName.download"
    $downloadDest = Join-Path -Path $OutDir -ChildPath $DownloadName
    $downloadDestTemp = Join-Path -Path $env:TEMP -ChildPath "$DownloadName.tmp"
    $headers = @{}

    # If the destination folder doesn't exist, delete the ETag file if it exists
    $destFolderExists = Test-Path -Path $downloadDest
    $etagFileExists = Test-Path -PathType Leaf $etagFile
    if ((-not $destFolderExists) -and $etagFileExists) {
        Remove-Item -Force $etagFile | Out-Null
    }

    if (Test-Path $etagFile)
    {
        $headers.Add("If-None-Match", [System.IO.File]::ReadAllText($etagFile))
    }

    # Add dummy exception type that is only in PS 6+
    if ($PSVersionTable.PSVersion.Major -lt 6)
    {
        $Source = "namespace System.Net.Http { public class HttpRequestException { public void Foo() {} } }"
        Add-Type -TypeDefinition $Source | Out-Null
    }

    try
    {
        # Removing the progress bar greatly speeds up Invoke-WebRequest
        $ProgressPreference = 'SilentlyContinue'
        $response = Invoke-WebRequest -Headers $headers -Uri $DownloadUrl -PassThru -OutFile $downloadPath -UseBasicParsing
    }
    catch [System.Net.WebException]
    {
        # Exception for PS 5
        $response = $_.Exception.Response
    }
    catch [System.Net.Http.HttpRequestException]
    {
        # Exception for PS 6+
        $response = $_.Exception.Response
    }
    finally
    {
        $ProgressPreference = 'Continue'
    }

    if ($response.StatusCode -eq 200)
    {
        Unblock-File $downloadPath

        if ($Unzip)
        {
            # Extract to a temp folder
            if (Test-Path -PathType Container $downloadDestTemp)
            {
                Remove-Item -Recurse -Force $downloadDestTemp
            }

            [System.IO.Compression.ZipFile]::ExtractToDirectory($downloadPath, $downloadDestTemp)
            Remove-Item $downloadPath
        }
        else
        {
            $downloadDestTemp = $downloadPath;
        }

        # Delete and rename to final dest
        if (Test-Path -Path $downloadDest)
        {
            Remove-Item -Recurse -Force $downloadDest
        }

        Move-Item -Force $downloadDestTemp $downloadDest

        [System.IO.File]::WriteAllText($etagFile, $response.Headers["ETag"])
    }
    elseif ($response.StatusCode -eq 304)
    {
        return
    }
    else
    {
        Write-Host
        Write-Warning "Failed to fetch updated NuGet tools from $DownloadUrl"
        if (!(Test-Path $downloadDest)) {
            throw "$DownloadName was not found at $downloadDest"
        } else {
            Write-Warning "$DownloadName may be out of date"
        }
    }
}

Function Install-AzCli()
{
    $Message = "AzureCLI"
    $Message = $Message + (" " + "." * (70 - $Message.Length) + " ")
    Write-Host  $Message -NoNewLine -ForegroundColor Cyan

    $ExpectedPath = "${env:ProgramFiles(x86)}\Microsoft SDKs\Azure\CLI2\wbin\"

    if (-not (Test-Path $ExpectedPath))
    {
        Invoke-WebRequest -Uri https://aka.ms/installazurecliwindows -OutFile .\AzureCLI.msi
        $Return = Start-Process msiexec.exe -Wait -ArgumentList '/I AzureCLI.msi /quiet /passive' -PassThru
        rm .\AzureCLI.msi

        if ($Return.ExitCode -ne 0)
        {
            Write-Host "Failed" -ForegroundColor Red
            throw "Unable to install AzCli"
        }

        if (-not (Test-Path $ExpectedPath))
        {
            Write-Host "Failed" -ForegroundColor Red
            throw "Unable to install AzCli"
        }

        $env:Path += ";$ExpectedPath"

        Write-Host "Done" -ForegroundColor Green
    }
    else
    {
        try
        {
            $Return = Invoke-Executable -exe "az" -exeArgs 'extension add --name azure-devops'
        }
        catch
        {
            Write-Host "Azure Devops install failed" -ForegroundColor Yellow
            Write-Host $Return
        }
        Write-Host "Exists" -ForegroundColor Green
        $env:Path += ";$ExpectedPath"
    }

}

Function Install-Nuget()
{
    $Message = "NuGet"
    $Message = $Message + (" " + "." * (70 - $Message.Length) + " ")
    Write-Host  $Message -NoNewLine -ForegroundColor Cyan
    mkdir -ErrorAction SilentlyContinue "$env:REPO_APP_EXTERNS_DIR/tools" | Out-Null

    Get-FileFromWeb -OutDir "$env:REPO_APP_EXTERNS_DIR/tools" -DownloadUrl "https://microsoft.pkgs.visualstudio.com/_apis/public/nuget/client/CredentialProviderBundle.zip" -Unzip $true -DownloadName "nuget" -ErrorAction Stop

    $NugetToolDir = Join-Path "$env:REPO_APP_EXTERNS_DIR/tools" -ChildPath "\nuget" -Resolve
    $env:REPO_APP_NUGET_EXE = Join-Path -Path $nugetToolDir -ChildPath "nuget.exe" -Resolve

    $Result = Invoke-Expression "$env:REPO_APP_NUGET_EXE update -self"

    Write-Host "Done" -ForegroundColor Green
}

Function Set-GitReferences($PackageFile)
{
    [Xml]$PackageConfig = Get-Content $PackageFile

    foreach($Package in $PackageConfig.packages.git.ChildNodes)
    {
        # Skip if $Package is a comment
        if ($Package -is [System.Xml.XmlComment])
        {
            continue
        }

        $Message = "$($Package.name) @ $($Package.tag)"
        $Message = $Message + (" " + "." * (70 - $Message.Length) + " ")
        Write-Host  $Message -NoNewLine -ForegroundColor Cyan
        [System.Environment]::SetEnvironmentVariable("REPO_APP_GIT_$($Package.name).URL", $Package.url)
        [System.Environment]::SetEnvironmentVariable("REPO_APP_GIT_$($Package.name).TAG", $Package.tag)
        Write-Host "Done" -ForegroundColor Green
    }
}

Function Get-AzPackages($PackageFile, $TargetDir)
{
    if (-not (Test-Path $TargetDir))
    {
        New-Item -Path $TargetDir -ItemType Directory | Out-Null
    }

    $TargetDir = (Get-Item $TargetDir).FullName

    if (-not $env:AZURE_DEVOPS_EXT_PAT)
    {
        $Return = Invoke-Executable -exe "az" -exeargs " account show --only-show-errors"

        # Auth is required
        if ($LASTEXITCODE -ne 0)
        {
            $Return = Invoke-Expression "az login --only-show-errors"
            if ($LASTEXITCODE -ne 0){ throw "Unable to authenticate user"}
        }
    }

    [Xml]$PackageConfig = Get-Content $PackageFile

    foreach($Package in $PackageConfig.packages.universal_packages.ChildNodes)
    {
        # Skip if $Package is a comment
        if ($Package -is [System.Xml.XmlComment])
        {
            continue
        }

        $Message = "$($Package.name) @ $($Package.version)"
        $Message = $Message + (" " + "." * (70 - $Message.Length) + " ")
        Write-Host  $Message -NoNewLine -ForegroundColor Cyan

        $PackageTargetPath = "$($TargetDir)\$($Package.name).$($Package.version)".replace("\", "/")
        [System.Environment]::SetEnvironmentVariable("REPO_APP_PATH_$($Package.name)", $PackageTargetPath )
        $env:Path += ";$PackageTargetPath"

        if (Test-Path $PackageTargetPath)
        {
            Write-Host "Exists" -ForegroundColor Green
            continue
        }

        $Command = "artifacts universal download " +
            "--organization $($Package.org) " +
            "--project=$($Package.project) " +
            "--scope $($Package.scope) " +
            "--feed $($Package.feed) " +
            "--name $($Package.name) " +
            "--version $($Package.version) " +
            "--path $PackageTargetPath"

        $Output = Invoke-Executable -exe "az" -exeArgs $Command

        if ($LASTEXITCODE -ne 0)
        {
            Write-Host "Failed" -ForegroundColor Red
            Write-Host $Output -ForegroundColor Red
            throw "Unable to download $($Package.name).$($Package.version) [$LASTEXITCODE]"
        }

        Write-Host "Done" -ForegroundColor Green
    }
}

Function Get-NugetPackages($PackageFile, $TargetDir)
{
    if (-not (Test-Path $TargetDir))
    {
        New-Item -Path $TargetDir -ItemType Directory | Out-Null
    }

    $TargetDir = (Get-Item $TargetDir).FullName

    # Set the nuget packages directory to a local directory (Prevents NuGet from using the user's profile directory)
    $env:NUGET_PACKAGES = "$TargetDir/temp"

    [Xml]$PackageConfig = Get-Content $PackageFile

    foreach($Package in $PackageConfig.packages.nuget_packages.ChildNodes)
    {
        # Skip if $Package is a comment
        if ($Package -is [System.Xml.XmlComment])
        {
            continue
        }

        $Message = "$($Package.name) @ $($Package.version)"
        $Message = $Message + (" " + "." * (70 - $Message.Length) + " ")
        Write-Host  $Message -NoNewLine -ForegroundColor Cyan

        $PackageTargetPath = "$($TargetDir)\$($Package.name).$($Package.version)".replace("\", "/")
        [System.Environment]::SetEnvironmentVariable("REPO_APP_PATH_$($Package.name)", $PackageTargetPath )
        $env:Path += ";$PackageTargetPath"

        if (Test-Path $PackageTargetPath)
        {
            Write-Host "Exists" -ForegroundColor Green
            continue
        }

        $Command = "install " +
            "$($Package.name) " +
            "-version $($Package.version) " +
            "-source $($Package.source) " +
            "-OutputDirectory $TargetDir"

        $Output = Invoke-Executable -exe $env:REPO_APP_NUGET_EXE -exeArgs $Command

        if ($LASTEXITCODE -ne 0)
        {
            Write-Host "Failed" -ForegroundColor Red
            Write-Host $Output -ForegroundColor Red
            throw "Unable to download $($Package.name).$($Package.version) [$LASTEXITCODE]"
        }

        Write-Host "Done" -ForegroundColor Green
    }
}

Function Get-PipPackages($PackageFile)
{
    [Xml]$PackageConfig = Get-Content $PackageFile

    foreach($Package in $PackageConfig.packages.pip_packages.ChildNodes)
    {
        # Skip if $Package is a comment
        if ($Package -is [System.Xml.XmlComment])
        {
            continue
        }
        $Message = "$($Package.name) @ $($Package.version)"
        $Message = $Message + (" " + "." * (70 - $Message.Length) + " ")
        Write-Host  $Message -NoNewLine -ForegroundColor Cyan

        # See https://github.com/pypa/pip/issues/8559 for json2html dependency
        $Command = "-m pip install $($Package.name)==$($Package.version) -i $($Package.source) --no-warn-script-location --disable-pip-version-check --use-pep517"

        $Output = Invoke-Executable -exe "${env:REPO_APP_PATH_python.win64}/tools/python.exe" -exeArgs $Command

        if ($LASTEXITCODE -ne 0)
        {
            Write-Host "Failed" -ForegroundColor Red
            Write-Host $Output -ForegroundColor Red
            throw "Unable to download $($Package.name).$($Package.version) [$LASTEXITCODE]"
        }

        Write-Host "Done" -ForegroundColor Green
    }

    $env:Path += ";${env:REPO_APP_PATH_python.win64}/tools/Scripts"
}

Function Write-Title($Title, $Color)
{
    $DecoratorSize = ($Host.UI.RawUI.WindowSize.Width - $Title.Length - 4)/2
    if ($DecoratorSize -lt 0)
    {
        $DecoratorSize = 0
    }

    Write-Host ""
    Write-Host -ForegroundColor $Color (("-" * $DecoratorSize) + " " + $Title + " " + ("-" * $DecoratorSize))
    Write-Host ""
}

Function Invoke-Executable($exe, $exeArgs)
{
    $Explode = $exeArgs.Split(' ')
    $Output =  [string](& $exe $Explode 2>&1)
    return $Output
}

<#
.SYNOPSIS
Downloads prerequisites and sets up the cmake structure for a given toolchain

.PARAMETER Toolchain
Toolchain to initialize the repo with

.PARAMETER Configuration
Artifact configuration debug/release

.EXAMPLE
Set-RepoEnv -Toolchain i386-pc-windows-msvc -Configuration debug
#>
Function Set-RepoEnv()
{
    [CmdletBinding()]
    param(
        [ValidateSet("Debug", "Release")] [string] $Configuration = "Debug",
        [boolean] $MSCP_vUART = $false
    )
    DynamicParam
    {
        $RepoRoot = "$PSScriptRoot\..\..\..\"
        $Attributes = New-Object System.Management.Automation.ParameterAttribute
        $Attributes.Mandatory = $false
        $Attributes.ParameterSetName = "__AllParameterSets"
        $Attributes.Position = 1

        $Values = (Get-ChildItem "$RepoRoot/tools/cmakes/toolchain" -ErrorAction SilentlyContinue).Name

        if($null -eq $Values)
        {
            $Values = "No Toolcahin Cmakes Found"
        }

        $ValidateSet = New-Object System.Management.Automation.ValidateSetAttribute($Values)

        $AttribColl = New-Object System.Collections.ObjectModel.Collection[System.Attribute]
        $AttribColl.Add($Attributes)
        $AttribColl.Add($ValidateSet)

        $Toolchain = New-Object System.Management.Automation.RuntimeDefinedParameter("Toolchain", [string], $AttribColl)
        $Toolchain.Value = $Values[0] # Default to the first toolchain
        
        $ParamDic = New-Object System.Management.Automation.RuntimeDefinedParameterDictionary
        $ParamDic.Add("Toolchain", $Toolchain)
        $PSBoundParameters["Toolchain"] = $Toolchain.Value

        return $ParamDic
    }

    process
    {
        # Clean path
        if ($env:REPO_APP_CACHED_PATH)
        {
            $env:PATH = $env:REPO_APP_CACHED_PATH
        }
        else
        {
            $env:REPO_APP_CACHED_PATH = $env:PATH
        }

        $Toolchain = $PSBoundParameters.Toolchain

        $MSCP_vUART = $PSBoundParameters.MSCP_vUART

        # Config
        Write-Title -Title "Configuration" -Color Cyan
        Write-Host "Toolchain:     $Toolchain"
        Write-Host "Configuration: $Configuration"
        Write-Host "MSCP vUART:    $MSCP_vUART"

        #env
        $env:REPO_APP_TOOLCHAIN = $Toolchain
        $env:REPO_APP_BUILD_CONFIG = $Configuration
        $env:REPO_APP_MSCP_VUART = $MSCP_vUART

        $env:REPO_APP_ROOT = (Get-Item "$PSScriptRoot\..\..\..\").ToString().Replace("\","/")
        $env:REPO_APP_BUILD_DIR = (Join-Path $env:REPO_APP_ROOT ".build").replace("\", "/")
        $env:REPO_APP_TEST_LOG_DIR = (Join-Path $env:REPO_APP_ROOT ".testlogs").replace("\", "/")
        $env:REPO_APP_EXTERNS_DIR = (Join-Path "$env:REPO_APP_ROOT" ".externs").replace("\", "/")
        $env:REPO_APP_AZPKG_DIR = (Join-Path "$env:REPO_APP_EXTERNS_DIR" "azpkg").replace("\", "/")
        $env:REPO_APP_NUGET_DIR = (Join-Path "$env:REPO_APP_EXTERNS_DIR" "nuget").replace("\", "/")
        $env:REPO_APP_TOOLS_DIR = (Join-Path "$env:REPO_APP_ROOT" "tools").replace("\", "/")
        $env:REPO_APP_CMAKE_TOOLCHAIN_DIR = (Join-Path "$env:REPO_APP_TOOLS_DIR" "cmakes\toolchain\$Toolchain").replace("\", "/")
        $env:LM_LICENSE_FILE = "40002@wanlic.svceng.com"

        $env:REPO_APP_LOG_FILE = "$env:REPO_APP_BUILD_DIR\repolog.txt"

        Get-VersionData

        # Pre-requisites
        Write-Title -Title "Pre-requisites" -Color Cyan
        Install-AzCli
        Install-Nuget
        Get-AzPackages -PackageFile "$env:REPO_APP_TOOLS_DIR/packages.xml" -TargetDir $env:REPO_APP_AZPKG_DIR
        Get-NugetPackages -PackageFile "$env:REPO_APP_TOOLS_DIR/packages.xml" -TargetDir $env:REPO_APP_NUGET_DIR
        Get-PipPackages -PackageFile "$env:REPO_APP_TOOLS_DIR/packages.xml"
        Set-GitReferences -PackageFile "$env:REPO_APP_TOOLS_DIR/packages.xml"

        Get-AzPackages -PackageFile "$env:REPO_APP_TOOLS_DIR/packages.$Toolchain.xml" -TargetDir $env:REPO_APP_AZPKG_DIR
        Get-NugetPackages -PackageFile "$env:REPO_APP_TOOLS_DIR/packages.$Toolchain.xml" -TargetDir $env:REPO_APP_NUGET_DIR
        Get-PipPackages -PackageFile "$env:REPO_APP_TOOLS_DIR/packages.$Toolchain.xml"
        Set-GitReferences -PackageFile "$env:REPO_APP_TOOLS_DIR/packages.$Toolchain.xml"

        # Set Build
        Write-Title -Title "Setting Cmake" -Color Cyan
        Set-Cmake -Toolchain $Toolchain -Configuration $Configuration

        $host.ui.RawUI.WindowTitle = $Name

        Write-Title "Welcome to the $Name repo" -Color Yellow

        $Links = @{
            "[Getting Started]" = "https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.MSCP?path=/docs/GettingStarted.md"
            "[Guidelines]" = "https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.MSCP?path=/docs/guidelines/GeneralGuidelines.md"
        }

        $ConcatString = ""
        $Length = 0
        $Links.GetEnumerator() | %{$ConcatString += " " + (Format-Url -Label $_.key -Url $_.value)}
        $Links.GetEnumerator() | %{$Length += $_.key.Length}
        $DecoratorSize = ($Host.UI.RawUI.WindowSize.Width - $Length - 5)/2
        Write-Host ((' ' * $DecoratorSize) + $ConcatString) -ForegroundColor Blue

        Write-Host "`nAvailable commands:`n"
        Get-RepoHelp
    }
}

<#
.SYNOPSIS
Creates the cmake tree structure for a given configuration

.PARAMETER Toolchain
Toolchain to initialize the repo with

.PARAMETER Configuration
Artifact configuration debug/release

.EXAMPLE
Set-Cmake -Toolchain win -Configuration debug
#>
Function Set-Cmake($Toolchain = $env:REPO_APP_TOOLCHAIN, $Configuration = $env:REPO_APP_BUILD_CONFIG)
{
    $env:REPO_APP_TARGET_BUILD_DIR = "$env:REPO_APP_BUILD_DIR\$Configuration\$Toolchain"
    $env:REPO_APP_TARGET_FLASH_DIR = Join-Path $env:REPO_APP_TARGET_BUILD_DIR/bin "flash"
    & ${env:REPO_APP_PATH_cmake.win64}/bin/cmake.exe --no-warn-unused-cli -G Ninja -B $env:REPO_APP_TARGET_BUILD_DIR -DCMAKE_TOOLCHAIN_FILE="$env:REPO_APP_CMAKE_TOOLCHAIN_DIR\toolchain.cmake" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE="$Configuration" -Wno-dev
}

<#
.SYNOPSIS
Builds for the current configuration

.EXAMPLE
Invoke-Build
#>
Function Invoke-Build()
{
    # Keeps x86 build from failing/breaking pipeline build stage
    $ErrorActionPreference = "SilentlyContinue"
    ninja -C $env:REPO_APP_TARGET_BUILD_DIR $args

    # check for successful build
    $build_status = Join-Path -Path $(Get-ChildItem -Path "Env:REPO_APP_TARGET_BUILD_DIR").Value -ChildPath '.allbuilt'
    if (Test-Path -Path $build_status){
        Remove-Item -Path $build_status -Force
        Invoke-Buildsize
        Invoke-CreateIfwi
    }
}

<#
.SYNOPSIS
Finds the size of .text, .data, and .bss sections of current build .elf file

.EXAMPLE
Invoke-Buildsize
#>
Function Invoke-Buildsize()
{
    # Set up the path to search for elf files
    $BuildDirectory = Join-Path -Path $(Get-ChildItem -Path "Env:REPO_APP_TARGET_BUILD_DIR").Value -ChildPath 'bin'

    # Use Get-ChildItem to find all files with .elf extension in the directory
    $elfFiles = Get-ChildItem -Path $BuildDirectory -Recurse -Filter *fw.elf

    # Iterate over each .elf file
    foreach ($file in $elfFiles) {
        # Process each .elf file for sizes
        & "$env:REPO_APP_TOOLS_DIR\pwsh\binsize\binsizes.ps1" -FileName $file
    }
}

<#
.SYNOPSIS
Creates the kingsgate hsp and scp ifwi file to run on SVP

.EXAMPLE
Invoke-CreateIfwi
#>
Function Invoke-CreateIfwi()
{
    if ($env:REPO_APP_TOOLCHAIN -ne "arm-eabi-aarch")
    {
        Write-Host "`nSkipping ifwi creation`n"
        return
    }

    # check if flash folder is already present if not create it
    if (-not (Test-Path -Path $env:REPO_APP_TARGET_FLASH_DIR -PathType Container)) {
        New-Item -Path $env:REPO_APP_TARGET_FLASH_DIR -ItemType Directory | Out-Null
        Write-Host "Created $env:REPO_APP_TARGET_FLASH_DIR"
    } else {
        Write-Host "$env:REPO_APP_TARGET_FLASH_DIR already exists"
    }

   
    # invoke datwizard to create the ifwi files
    & ${env:REPO_APP_PATH_1pfw.datwizard-preview-x86_64-pc-windows-msvc}\tools\datwizard.exe build --manifest-path $env:REPO_APP_ROOT/config/dat_config/dat.toml --flavor kingsgate.ifwi.svp.debug.custom --outdir $env:REPO_APP_TARGET_FLASH_DIR
    & ${env:REPO_APP_PATH_1pfw.datwizard-preview-x86_64-pc-windows-msvc}\tools\datwizard.exe build --manifest-path $env:REPO_APP_ROOT/config/dat_config/dat.toml --flavor kingsgate.ifwi.soc.debug.custom --outdir $env:REPO_APP_TARGET_FLASH_DIR
    & ${env:REPO_APP_PATH_1pfw.datwizard-preview-x86_64-pc-windows-msvc}\tools\datwizard.exe build --manifest-path $env:REPO_APP_ROOT/config/dat_config/dat.toml --flavor kingsgate.ifwi.fpga.debug.custom --outdir $env:REPO_APP_TARGET_FLASH_DIR
    & ${env:REPO_APP_PATH_1pfw.datwizard-preview-x86_64-pc-windows-msvc}\tools\datwizard.exe build --manifest-path $env:REPO_APP_ROOT/config/dat_config/dat.toml --flavor kingsgate.ifwi.ap.baremetal.debug --outdir $env:REPO_APP_TARGET_FLASH_DIR
}

<#
.SYNOPSIS
Cleans the current build for the current configuration

.EXAMPLE
Invoke-Clean
#>
Function Invoke-Clean()
{
    ninja -C $env:REPO_APP_TARGET_BUILD_DIR clean
}

<#
.SYNOPSIS
Generates coverage from the unit tests.
NB: requires the unit tests to be built and run first
Also requires the FW to be built with the ARM configuration

.EXAMPLE
Generate-Coverage
#>
Function Generate-Coverage($TargetLogDir)
{
    mkdir "$TargetLogDir/gcov_reports" -Force
    $GcovOutput = "$TargetLogDir/gcov_reports/".replace("\", "/")
    $RepoRoot = $env:REPO_APP_ROOT.replace("\", "/")
    & gcovr --filter "${RepoRoot}src/" --gcov-exclude-directories "${RepoRoot}/.build/Debug/arm-eabi-aarch/src/externs/" --verbose --exclude-unreachable-branches `
            --gcov-executable "${env:REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/bin/arm-none-eabi-gcov.exe" `
            --exclude ".+_cli.c" --exclude ".+cli_power.c" --exclude ".+cli_ddr.c" --cobertura --output $GcovOutput

    & "${env:REPO_APP_PATH_report-generator}/tools/net6.0/ReportGenerator.exe" `
        -reports:"$TargetLogDir/**/cobertura.xml" `
        -targetdir:"$TargetLogDir/Coverage" `
        -reporttypes:"Cobertura;Html"
}

<#
.SYNOPSIS
Runs the unit tests for the current configuration

.EXAMPLE
Invoke-UnitTests
#>
Function Invoke-UnitTests($Suite)
{
    if ($Suite)
    {
        $SuiteArg = "/suite:$Suite"
    }

    if (($env:REPO_APP_TOOLCHAIN -ne "i386-pc-windows-msvc") -Or ($env:REPO_APP_BUILD_CONFIG -ne "Debug"))
    {
        Write-Host -ForegroundColor Red "Incorrect toolchain / configuration for unit tests, use Toolchain - i386-pc-windows-msvc and config - Debug"
        return
    }

    if(Test-Path -Path "$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/$env:REPO_APP_TOOLCHAIN/bin/*test*")
    {
        & ${env:REPO_APP_PATH_1pfw.tools.win64}/sptest.exe `
        /devicesfile:"$env:REPO_APP_ROOT/tests/unit/SpTestDevices.xml" `
        /packagesdir:"$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/$env:REPO_APP_TOOLCHAIN/bin" `
        /logsdir:"$env:REPO_APP_TEST_LOG_DIR" `
        /define:test_bin_dir="$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/$env:REPO_APP_TOOLCHAIN/bin" `
        /define:test_src_dir="$env:REPO_APP_ROOT" `
        $SuiteArg /runtests /hideWindows

       $TargetLogDir = (Get-ChildItem $env:REPO_APP_TEST_LOG_DIR | Sort -Descending)[0].FullName

       Generate-Coverage $TargetLogDir
    }
    else {
        Write-Host -ForegroundColor Red "No packages found at path:" "$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/$env:REPO_APP_TOOLCHAIN/bin"
        Write-Host -ForegroundColor Yellow "Are any unit tests enabled for the configuration?" $env:REPO_APP_TOOLCHAIN $env:REPO_APP_BUILD_CONFIG
    }
}

<#
.SYNOPSIS
Runs a local unit test suite with coverage. Skips entire system  code coverage.

.EXAMPLE
Invoke-LocalUnitTests
#>
Function Invoke-LocalUnitTests($Suite)
{
    if ($Suite)
    {
        $SuiteArg = "/suite:$Suite"
    }

    if (($env:REPO_APP_TOOLCHAIN -ne "i386-pc-windows-msvc") -Or ($env:REPO_APP_BUILD_CONFIG -ne "Debug"))
    {
        Write-Host -ForegroundColor Red "Incorrect toolchain / configuration for unit tests, use Toolchain - i386-pc-windows-msvc and config - Debug"
        return
    }

    if(Test-Path -Path "$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/$env:REPO_APP_TOOLCHAIN/bin/*test*")
    {
        & ${env:REPO_APP_PATH_1pfw.tools.win64}/sptest.exe `
        /devicesfile:"$env:REPO_APP_ROOT/tests/unit/SpTestDevices.xml" `
        /packagesdir:"$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/$env:REPO_APP_TOOLCHAIN/bin" `
        /logsdir:"$env:REPO_APP_TEST_LOG_DIR" `
        /define:test_bin_dir="$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/$env:REPO_APP_TOOLCHAIN/bin" `
        /define:test_src_dir="$env:REPO_APP_ROOT" `
        $SuiteArg /runtests /hideWindows

       $TargetLogDir = (Get-ChildItem $env:REPO_APP_TEST_LOG_DIR | Sort -Descending)[0].FullName

       & "${env:REPO_APP_PATH_report-generator}/tools/net6.0/ReportGenerator.exe" `
       -reports:"$TargetLogDir/**/cobertura.xml" `
       -targetdir:"$TargetLogDir/Coverage" `
       -reporttypes:"Cobertura;Html"
    }
    else {
        Write-Host -ForegroundColor Red "No packages found at path:" "$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/$env:REPO_APP_TOOLCHAIN/bin"
        Write-Host -ForegroundColor Yellow "Are any unit tests enabled for the configuration?" $env:REPO_APP_TOOLCHAIN $env:REPO_APP_BUILD_CONFIG
    }
}

<#
.SYNOPSIS
Returns a url string for display

.EXAMPLE
Format-URL -Label "Guideline" -Url "https://azurecsi.visualstudio.com/DuvallFw/_wiki/wikis/1PFw%20Firmware%20Libs?pagePath=/Guidelines"
#>
Function Format-URL($Label, $Url)
{
    return "$([char]0x1b)]8;;$Url$([char]0x1b)\$Label$([char]0x1b)]8;;$([char]0x1b)\"
}

<#
.SYNOPSIS
Gets the help information of the commands available

.EXAMPLE
Get-RepoHelp
#>
Function Get-RepoHelp()
{
    $Commands = (Get-Module Build-Utils).ExportedAliases.Values.GetEnumerator()

    foreach($Command in $Commands)
    {
        $Message = $Command.Name
        $Message = $Message + (" " * (20 - $Message.Length))
        $Help = Get-Help $Command
        Write-Host $Message -NoNewLine -ForegroundColor Yellow
        Write-Host $Help.Synopsis
    }

    Write-Host "`nFor more help on a given command: help <command>"
}

<#
.SYNOPSIS
Parse Version Data from the CI and Release Pipelines, and set to environment variables.

.EXAMPLE
Get-VersionData
#>
Function Get-VersionData()
{
    # Install powershell-yaml if not available
    Install-Module powershell-yaml -Force -MinimumVersion "0.4.7" -Repository PSGallery -Scope CurrentUser -WarningAction SilentlyContinue

    $ci_yml_data = Get-Content -Path './tools/pipelines/ci.yaml' | ConvertFrom-Yaml
    $ci_version = $ci_yml_data.name

    $release_yml_data = Get-Content -Path './tools/pipelines/release.yaml' | ConvertFrom-Yaml
    $release_version = $release_yml_data.name

    if ($ci_version -ne $release_version)
    {
        throw "CI vs. Release Versions don't match: $ci_version vs. $release_version"
    }

    $version = $release_version.split(".")
    $env:MajorVersion = $version[0]
    $env:MinorVersion = $version[1]
    $env:PatchVersion = $version[2]
}

New-Alias -Name setenv -Value Set-RepoEnv
New-Alias -Name remake -Value Set-Cmake
New-Alias -Name build -Value Invoke-Build
New-Alias -Name cleanbuild -Value Invoke-Clean
New-Alias -Name unittest -Value Invoke-UnitTests
New-Alias -Name repohelp -Value Get-RepoHelp
New-Alias -Name localunittest -Value Invoke-LocalUnitTests

Export-ModuleMember -Alias * -Function *