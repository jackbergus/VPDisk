/*
 * DiskVP.h
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

#ifndef SIMMATCH_DISKVP_H
#define SIMMATCH_DISKVP_H

#include <filesystem>
#include <vector>
#include <functional>
#include <random>
#include "mmapFile.h"
#include "disk_vp_node_header.h"
#include <queue>
#include <stack>


/**
 * From Knuth's the art of computer programming
 */
template <typename T> bool approximatelyEqual(T a, T b)
{
    return std::abs(a - b) <= ( (std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * std::numeric_limits<T>::epsilon());
}

/**
 * From Knuth's the art of computer programming
 */
template  <typename T>  bool definitelyLessThan(T a, T b)
{
    return (b - a) > ( (std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * std::numeric_limits<T>::epsilon());
}

#include <set>

struct DiskVP {
    unsigned int d;
    std::filesystem::path vptree;
    size_t idx;
    FILE* myfile;
    std::vector<size_t> index;
    unsigned long mmapfilelen, idxLen;
    mmap_file fileptr, idxPtr;
    char* file;
    size_t* idxFile;
    bool start_to_write;
    std::function<float(size_t,float*,float*)> ker;
    std::mt19937 rng;                 ///<@ random number generator
    int blockade;
    bool doBalancedSorting;
    float* ptrMemory;

    DiskVP(unsigned int d,
           const std::filesystem::path& vptree,
           const std::function<float(size_t,float*,float*)>& ker,
           int blockade = -1,
           bool doBalancedSorting = false) :

           file{nullptr}, d(d), start_to_write{false}, vptree(vptree), idx{0}, ker{ker}, blockade{blockade},
           doBalancedSorting{doBalancedSorting} {
        if (doBalancedSorting) {
            ptrMemory = new float[d];
        } else {
            ptrMemory = nullptr;
        }
    }

    virtual ~DiskVP() {
        if (ptrMemory)
            delete ptrMemory;
    }

    inline void start_write_to_disk() {
        if (!start_to_write) {
//            std::ios_base::sync_with_stdio(false);
            myfile = fopen(vptree.c_str(), "w");
            fwrite(&d, sizeof(float), 1, myfile);
//            myfile.write((char*)(&d), sizeof(float));
            start_to_write = true;
        }

    }

    inline void openSortedFile(bool actual=true) {
        file = (char *) mmapFile(vptree.string(), &mmapfilelen, &fileptr);
        if (*((unsigned int *) file) != d) {
            throw std::runtime_error("ERROR: DIMENSIONS DO NOT MATCH");
        }
        idx = (mmapfilelen-sizeof(unsigned int))/(sizeof(disk_vp_node_header)+sizeof(float)*d);
        if (actual) {
            std::string indexFN = vptree.string()+"_idx";
            idxFile = (size_t *) mmapFile(indexFN, &idxLen, &idxPtr);
            if (idx != (idxLen)/sizeof(size_t) ) {
                throw std::runtime_error("ERROR: LENGTH DOES NOT MATCH");
            }
        }
        start_to_write = true;
    }

    void printSortedFile(std::ostream& out) {
        for (size_t i = 0;i<idx; i++) {
            auto ptr = getEntryPoint(i);
            out << "#" << i << ":" << std::endl;
            out << "\t id = " << ptr->id<< std::endl;
            out << "\t radius = " << ptr->radius << std::endl;
            if (ptr->isLeaf)
                out << "\t leaf " << std::endl;
            if (ptr->leftChild != std::numeric_limits<unsigned int>::max())
                out << "\t leftChild = " << ptr->leftChild << std::endl;
            if (ptr->rightChild != std::numeric_limits<unsigned int>::max())
                out << "\t rightChild = " << ptr->rightChild << std::endl;
        }
    }

    inline void closeSortedFile() {
        if (file) {
            mmapClose(file, &fileptr);
        }
        if (idxFile) {
            mmapClose(idxFile, &idxPtr);
        }
    }

    inline size_t size() const {
        return idx;
    }

    inline void write_entry_to_disk(disk_vp_node_header *to_dist, float *ptr) {
        start_write_to_disk();
//        constexpr auto maxx = std::numeric_limits<unsigned int>::max();
//        disk_vp_node_header to_disk;
//        to_disk.id = to_dist->id;
//        to_disk.radius = to_dist->radius;
//        to_disk.leftChild = to_dist->leftChild == maxx ? maxx :  index[to_dist->leftChild];
//        to_disk.rightChild = to_dist->rightChild == maxx ? maxx : index[to_dist->rightChild];
//        to_disk.isLeaf = to_dist->isLeaf;
        fwrite(to_dist, sizeof(disk_vp_node_header), 1, myfile);
        fwrite(ptr, sizeof(float), d, myfile);
        fflush(myfile);
        this->index.emplace_back(idx);
        idx++;
    }

    inline void write_entry_to_disk(const std::vector<float>& entry) {
        start_write_to_disk();
        disk_vp_node_header to_disk;
        to_disk.id = idx;
        to_disk.radius = 0;
        to_disk.leftChild = to_disk.rightChild = std::numeric_limits<unsigned int>::max();
        to_disk.isLeaf = false;
        fwrite(&to_disk, sizeof(disk_vp_node_header), 1, myfile);
        fwrite(entry.data(), sizeof(float), d, myfile);
//        fflush(myfile);
        index.emplace_back(idx);
        idx++;
    }

    inline void restruct_index(DiskVP& b2) {
        if (start_to_write) {
            finaliseFile();
            recursive_restruct_tree(0, idx-1);
            std::string indexFN = vptree.string()+"_idx";
            std::vector<size_t> indexMemory(size(), 0);
            for (size_t i = 0, N = size(); i<N; i++) {
                auto ptr = getShuffledEntryPoint(i);
                indexMemory[ptr->id] = i;
                b2.write_entry_to_disk(ptr, getSPTR(i));
            }
            auto idxFile = fopen(indexFN.c_str(), "w");
            for (size_t i = 0, N = size(); i<N; i++) {
                fwrite(&indexMemory[i], sizeof(size_t), 1, idxFile);
            }
            fclose(idxFile);
            b2.finaliseFile();
        }
    }

    inline struct disk_vp_node_header* getEntryPoint(size_t idx) {
        finaliseFile();
        return (struct disk_vp_node_header*)(file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx);
    }

    inline struct disk_vp_node_header* getEntryPoint(size_t idx) const {
//        finaliseFile();
        return (struct disk_vp_node_header*)(file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx);
    }

    inline struct disk_vp_node_header* getShuffledEntryPoint(size_t idx) {
        finaliseFile();
        idx = index[idx];
        return (struct disk_vp_node_header*)(file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx);
    }

    inline void updateNode(size_t idx, float distance, size_t lchild,size_t rchild, bool lf=false) {
        if (lf)
            *(bool*)(file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx+ offsetof(disk_vp_node_header, isLeaf)) = lf;
        if (distance != 0.0)
            *(float*)(file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx+ offsetof(disk_vp_node_header, radius)) = distance;
        if (lchild != std::numeric_limits<unsigned int>::max())
            *(size_t*)(file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx+ offsetof(disk_vp_node_header, leftChild)) = lchild;
        if (lchild != std::numeric_limits<unsigned int>::max())
            *(size_t*)(file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx+ offsetof(disk_vp_node_header, rightChild)) = rchild;
    }

    inline void finaliseFile() {
        if (start_to_write) {
            if (!file) {
                fclose(myfile);
                openSortedFile(false);
            }
        }
    }

    inline float* getPTR(size_t idx) const {
        float* pt = nullptr;
        if (start_to_write) {
            pt = (float*)((file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx+sizeof(disk_vp_node_header)));
        }
        return pt;
    }

    inline float* getSPTR(size_t idx) const {
        idx = index[idx];
        std::vector<float> r;
        float* pt = nullptr;
        if (start_to_write) {
//            finaliseFile();
            pt = (float*)((file + sizeof(unsigned int) + (sizeof(disk_vp_node_header) + (sizeof(float)*d))*idx+sizeof(disk_vp_node_header)));
        }
        return pt;
    }

    inline float distance(size_t l, size_t r) {
        return ker(d, getPTR(l), getPTR(r));
    }

    struct HeapItem {
        bool operator < (const HeapItem& other) const {
            return dist < other.dist;
        }

        unsigned int item;
        float dist;
    };

//    std::vector<size_t> found;
//    std::priority_queue<HeapItem> heap_;
//    size_t k = 0;

//    inline std::vector<HeapItem> maxDistanceSearch(size_t id, double maxDistance = std::numeric_limits<double>::max()) {
//        id = idxFile[id];
//        heap_.clear();
////        tau    = maxDistance;
//        lookUpNearsetTo(0, getPTR(id), maxDistance);
//        std::vector<HeapItem> results{heap_.begin(), heap_.end()};
//        return results;
//    }
//
//    inline std::vector<HeapItem> maxDistanceSearch(float* ptr, double maxDistance = std::numeric_limits<double>::max()) {
//        heap_.clear();
////        tau    = maxDistance;
//        lookUpNearsetTo(0, ptr, maxDistance);
//        std::vector<HeapItem> results{heap_.begin(), heap_.end()};
//        return results;
//    }
//
//    inline std::vector<HeapItem> topkSearchIdx(size_t id, size_t k=1) {
//        this->k = k;
//        id = idxFile[id];
//        heap_.clear();
//        tau    = std::numeric_limits<double>::max();
//        lookUpNearsetTo(0, getPTR(id), maxDistance);
//        std::vector<HeapItem> results{heap_.begin(), heap_.end()};
//        return results;
//    }
//
//    inline std::vector<HeapItem> topkSearch(float* ptr, size_t k=1) {
//        this->k = k;
//        heap_.clear();
//        tau    = std::numeric_limits<double>::max();
//        lookUpNearsetTo(0, ptr, maxDistance);
//        std::vector<HeapItem> results{heap_.begin(), heap_.end()};
//        return results;
//    }

    struct TopKSearch {
        float* ptr;
        const DiskVP* vp;
        std::priority_queue<HeapItem> heap_;
        size_t k;

        TopKSearch(const DiskVP* vp, size_t id, size_t k);
        TopKSearch(const DiskVP* vp, float* id, size_t k);

        inline std::vector<HeapItem> run() {
            float tau = std::numeric_limits<float>::max();
            std::stack<size_t> s;
            s.emplace(0);
            while (!s.empty()) {
                auto root_id = s.top();
                s.pop();
                auto root = vp->getEntryPoint(root_id);
                double rootRadius = root->radius;
                float dist = vp->ker(vp->d, (float*) vp->getPTR(root_id), ptr);

                if (definitelyLessThan(dist,tau)) {
                    heap_.push(HeapItem{root->id, dist});
                    if (heap_.size() > k)
                        heap_.pop();


                    if (heap_.size() == k)
                        tau = heap_.top().dist;
                } else
                    // Otherwise, if they are very similar, then I could add this other one too
                        if (approximatelyEqual(dist, tau)) {
                            heap_.push(HeapItem{root->id, dist});
                            if (heap_.size() > k)
                                heap_.pop();

                            if (heap_.size() == k)
                                tau = heap_.top().dist;
                            tau = std::min(heap_.top().dist, tau);
                        }

                if (dist < rootRadius) {
                    if (root->leftChild != std::numeric_limits<unsigned int>::max() && dist - tau <= rootRadius) {
                        s.push(root->leftChild);
                    }
                    // At this stage, the tau value might be updated from the previous recursive call
                    if (root->rightChild != std::numeric_limits<unsigned int>::max() && dist + tau >= rootRadius) {
                        s.push(root->rightChild);
                    }
                } else {
                    if (root->rightChild != std::numeric_limits<unsigned int>::max() && dist + tau >= rootRadius) {
                        s.push(root->rightChild);
                    }
                    // At this stage, the tau value might be updated from the previous recursive call
                    if (root->leftChild != std::numeric_limits<unsigned int>::max() && dist - tau <= rootRadius) {
                        s.push(root->leftChild);
                    }
                }
            }

            std::vector<HeapItem> result;
            while (!heap_.empty()) {
                result.emplace_back(heap_.top());
                heap_.pop();
            }
            std::reverse(result.begin(), result.end());
            return result;
        }
    };

    struct MaxDistanceSearch {
        double maxDistance;
        float* ptr;
        const DiskVP* vp;
        std::set<HeapItem> heap_;

        MaxDistanceSearch(const DiskVP* vp, size_t id, double maxDistance = std::numeric_limits<double>::max());
        MaxDistanceSearch(const DiskVP* vp, float* id, double maxDistance = std::numeric_limits<double>::max());

        inline std::set<HeapItem> run() {
            std::stack<std::pair<size_t,double>> s;
            s.emplace(0, maxDistance);
            while (!s.empty()) {
                auto top = s.top();
                s.pop();
                auto root = vp->getEntryPoint(top.first);
                double rootRadius = root->radius;
                float dist = vp->ker(vp->d, (float*) vp->getPTR(top.first), ptr);
                if (dist <= maxDistance)
                    heap_.emplace(HeapItem{root->id, dist});
//                double maxDist = heap_.empty()  ? maxDepth : heap_.rbegin()->dist ;

                if ((root->leftChild == std::numeric_limits<unsigned int>::max()) &&
                    (root->rightChild == std::numeric_limits<unsigned int>::max())) {
                    continue;
                }
                double ddd = dist-rootRadius;
                if(definitelyLessThan(ddd,maxDistance) || approximatelyEqual(ddd, maxDistance)) {
                    if (root->leftChild != std::numeric_limits<unsigned int>::max() )
                        s.emplace(root->leftChild, maxDistance);
                }
                if (root->rightChild != std::numeric_limits<unsigned int>::max())
                    s.emplace(root->rightChild, maxDistance);
            }
            return heap_;
        }



//    while (heap_.size() > k) {
//        auto it = heap_.end();
//        it--;
//        heap_.erase(it);
//    }
//    if (definitelyLessThan(dist,tau)) {
//        heap_.push(HeapItem{root->id, dist});
//        while (heap_.size() > k)
//            heap_.pop();
//
//        if (heap_.size() == k)
//            tau = heap_.top().dist;
//    } else
//        // Otherwise, if they are very similar, then I could add this other one too
//    if (approximatelyEqual(dist, tau)) {
//        heap_.push(HeapItem{root->id, dist});
//        while (heap_.size() > k)
//            heap_.pop();
//
//        if (heap_.size() == k)
//            tau = heap_.top().dist;
//        tau = std::min(heap_.top().dist, tau);
//    }

//    if(d + nndist[k-1] >= mu) {
//        nn_query(vp, nd->ge, p, k, nn, nndist);
//    }

//    // Continuing with the traversal depending on the distance from the root
//    if (dist < rootRadius) {
//        if (root->leftChild != std::numeric_limits<unsigned int>::max() && dist - tau <= rootRadius) {
//            lookUpNearsetTo(root->leftChild, id, depth+1);
//        }
//
//        // At this stage, the tau value might be updated from the previous recursive call
//        if (root->rightChild != std::numeric_limits<unsigned int>::max() && dist + tau >= rootRadius) {
//            lookUpNearsetTo(root->rightChild, id, depth+1);
//        }
//    } else {
//        if (root->rightChild != std::numeric_limits<unsigned int>::max() && dist + tau >= rootRadius) {
//            lookUpNearsetTo(root->rightChild, id, depth+1);
//        }
//
//        // At this stage, the tau value might be updated from the previous recursive call
//        if (root->leftChild != std::numeric_limits<unsigned int>::max() && dist - tau <= rootRadius) {
//            lookUpNearsetTo(root->leftChild, id, depth+1);
//        }
//    }

    };


    void lookUpNearsetTo(size_t root_id, float* id, double maxDistance);
    void recursive_restruct_tree(size_t first, size_t last);

};


#endif //SIMMATCH_DISKVP_H
