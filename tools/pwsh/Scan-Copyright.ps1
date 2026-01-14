# Copyright (c) Microsoft Corporation. All rights reserved.

<#
.SYNOPSIS
    Scans for files with improper or missing Microsoft notice headers.

.DESCRIPTION
    Recursively searches the current directory for files that:
    - Do not contain "Copyright (c) Microsoft Corporation. All rights reserved."
    - OR contains a notice for another company

#>

[CmdletBinding()]
param()

# Pattern to match valid Microsoft notices (various formats)
# Examples: "Copyright Microsoft Corporation", "Copyright (c) 2022, Microsoft Corporation. All rights reserved."
$MicrosoftNoticePattern = "Copyright\s+(\(c\)\s*)?(\d{4},?\s*)?Microsoft Corporation"

# Notice to insert when adding a new notice
$MicrosoftNotice = "Copyright (c) Microsoft Corporation. All rights reserved."

# Pattern to detect other company notices - matches "Copyright" followed by a company name that is NOT Microsoft Corporation
$OtherNoticePattern = "Copyright\s+(\(c\)\s*)?(\d{4},?\s*)?(?!Microsoft)[A-Z][\w]*(?:\s+[A-Z][\w]*)*"

# File extensions to scan
$SupportedExtensions = @(
    '.c', '.cpp', '.h', '.hpp', '.cc', '.cxx', '.hxx', '.inl',  # C/C++
    '.rs',                                                       # Rust
    '.md',                                                       # Markdown
    '.py', '.pyw', '.pyx', '.pxd', '.pxi',                      # Python
    '.ps1', '.psm1',                                             # PowerShell
    '.sh', '.bash',                                              # Shell
    '.yml', '.yaml',                                             # YAML
    '.toml',                                                     # TOML
    '.json',                                                     # JSON
    '.cs',                                                       # C#
    '.js', '.ts',                                                # JavaScript/TypeScript
    '.cmake',                                                    # CMake
    '.S', '.asm',                                                # Assembly
    '.xml', '.xsd', '.xsl'                                       # XML
)
$SupportedFilenames = @('CMakeLists.txt', 'Dockerfile', 'Makefile', '.gitignore', '.gitattributes', 'nuget.config')

function Test-FileHasProperNotice {
    param(
        [string]$FilePath
    )

    $content = Get-Content -Path $FilePath -Raw -ErrorAction SilentlyContinue
    if ($null -eq $content) {
        return $false
    }

    # Check for Microsoft notice (accepts various valid formats)
    $hasMicrosoftNotice = $content -match $MicrosoftNoticePattern

    # Check for other company notices
    $hasOtherNotice = $content -match $OtherNoticePattern

    # File is proper if it has Microsoft notice AND no other notices
    return $hasMicrosoftNotice -and (-not $hasOtherNotice)
}

function Get-FilesToScan {
    $files = @()

    # Get files by extension
    foreach ($ext in $SupportedExtensions) {
        $files += Get-ChildItem -Path . -Recurse -File -Filter "*$ext" -ErrorAction SilentlyContinue
    }

    # Get files by name
    foreach ($filename in $SupportedFilenames) {
        $files += Get-ChildItem -Path . -Recurse -File -Filter $filename -ErrorAction SilentlyContinue
    }

    return $files | Sort-Object -Property FullName -Unique
}

# Main script
$filesToScan = Get-FilesToScan

foreach ($file in $filesToScan) {
    if (-not (Test-FileHasProperNotice -FilePath $file.FullName)) {
        # Output the full path for pipeline consumption
        $file.FullName
    }
}
