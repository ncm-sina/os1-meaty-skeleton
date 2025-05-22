# Define the sysroot path (modify this as needed)
$sysrootPath = "..\sysroot"

# Path to the mount-disk.ps1 script
$mountScript = ".\mount-disk.ps1"


# Call the mount-disk.ps1 script to mount the disk
Write-Host "Running mount-disk.ps1 to mount the disk..."
& $mountScript

# Brief delay to ensure volume is recognized
Start-Sleep -Milliseconds 2000


# Check if the disk is mounted at I:\
if (Test-Path "I:\") {
    Write-Host "Disk is mounted at I:\. Proceeding with cleanup and file copy..."

    # Remove all contents from I:\
    try {
        Write-Host "Removing all contents from I:\..."
        Get-ChildItem -Path "I:\" -Recurse -Force | Remove-Item -Recurse -Force -ErrorAction Stop
        Write-Host "Contents of I:\ removed successfully."
    }
    catch {
        Write-Error "Error removing contents from I:\: $_"
        exit 1
    }

    # Copy files from sysroot to I:\
    try {
        Write-Host "Copying files from $sysrootPath to I:\..."
        Copy-Item -Path "$sysrootPath\*" -Destination "I:\" -Recurse -Force -ErrorAction Stop
        Write-Host "Files copied successfully."
    }
    catch {
        Write-Error "Error copying files: $_"
        exit 1
    }

}
else {
    Write-Error "Disk not mounted at I:\. Please check mount-disk.ps1."
    exit 1
}