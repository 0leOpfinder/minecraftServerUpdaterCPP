#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

using json = nlohmann::json;

// Function to handle CURL response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t totalSize = size * nmemb;
    s->append((char*)contents, totalSize);
    return totalSize;
}

std::string httpGet(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    return response;
}

void stopMinecraftServer() {
    int ret_code = std::system("screen -S minecraft -X stuff \"stop\\n\"");
    if (ret_code == 0) {
        std::cout << "Minecraft server stopped successfully." << std::endl;
    } else {
        std::cout << "Failed to stop Minecraft server." << std::endl;
    }
}

void startMinecraftServer() {
    int ret_code = std::system("screen -dmS minecraft java -Xmx1024M -Xms1024M -jar server.jar nogui");
    if (ret_code == 0) {
        std::cout << "Minecraft server started successfully." << std::endl;
    } else {
        std::cout << "Failed to start Minecraft server." << std::endl;
    }
}

void waitForMinecraftShutdown() {
    while (true) {
        int ret_code = std::system("screen -ls | grep minecraft > /dev/null");
        if (ret_code != 0) {
            std::cout << "Minecraft server has fully shut down." << std::endl;
            break;
        }
        std::cout << "Waiting for Minecraft server to shut down..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int main() {
    // ==== Get the most current Minecraft version online ==========================================
    std::string version_manifest_url = "https://launchermeta.mojang.com/mc/game/version_manifest.json";
    std::string response = httpGet(version_manifest_url);
    if (response.empty()) {
        std::cerr << "Failed to fetch version manifest." << std::endl;
        return 1;
    }
    json version_manifest = json::parse(response);
    std::string latest_version = version_manifest["latest"]["release"].get<std::string>();
    std::cout << "Latest version: " << latest_version << std::endl;
    // ==============================================================================================

    // Get which version we are currently running from mc_version.txt
    std::ifstream versionFile("mc_version.txt");
    std::string old_version;
    if (versionFile.is_open()) {
        std::getline(versionFile, old_version);
        versionFile.close();
    }

    // ==== Compare with release versions and update if needed ======================================
    if (old_version != latest_version) {
        stopMinecraftServer();
        waitForMinecraftShutdown();

        // ---- Download the new version of the server ---------------------------------------------
        std::string server_download_page = "https://www.minecraft.net/da-dk/download/server";
        std::string server_page_response = httpGet(server_download_page);
        if (server_page_response.empty()) {
            std::cerr << "Failed to fetch server download page." << std::endl;
            return 1;
        }
        size_t href_pos = server_page_response.find("href=\"");
        if (href_pos != std::string::npos) {
            size_t start = href_pos + 6;
            size_t end = server_page_response.find('"', start);
            std::string download_url = server_page_response.substr(start, end - start);
            std::cout << "Download URL: " << download_url << std::endl;
            std::string download_command = "curl -o server.jar " + download_url;
            int download_ret_code = std::system(download_command.c_str());
            if (download_ret_code != 0) {
                std::cerr << "Failed to download the server jar." << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Could not find Minecraft server download link." << std::endl;
            return 1;
        }
        // ------------------------------------------------------------------------------------------

        // ---- Start Minecraft server up again -----------------------------------------------------
        startMinecraftServer();
        // ------------------------------------------------------------------------------------------
    } else {
        std::cout << "Minecraft server is already up to date." << std::endl;
    }
    // ==============================================================================================

    return 0;
}

