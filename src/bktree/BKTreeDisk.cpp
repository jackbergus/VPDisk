//
// Created by giacomo on 02/04/24.
//

#include "bktree/BKTreeDisk.h"

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

bool create_empty_file(const std::string& filename, size_t length) {
    int fd;
    int result;
    fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_EXCL, (mode_t)0600);
    if (fd == -1) {
        perror("Error opening file for writing");
        return false;
    }

    /* Stretch the file size.
     * Note that the bytes are not actually written,
     * and cause no IO activity.
     * Also, the diskspace is not even used
     * on filesystems that support sparse files.
     * (you can verify this with 'du' command)
     */
    result = lseek(fd, length-1, SEEK_SET);
    if (result == -1) {
        close(fd);
        perror("Error calling lseek() to 'stretch' the file");
        return false;
    }

    /* write just one byte at the end */
    result = write(fd, "", 1);
    if (result < 0) {
        close(fd);
        perror("Error writing a byte at the end of the file");
        return false;
    }

    /* do other things here */
    close(fd);
    return true;
}

std::unique_ptr<BKTreeDisk>
BKTreeDisk::createNewDiskFile(const std::string &filename, size_t maximum_node_size, size_t maximum_data_storage,
                              size_t distance_method, size_t maximum_discrete_distance) {
    BKTReeHeader header;
    header.total_node_size = maximum_node_size;
    header.data_value_storage_size = maximum_data_storage;
    header.distance_method = distance_method;
    header.maximum_discrete_distance = maximum_discrete_distance;
    header.last_free_offset_pointer = sizeof(BKTReeHeader);
    auto total_size = sizeof(BKTReeHeader) + maximum_node_size * (ENTRY_BLOCK_RECORD_SIZE(&header));
    if (!create_empty_file(filename, total_size))
        return nullptr;
    if (!create_empty_file(filename+"_primary_index.bin", maximum_node_size*sizeof(size_t)))
        return nullptr;
    if (!create_empty_file(filename+"_distinct_roots.bin", (1+maximum_node_size)*sizeof(size_t)))
        return nullptr;
    auto ptr = std::make_unique<BKTreeDisk>(filename);
    *((BKTReeHeader*)ptr->memory) = header;
    return ptr;
}

template<typename T>
typename T::size_type LevenshteinDistance(const T &source, const T &target) {
    if (source.size() > target.size()) {
        return LevenshteinDistance(target, source);
    }

    using TSizeType = typename T::size_type;
    const TSizeType min_size = source.size(), max_size = target.size();
    std::vector<TSizeType> lev_dist(min_size + 1);

    for (TSizeType i = 0; i <= min_size; ++i) {
        lev_dist[i] = i;
    }

    for (TSizeType j = 1; j <= max_size; ++j) {
        TSizeType previous_diagonal = lev_dist[0], previous_diagonal_save;
        ++lev_dist[0];

        for (TSizeType i = 1; i <= min_size; ++i) {
            previous_diagonal_save = lev_dist[i];
            if (source[i - 1] == target[j - 1]) {
                lev_dist[i] = previous_diagonal;
            } else {
                lev_dist[i] = std::min(std::min(lev_dist[i - 1], lev_dist[i]), previous_diagonal) + 1;
            }
            previous_diagonal = previous_diagonal_save;
        }
    }

    return lev_dist[min_size];
}


static inline size_t editDistance(char* a,char* b)
{
    std::string sa{a}, sb{b};
    size_t ld = LevenshteinDistance(sa, sb);
    return ld;
}

size_t BKTreeDisk::calculate_distance(void *srcDatum, void *dstDatum) const {
    switch ((distance_method)((BKTReeHeader*)memory)->distance_method) {
        case INT_DISTANCE: {
            size_t l = *(size_t*)srcDatum;
            size_t r = *(size_t*)dstDatum;
            return l<r ? r-l : l-r;
        }

        case STRING_DISTANCE: {
            return editDistance((char*)srcDatum, (char*)dstDatum);
        }
            break;
    }
    return -1;
}

#include <stxxl/vector>

void BKTreeDisk::finalise_insertion() {
    stxxl::deque<std::tuple<size_t, void*, int>> Q2;
    stxxl::vector<size_t> W;
    size_t orig, after;
    bool firstIteration = true;
    do {
        if (firstIteration) {
            // If this is the first insertion, we first try to insert the elements in Q
            // Within the tree after restructuring

        } else {
            // Otherwise, if I am still here it is because some nodes cannot be inserted.
            // So, I need to try and add a new root.
            if (!Q.empty()) {
                auto tp = Q.front();
                addRoot(std::get<0>(tp), std::get<1>(tp), std::get<2>(tp));
                Q.pop_front();
                if (Q.empty())
                    return; // If Q is empty, I have no other elements to insert: just stop
            }
        }

        // First, attempting at reconstructing the graphs, as much as possible
        // Or, re-attempting at inserting the elements for new roots
        do {
            // Obtaining the number of the originally missed-insertion elements,
            // as they do not follow the chain with the root
            orig = Q.size();
            // For each element within the missing elements
            while (!Q.empty()) {
                auto x = Q.front();
                // Attempting at re-inserting one of the previously-failed attempts
                // If not successful, try another time by adding it in Q2 now.
                // Adding only on the lastly inserted root only if this is the non-first iteration,
                // On which we re-attempt at inserting all the elements back again, after being inserted
                add(std::get<0>(x), std::get<1>(x), std::get<2>(x), Q2, !firstIteration);
                Q.pop_front();
            }
            // Obtaining how many elements are still not inserted
            after = Q2.size();
            // Replacing the depleted queue with the remaining elements
            Q.swap(Q2);
        } while (orig != after); // Continue this waltz until I do no further updates

        firstIteration = false;
    } while (!Q.empty());
}
