# umount-disk.ps1
# Unmounts a VHD specified in disk_path.txt
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
$vhdPath = Get-Content $diskPathFile -Raw | ForEach-Object { $_.Value.Trim() }
if (-not $vhdPath -or -not (Test-Path $vhdPath)) {
    Write-Error "VHD path '$vhdPath' from disk_path.txt is invalid or does not exist"
    exit 1
}

# Ensure temp directory exists
$tempDir = "C:\Temp"
if (-not (Test-Path $tempDir)) {
    New-Item -ItemType Directory -Path $tempDir | Out-Null
}

# Check if VHD is attached
$diskpartCheckScript = @"
select vdisk file="$vhdPath"
detail vdisk
"@
$diskpartCheckPath = "$tempDir\diskpart_check.txt"
$diskpartCheckScript | Out-File -FilePath $diskpartCheckPath -Encoding ASCII
$diskpartOutput = diskpart /s $diskpartCheckPath 2>&1
$diskpartOutput | Out-File -FilePath "$tempDir\diskpart_check.log" -Encoding ASCII
Remove-Item $diskpartCheckPath -ErrorAction SilentlyContinue

if ($diskpartOutput -notmatch "is attached") {
    Write-Warning "VHD is not attached."
    exit 0
}

# Create diskpart script to detach VHD
$diskpartScript = @"
select vdisk file="$vhdPath"
detach vdisk
"@
$diskpartScriptPath = "$tempDir\diskpart_umount.txt"
$diskpartScript | Out-File -FilePath $diskpartScriptPath -Encoding ASCII

# Run diskpart to detach VHD
Write-Host "Detaching VHD at $vhdPath..."
$diskpartOutput = diskpart /s $diskpartScriptPath 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Error "Diskpart failed to detach VHD. Output: $diskpartOutput"
    exit 1
}
$diskpartOutput | Out-File -FilePath "$tempDir\diskpart_umount.log" -Encoding ASCII
Remove-Item $diskpartScriptPath -ErrorAction SilentlyContinue

Write-Host "VHD detached successfully."