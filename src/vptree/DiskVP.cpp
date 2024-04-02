/*
 * DiskVP.cpp
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

#include "vptree/DiskVP.h"
#include <string.h>

void DiskVP::recursive_restruct_tree(size_t first, size_t last) {
    if (first >= last) {
        updateNode(index[first],0, std::numeric_limits<unsigned int>::max(), std::numeric_limits<unsigned int>::max(), false);
    } else {
        if ((last - first) <= 1) {
            updateNode(index[first], ker(d, (float*)getSPTR(first), (float*)getSPTR(last)), last, std::numeric_limits<unsigned int>::max(), false);
        } else {
            size_t root;
            size_t median;
            bool doBalanced;
            if (doBalancedSorting != RANDOM_ROOT_UNBALANCED) {
                if (blockade!=-1) {
                    doBalanced = (last-first)/2>blockade;
                } else {
                    doBalanced = (last-first)>2;
                }
            } else {
                doBalanced = false;
            }
            if (doBalanced) {
                memset(ptrMemory, 0, sizeof(float)*d);
                for (size_t i = first; i<last; i++) {
                    const float* memo = getSPTR(first);
                    for (size_t j = 0; j<d; j++)
                        ptrMemory[j] += memo[j];
                }
                std::sort(index.begin()+first, index.begin()+last, [this](size_t l, size_t r) {
                    auto lptr = getPTR(l);
                    auto rptr = getPTR(r);
                    auto dr = ker(d, ptrMemory, (float*)rptr);
                    auto dl = ker(d, ptrMemory, (float*)lptr);
                    return dl < dr;
                });
                bool requireResorting = false;
                if ((blockade != -1) && (first+blockade<last)) {
                    switch (doBalancedSorting) {
                        case BALANCED_ROOT_IS_GEOMETRIC_MEDIAN:
                            root = first;
                            break;

                        case BALANCED_ROOT_IS_HALFWAY_BLOCKADE:
                            root = first+(blockade/2);
                            median = (first + last) / 2;
                            requireResorting = true;
                            break;

                        case RANDOM_ROOT_UNBALANCED:
                            throw std::runtime_error("ERROR: this should never happen");
                    }
                } else {
                    root = first;
                }
                std::swap(index[first], index[root]);
                if (requireResorting) {
                    auto fptr = getSPTR(first);
                    /*
                     * nth_element is a partial sorting algorithm that rearranges elements in [first, last) such that:
                     *
                     * - The element pointed at by median is changed to whatever element would occur in that position if [first, last) were sorted.
                     * - All the elements before this new nth element are less than or equal to the elements after the new nth element.
                     */
                    std::nth_element(
                            index.begin() + first + 1,//first
                            index.begin() + median,   //median
                            index.begin() + last,    //last
                            [fptr,this] (size_t l, size_t r) {
                                auto lptr = getPTR(l);
                                auto rptr = getPTR(r);
                                auto dr = ker(d, fptr, (float*)rptr);
                                auto dl = ker(d, fptr, (float*)lptr);
                                return dl < dr;
                            });
                }
            } else {
                std::uniform_int_distribution<size_t> uni(first, last - 1);
                root = uni(rng);
                std::swap(index[first], index[root]);
                if ((blockade != -1) && (first+blockade<last)) {
                    median = blockade;
                } else {
                    median = (first + last) / 2;
                }
                auto fptr = getSPTR(first);
                /*
                 * nth_element is a partial sorting algorithm that rearranges elements in [first, last) such that:
                 *
                 * - The element pointed at by median is changed to whatever element would occur in that position if [first, last) were sorted.
                 * - All the elements before this new nth element are less than or equal to the elements after the new nth element.
                 */
                std::nth_element(
                        index.begin() + first + 1,//first
                        index.begin() + median,   //median
                        index.begin() + last,    //last
                        [fptr, first,this] (size_t l, size_t r) {
                            auto lptr = getPTR(l);
                            auto rptr = getPTR(r);
                            auto dr = ker(d, fptr, (float*)rptr);
                            auto dl = ker(d, fptr, (float*)lptr);
                            return dl < dr;
                        });
            }
            auto tree_median = getSPTR(median);

            // Setting the separating elements
            auto radius = ker(d, (float*) getSPTR(first), (float*)tree_median);
            updateNode(index[first], radius, first+1,(first + last) / 2 + 1, false );
            if ((doBalanced) && (blockade != -1)) {
                // In this case, it means that I found an entry-point node for the search!
                blockade_elements.emplace_back(index[root], radius);
            }

            // Recursively splitting in half the elements within my radius and the ones out
            size_t rc = (first + last) / 2 + 1;
            recursive_restruct_tree(first+1, rc-1);
            recursive_restruct_tree(rc, last);
        }
    }
}

DiskVP::TopKSearch::TopKSearch(const DiskVP* vp, size_t id, size_t k) : vp{vp}, k{k} {
    id = vp->idxFile[id];
    ptr = vp->getPTR(id);
}

DiskVP::TopKSearch::TopKSearch(const DiskVP* vp, float* id, size_t k) : vp{vp}, k{k}, ptr{id} {
}

DiskVP::MaxDistanceSearch::MaxDistanceSearch(const DiskVP* vp, size_t id, double maxDistance) : vp{vp}, maxDistance{maxDistance} {
    id = vp->idxFile[id];
    ptr = vp->getPTR(id);
}

DiskVP::MaxDistanceSearch::MaxDistanceSearch(const DiskVP* vp, float* id, double maxDistance) : vp{vp}, maxDistance{maxDistance}, ptr{id} {
}
