param (
    [string]$hsp_ifwi_image = "kingsgate_ifwi.flash",
    [string]$scp_img = "scp_boot_loader_embed_fw.bin",
    [string]$output_file = "kingsgate_ifwi_scp.flash",
    [int]$scp_image_address = 95000
)

# Binary paths
$hsp_ifwi_image_path = "${env:REPO_APP_PATH_kingsgate.sprt.release}/release/sp1/$hsp_ifwi_image"
$scp_img_path = "$env:REPO_APP_BUILD_DIR/$env:REPO_APP_BUILD_CONFIG/arm-eabi-aarch/bin/scp/$scp_img"
$scp_img_addr = $scp_image_address

# Get total size of hsp_ifwi_image
$total_size = (Get-Item $hsp_ifwi_image_path).length
Write-Output "total_size: $total_size"

$total_zeros = $scp_img_addr - $total_size
if ($total_zeros -lt 0) {
    Write-Output "hsp image is overlapping with scp image address"
    return
}

# Ensure output file is in the root directory
$root_directory = [System.IO.Path]::GetPathRoot((Get-Location).Path)
$output_file_path = join-path "$root_directory" "$output_file"

# Concatenate sp1_img, fw_key_manifest, and sprt_img
Copy-Item -Path $hsp_ifwi_image_path -Destination $output_file_path

Write-Output "Adding $total_zeros zeros to $output_file_path"
# Add total_zeros number of zeros
[byte[]]$zeros = @(0) * $total_zeros
[System.IO.File]::WriteAllBytes($output_file_path, [System.IO.File]::ReadAllBytes($output_file_path) + $zeros)

# Append scp_img
Write-Output "Appending $scp_img to $output_file_path"
[System.IO.File]::WriteAllBytes($output_file_path, [System.IO.File]::ReadAllBytes($output_file_path) + [System.IO.File]::ReadAllBytes($scp_img_path))
Write-Output "Done creating $output_file_path"
