# CreateBootableVHD-NoHyperV.ps1
# Creates a 100MB VHD, initializes with MBR, creates a bootable FAT32 partition using diskpart
# Works on all Windows editions, no Hyper-V required

# Requires administrative privileges
#Requires -RunAsAdministrator

# Parameters
$vhdPath = "D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\tmp\os-disk.vhd"
$vhdSizeMB = 100

# Create diskpart script
$diskpartScript = @"
create vdisk file="$vhdPath" maximum=$vhdSizeMB type=fixed
select vdisk file="$vhdPath"
attach vdisk
create partition primary
active
format fs=fat32 label="OSDISK" quick
assign
detach vdisk
"@

# Save diskpart script to a temporary file
$diskpartScriptPath = "$env:TEMP\diskpart_vhd.txt"
$diskpartScript | Out-File -FilePath $diskpartScriptPath -Encoding ASCII

# Run diskpart with the script
Write-Host "Creating and formatting VHD at $vhdPath..."
diskpart /s $diskpartScriptPath

# Clean up
Remove-Item $diskpartScriptPath

Write-Host "VHD creation complete. VHD is at $vhdPath and ready for bootloader installation."
Write-Host "To copy files, mount the VHD manually using Disk Management or diskpart."