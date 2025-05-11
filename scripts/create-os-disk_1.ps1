# CreateBootableVHD-NoHyperV.ps1
# Creates a 100MB VHD, formats as FAT32, copies /path/to/sysroot/*, installs GRUB, for Windows Home
# Requires administrative privileges
#Requires -RunAsAdministrator

# Parameters
$vhdPath = "D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\os-disk.vhd"
$vhdSizeMB = 100
$sysrootPath = "D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\sysroot"
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
$diskpartScriptPath = "$env:TEMP\diskpart_vhd.txt"
$diskpartScript | Out-File -FilePath $diskpartScriptPath -Encoding ASCII

# Run diskpart to create VHD
Write-Host "Creating and formatting VHD at $vhdPath..."
diskpart /s $diskpartScriptPath

# Brief delay to ensure volume is recognized
Start-Sleep -Milliseconds 1000

# Verify drive letter or assign a free one
$driveLetter = $preferredDriveLetter
if (-not (Test-Path "$driveLetter`:\")) {
    Write-Warning "Drive $driveLetter`: not found. Attempting to assign a free drive letter..."
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
    $volumeNumber = $null
    foreach ($line in $diskpartOutput) {
        $line = $line.Trim()
        if ($line -match "OSDISK" -or $line -match "osdisk") {
            if ($line -match "Volume\s+(\d+)\s+\w*\s+OSDISK") {
                $volumeNumber = $matches[1]
                Write-Host "Found OSDISK volume: Volume $volumeNumber"
                break
            }
        }
    }

    if (-not $volumeNumber) {
        Write-Error "No OSDISK volume found. Check diskpart output above."
        exit 1
    }

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

# Install GRUB via WSL2 (if available)
Write-Host "Checking for WSL2 to install GRUB..."
$wslAvailable = wsl --status 2>$null
if ($?) {
    Write-Host "WSL2 detected. Attempting to install GRUB..."
    $wslScript = @"
sudo apt update
sudo apt install -y grub-pc parted
sudo mkdir -p /mnt/vhd
sudo losetup -P /dev/loop0 /mnt/c$(($vhdPath -replace '\\', '/') -replace ':', '')
sudo mount /dev/loop0p1 /mnt/vhd
sudo grub-install --target=i386-pc --root-directory=/mnt/vhd --no-floppy /dev/loop0
ls /mnt/vhd/boot/grub
sudo umount /mnt/vhd
sudo losetup -d /dev/loop0
"@
    $wslScriptPath = "$env:TEMP\wsl_grub.sh"
    $wslScript | Out-File -FilePath $wslScriptPath -Encoding ASCII
    Write-Host "Running GRUB installation in WSL2..."
    wsl bash $wslScriptPath
    if ($LASTEXITCODE -eq 0) {
        Write-Host "GRUB installed successfully."
    } else {
        Write-Warning "GRUB installation failed. Please install GRUB manually using WSL2 or a Linux environment."
    }
    Remove-Item $wslScriptPath -ErrorAction SilentlyContinue
} else {
    Write-Warning "WSL2 not detected. You must install GRUB manually using WSL2 or a Linux environment."
}

# Detach VHD
$detachScript = @"
select vdisk file="$vhdPath"
detach vdisk
"@
$detachScriptPath = "$env:TEMP\diskpart_detach.txt"
$detachScript | Out-File -FilePath $detachScriptPath -Encoding ASCII
Write-Host "Detaching VHD..."
diskpart /s $detachScriptPath

# Clean up
Remove-Item $diskpartScriptPath -ErrorAction SilentlyContinue
Remove-Item $detachScriptPath -ErrorAction SilentlyContinue

Write-Host "VHD created at $vhdPath with sysroot contents and GRUB (if WSL2 was available)."
Write-Host "To boot, run: qemu-system-i386 -hda $vhdPath -m 512"