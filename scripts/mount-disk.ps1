# mount-disk.ps1
# Mounts a VHD specified in disk_path.txt
# Requires administrative privileges
#Requires -RunAsAdministrator

# Get script directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Read VHD path from disk_path.txt
$diskPathFile = Join-Path $scriptDir "disk_path.txt"
if (-not (Test-Path $diskPathFile)) {
    Write-Error "disk_path.txt not found in $scriptDir"
    exit 1
}
$vhdPath = Get-Content $diskPathFile -Raw | ForEach-Object { $_.Trim() }
if (-not $vhdPath -or -not (Test-Path $vhdPath)) {
    Write-Error "VHD path '$vhdPath' from disk_path.txt is invalid or does not exist"
    exit 1
}

# Preferred drive letter
$preferredDriveLetter = "I"

# Ensure temp directory exists
$tempDir = "C:\Temp"
if (-not (Test-Path $tempDir)) {
    New-Item -ItemType Directory -Path $tempDir | Out-Null
}

# Check if VHD is already attached
$diskpartCheckScript = @"
select vdisk file="$vhdPath"
detail vdisk
"@
$diskpartCheckPath = "$tempDir\diskpart_check.txt"
$diskpartCheckScript | Out-File -FilePath $diskpartCheckPath -Encoding ASCII
$diskpartOutput = diskpart /s $diskpartCheckPath 2>&1
$diskpartOutput | Out-File -FilePath "$tempDir\diskpart_check.log" -Encoding ASCII
Remove-Item $diskpartCheckPath -ErrorAction SilentlyContinue

if ($diskpartOutput -match "is attached") {
    Write-Warning "VHD is already attached. Check drive letter in Disk Management."
    exit 1
}

# Create diskpart script to attach VHD
$diskpartScript = @"
select vdisk file="$vhdPath"
attach vdisk
"@
$diskpartScriptPath = "$tempDir\diskpart_mount.txt"
$diskpartScript | Out-File -FilePath $diskpartScriptPath -Encoding ASCII

# Run diskpart to attach VHD
Write-Host "Attaching VHD at $vhdPath..."
$diskpartOutput = diskpart /s $diskpartScriptPath 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Error "Diskpart failed to attach VHD. Output: $diskpartOutput"
    exit 1
}
$diskpartOutput | Out-File -FilePath "$tempDir\diskpart_mount.log" -Encoding ASCII
Remove-Item $diskpartScriptPath -ErrorAction SilentlyContinue

# Brief delay to ensure volume is recognized
Start-Sleep -Milliseconds 1000

# Check if preferred drive letter is assigned
if (-not (Test-Path "$preferredDriveLetter`:\")) {
    Write-Warning "Drive $preferredDriveLetter`: not found. Assigning a drive letter..."
    $diskpartListScript = @"
select vdisk file="$vhdPath"
list volume
"@
    $diskpartListPath = "$tempDir\diskpart_list.txt"
    $diskpartListScript | Out-File -FilePath $diskpartListPath -Encoding ASCII
    $diskpartOutput = diskpart /s $diskpartListPath 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Diskpart failed to list volumes. Output: $diskpartOutput"
        exit 1
    }
    $diskpartOutput | Out-File -FilePath "$tempDir\diskpart_list.log" -Encoding ASCII
    Remove-Item $diskpartListPath -ErrorAction SilentlyContinue

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
        Write-Error "No OSDISK volume found. Check log: $tempDir\diskpart_list.log"
        exit 1
    }

    # Assign preferred or free drive letter
    $availableLetters = if (Test-Path "$preferredDriveLetter`:\") {
        [char[]](67..90) | Where-Object { -not (Test-Path "$_`:") } # C-Z
    } else {
        @($preferredDriveLetter)
    }
    if ($availableLetters) {
        $driveLetter = $availableLetters[0]
        $assignScript = @"
select vdisk file="$vhdPath"
select volume $volumeNumber
assign letter=$driveLetter
"@
        $assignScriptPath = "$tempDir\diskpart_assign.txt"
        $assignScript | Out-File -FilePath $assignScriptPath -Encoding ASCII
        $diskpartOutput = diskpart /s $assignScriptPath 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Diskpart failed to assign drive letter. Output: $diskpartOutput"
            exit 1
        }
        $diskpartOutput | Out-File -FilePath "$tempDir\diskpart_assign.log" -Encoding ASCII
        Remove-Item $assignScriptPath -ErrorAction SilentlyContinue
        Write-Host "Assigned drive letter $driveLetter`:"
    } else {
        Write-Error "No available drive letters found."
        exit 1
    }
} else {
    Write-Host "VHD mounted as $preferredDriveLetter`:"
}

Write-Host "VHD mounted successfully. Access at $driveLetter`:"