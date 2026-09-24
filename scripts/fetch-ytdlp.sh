#!/usr/bin/env bash
# fetch-ytdlp.sh — downloads the latest yt-dlp.exe into bin/
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
dest="$here/../bin/yt-dlp.exe"
mkdir -p "$(dirname "$dest")"
echo "Downloading yt-dlp.exe ..."
curl -L --fail -o "$dest" "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe"
echo "Saved to $dest ($(du -h "$dest" | cut -f1))"
