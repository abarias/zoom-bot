#include "zoom_auth.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>

// --- libcurl callback ---
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// --- Base64 encode helper ---
std::string base64_encode(const std::string &in) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string out;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(in.c_str());
    unsigned int in_len = in.length();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                out += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++)
            out += base64_chars[char_array_4[j]];

        while((i++ < 3))
            out += '=';
    }

    return out;
}

// Implementation of HttpClient request method
std::string HttpClient::request(const std::string& url, 
                              const std::vector<std::string>& headers, 
                              bool isPost,
                              const std::string& postFields) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    struct curl_slist* headersList = NULL;
    std::cout << "\nRequest details:" << std::endl;
    std::cout << "URL: " << url << std::endl;
    std::cout << "Headers:" << std::endl;
    for (const auto& header : headers) {
        std::cout << "  " << header << std::endl;
        headersList = curl_slist_append(headersList, header.c_str());
    }
    if (isPost && !postFields.empty()) {
        std::cout << "POST data: " << postFields << std::endl;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headersList);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    if (isPost) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (!postFields.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        }
    }

    res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headersList);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
    }

    std::cout << "\nResponse received:" << std::endl;
    std::cout << "HTTP Status: " << http_code << std::endl;
    std::cout << "Response body: " << readBuffer << std::endl;

    if (http_code != 200) {
        throw std::runtime_error("HTTP request failed with code " + std::to_string(http_code) + ": " + readBuffer);
    }

    return readBuffer;
}

// --- Fetch Zoom Access Token ---

std::string getZoomAccessToken(const std::string& clientId, const std::string& clientSecret, const std::string& accountId) {
    const std::string url = "https://zoom.us/oauth/token";
    const std::string grantType = "account_credentials";

    // Prepare auth header
    std::string authString = clientId + ":" + clientSecret;
    std::string base64Auth = base64_encode(authString);
    std::vector<std::string> headers = {
        "Authorization: Basic " + base64Auth,
        "Content-Type: application/x-www-form-urlencoded"
    };

    // Prepare post data
    std::string postData = "grant_type=" + grantType + "&account_id=" + accountId;
    std::cout << "Post data: " << postData << std::endl;

    try {
        HttpClient client;
        std::cout << "Sending request..." << std::endl;
        std::string response = client.request(url, headers, true, postData);
        std::cout << "Response received, length: " << response.length() << " chars" << std::endl;
        std::cout << "Response: " << response << std::endl;
        
        nlohmann::json jsonResponse = nlohmann::json::parse(response);
        
        if (jsonResponse.contains("access_token")) {
            std::string token = jsonResponse["access_token"].get<std::string>();
            std::cout << "Successfully got access token" << std::endl;
            return token;
        } else {
            throw std::runtime_error("Response does not contain access_token field");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting OAuth token: " << e.what() << std::endl;
        throw;
    }
}

// --- Get meeting numeric ID ---
uint64_t getMeetingNumericId(const std::string& accessToken, uint64_t meetingNumber) {
    std::string url = "https://api.zoom.us/v2/meetings/" + std::to_string(meetingNumber);
    
    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + accessToken
    };

    try {
        HttpClient client;
        std::string response = client.request(url, headers);
        nlohmann::json jsonResp = nlohmann::json::parse(response);
        
        if (jsonResp.contains("numeric_id")) {
            std::string numericId = jsonResp["numeric_id"].get<std::string>();
            return std::stoull(numericId);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to get numeric meeting ID: " << e.what() << std::endl;
    }
    
    return meetingNumber; // Return original number if we can't get numeric ID
}

// --- Check if meeting exists ---
bool checkMeetingExists(const std::string& accessToken, uint64_t meetingNumber) {
    std::string url = "https://api.zoom.us/v2/meetings/" + std::to_string(meetingNumber);
    
    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + accessToken
    };

    try {
        HttpClient client;
        std::string response = client.request(url, headers);
        nlohmann::json jsonResp = nlohmann::json::parse(response);
        
        // If we got a valid response with an id field, the meeting exists
        return jsonResp.contains("id");
    } catch (const std::exception& e) {
        std::cerr << "Failed to check if meeting exists: " << e.what() << std::endl;
        return false; // If we get an error, assume the meeting doesn't exist
    }
}

// --- Get Zoom ZAK token ---
std::string getZoomZAK(const std::string& accessToken) {
    std::string url = "https://api.zoom.us/v2/users/me/token?type=zak";
    
    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + accessToken
    };

    try {
        HttpClient client;
        std::string response = client.request(url, headers);
        nlohmann::json jsonResp = nlohmann::json::parse(response);
        return jsonResp["token"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "Failed to get ZAK token: " << e.what() << std::endl;
        throw;
    }
}
