/*
 * main.cpp
 * This file is part of FAISSExample
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



#include <cstdio>
#include <vector>
//std::vector<std::vector<float>> vdd{{0,0,0},
//                                     {1,2,3},
//                                     {1.1,2.1,2.9},
//                                     {.9,1.9,3},
//                                     {3,3,3},
//                                     {.001,.001,.001}};

#include <random>
//#include "faiss/IndexIVFFlat.h"
//#include "faiss/index_factory.h"
//#include "faiss/index_io.h"
//
//
//#include <faiss/IndexFlat.h>
//#include <faiss/IndexIVFPQ.h>
//#include <sys/stat.h>

//using idx_t = faiss::idx_t;


#include <sys/time.h>
#include <iostream>

//double elapsed() {
//    struct timeval tv;
//    gettimeofday(&tv, nullptr);
//    return tv.tv_sec + tv.tv_usec * 1e-6;
//}

#include <filesystem>
#include <cstring>
#include <queue>



/*
 * mmapFile.h
 * This file is part of varsorter
 *
 * Copyright (C) 2017 - Giacomo Bergami
 *
 * Created on 10/08/17
 *
 * varsorter is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * varsorter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with varsorter. If not, see <http://www.gnu.org/licenses/>.
 */






#if 0


#include <faiss/invlists/OnDiskInvertedLists.h>

struct FAISSHDD {
//    const char* index_key;
    std::string ivx;
    faiss::Index* index;
    const std::filesystem::path& parent_folder;
    int prev;
    int d;
    int idx_block;
    std::vector<std::string> indexes;
    bool firstCluster = true;
    double t0;

    FAISSHDD(const std::filesystem::path& parent_folder, int d, int nlist) : index{nullptr}, parent_folder{parent_folder}, prev{0}, d{d}, idx_block{0} {
        ivx = "IVF"+std::to_string(nlist)+",Flat";

        t0 = elapsed();
        size_t nt;
        printf("[%.3f s] Loading train set\n", elapsed() - t0);
//        float* xt = fvecs_read("sift1M/sift_learn.fvecs", &d, &nt);
        index = faiss::index_factory(d, ivx.c_str());
    }

    void addBatch(const FAISSBatch& batch) {
        if (firstCluster) {
            printf("[%.3f s] Training on %ld vectors\n", elapsed() - t0, batch.nb);
            index->train(batch.nb, batch.xb);
            auto p = (parent_folder / "trained.index");
            faiss::write_index(index, p.c_str());
            firstCluster = false;
        }
        auto p = (parent_folder / "trained.index");
        auto idx = faiss::read_index(p.c_str());
        idx_t* I = new idx_t[batch.nb];
        for (int i = 0; i<batch.nb; i++) {
            I[i] = i+prev;
        }
        prev += batch.nb;
        idx->add_with_ids(batch.nb, batch.xb, I);
        auto p2 = (parent_folder / ("block_" + std::to_string(idx_block) + ".index"));
        indexes.emplace_back(p2.string());
        faiss::write_index(idx, p2.c_str());
        delete I;
        idx_block++;
    }

    void merge(const std::filesystem::path& p) {
//        std::string p2 = p.string();
// Gather all partitions into this list.
        std::vector<faiss::InvertedLists *> invlists;

        // Create inv indexes for all partitions.
        for (const std::string& index_file : indexes) {
            faiss::IndexIVFFlat* index = (faiss::IndexIVFFlat*) faiss::read_index(
                    index_file.c_str(), faiss::IO_FLAG_MMAP); // IO_FLAG_ONDISK_SAME_DIR);
            invlists.push_back(index->invlists);

            // Avoid that the invlists get deallocated with the index. Then safely delete the index.
            // This prevents all indexes from getting loaded into memory for merger.
            index->own_invlists = false;
            delete index;
        }

        auto pt = (parent_folder / "trained.index");
        // The trained index into which we'll merge all partitions.
        faiss::IndexIVFFlat* index =(faiss::IndexIVFFlat*) faiss::read_index(pt.c_str());
        auto ondisk = new faiss::OnDiskInvertedLists(index->nlist, index->code_size,
                                                     "/tmp/merged_index.ivfdata");
        // Okay, let's now merge all inv indexes into a single ivfdata file.
        const faiss::InvertedLists **ils = (const faiss::InvertedLists**) invlists.data();
        auto ondisk_merged_ntotal = ondisk->merge_from(ils, invlists.size(), true /* verbose */);
        faiss::write_index(index, p.c_str());
        delete index;
        delete this->index;
        this->index = nullptr;
        this->index = faiss::read_index(p.c_str());
    }

    std::vector<float> get(size_t idx) {
        float* F = new float[d];
        this->index->reconstruct(idx,F);
        std::vector<float> result;
        result.reserve(d);
        for (size_t i = 0; i<d; i++)
            result.emplace_back(F[i]);
        delete F;
        return result;
    }

    void search(FAISSBatch& q, int k) {
        int nq = (int)q.nb;
        { // search xq
            idx_t* I = new idx_t[k * nq];
            float* D = new float[k * nq];
            memset(I, 0, sizeof(idx_t)*k*nq);
            memset(D, 0, sizeof(float)*k*nq);

//            index->nprobe = 10;
            index->search(nq, q.xb, k, D, I);

            printf("I=\n");
            for (int i = 0; i < nq; i++) {
                for (int j = 0; j < k; j++)
                    printf("%5zd ", I[i * k + j]);
                printf("\n");
            }

            printf("D=\n");
            for (int i = 0; i < nq; i++) {
                for (int j = 0; j < k; j++)
                    printf("%f ", D[i * k + j]);
                printf("\n");
            }

            delete[] I;
            delete[] D;
        }
    }

};


int main() {
    std::filesystem::path current = "dataset/test";
    auto merged = current / "merged";

    FAISSHDD hd{current, 3, 3};
    {
        FAISSBatch b1(3, 3);
        b1.addToBatch(0, {0,0,0});
        b1.addToBatch(1, {1,2,3});
        b1.addToBatch(2, {1.1,2.1,2.9});
        hd.addBatch(b1);
    }
    {
        FAISSBatch b2(3, 3);
        b2.addToBatch(0, {.9,1.9,3});
        b2.addToBatch(1, {3,3,3});
        b2.addToBatch(2, {.001,.001,.001});
        hd.addBatch(b2);
    }
    hd.merge(merged);

    auto v1 = hd.get(0);
    auto v2 = hd.get(1);
    auto v3 = hd.get(2);
    auto v4 = hd.get(3);

    {
        FAISSBatch b1(3, 2);
        b1.addToBatch(0, {0.00001,0.00001,0.00001});
        b1.addToBatch(0, {3.1,3.1,3.1});
        hd.search(b1, 2);
    }
}
#endif

/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstdio>
#include <cstdlib>
#include <random>
#include <fstream>


//#include "vptree.h"



//struct disk_vp_node {
//    unsigned int id; ///<@ id associated to the current node (i.e., insertion order)
//    float radius;   ///<@ radius that, in the standard definition, provides the boundary between the left and right nodes
//    unsigned int leftChild; // = std::numeric_limits<unsigned int>::max(); ///<@ setting the leftChild to nullptr
//    unsigned int rightChild; // = std::numeric_limits<unsigned int>::max(); ///<@ setting the rightChild to nullptr
//    bool isLeaf; // = false; ///<@ nevertheless, each node is not necessairly a leaf
//    unsigned char pt[];
//};

#include <functional>
#include "include/Builder.h"






//long long option_3(std::size_t bytes)
//{
//    std::vector<uint64_t> data = GenerateData(bytes);
//
//    std::ios_base::sync_with_stdio(false);
//    auto startTime = std::chrono::high_resolution_clock::now();
//    auto myfile = std::fstream("file.binary", std::ios::out | std::ios::binary);
//    myfile.write((char*)&data[0], bytes);
//    myfile.close();
//    auto endTime = std::chrono::high_resolution_clock::now();
//
//    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
//}


//struct DoubleKernel : public DistanceFunction<std::vector<float>> {
//    double distance(const std::vector<float> &a, const std::vector<float> &b) override {
//        float s = 0;
//        for (size_t i = 0, N = std::min(a.size(), b.size()); i<N; i++) {
//            float f=a[i]- b[i];
//            s+= (f*f);
//        }
//        return s;
//    }
//};




int main() {
    {
        Builder b1{3, "dataset/vp.bin"};
        b1.write_entry_to_disk( {0,0,0});
        b1.write_entry_to_disk( {1,2,3});
        b1.write_entry_to_disk( {1.1,2.1,2.9});
        b1.write_entry_to_disk( {.9,1.9,3});
        b1.write_entry_to_disk( {3,3,3});
        b1.write_entry_to_disk( {.001,.001,.001});
        b1.build();
    }


    DiskVP b1(3, "dataset/vp.bin", squared_distance);
    b1.openSortedFile();
   auto r = b1.topkSearchIdx(4ULL,5);
    unlink("dataset/vp.bin");
    unlink("dataset/vp.bin_idx");
}