/*
 * main.cpp
 * This file is part of DiskVP
 *
 * Copyright (C) 2024 - Giacomo Bergami
 *
 * FAISSExample is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * FAISSExample is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FAISSExample. If not, see <http://www.gnu.org/licenses/>.
 */
//
// Created by giacomo on 01/04/24.
//

#ifndef SIMMATCH_SIMILARITIES_H
#define SIMMATCH_SIMILARITIES_H

#include <vector>
#include <string>
#include <immintrin.h>


enum SimilarityType {
    RandomProjectionHash, // https://dl.acm.org/doi/pdf/10.5555/1858842.1858885
    SimHashF, // https://www.cs.princeton.edu/courses/archive/spring04/cos598B/bib/CharikarEstim.pdf
    LSPStable_Cauchy, // https://www.cs.princeton.edu/courses/archive/spring05/cos598E/bib/p253-datar.pdf
    LSPStable_Gaussian, // https://www.cs.princeton.edu/courses/archive/spring05/cos598E/bib/p253-datar.pdf
    MinHash_Convolution,
    MinHash // https://web.archive.org/web/20150131043133/http://gatekeeper.dec.com/ftp/pub/dec/SRC/publications/broder/positano-final-wpnums.pdf
};

#include <random>
#include <algorithm>

template <typename T>
class RandomProjection { // -----> SimilarityType::RandomProjectionHash
    std::normal_distribution<T> d{0.0, 1.0};
    std::vector<std::vector<T>> planes;
    size_t dim;
public:

    RandomProjection(size_t dim) : dim{dim} {
        std::mt19937 gen{0};
        for (size_t j = 0; j<sizeof(T); j++) {
            auto& w = planes.emplace_back();
            for (size_t i = 0; i<dim; i++)
                w.emplace_back(d(gen));
        }
    }

    T hash(const std::vector<T>& data) const {
        T sig = 0;
        for (const auto& p : planes) {
            sig = sig << 1;
            T count = 0;
            for (size_t i = 0; i<data.size(); i++)
                count += data[i] * p[i];
            if (count >= 0)
                sig |= 1;
        }
        return sig;
    }

    T hash_distance(T lhs, T rhs) const {
        T r = lhs ^ rhs;
        T res = 1;
        r = r & (r-1);
        while(r>0) {
            res += 1;
        }
        r = r & (r-1);
        return res;
    }
};

#include <memory.h>

// This provides an estimation of the similarity of the two vectors via their hash functions!
template <typename T>
class SimHash { // ---> SimilarityType::SimHash
     inline T hash(const std::vector<T>& tokens) const
    {
        // https://github.com/odinliu/simhash/blob/master/simhash.c
        // https://underthehood.meltwater.com/blog/2019/02/25/locality-sensitive-hashing-in-elixir/
        constexpr const size_t SIMHASH_BIT = sizeof(T);
        float hash_vector[SIMHASH_BIT];
        memset(hash_vector, 0, SIMHASH_BIT * sizeof(float));
        T token_hash = 0;
        T simhash = 0;
        int current_bit = 0;

        for(int i=0; i<tokens.size(); i++) {
            token_hash = tokens[i]; //sh_hash(tokens[i], strlen(tokens[i]));
            for(int j=SIMHASH_BIT-1; j>=0; j--) {
                if(token_hash & 0x1) {
                    hash_vector[j] += 1;
                } else {
                    hash_vector[j] -= 1;
                }
                token_hash = token_hash >> 1;
            }
        }
        for(int i=0; i<SIMHASH_BIT; i++) {
            if(hash_vector[i] > 0) {
                simhash = (simhash << 1) + 0x1;
            } else {
                simhash = simhash << 1;
            }
        }
        return simhash;
    }

    T hash_distance(T lhs, T rhs) const {
        return lhs < rhs ? rhs-lhs : lhs-rhs;
    }
};




template <typename T>
class PStableDistributions { // SimilarityType::LSPStable_Cauchy, (cauchy_gau_otherwise=true)
                             // SimilarityType::LSPStable_Gaussian (cauchy_gau_otherwise=false)
    /// Window size
    float W;
    size_t dim;
    std::vector<float> a;
    float rndBs;

            public:
    PStableDistributions(float W,  size_t dim, bool cauchy_gau_otherwise) : W{W}, dim{dim} {
        std::mt19937 gen{0};
        std::uniform_real_distribution<float> ur(0, W);
        if (cauchy_gau_otherwise) {
            std::cauchy_distribution<float> cd;
            for (unsigned i = 0; i < dim; ++i) {
                a.emplace_back(cd(gen));
            }
        } else {
            std::normal_distribution<float> cd;
            for (unsigned i = 0; i < dim; ++i) {
                a.emplace_back(cd(gen));
            }
        }
        rndBs = (ur(gen));
    }

    T hash(const std::vector<T>& data) const {
        float sum(0);
        for (unsigned j = 0; j < dim; ++j) {
            sum += data[j] * a[j];
        }
       return T(std::floor((sum + rndBs) / W));
    }

    T hash_distance(T lhs, T rhs) const {
        return lhs < rhs ? rhs-lhs : lhs-rhs;
    }
};

template <typename T>
class UniqueRandom {
    std::mt19937_64 gen64;
    std::uniform_int_distribution<T> dis;
public:

    UniqueRandom() : dis(0, std::numeric_limits<T>::max()) {}

    std::vector<T> run(size_t fill_size) {
        std::vector<T> vec;
        while (vec.size() < fill_size) {
            vec.emplace_back(dis(gen64)); // create new random number
            std::sort(vec.begin(), vec.end()); // sort before call to unique
            auto last = std::unique(vec.begin(), vec.end());
            vec.erase(last, vec.end());       // erase duplicates
        }

        std::random_shuffle(vec.begin(), vec.end()); // mix up the sequence
        return vec;
    }

};



template <typename T>
T getModulo(int len) {
//    static_assert(std::is_unsigned<T>::value, "I am expecting an unsigned number");
//    constexpr size_t len = sizeof(T);
    if (len >= 4) {
        return 18446744073709551557ULL;
    } else if (len == 3) {
        return 4294967311UL;
    } else if (len == 2) {
        return 65521;
    } else /*if (len == 1)*/ {
        return 251;
    }
}


template <typename T>
class MinHashConvolution { // SimilarityType::MinHash_Convolution,
    size_t dim;
    size_t numHashes;
    T modulo;
    std::vector<T> coeffA, coeffB;

public:
    MinHashConvolution(size_t numHashes, size_t dim, int tokenMaxValue, int tokenCnt) : numHashes{sizeof(T)}, dim{dim}, tokenMaxValue{tokenMaxValue}, tokenCnt{tokenCnt} {
        UniqueRandom<T> ur{};
        coeffA = ur.run(numHashes);
        coeffB = ur.run(numHashes);
        modulo = getModulo<T>(numHashes-1);
    };

    T hash(const std::vector<T>& data) const {
        // https://mesuvash.github.io/blog/2019/Hashing-for-similarity/
        T v = std::numeric_limits<T>::max();
//        for (const T& x : data) {
            for (size_t i = 0; i<numHashes; i++) {
                T output = minhash(coeffA[i], coeffB[i], data);
                if (output < v)
                    v = output;
            }
//        }
        return v;
    }

    T hash_distance(T lhs, T rhs) const {
        return lhs < rhs ? rhs-lhs : lhs-rhs;
    }

private:
    inline T minhash(int coeff1, int coeff2, const std::vector<T>& tokens) const {
        T hash = 0, prevHashFactor = 1;
        for (int i = 0; i < dim && i < tokenCnt; i++) {
            hash *= tokenMaxValue;
            hash += (T)(coeff1 * tokens[i] + coeff2)% modulo ;
            prevHashFactor *= tokenMaxValue;
        }
        T result = hash;
        for (int i = dim; i < tokenCnt; i++) {
            hash *= tokenMaxValue;
            hash -= ((T)coeff1 * tokens[i-dim] + coeff2)% modulo  * prevHashFactor;
            hash += (T)(coeff1 * tokens[i] + coeff2)% modulo;
            result = hash < result ? hash : result;
        }
        return result;
    }

    int tokenMaxValue;
    int tokenCnt;
};



template <typename T>
class MinHashes { // SimilarityType::MinHash
    /*
* # This is the number of components in the resulting MinHash signatures.
# Correspondingly, it is also the number of random hash functions that
# we will need in order to calculate the MinHash.
*/
    size_t numHashes;
    T modulo;
    std::vector<T> coeffA, coeffB;
    size_t dim;

    public:
    MinHashes(size_t numHashes, size_t dim) : numHashes{sizeof(T)}, dim{dim} {
        UniqueRandom<T> ur{};
        coeffA = ur.run(numHashes);
        coeffB = ur.run(numHashes);
        modulo = 251;
    };

    T hash(const std::vector<T>& data) const {
        // https://mesuvash.github.io/blog/2019/Hashing-for-similarity/
        std::vector<unsigned char> v(sizeof(T), 255);
        for (const T& x : data) {
            for (size_t i = 0; i<numHashes; i++) {
                unsigned char output = (coeffA[i] * x + coeffB[i]) % modulo;
                if (output < v[i])
                    v[i] = output;
            }
        }
        return *reinterpret_cast<const T*>(&v[0]);
    }

    T hash_distance(T lhs, T rhs) const {
        return lhs < rhs ? rhs-lhs : lhs-rhs;
    }
};


#endif //SIMMATCH_SIMILARITIES_H
