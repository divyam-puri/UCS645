#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

// Hash algorithm enumeration
enum class HashAlgorithm {
    MD5,
    SHA256,
    SHA1
};

class HashUtils {
public:
    // CPU-side hash computation
    static std::string computeMD5(const std::string& input);
    static std::string computeSHA256(const std::string& input);
    static std::string computeSHA1(const std::string& input);

    // Generic hash function
    static std::string computeHash(
        const std::string& input,
        HashAlgorithm algo
    );

    // Batch comparison
    static int compareHashBatch(
        const std::vector<std::string>& passwords,
        const std::string& targetHash,
        const std::string& foundPassword,
        HashAlgorithm algo
    );

    // Convert hex string to bytes
    static std::vector<uint8_t> hexToBytes(const std::string& hex);

    // Convert bytes to hex string
    static std::string bytesToHex(const std::vector<uint8_t>& bytes);

    // Hash algorithm names
    static const char* getAlgorithmName(HashAlgorithm algo);
};

#endif // HASH_UTILS_H
