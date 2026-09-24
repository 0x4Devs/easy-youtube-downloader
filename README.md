# Easy Youtube Downloader

A minimal, native-looking Qt6 GUI wrapper around
[`yt-dlp`](https://github.com/yt-dlp/yt-dlp) and
[`ffmpeg`](https://ffmpeg.org/) for Windows. Written in C++20, no installer,
portable folder distribution.

## Features

- **Video or audio download** in one click (Best MP4 / 1080p / 720p / MP3)
- **Trim** — download only a section of a video (`HH:MM:SS` → `HH:MM:SS`)
- **Live video info preview** — title + duration appear as you paste
- **Deutsch / English** — language toggle in the settings dialog
- **Native Windows 11 look**, follows system dark/light theme
- **Auto-detects ffmpeg** in `PATH` or bundled next to the exe
- **CLI** (`ytdl.exe`) also included for scripts

## Download

Grab a pre-built portable folder from
[Releases](../../releases). Extract the zip and double-click
`Easy Youtube Downloader.exe`.

## Build from source

### Prerequisites (Windows)

Install [MSYS2](https://www.msys2.org/) with:

```
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-qt6-base \
          mingw-w64-x86_64-qt6-tools
```

`ffmpeg` needs to be on `PATH` at runtime (or drop `ffmpeg.exe` next to the built
exe). Install via `winget install Gyan.FFmpeg` if unsure.

### Steps

```
scripts\fetch-ytdlp.ps1       # downloads latest yt-dlp.exe into bin/
build.bat                      # builds ytdl.exe + Easy Youtube Downloader.exe
```

Output in `build/`. The build script also copies all transitive Qt / MinGW DLLs
next to the exe so the folder is self-contained.

### Manual build

```
export PATH=/c/msys64/mingw64/bin:$PATH
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Project layout

```
src/
  main.cpp           # CLI target (ytdl.exe)
  gui.cpp            # Qt6 GUI target (Easy Youtube Downloader.exe)
scripts/
  copy_mingw_deps.sh # POST_BUILD: copies MinGW/Qt transitive DLL deps
  fetch-ytdlp.ps1    # downloads latest yt-dlp
vendor/runtime_libs/ # ABI-compat libstdc++/libgcc — see note below
```

### Note on `vendor/runtime_libs/`

MSYS2 currently ships Qt6 6.11.2 built against GCC 16.1, but the toolchain has
already moved to GCC 16.2 which dropped some ABI symbols
(`_ZSt15__get_once_callv` etc.). Until Qt6 is rebuilt upstream, this repo ships
compatible GCC 16.1 versions of `libstdc++-6.dll` and `libgcc_s_seh-1.dll`
which the build system copies over the newer ones. Remove this override once
MSYS2 rebuilds Qt6.

## Contributing

Issues and pull requests welcome. This is a small hobby project — feature
requests, UI tweaks and bug reports all appreciated.

## License

[MIT](LICENSE). `yt-dlp` (Unlicense) and `ffmpeg` (LGPL) are external
dependencies you download yourself; they are not redistributed with this
repository.
