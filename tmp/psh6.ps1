# CreateBootableVHD-NoHyperV.ps1
# Creates a 100MB VHD, formats as FAT32, copies /path/to/sysroot/* to root, for Windows Home
# Requires administrative privileges
#Requires -RunAsAdministrator

# Parameters
$vhdPath = "C:\Users\YourName\os-disk.vhd"  # Update to your desired VHD path
$vhdSizeMB = 100
$sysrootPath = "C:\Path\To\sysroot"  # Update to your sysroot directory

# Ensure paths exist
$directory = Split-Path $vhdPath -Parent
if (-not (Test-Path $directory)) {
    New-Item -ItemType Directory -Path $directory | Out-Null
}
if (-not (Test-Path $sysrootPath)) {
    Write-Error "Sysroot path $sysrootPath does not exist. Please update `$sysrootPath`."
    exit 1
}

# Create diskpart script to create and format VHD
$diskpartScript = @"
create vdisk file="$vhdPath" maximum=$vhdSizeMB type=fixed
select vdisk file="$vhdPath"
attach vdisk
create partition primary
active
format fs=fat32 label="OSDISK" quick
assign letter=X
"@

# Save diskpart script
$diskpartScriptPath = "$env:TEMP\diskpart_vhd.txt"
$diskpartScript | Out-File -FilePath $diskpartScriptPath -Encoding ASCII

# Run diskpart to create VHD
Write-Host "Creating and formatting VHD at $vhdPath..."
diskpart /s $diskpartScriptPath

# Copy sysroot contents to VHD (X:\)
Write-Host "Copying contents of $sysrootPath to VHD (X:\)..."
Copy-Item -Path "$sysrootPath\*" -Destination "X:\" -Recurse -Force

# Verify copied files
Write-Host "Verifying copied files..."
Get-ChildItem -Path "X:\"

# Create diskpart script to detach VHD
$detachScript = @"
select vdisk file="$vhdPath"
detach vdisk
"@

# Save and run detach script
$detachScriptPath = "$env:TEMP\diskpart_detach.txt"
$detachScript | Out-File -FilePath $detachScriptPath -Encoding ASCII
Write-Host "Detaching VHD..."
diskpart /s $detachScriptPath

# Clean up
Remove-Item $diskpartScriptPath
Remove-Item $detachScriptPath

Write-Host "VHD created at $vhdPath with sysroot contents copied."
Write-Host "Next step: Install GRUB using WSL2 or a Linux environment."