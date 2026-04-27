#include "password_generator.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

void PasswordGenerator::generateBruteForceRange(
    const std::string& charset,
    int minLen,
    int maxLen,
    long long startIdx,
    long long endIdx,
    std::vector<std::string>& candidates
) {
    candidates.clear();
    candidates.reserve(endIdx - startIdx + 1);

    for (long long idx = startIdx; idx <= endIdx; ++idx) {
        candidates.push_back(indexToPassword(charset, idx, minLen));
    }
}

std::string PasswordGenerator::indexToPassword(
    const std::string& charset,
    long long idx,
    int minLen
) {
    std::string password;
    int charsetSize = charset.length();

    // ✅ BUG FIX: Replace floating-point pow() with __uint128_t integer arithmetic.
    //
    // WHY: double has only 53 bits of mantissa (~15-16 significant digits).
    //   pow(91, 9) has an error of -5 and pow(91, 10) an error of -967 vs the true
    //   integer value.  This corrupts the cumulative bucket boundary, so idx is
    //   assigned to the WRONG length bucket — the target password is then generated
    //   with the wrong number of characters and is never matched.
    __uint128_t cum128 = 0;
    int pwLen = minLen;

    while (true) {
        // Compute charsetSize^pwLen using integer multiplication, detect overflow early.
        __uint128_t cnt = 1;
        bool overflow = false;
        for (int i = 0; i < pwLen; ++i) {
            if (cnt > (__uint128_t)9223372036854775807ULL / (unsigned)charsetSize) {
                overflow = true; break;
            }
            cnt *= (unsigned)charsetSize;
        }
        if (overflow || cum128 + cnt > (__uint128_t)(long long)idx) break;
        cum128 += cnt;
        ++pwLen;
        if (pwLen > 32) return "";  // safety cap (matches GPU kernel)
    }

    // Convert cumulative back to long long (safe: always <= idx <= LLONG_MAX).
    long long cumulative = (long long)(unsigned long long)cum128;

    // Generate password of length pwLen
    idx -= cumulative;
    for (int i = 0; i < pwLen; ++i) {
        password += charset[idx % charsetSize];
        idx /= charsetSize;
    }

    // Reverse to get correct order
    std::reverse(password.begin(), password.end());
    return password;
}

void PasswordGenerator::loadDictionary(
    const std::string& dictFile,
    std::vector<std::string>& words
) {
    words.clear();
    std::ifstream file(dictFile);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open dictionary file: " + dictFile);
    }

    std::string word;
    while (std::getline(file, word)) {
        // Trim whitespace
        word.erase(0, word.find_first_not_of(" \t\r\n"));
        word.erase(word.find_last_not_of(" \t\r\n") + 1);

        if (!word.empty()) {
            words.push_back(word);
        }
    }

    file.close();
}

long long PasswordGenerator::getTotalCombinations(
    const std::string& charset,
    int minLen,
    int maxLen
) {
    __uint128_t total = 0;
    __uint128_t cap = (__uint128_t)9223372036854775807LL; // LLONG_MAX
    int charsetSize = charset.length();

    for (int len = minLen; len <= maxLen; ++len) {
        __uint128_t lenCombs = 1;
        for (int i = 0; i < len; ++i) {
            lenCombs *= charsetSize;
            if (lenCombs >= cap) { return (long long)cap; }
        }
        total += lenCombs;
        if (total >= cap) { return (long long)cap; }
    }

    return (long long)total;
}
