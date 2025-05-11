# Define the sysroot path (modify this as needed)
$sysrootPath = "..\sysroot"

# Path to the mount-disk.ps1 script
$mountScript = ".\mount-disk.ps1"

# Function to unmount the disk from I:\
function Unmount-Disk {
    param (
        [string]$DriveLetter = "I"
    )
    try {
        Write-Host "Unmounting disk from ${DriveLetter}:\..."

        # Get script directory and read VHD path from disk_path.txt
        $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
        Write-Host "Script directory: $scriptDir"
        $diskPathFile = Join-Path $scriptDir "disk_path.txt"
        Write-Host "Looking for disk_path.txt at: $diskPathFile"

        if (-not (Test-Path $diskPathFile)) {
            Write-Warning "disk_path.txt not found at $diskPathFile. Attempting to remove drive letter only."
            $vhdPath = $null
        }
        else {
            $vhdPath = Get-Content $diskPathFile -Raw -ErrorAction Stop | ForEach-Object { $_.Trim() }
            if (-not $vhdPath) {
                Write-Warning "disk_path.txt is empty. Attempting to remove drive letter only."
            }
            elseif (-not (Test-Path $vhdPath)) {
                Write-Warning "VHD path '$vhdPath' from disk_path.txt does not exist. Attempting to remove drive letter only."
                $vhdPath = $null
            }
            else {
                Write-Host "VHD path: $vhdPath"
            }
        }

        # Ensure temp directory exists
        $tempDir = "C:\Temp"
        if (-not (Test-Path $tempDir)) {
            New-Item -ItemType Directory -Path $tempDir | Out-Null
        }

        # If VHD path is valid, detach the VHD
        if ($vhdPath) {
            $diskpartScript = @"
select vdisk file="$vhdPath"
detach vdisk
"@
            $diskpartScriptPath = "$tempDir\diskpart_unmount.txt"
            $diskpartScript | Out-File -FilePath $diskpartScriptPath -Encoding ASCII

            # Run diskpart to detach VHD
            Write-Host "Running diskpart to detach VHD..."
            $diskpartOutput = diskpart /s $diskpartScriptPath 2>&1
            $diskpartOutput | Out-File -FilePath "$tempDir\diskpart_unmount.log" -Encoding ASCII
            Remove-Item $diskpartScriptPath -ErrorAction SilentlyContinue

            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Diskpart failed to detach VHD. Output: $diskpartOutput"
            }
            else {
                Write-Host "VHD detached successfully."
            }
        }

        # Verify the drive is no longer accessible, and remove drive letter if needed
        if (Test-Path "${DriveLetter}:\") {
            Write-Host "Drive ${DriveLetter}:\ still accessible. Attempting to remove drive letter..."
            $diskpartRemoveScript = @"
select volume ${DriveLetter}
remove letter=${DriveLetter}
"@
            $diskpartRemovePath = "$tempDir\diskpart_remove_letter.txt"
            $diskpartRemoveScript | Out-File -FilePath $diskpartRemovePath -Encoding ASCII
            $diskpartRemoveOutput = diskpart /s $diskpartRemovePath 2>&1
            $diskpartRemoveOutput | Out-File -FilePath "$tempDir\diskpart_remove_letter.log" -Encoding ASCII
            Remove-Item $diskpartRemovePath -ErrorAction SilentlyContinue
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Diskpart failed to remove drive letter. Output: $diskpartRemoveOutput"
                exit 1
            }
            Write-Host "Drive letter ${DriveLetter} removed successfully."
        }

        Write-Host "Disk unmounted successfully from ${DriveLetter}:\."
    }
    catch {
        Write-Error "Error unmounting disk from ${DriveLetter}:\: $_"
        exit 1
    }
}

# Call the mount-disk.ps1 script to mount the disk
Write-Host "Running mount-disk.ps1 to mount the disk..."
& $mountScript

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

    # Unmount the disk
    # Unmount-Disk -DriveLetter "I"
}
else {
    Write-Error "Disk not mounted at I:\. Please check mount-disk.ps1."
    exit 1
}