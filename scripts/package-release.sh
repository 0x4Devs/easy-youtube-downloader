#!/usr/bin/env bash
# package-release.sh — baut die Portable-Zip fuer ein Release.
# Erwartet: build/ ist frisch gebaut.
set -euo pipefail

version="${1:-v0.1.0}"
here="$(cd "$(dirname "$0")/.." && pwd)"
build="$here/build"
name="easy-youtube-downloader-$version-windows-x64"
staging="$here/dist/$name"

if [[ ! -f "$build/Easy Youtube Downloader.exe" ]]; then
    echo "Fehler: build/Easy Youtube Downloader.exe nicht gefunden. Erst 'build.bat' ausfuehren."
    exit 1
fi

echo "Staging -> $staging"
rm -rf "$here/dist"
mkdir -p "$staging"

# App + Qt-DLLs + MinGW-Runtime + yt-dlp
cp "$build/Easy Youtube Downloader.exe"        "$staging/"
cp "$build/ytdl.exe"                            "$staging/"
cp "$build/yt-dlp.exe"                          "$staging/"
cp "$build/"*.dll                               "$staging/"

# Qt-Plugin-Ordner (platforms/, styles/, imageformats/, ...)
for d in "$build"/*/; do
    dn="$(basename "$d")"
    # nur echte Plugin-Ordner (nicht CMakeFiles etc.)
    case "$dn" in
        CMakeFiles|_deps|easy_youtube_downloader_autogen) continue ;;
    esac
    # Ordner mit .dll drin sind Qt-Plugins.
    if compgen -G "$d"*.dll > /dev/null; then
        cp -r "$d" "$staging/"
    fi
done

# ffmpeg-Helper: Doppelklick → winget install Gyan.FFmpeg
cat > "$staging/Get-FFmpeg.bat" <<'EOF'
@echo off
echo Installing ffmpeg via winget (needs Windows Package Manager)...
winget install --exact --id Gyan.FFmpeg --accept-source-agreements --accept-package-agreements
pause
EOF

# Kurze USE.txt fuer Endnutzer
cat > "$staging/USE.txt" <<EOF
Easy Youtube Downloader — Portable
===================================

Doppelklick auf "Easy Youtube Downloader.exe" — fertig.

Voraussetzung: ffmpeg auf dem System.
  * Entweder "Get-FFmpeg.bat" doppelklicken (installiert via winget), oder
  * "ffmpeg.exe" von https://ffmpeg.org/download.html hier in diesen Ordner legen.

Ohne ffmpeg funktioniert nur MP3-Download; Video+Audio-Merge braucht ffmpeg.

Projekt: https://github.com/0x4Devs/easy-youtube-downloader
EOF

# Zippen
cd "$here/dist"
zipfile="$name.zip"
echo "Zipping -> $zipfile"
if command -v powershell.exe > /dev/null; then
    powershell.exe -NoProfile -Command "Compress-Archive -Path '$name/*' -DestinationPath '$zipfile' -Force" > /dev/null
else
    zip -qr "$zipfile" "$name"
fi

size_mb=$(du -m "$zipfile" | cut -f1)
echo ""
echo "Fertig:  $here/dist/$zipfile   (${size_mb} MB)"
