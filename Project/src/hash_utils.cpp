#include "hash_utils.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <algorithm>

// Simple MD5-like hash for demonstration (NOT cryptographically secure)
// In production, use OpenSSL: -lssl -lcrypto
std::string HashUtils::computeMD5(const std::string& input) {
    // For academic purposes, use a simple hash function
    // In production, link with OpenSSL
    #ifdef USE_OPENSSL
    // OpenSSL implementation would go here
    #else
    // Fallback: simple hash
    unsigned int hash = 5381;
    for (char c : input) {
        hash = ((hash << 5) + hash) + c;
    }
    std::stringstream ss;
    ss << std::hex << hash;
    std::string result = ss.str();
    // Pad to 32 hex chars (MD5 length)
    while (result.length() < 32) {
        result = "0" + result;
    }
    return result.substr(0, 32);
    #endif
}

std::string HashUtils::computeSHA256(const std::string& input) {
    // For academic purposes, use a simple hash function
    #ifdef USE_OPENSSL
    // OpenSSL implementation would go here
    #else
    unsigned long hash = 0;
    for (size_t i = 0; i < input.length(); ++i) {
        hash = input[i] + (hash << 6) + (hash << 16) - hash;
    }
    std::stringstream ss;
    ss << std::hex << hash;
    std::string result = ss.str();
    // Pad to 64 hex chars (SHA256 length)
    while (result.length() < 64) {
        result = "0" + result;
    }
    return result.substr(0, 64);
    #endif
}

std::string HashUtils::computeSHA1(const std::string& input) {
    // For academic purposes, use a simple hash function
    #ifdef USE_OPENSSL
    // OpenSSL implementation would go here
    #else
    unsigned int hash = 0;
    for (char c : input) {
        hash = (hash >> 1) ^ ((hash & 1) ? 0xedb88320 : 0);
        hash ^= c;
    }
    std::stringstream ss;
    ss << std::hex << hash;
    std::string result = ss.str();
    // Pad to 40 hex chars (SHA1 length)
    while (result.length() < 40) {
        result = "0" + result;
    }
    return result.substr(0, 40);
    #endif
}

std::string HashUtils::computeHash(
    const std::string& input,
    HashAlgorithm algo
) {
    switch (algo) {
        case HashAlgorithm::MD5:
            return computeMD5(input);
        case HashAlgorithm::SHA256:
            return computeSHA256(input);
        case HashAlgorithm::SHA1:
            return computeSHA1(input);
        default:
            return computeMD5(input);
    }
}

int HashUtils::compareHashBatch(
    const std::vector<std::string>& passwords,
    const std::string& targetHash,
    const std::string& foundPassword,
    HashAlgorithm algo
) {
    for (size_t i = 0; i < passwords.size(); ++i) {
        std::string computedHash = computeHash(passwords[i], algo);
        if (computedHash == targetHash) {
            const_cast<std::string&>(foundPassword) = passwords[i];
            return i;
        }
    }
    return -1;
}

std::vector<uint8_t> HashUtils::hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::string HashUtils::bytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    for (uint8_t byte : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

const char* HashUtils::getAlgorithmName(HashAlgorithm algo) {
    switch (algo) {
        case HashAlgorithm::MD5:
            return "MD5";
        case HashAlgorithm::SHA256:
            return "SHA256";
        case HashAlgorithm::SHA1:
            return "SHA1";
        default:
            return "UNKNOWN";
    }
}
