// ytdl — minimal C++20 wrapper around yt-dlp.exe + ffmpeg.exe
// MVP: forwards args to a bundled yt-dlp.exe located next to this binary.
// Live stdout/stderr passthrough via CreateProcess + inherited handles.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

fs::path exe_dir() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return fs::path(std::wstring(buf, n)).parent_path();
}

// Wrap an argument for CommandLineToArgvW round-trip. yt-dlp URLs are safe,
// but user-supplied output paths may contain spaces.
std::wstring quote_arg(const std::wstring& a) {
    if (!a.empty() && a.find_first_of(L" \t\"") == std::wstring::npos)
        return a;
    std::wstring out = L"\"";
    for (size_t i = 0; i < a.size(); ++i) {
        size_t bs = 0;
        while (i < a.size() && a[i] == L'\\') { ++bs; ++i; }
        if (i == a.size()) { out.append(bs * 2, L'\\'); break; }
        if (a[i] == L'"') { out.append(bs * 2 + 1, L'\\'); out.push_back(L'"'); }
        else              { out.append(bs, L'\\');          out.push_back(a[i]); }
    }
    out.push_back(L'"');
    return out;
}

std::wstring to_wide(std::string_view s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

int run(const fs::path& program, const std::vector<std::wstring>& args) {
    std::wstring cmd = quote_arg(program.wstring());
    for (const auto& a : args) { cmd.push_back(L' '); cmd += quote_arg(a); }

    STARTUPINFOW si{ .cb = sizeof(si) };
    PROCESS_INFORMATION pi{};

    // Mutable buffer required by CreateProcessW.
    std::vector<wchar_t> cmdbuf(cmd.begin(), cmd.end());
    cmdbuf.push_back(L'\0');

    if (!CreateProcessW(program.c_str(), cmdbuf.data(),
                        nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        std::cerr << "CreateProcess failed: " << GetLastError() << "\n";
        return 1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)code;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    SetConsoleOutputCP(CP_UTF8);

    const fs::path here    = exe_dir();
    const fs::path ytdlp   = here / "yt-dlp.exe";
    const fs::path ffmpeg_local = here / "ffmpeg.exe";

    if (!fs::exists(ytdlp)) {
        std::cerr << "yt-dlp.exe nicht gefunden neben " << here.string() << "\n";
        return 2;
    }

    if (argc < 2) {
        std::cout <<
            "ytdl — minimal YouTube downloader\n"
            "Usage: ytdl <URL> [zusaetzliche yt-dlp args]\n"
            "Default: bestes mp4 (video+audio) in aktuelles Verzeichnis.\n";
        return 0;
    }

    std::vector<std::wstring> args = {
        L"-f", L"bv*+ba/b",             // best video + best audio, fallback best
        L"--merge-output-format", L"mp4",
        L"-o", L"%(title)s [%(id)s].%(ext)s",
        L"--newline",                   // ein Progress-Update pro Zeile (schoener im CLI)
    };
    // Bundled ffmpeg bevorzugen (macht Distribution portabel); sonst PATH.
    if (fs::exists(ffmpeg_local)) {
        args.emplace_back(L"--ffmpeg-location");
        args.emplace_back(here.wstring());
    }
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    return run(ytdlp, args);
}
