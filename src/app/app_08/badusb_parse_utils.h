#pragma once
#include <cstdint>
#include <climits>
#include <cstring>

namespace badusb_parse {
inline bool space(char c) { return c == ' ' || c == '\t'; }
inline const char* skipSpace(const char* p) {
    while (space(*p)) ++p;
    return p;
}
inline uint32_t commandLength(const char* p) {
    uint32_t n = 0;
    while (p[n] && !space(p[n])) ++n;
    return n;
}
// Return the terminator itself when a parameter is missing, never one past it.
inline const char* arguments(const char* p, uint32_t length) {
    p += length;
    return *p ? p + 1 : p;
}
inline bool unsignedToken(const char*& p, uint32_t& value) {
    p = skipSpace(p);
    if (*p < '0' || *p > '9') return false;
    uint32_t n = 0;
    do {
        const unsigned digit = *p - '0';
        if (n > (UINT32_MAX - digit) / 10) return false;
        n = n * 10 + digit;
        ++p;
    } while (*p >= '0' && *p <= '9');
    if (*p && !space(*p)) return false;
    value = n;
    return true;
}
inline bool unsignedValue(const char* p, uint32_t& value) {
    uint32_t n;
    if (!unsignedToken(p, n) || *skipSpace(p)) return false;
    value = n;
    return true;
}
inline bool signedToken(const char*& p, int32_t& value) {
    p = skipSpace(p);
    const bool negative = *p == '-';
    if (negative) ++p;
    if (*p < '0' || *p > '9') return false;
    uint32_t n;
    if (!unsignedToken(p, n) || n > (negative ? 2147483648u : 2147483647u)) return false;
    value = negative ? (n == 2147483648u ? INT32_MIN : -int32_t(n)) : int32_t(n);
    return true;
}
inline bool signedValue(const char* p, int32_t& value) {
    int32_t n;
    if (!signedToken(p, n) || *skipSpace(p)) return false;
    value = n;
    return true;
}
inline bool deadlineReached(uint32_t now, uint32_t deadline) {
    return int32_t(now - deadline) >= 0;
}
inline uint32_t delaySum(uint32_t a, uint32_t b) {
    // Deadlines use signed differences; keep them within the half-range.
    return a >= uint32_t(INT32_MAX) || b >= uint32_t(INT32_MAX) || a > uint32_t(INT32_MAX) - b
        ? uint32_t(INT32_MAX) : a + b;
}
} // namespace badusb_parse
