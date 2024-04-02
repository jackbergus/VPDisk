/*
 * Builder.h
 * This file is part of DiskVP
 *
 * Copyright (C) 2024 - Giacomo Bergami
 *
 * DiskVP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DiskVP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DiskVP. If not, see <http://www.gnu.org/licenses/>.
 */


//
// Created by giacomo on 31/03/24.
//

#ifndef SIMMATCH_BUILDER_H
#define SIMMATCH_BUILDER_H

#include "DiskVP.h"

static inline float squared_distance(size_t n, float* l, float* r) {
    float s = 0;
    for (size_t i = 0; i<n; i++) {
        float f = l[i]-r[i];
        s+=(f*f);
    }
    return std::sqrt(s);
}

class Builder {
    int d;
    std::filesystem::path p;
    DiskVP b1;
    DiskVP b2;
    std::string sorted;

public:
    Builder(int d, const std::filesystem::path &p, int blockade=-1, VPTRee_Strategies doMedian=RANDOM_ROOT_UNBALANCED) : d(d), p(p), sorted(p.string()+"_sorted"),
                                                     b1(d, p.c_str(), squared_distance, blockade, doMedian),
                                                     b2(d, p.string()+"_sorted", squared_distance){

    }

    inline void write_entry_to_disk(const std::vector<float>& ptr) {
        b1.write_entry_to_disk(ptr);
    }

    inline void build() {
        b1.restruct_index( b2);
        std::string name1 = std::tmpnam(nullptr);
        std::rename(p.c_str(), name1.c_str());
        std::rename(sorted.c_str(), p.c_str());
        unlink(name1.c_str());
    }

};

#endif //SIMMATCH_BUILDER_H
