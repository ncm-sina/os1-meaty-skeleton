# Define paths
$DiskImg = [System.IO.Path]::GetFullPath("D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\tmp\disk.img")
$Sysroot = [System.IO.Path]::GetFullPath("D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\sysroot")
$MountPoint = "D:"
$MSYS2Path = "C:\msys64\usr\bin"

# Ensure running as admin
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Error "Please run PowerShell as Administrator"
    exit 1
}

# Ensure MSYS2 tools are available
if (-not (Test-Path "$MSYS2Path\parted.exe") -or -not (Test-Path "$MSYS2Path\mkfs.fat.exe")) {
    Write-Error "MSYS2 tools (parted, dosfstools) not found in $MSYS2Path. Install with: pacman -S parted dosfstools"
    exit 1
}

# Ensure directories exist
$DiskDir = Split-Path $DiskImg -Parent
if (-not (Test-Path $DiskDir)) {
    New-Item -ItemType Directory -Path $DiskDir -Force
}
if (-not (Test-Path $Sysroot)) {
    Write-Error "Sysroot directory $Sysroot does not exist"
    exit 1
}

# Clean up existing disk image
if (Test-Path $DiskImg) {
    Remove-Item $DiskImg
}

# Create disk image (200MB)
Write-Host "Creating disk image: $DiskImg"
fsutil file createnew $DiskImg 209715200
if (-not (Test-Path $DiskImg)) {
    Write-Error "Failed to create $DiskImg"
    exit 1
}

# Partition with parted
Write-Host "Partitioning: $DiskImg"
& "$MSYS2Path\parted.exe" -s $DiskImg mklabel msdos mkpart primary fat32 1MiB 100% set 1 boot on
if ($LASTEXITCODE -ne 0) {
    Write-Error "Parted failed"
    exit 1
}

# Format with mkfs.fat
Write-Host "Formatting FAT32: $DiskImg"
& "$MSYS2Path\mkfs.fat.exe" -F 32 -o $((2048*512)) $DiskImg
if ($LASTEXITCODE -ne 0) {
    Write-Error "mkfs.fat failed"
    exit 1
}

# Manual step: Mount with ImDisk
Write-Host "Please mount $DiskImg with ImDisk (run as admin):"
Write-Host "- File: $DiskImg"
Write-Host "- Offset: 1048576 bytes (1MiB)"
Write-Host "- Drive letter: $MountPoint"
Write-Host "- Check 'Mount as removable media'"
Write-Host "Press Enter after mounting..."
Read-Host

# Copy sysroot
Write-Host "Copying sysroot to $MountPoint"
if (Test-Path $MountPoint) {
    xcopy $Sysroot\* $MountPoint\ /E /H /C /I
} else {
    Write-Error "Mount point $MountPoint not found"
    exit 1
}

# Create GRUB config on mounted drive
Write-Host "Creating GRUB config"
$grubCfg = @"
set timeout=5
set default=0

menuentry "MyOS" {
    multiboot /boot/myos.kernel
    boot
}
"@
$grubDir = "$MountPoint\boot\grub"
if (-not (Test-Path $grubDir)) {
    New-Item -ItemType Directory -Path $grubDir -Force
}
$grubCfg | Out-File -FilePath "$grubDir\grub.cfg" -Encoding ASCII

# Manual step: Unmount
Write-Host "Please unmount $MountPoint in ImDisk"
Write-Host "Press Enter after unmounting..."
Read-Host

# Manual step: GRUB installation with MSYS2 and qemu-nbd
Write-Host "Run the following in MSYS2 (install grub: pacman -S grub):"
Write-Host "1. Connect disk.img with qemu-nbd (run as admin in PowerShell):"
Write-Host "   cd 'C:\Program Files\qemu'"
Write-Host "   .\qemu-nbd.exe -c \\.\PhysicalDrive1 '$DiskImg'"
Write-Host "2. In MSYS2 terminal:"
Write-Host "   mkdir -p /mnt/osdisk"
Write-Host "   mount -t vfat /dev/sdb1 /mnt/osdisk"
Write-Host "   grub-install --boot-directory=/mnt/osdisk/boot --root-directory=/mnt/osdisk --modules=\"part_msdos fat\" --target=i386-pc /dev/sdb"
Write-Host "   umount /mnt/osdisk"
Write-Host "3. Disconnect qemu-nbd (in PowerShell):"
Write-Host "   .\qemu-nbd.exe -d \\.\PhysicalDrive1"
Write-Host "Press Enter after completing GRUB installation..."
Read-Host

Write-Host "Disk image created at $DiskImg"
Write-Host "Test with QEMU:"
Write-Host "cd 'C:\Program Files\qemu'"
Write-Host ".\qemu-system-i386.exe -hda '$DiskImg' -s -S"