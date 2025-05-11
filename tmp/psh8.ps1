# CreateBootableVHD-NoHyperV.ps1
# Creates a 100MB VHD, formats as FAT32, copies /path/to/sysroot/* to root, for Windows Home
# Requires administrative privileges
#Requires -RunAsAdministrator

# Parameters
$vhdPath = "D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\tmp\os-disk.vhd"  # Update to your desired VHD path
$vhdSizeMB = 100
$sysrootPath = "D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\sysroot"  # Update to your sysroot directory
$preferredDriveLetter = "I"

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
assign letter=$preferredDriveLetter
"@

# Save diskpart script
$diskpartScriptPath = "$env:TEMP\diskpart_vhd.txt"
$diskpartScript | Out-File -FilePath $diskpartScriptPath -Encoding ASCII

# Run diskpart to create VHD
Write-Host "Creating and formatting VHD at $vhdPath..."
diskpart /s $diskpartScriptPath

# Verify drive letter or assign a free one
$driveLetter = $preferredDriveLetter
if (-not (Test-Path "$driveLetter`:\")) {
    Write-Warning "Drive $driveLetter`: not found. Attempting to assign a free drive letter..."
    # Run diskpart to list volumes and find the VHD
    $diskpartListScript = @"
select vdisk file="$vhdPath"
attach vdisk
list volume
"@

    $diskpartListPath = "$env:TEMP\diskpart_list.txt"
    $diskpartListScript | Out-File -FilePath $diskpartListPath -Encoding ASCII
    $diskpartOutput = diskpart /s $diskpartListPath
    Remove-Item $diskpartListPath -ErrorAction SilentlyContinue

    # Debug: Output raw diskpart result
    Write-Host "Diskpart output:"
    Write-Host ($diskpartOutput -join "`n")

    # Parse diskpart output to find the VHD volume
    $volumeLine = $diskpartOutput | Where-Object { $_ -like "*OSDISK*" }
    if ($volumeLine -and $volumeLine -match "Volume\s+(\d+)\s+\w*\s+OSDISK") {
        $volumeNumber = $matches[1]
        Write-Host "Found OSDISK volume: Volume $volumeNumber"
        # Assign a free drive letter
        $availableLetters = [char[]](67..90) | Where-Object { -not (Test-Path "$_`:") } # C-Z
        if ($availableLetters) {
            $driveLetter = $availableLetters[0]
            $assignScript = @"
select vdisk file="$vhdPath"
select volume $volumeNumber
assign letter=$driveLetter
"@

            $assignScriptPath = "$env:TEMP\diskpart_assign.txt"
            $assignScript | Out-File -FilePath $assignScriptPath -Encoding ASCII
            diskpart /s $assignScriptPath
            Remove-Item $assignScriptPath -ErrorAction SilentlyContinue
            Write-Host "Assigned drive letter $driveLetter`:"
        } else {
            Write-Error "No available drive letters found."
            exit 1
        }
    } else {
        Write-Error "Could not find VHD volume labeled OSDISK. Check diskpart output above."
        exit 1
    }
}

# Verify drive exists
if (-not (Test-Path "$driveLetter`:\")) {
    Write-Error "Drive $driveLetter`: still not accessible. Check diskpart logs or permissions."
    exit 1
}

# Copy sysroot contents to VHD
Write-Host "Copying contents of $sysrootPath to VHD ($driveLetter`:)..."
Copy-Item -Path "$sysrootPath\*" -Destination "$driveLetter`:\" -Recurse -Force

# Verify copied files
Write-Host "Verifying copied files..."
Get-ChildItem -Path "$driveLetter`:\"

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
Remove-Item $diskpartScriptPath -ErrorAction SilentlyContinue
Remove-Item $detachScriptPath -ErrorAction SilentlyContinue

Write-Host "VHD created at $vhdPath with sysroot contents copied."
Write-Host "Next step: Install GRUB using WSL2 or a Linux environment."