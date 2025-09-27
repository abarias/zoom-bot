#include "jwt_helper.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include "zoom_auth.h"  // for base64_encode

std::string base64url_encode(const std::string& input) {
    std::string encoded = base64_encode(input);
    // Replace URL unsafe characters
    size_t pos = 0;
    while ((pos = encoded.find('+', pos)) != std::string::npos) {
        encoded.replace(pos, 1, "-");
        pos++;
    }
    pos = 0;
    while ((pos = encoded.find('/', pos)) != std::string::npos) {
        encoded.replace(pos, 1, "_");
        pos++;
    }
    // Remove padding
    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }
    return encoded;
}

std::string hmacSha256(const std::string& data, const std::string& secret) {
    unsigned char hash[32];
    unsigned int hashLen = 32;
    
    HMAC_CTX *ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, secret.c_str(), secret.length(), EVP_sha256(), nullptr);
    HMAC_Update(ctx, (unsigned char*)data.c_str(), data.length());
    HMAC_Final(ctx, hash, &hashLen);
    HMAC_CTX_free(ctx);
    
    return std::string((char*)hash, hashLen);
}

std::string generateJWTToken(const nlohmann::json& header, 
                           const nlohmann::json& payload,
                           const std::string& secret) {
    std::string headerStr = base64url_encode(header.dump());
    std::string payloadStr = base64url_encode(payload.dump());
    
    std::string signInput = headerStr + "." + payloadStr;
    std::string signature = base64url_encode(hmacSha256(signInput, secret));
    
    return signInput + "." + signature;
}