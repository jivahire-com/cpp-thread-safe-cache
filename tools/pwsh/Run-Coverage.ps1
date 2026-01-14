# Copyright (c) Microsoft Corporation. All rights reserved.

param($TestExecutable, $CoverageFile, $ExecutableArgs, $LogPath)

# Create temporal files for subproducts
$ProfRawFile = "$LogPath/raw.profraw"
$ProfDataFile = "$LogPath/data.profdata"
$LCovFile = "$LogPath/cov.lcov"

# Regex match the paths to exclude
$ExcludedPaths = @((".*\\\.externs\\.*"), (".*\\tests\\.*"), (".+\\power_stub.c"), (".+_cli.c"), (".+cli_.+.c"), (".+\\mu_public.h"), (".+\\mu_profile.h"), (".+\\mu_service.h"))
$ExcludedPaths = $ExcludedPaths | %{"--ignore-filename-regex=`"" + $_ + "`""}

# Set up the file modifier for the raw coverage
$env:LLVM_PROFILE_FILE = $ProfRawFile

# Invoke test target
& "$TestExecutable" $ExecutableArgs
Write-Host "Exit Code $LASTEXITCODE"

# Create ADO compatible coverage report
& "${env:REPO_APP_PATH_llvm.win64}/bin/llvm-profdata.exe" merge -sparse "$ProfRawFile" -o "$ProfDataFile"
& "${env:REPO_APP_PATH_llvm.win64}/bin/llvm-cov.exe" export -format=lcov -instr-profile "$ProfDataFile" "$TestExecutable" $ExcludedPaths | Out-File -File $LCovFile -Encoding ascii
& "${env:REPO_APP_PATH_python.win64}\tools\python.exe" "${env:REPO_APP_PATH_lcov.cobertura}\lcov_cobertura.py" "$LCovFile" --base-dir "$env:REPO_APP_ROOT" -o "$CoverageFile"

# Convert class names into file names
$xmlContent = Get-Content $CoverageFile -Raw
$pattern = 'class .+ name="(?<classname>.+)"'
$matches = [regex]::Matches($xmlContent, $pattern)


foreach ($match in $matches) {
    $className = $match.Groups["classname"].value
    $fileName = $className.Split('.')[-2..-1] -Join "_"
    $xmlContent = $xmlContent.Replace($className, $fileName)
}
Set-Content -Path $CoverageFile -Value $xmlContent