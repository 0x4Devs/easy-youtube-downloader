cmake_minimum_required(VERSION 3.20)
project(ytdl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------- CLI: ytdl.exe (statisch gelinkt, portable) ----------
add_executable(ytdl src/main.cpp)

if(WIN32)
    target_compile_definitions(ytdl PRIVATE UNICODE _UNICODE)
    if(MINGW)
        target_link_options(ytdl PRIVATE
            -static -static-libgcc -static-libstdc++
            -municode
        )
    endif()
endif()

add_custom_command(TARGET ytdl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/bin/yt-dlp.exe"
            "$<TARGET_FILE_DIR:ytdl>/yt-dlp.exe"
)

# ---------- GUI: Easy Youtube Downloader.exe (Qt6 Widgets) ----------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(easy_youtube_downloader WIN32 src/gui.cpp)
set_target_properties(easy_youtube_downloader PROPERTIES
    OUTPUT_NAME "Easy Youtube Downloader"
)
target_link_libraries(easy_youtube_downloader PRIVATE
    Qt6::Widgets
    # winpthread statisch NACH den Qt-libs — sonst greift ld auf die DLL zurueck
    # weil libstdc++.a-Referenzen erst spaeter aufgeloest werden.
    -Wl,-Bstatic
    -lwinpthread
    -Wl,-Bdynamic
)

if(WIN32)
    target_compile_definitions(easy_youtube_downloader PRIVATE UNICODE _UNICODE)
    if(MINGW)
        target_link_options(easy_youtube_downloader PRIVATE
            -mwindows                       # kein Konsolen-Fenster
            -static-libgcc -static-libstdc++
        )
    endif()
endif()

# yt-dlp.exe neben die exe kopieren
add_custom_command(TARGET easy_youtube_downloader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/bin/yt-dlp.exe"
            "$<TARGET_FILE_DIR:easy_youtube_downloader>/yt-dlp.exe"
)

# windeployqt kopiert die Qt-DLLs + Plugins.
find_program(WINDEPLOYQT windeployqt6 HINTS ${CMAKE_PREFIX_PATH}/bin /mingw64/bin)
if(WINDEPLOYQT)
    add_custom_command(TARGET easy_youtube_downloader POST_BUILD
        COMMAND ${WINDEPLOYQT}
            --release
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            --no-quick-import
            "$<TARGET_FILE:easy_youtube_downloader>"
        COMMENT "windeployqt: Qt-DLLs neben die exe kopieren"
    )
endif()

# windeployqt handhabt KEINE MinGW-Runtime- und Nicht-Qt-Transitive-Deps
# (libgcc_s_seh, libwinpthread, libstdc++, libicu*, libpcre2, libb2, zlib1, libzstd,
#  libfreetype, libharfbuzz, libmd4c, libpng16, libdouble-conversion). Wir loesen
# sie rekursiv via ldd auf und kopieren alles aus /mingw64/bin/ das gebraucht wird.
find_program(MSYS_BASH bash HINTS "C:/msys64/usr/bin" NO_DEFAULT_PATH)
if(MSYS_BASH)
    add_custom_command(TARGET easy_youtube_downloader POST_BUILD
        COMMAND "${MSYS_BASH}"
                "${CMAKE_SOURCE_DIR}/scripts/copy_mingw_deps.sh"
                "$<TARGET_FILE:easy_youtube_downloader>"
        COMMENT "Kopiere MinGW/Qt transitive DLL-Deps"
        VERBATIM
    )
endif()

# ABI-Fix: MSYS2 aktueller GCC 16.2 libstdc++ hat _ZSt15__get_once_callv nicht mehr;
# das in MSYS2 vorgebaute Qt6 6.11.2 wurde aber gegen 16.1 gebaut und braucht es.
# Bis MSYS2 Qt6 neu baut ueberschreiben wir libstdc++/libgcc mit den 16.1er Varianten.
add_custom_command(TARGET easy_youtube_downloader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/vendor/runtime_libs/libstdc++-6.dll"
            "$<TARGET_FILE_DIR:easy_youtube_downloader>/libstdc++-6.dll"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/vendor/runtime_libs/libgcc_s_seh-1.dll"
            "$<TARGET_FILE_DIR:easy_youtube_downloader>/libgcc_s_seh-1.dll"
    COMMENT "Runtime-libs auf Qt6-kompatible 16.1er Version zurueckdrehen"
)
