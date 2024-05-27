#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <Windows.h>

#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#define CURL_STATICLIB
#include <curl/curl.h>
#include <set>


using json = nlohmann::json;

struct Auction {
    std::string itemName;
    int startingBid;
};

// Function to handle data received from cURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to get JSON data from a URL
std::string getJSON(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

// Function to play sound
void playSound(const std::wstring& soundFile) {
    PlaySound(soundFile.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

// Function to read items and their prices from a file
std::vector<std::pair<std::string, int>> readItemsFromFile(const std::string& fileName) {
    std::vector<std::pair<std::string, int>> items;
    std::ifstream file(fileName);
    std::string line;

    while (std::getline(file, line)) {
        size_t commaPos = line.find(',');
        if (commaPos != std::string::npos) {
            std::string itemName = line.substr(0, commaPos);
            int lowerPrice = std::stoi(line.substr(commaPos + 1));
            items.push_back(std::make_pair(itemName, lowerPrice));
        }
    }

    return items;
}

// Function to parse auction data and check for price drop for multiple items
void monitorAuctions(const std::string& apiKey, const std::string& fileName) {
    std::set<std::string> processedAuctions; // Set to store processed auction IDs

    while (true) {
        std::vector<std::pair<std::string, int>> items = readItemsFromFile(fileName);

        for (const auto& item : items) {
            std::string url = "https://api.hypixel.net/skyblock/auctions?key=" + apiKey;
            std::string jsonData = getJSON(url);

            try {
                auto root = json::parse(jsonData);

                const auto& auctions = root["auctions"];
                for (const auto& auction : auctions) {
                    if (auction["item_name"] == item.first && auction["bin"] == true) {
                        int currentPrice = auction["starting_bid"].get<int>();
                        std::string auctionID = auction["uuid"].get<std::string>();

                        // Check if the auction ID has already been processed
                        if (processedAuctions.find(auctionID) == processedAuctions.end()) {
                            if (currentPrice < item.second) {
                                std::cout << "Price dropped below " << item.second << " coins for item '" << item.first << "'! Current price: " << currentPrice << " coins." << std::endl;
                                std::cout << "View the auction: /viewauction " << auctionID << std::endl; // Print auction link
                                processedAuctions.insert(auctionID); // Add auction ID to processed set

                                // Play sound
                                playSound(L"C:\\Users\\NGON\\source\\repos\\HypixelAU\\HypixelAU\\news-ting-6832.wav");
                            }
                        }
                        break; // Stop searching for more instances of this item
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error parsing JSON data: " << e.what() << std::endl;
            }
        }

        // Wait for 30 seconds before checking again
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int main() {
    std::string apiKey = "APIKEY";  // Your Hypixel API key
    std::string fileName = "C:\\Users\\NGON\\source\\repos\\HypixelAU\\HypixelAU\\ItemSearch.txt"; // File containing item names and their lowest prices

    std::thread monitorThread(monitorAuctions, apiKey, fileName);
    monitorThread.detach(); // Detach the thread to let it run independently

    // Keep the main thread alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(60)); // Sleep for 60 seconds before exiting
    }

    return 0;
}