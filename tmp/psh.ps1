$DiskImg = "d:/Projects\coding\system_design\osdev\os1-meaty-skeleton\tmp\disk.img"
$Sysroot = "d:/Projects\coding\system_design\osdev\os1-meaty-skeleton\tmp\sysroot"
$MountPoint = "D:"

# Create disk image
fsutil file createnew $DiskImg 209715200

# Partition and format
$diskpartScript = @"
select vdisk file="$DiskImg"
attach vdisk
create partition primary
select partition 1
format fs=fat32 quick
active
detach vdisk
exit
"@
$diskpartScript | diskpart

# Mount with ImDisk (manual step, run ImDisk GUI as admin)
Write-Host "Mount $DiskImg with ImDisk, offset 1048576, drive $MountPoint"

# Copy sysroot
xcopy $Sysroot\* $MountPoint\ /E /H /C /I

# Unmount (manual step)
Write-Host "Unmount $MountPoint in ImDisk"

# GRUB installation requires MSYS2 and qemu-nbd (manual step)
Write-Host "Run MSYS2 and qemu-nbd to install GRUB (see instructions)"

Write-Host "Disk image created at $DiskImg"