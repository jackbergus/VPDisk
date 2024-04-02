//
// Created by giacomo on 01/04/24.
//

#ifndef SIMMATCH_VECTORHASH_H
#define SIMMATCH_VECTORHASH_H

#include <vector>
#include <cstdint>

namespace std {
    template <typename T> struct hash<std::vector<T>> {
        std::size_t operator()(std::vector<uint32_t> const& vec) const {
            // https://stackoverflow.com/a/12996028/1376095
            // https://stackoverflow.com/a/72073933/1376095
            std::size_t seed = vec.size();
            for(auto x : vec) {
                x = ((x >> 16) ^ x) * 0x45d9f3b;
                x = ((x >> 16) ^ x) * 0x45d9f3b;
                x = (x >> 16) ^ x;
                seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
}



#endif //SIMMATCH_VECTORHASH_H
