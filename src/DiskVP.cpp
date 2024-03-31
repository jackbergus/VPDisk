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

#include "../include/DiskVP.h"

void DiskVP::recursive_restruct_tree(size_t first, size_t last) {
    if (first >= last) {
        // If the elements overlaps, then I reached a leaf node
        auto tree_first = getShuffledEntryPoint(first);
        tree_first->isLeaf = true;
        tree_first->leftChild = std::numeric_limits<unsigned int>::max();
        tree_first->rightChild = std::numeric_limits<unsigned int>::max();
    } else {
        if ((last - first) <= 1) {
            auto tree_first = getShuffledEntryPoint(first);
            auto tree_last = getShuffledEntryPoint(last);
            // If the elements differ by two, then I decide that one is the root, and the other is the child
            tree_first->radius = ker(d, (float*)getSPTR(first), (float*)getSPTR(last));
            tree_first->leftChild = last;
            tree_first->rightChild = std::numeric_limits<unsigned int>::max();
        } else {
            // Picking the root randomly as the first element of the tree ~ O(1)
            auto tree_first = getShuffledEntryPoint(first);
            std::uniform_int_distribution<size_t> uni(first, last - 1);
            size_t root = uni(rng);
//                std::cout << root<<"?"<<std::endl;
            std::swap(index[first], index[root]);
            auto fptr = getSPTR(first);
            size_t median = (first + last) / 2; // TODO: other heuristic, separating the elements within a fixed radius from the object, and the ones out
            /*
             * nth_element is a partial sorting algorithm that rearranges elements in [first, last) such that:
             *
             * - The element pointed at by median is changed to whatever element would occur in that position if [first, last) were sorted.
             * - All of the elements before this new nth element are less than or equal to the elements after the new nth element.
             */
            std::nth_element(
                    index.begin() + first + 1,//first
                    index.begin() + median,   //median
                    index.begin() + last,    //last
                    [fptr, first,this] (size_t l, size_t r) {
                        auto lptr = getPTR(l);
                        auto rptr = getPTR(r);
//                            if ((first == index[r]) ||(first == index[l]))
//                                std::cout << "DEBUG"<<std::endl;
                        auto dr = ker(d, fptr, (float*)rptr);
//                            std::cout << "d("<<index[first]<<","<<index[r]<<")="<<dr <<std::endl;
                        auto dl = ker(d, fptr, (float*)lptr);
//                            std::cout << "d("<<index[first]<<","<<index[l]<<")="<<dl <<std::endl;
                        return dl < dr;
                    });
//                std::cout << "R2" << std::endl;
//                for (const auto& r : index) {
//                    std::cout << r << ", ";
//                }
//                std::cout << "--" << std::endl;
            auto tree_median = getSPTR(median);

            // Setting the separating elements
//                tree_first->radius = ker(d, (float*) getSPTR(first), (float*)tree_median);
            updateNode(index[first], ker(d, (float*) getSPTR(first), (float*)tree_median), first+1,(first + last) / 2 + 1 );
            // Recursively splitting in half the elements within my radius and the ones out
//                tree_first->leftChild = first+1;
//                tree_first->rightChild = (first + last) / 2 + 1;
            size_t rc = (first + last) / 2 + 1;

            recursive_restruct_tree(first+1, rc-1);
            recursive_restruct_tree(rc, last);
        }
    }
}

void DiskVP::lookUpNearsetTo(size_t root_id, float *id, size_t depth) {
    auto root = getEntryPoint(root_id);
    double rootRadius = root->radius;
    double dist = ker(d, (float*) getPTR(root_id), id);

    if (definitelyLessThan(dist,tau)) {
        if (heap_.size() == k)
            heap_.pop();

        heap_.push(HeapItem{root->id, dist});

        if (heap_.size() == k)
            tau = heap_.top().dist;
    } else
        // Otherwise, if they are very similar, then I could add this other one too
    if (approximatelyEqual(dist, tau)) {
        if (heap_.size() == k)
            heap_.pop();

        heap_.push(HeapItem{root->id, dist});

        if (heap_.size() == k)
            tau = heap_.top().dist;
        tau = std::min(heap_.top().dist, tau);
    }

    // Continuing with the traversal depending on the distance from the root
    if (dist < rootRadius) {
        if (root->leftChild != std::numeric_limits<unsigned int>::max() && dist - tau <= rootRadius) {
            lookUpNearsetTo(root->leftChild, id, depth+1);
        }

        // At this stage, the tau value might be updated from the previous recursive call
        if (root->rightChild != std::numeric_limits<unsigned int>::max() && dist + tau >= rootRadius) {
            lookUpNearsetTo(root->rightChild, id, depth+1);
        }
    } else {
        if (root->rightChild != std::numeric_limits<unsigned int>::max() && dist + tau >= rootRadius) {
            lookUpNearsetTo(root->rightChild, id, depth+1);
        }

        // At this stage, the tau value might be updated from the previous recursive call
        if (root->leftChild != std::numeric_limits<unsigned int>::max() && dist - tau <= rootRadius) {
            lookUpNearsetTo(root->leftChild, id, depth+1);
        }
    }
}
