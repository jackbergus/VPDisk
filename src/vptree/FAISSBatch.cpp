/*
 * FAISSBatch.cpp
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

#include "vptree/FAISSBatch.h"

#include <cstdlib>
#include <string.h>

FAISSBatch::FAISSBatch(int d, int nb) : d(d), nb(nb), xb{nullptr} {
    xb = new float[d * nb];
    memset((void*)xb, 0, sizeof(float)*d*nb);
}

bool FAISSBatch::addToBatch(int idx, const std::vector<float> &row) {
    if (idx >= nb)
        return false;
//         rand()/double(RAND_MAX)*24.f+1.f;
    for (int j = 0; j < std::min(d, (int)row.size()); j++)
        *(xb + idx*d + j) = row[j];
    return true;
}

FAISSBatch::~FAISSBatch() {
    if (xb)
        delete xb;
}
