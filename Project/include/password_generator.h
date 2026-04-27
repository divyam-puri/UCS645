#ifndef PASSWORD_GENERATOR_H
#define PASSWORD_GENERATOR_H

#include <string>
#include <vector>
#include <cstring>

class PasswordGenerator {
public:
    // Brute force password generation
    static void generateBruteForceRange(
        const std::string& charset,
        int minLen,
        int maxLen,
        long long startIdx,
        long long endIdx,
        std::vector<std::string>& candidates
    );

    // Convert index to password (brute force)
    static std::string indexToPassword(
        const std::string& charset,
        long long idx,
        int minLen
    );

    // Dictionary attack - read words from file
    static void loadDictionary(
        const std::string& dictFile,
        std::vector<std::string>& words
    );

    // Get total combinations for brute force
    static long long getTotalCombinations(
        const std::string& charset,
        int minLen,
        int maxLen
    );

    // Charset definitions
    static constexpr const char* CHARSET_LOWER = "abcdefghijklmnopqrstuvwxyz";
    static constexpr const char* CHARSET_UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static constexpr const char* CHARSET_DIGITS = "0123456789";
    static constexpr const char* CHARSET_SPECIAL = "!@#$%^&*";
    static constexpr const char* CHARSET_ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    // Full charset: lowercase + uppercase + digits + common special characters
    static constexpr const char* CHARSET_FULL = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:,.<>?";
};

#endif // PASSWORD_GENERATOR_H
