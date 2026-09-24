# fetch-ytdlp.ps1 — downloads the latest yt-dlp.exe into bin/
$ErrorActionPreference = "Stop"
$url  = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe"
$dest = Join-Path (Split-Path -Parent $PSScriptRoot) "bin\yt-dlp.exe"

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dest) | Out-Null
Write-Host "Downloading yt-dlp.exe from $url ..."
Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing
Write-Host "Saved to $dest ($([math]::Round((Get-Item $dest).Length / 1MB, 1)) MB)"
