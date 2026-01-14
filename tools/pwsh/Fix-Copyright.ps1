# Copyright (c) Microsoft Corporation. All rights reserved.

<#
.SYNOPSIS
    Fixes missing Microsoft notice in files.

.DESCRIPTION
    Takes file paths as input (via pipeline or parameter) and:
    - Does nothing if the file already has a proper Microsoft notice
    - Adds a Microsoft notice header if no notice exists
    - Returns an error if another company's notice is found

.PARAMETER Path
    The path to the file to fix. Accepts pipeline input.

#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, ValueFromPipeline = $true, ValueFromPipelineByPropertyName = $true)]
    [Alias('FullName')]
    [string]$Path
)

begin {
    # Pattern to match valid Microsoft notices (various formats)
    # Examples: "Copyright Microsoft Corporation", "Copyright (c) 2022, Microsoft Corporation. All rights reserved."
    $MicrosoftNoticePattern = "Copyright\s+(\(c\)\s*)?(\d{4},?\s*)?Microsoft Corporation"

    # Notice to insert when adding a new notice
    $MicrosoftNotice = "Copyright (c) Microsoft Corporation. All rights reserved."

    # Pattern to detect other company notices - matches "Copyright" followed by a company name that is NOT Microsoft Corporation
    $OtherNoticePattern = "Copyright\s+(\(c\)\s*)?(\d{4},?\s*)?(?!Microsoft)[A-Z][\w]*(?:\s+[A-Z][\w]*)*"

    function Get-NoticeHeader {
        param(
            [string]$FilePath
        )

        $fileName = [System.IO.Path]::GetFileName($FilePath)
        $extension = [System.IO.Path]::GetExtension($FilePath).ToLower()

        # Check for specific filenames first
        switch ($fileName) {
            # Hash comments
            { $_ -in @('CMakeLists.txt', 'Dockerfile', 'Makefile', '.gitignore', '.gitattributes') } {
                return "# $MicrosoftNotice`n`n"
            }
            # XML comments
            'nuget.config' {
                return "<!-- $MicrosoftNotice -->`n`n"
            }
        }

        # Check by extension
        switch ($extension) {
            # C-style comments (// ...)
            { $_ -in @('.c', '.cpp', '.h', '.hpp', '.cc', '.cxx', '.hxx', '.inl', '.rs', '.cs', '.js', '.ts') } {
                return "//`n// $MicrosoftNotice`n//`n"
            }
            # Hash comments (# ...)
            { $_ -in @('.py', '.pyw', '.pyx', '.pxd', '.pxi', '.ps1', '.psm1', '.sh', '.bash', '.yml', '.yaml', '.toml', '.cmake') } {
                return "# $MicrosoftNotice`n`n"
            }
            # XML/HTML comments (<!-- ... -->)
            { $_ -in @('.md', '.xml', '.xsd', '.xsl') } {
                return "<!-- $MicrosoftNotice -->`n`n"
            }
            # Assembly comments (; ... or // ...)
            { $_ -in @('.S', '.asm') } {
                return "//`n// $MicrosoftNotice`n//`n"
            }
            # JSON - no standard comment syntax, skip
            '.json' {
                return $null
            }
            default {
                return $null
            }
        }
    }

    function Get-FileEncoding {
        param(
            [string]$FilePath
        )

        # Read the BOM to detect encoding
        $bytes = [System.IO.File]::ReadAllBytes($FilePath)

        if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
            return [System.Text.Encoding]::UTF8
        }
        elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
            return [System.Text.Encoding]::Unicode
        }
        elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF) {
            return [System.Text.Encoding]::BigEndianUnicode
        }
        else {
            # Default to UTF8 without BOM
            return [System.Text.UTF8Encoding]::new($false)
        }
    }

    function Get-LineEnding {
        param(
            [string]$Content
        )

        if ($Content -match "`r`n") {
            return "`r`n"
        }
        elseif ($Content -match "`n") {
            return "`n"
        }
        elseif ($Content -match "`r") {
            return "`r"
        }
        else {
            # Default to Windows line endings
            return "`r`n"
        }
    }
}

process {
    # Resolve the path in case it's relative
    $resolvedPath = $Path
    if (-not [System.IO.Path]::IsPathRooted($Path)) {
        $resolvedPath = Join-Path -Path (Get-Location) -ChildPath $Path
    }

    if (-not (Test-Path -Path $resolvedPath -PathType Leaf)) {
        Write-Error "File not found: $resolvedPath"
        return
    }

    # Read the file content
    $content = Get-Content -Path $resolvedPath -Raw -ErrorAction Stop
    if ($null -eq $content) {
        $content = ""
    }

    # Check if file already has proper Microsoft notice (accepts various valid formats)
    $hasMicrosoftNotice = $content -match $MicrosoftNoticePattern

    # Check for other company notices
    $hasOtherNotice = $content -match $OtherNoticePattern

    # If file has other company notice, return error
    if ($hasOtherNotice) {
        Write-Error "File contains another company's notice and cannot be automatically fixed: $resolvedPath"
        return
    }

    # If file already has Microsoft notice, do nothing
    if ($hasMicrosoftNotice) {
        Write-Verbose "File already has proper notice: $resolvedPath"
        return
    }

    # Get the appropriate notice header for this file type
    $header = Get-NoticeHeader -FilePath $resolvedPath

    if ($null -eq $header) {
        Write-Error "Unrecognized file type, cannot add notice: $resolvedPath"
        return
    }

    # Detect original encoding and line ending
    $encoding = Get-FileEncoding -FilePath $resolvedPath
    $lineEnding = Get-LineEnding -Content $content

    # Normalize the header to use the same line ending as the file
    $header = $header -replace "`r`n|`n|`r", $lineEnding

    # For XML files, insert after the <?xml declaration if present
    if ($content -match "^(\s*<\?xml[^?]*\?>\s*)") {
        $xmlDeclaration = $Matches[1]
        $restOfContent = $content.Substring($xmlDeclaration.Length)
        $newContent = $xmlDeclaration + $header + $restOfContent
    }
    else {
        # Prepend the notice header
        $newContent = $header + $content
    }

    # Write the file back with original encoding, avoiding extra line ending changes
    [System.IO.File]::WriteAllText($resolvedPath, $newContent, $encoding)

    Write-Host "Added notice header to: $resolvedPath"
}

end {
}
