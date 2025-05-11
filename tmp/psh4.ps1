# CreateBootableVHD.ps1
# Creates a 100MB VHD, initializes with MBR, creates a bootable FAT32 partition

# Requires administrative privileges
#Requires -RunAsAdministrator

# Parameters
$vhdPath = "C:\Path\To\os-disk.vhd"
$vhdSizeMB = 100

# Create VHD
$vhdSizeBytes = $vhdSizeMB * 1024 * 1024
New-VHD -Path $vhdPath -SizeBytes $vhdSizeBytes -Fixed | Out-Null
Write-Host "Created VHD at $vhdPath"

# Mount VHD
$disk = Mount-VHD -Path $vhdPath -Passthru
$diskNumber = $disk.DiskNumber
Write-Host "Mounted VHD as Disk $diskNumber"

# Initialize disk with MBR
Initialize-Disk -Number $diskNumber -PartitionStyle MBR -Confirm:$false
Write-Host "Initialized disk with MBR"

# Create a single partition, make it active (bootable)
$partition = New-Partition -DiskNumber $diskNumber -UseMaximumSize -IsActive:$true
Write-Host "Created bootable partition"

# Format partition as FAT32
Format-Volume -Partition $partition -FileSystem FAT32 -NewFileSystemLabel "OSDISK" -Confirm:$false
Write-Host "Formatted partition as FAT32"

# Assign a drive letter
$driveLetter = Add-PartitionAccessPath -DiskNumber $diskNumber -PartitionNumber $partition.PartitionNumber -AssignDriveLetter
$driveLetter = (Get-Partition -DiskNumber $diskNumber -PartitionNumber $partition.PartitionNumber).DriveLetter
Write-Host "Assigned drive letter $driveLetter"

# Dismount VHD
Dismount-VHD -Path $vhdPath
Write-Host "Dismounted VHD"

Write-Host "VHD creation complete. VHD is at $vhdPath and ready for bootloader installation."