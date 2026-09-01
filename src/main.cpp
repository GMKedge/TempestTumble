#include "Application.hpp"
#include <filesystem>
#include <iostream>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

static std::filesystem::path exeDirectory(const char* argv0) {
    namespace fs = std::filesystem;
    std::error_code ec;
#if defined(_WIN32)
    wchar_t wbuf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, wbuf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        return fs::path(wbuf).parent_path();
    }
#else
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return fs::path(buf).parent_path();
    }
#endif
    if (argv0 && argv0[0]) {
        fs::path p(argv0);
        if (p.has_parent_path()) {
            fs::path abs = fs::absolute(p, ec);
            if (!ec) return abs.parent_path();
        }
    }
    return fs::current_path(ec);
}

static bool looksLikeRoot(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::exists(p / "assets" / "sprites" / "atlas.png", ec) &&
           std::filesystem::exists(p / "shaders" / "sprite.vert", ec);
}

static std::string findRoot(const char* argv0) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    fs::path exe = exeDirectory(argv0);
    const fs::path markers[] = {
        exe,
        exe / "..",
        cwd,
        cwd / "..",
        cwd / "tempest-tumble",
        cwd / ".." / "tempest-tumble",
        exe / "tempest-tumble",
    };
    for (const auto& p : markers) {
        fs::path can = fs::weakly_canonical(p, ec);
        if (looksLikeRoot(can)) return can.string();
        if (looksLikeRoot(p)) return fs::weakly_canonical(p, ec).string();
    }
    return cwd.string();
}

int main(int argc, char** argv) {
    tempest::Application app;
    const std::string root = findRoot(argc > 0 ? argv[0] : nullptr);
    std::cout << "Data root: " << root << "\n";
    if (!app.init(root)) {
        std::cerr << "Failed to start. Need OpenGL 3.3, GLFW, and the assets/ folder.\n";
        std::cerr << "Looked for atlas at: " << root << "/assets/sprites/atlas.png\n";
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
