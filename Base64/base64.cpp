#include "base64.h"

static inline bool is_base64(BYTE c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(BYTE const* buf, unsigned int bufLen) {
    std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int i = 0;
    int j;
    BYTE char_array_3[3], char_array_4[4];

    while (bufLen--) {
        char_array_3[i++] = *(buf++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 252u) / 4;
            char_array_4[1] = ((char_array_3[0] & 3u) * 16) + ((char_array_3[1] & 240u) / 16);
            char_array_4[2] = ((char_array_3[1] & 15u) * 4) + ((char_array_3[2] & 192u) / 64);
            char_array_4[3] = char_array_3[2] & 63u;

            for (i = 0; (i < 4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {    // If the buffer dimension is not divisible by 3
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 252u) / 4;
        char_array_4[1] = ((char_array_3[0] & 3u) * 16) + ((char_array_3[1] & 240u) / 16);
        char_array_4[2] = ((char_array_3[1] & 15u) * 4) + ((char_array_3[2] & 192u) / 64);
        char_array_4[3] = char_array_3[2] & 63u;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))   // Adding termination characters
            ret += '=';
    }

    return ret;
}

std::vector<BYTE> base64_decode(std::string const& encoded_string) {
    std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int in_len = encoded_string.size();
    int i = 0;
    int j;
    int in_ = 0;
    BYTE char_array_4[4], char_array_3[3];
    std::vector<BYTE> ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);   // Substituting the character with its index in base64_chars

            char_array_3[0] = (char_array_4[0] * 4) + ((char_array_4[1] & 48u) / 16);
            char_array_3[1] = ((char_array_4[1] & 15u) * 16) + ((char_array_4[2] & 60u) / 4);
            char_array_3[2] = ((char_array_4[2] & 3u) * 64) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);   // Substituting the character with its index in base64_chars

        char_array_3[0] = (char_array_4[0] * 4) + ((char_array_4[1] & 48u) / 16);
        char_array_3[1] = ((char_array_4[1] & 15u) * 16) + ((char_array_4[2] & 60u) / 4);
        char_array_3[2] = ((char_array_4[2] & 3u) * 64) + char_array_4[3];

        for (j = 0; (j < i - 1); j++)
            ret.push_back(char_array_3[j]);
    }

    return ret;
}
