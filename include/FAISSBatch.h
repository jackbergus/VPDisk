/*
 * FAISSBatch.h
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

#ifndef SIMMATCH_FAISSBATCH_H
#define SIMMATCH_FAISSBATCH_H

#include <vector>

struct FAISSBatch {
    int d;      // dimension
    int nb; // database size
    float *xb;

    FAISSBatch(int d, int nb);

    bool addToBatch(int idx, const std::vector<float>& row);

    virtual ~FAISSBatch();
};

#endif //SIMMATCH_FAISSBATCH_H
