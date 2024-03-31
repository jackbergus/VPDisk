/*
 * disk_vp_node_header.h
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

#ifndef SIMMATCH_DISK_VP_NODE_HEADER_H
#define SIMMATCH_DISK_VP_NODE_HEADER_H

struct disk_vp_node_header {
    unsigned int id; ///<@ id associated to the current node (i.e., insertion order)
    float radius;   ///<@ radius that, in the standard definition, provides the boundary between the left and right nodes
    unsigned int leftChild; // = std::numeric_limits<unsigned int>::max(); ///<@ setting the leftChild to nullptr
    unsigned int rightChild; // = std::numeric_limits<unsigned int>::max(); ///<@ setting the rightChild to nullptr
    bool isLeaf; // = false; ///<@ nevertheless, each node is not necessairly a leaf
};

#endif //SIMMATCH_DISK_VP_NODE_HEADER_H
