# Define paths
$DiskImg = [System.IO.Path]::GetFullPath("D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\tmp\disk.img")
$Sysroot = [System.IO.Path]::GetFullPath("D:\Projects\coding\system_design\osdev\os1-meaty-skeleton\sysroot")
$MountPoint = "I:"

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
fsutil file createnew $DiskImg 209715200
if (-not (Test-Path $DiskImg)) {
    Write-Error "Failed to create $DiskImg"
    exit 1
}

# Partition and format with DiskPart
$diskpartScript = @"
select vdisk file="$($DiskImg.Replace('\', '\\'))"
attach vdisk
create partition primary
select partition 1
format fs=fat32 quick
active
detach vdisk
exit
"@
$diskpartScript | Out-File -FilePath "$env:TEMP\diskpart.txt" -Encoding ASCII
diskpart /s "$env:TEMP\diskpart.txt"
Remove-Item "$env:TEMP\diskpart.txt"

# Manual step: Mount with ImDisk
Write-Host "Please mount $DiskImg with ImDisk (run as admin):"
Write-Host "- File: $DiskImg"
Write-Host "- Offset: 1048576 bytes (1MiB)"
Write-Host "- Drive letter: $MountPoint"
Write-Host "- Check 'Mount as removable media'"
Write-Host "Press Enter after mounting..."
Read-Host

# Copy sysroot
if (Test-Path $MountPoint) {
    xcopy $Sysroot\* $MountPoint\ /E /H /C /I
} else {
    Write-Error "Mount point $MountPoint not found"
    exit 1
}

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
Write-Host "   mkdir -p /mnt/osdisk/boot/grub"
Write-Host "   cat <<EOF > /mnt/osdisk/boot/grub/grub.cfg"
Write-Host "set timeout=5"
Write-Host "set default=0"
Write-Host ""
Write-Host "menuentry \"MyOS\" {"
Write-Host "    multiboot /boot/kernel.elf"
Write-Host "    boot"
Write-Host "}"
Write-Host "EOF"
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