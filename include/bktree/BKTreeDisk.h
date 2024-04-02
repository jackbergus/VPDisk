//
// Created by giacomo on 02/04/24.
//

#ifndef SIMMATCH_BKTREEDISK_H
#define SIMMATCH_BKTREEDISK_H

enum distance_method {
    INT_DISTANCE = 0,
    STRING_DISTANCE = 1
};

#include "mmapFile.h"
#include "PrimaryIndexInformation.h"
#include "BKTReeHeader.h"
#include <memory>

#define GET_MAXIMUM_DISCRETE_DISTANCE(HEADER)           (((BKTReeHeader*)(HEADER))->maximum_discrete_distance)
#define GET_MAXIMUM_DATUM_OCCUPATION(HEADER)            (((BKTReeHeader*)(HEADER))->data_value_storage_size)
#define ENTRY_BLOCK_RECORD_SIZE(HEADER)                 ((sizeof(size_t)*((GET_MAXIMUM_DISCRETE_DISTANCE(HEADER)+1)))+(GET_MAXIMUM_DATUM_OCCUPATION(HEADER)))
#define ENTRY_NODES_OFFSET(HEADER)                      (((char*)(HEADER))+sizeof(BKTReeHeader))
#define ENTRY_NODE_BLOCK(HEADER, id)                    ((ENTRY_NODES_OFFSET(HEADER))+(ENTRY_BLOCK_RECORD_SIZE(HEADER))*(id))

#define ENTRY_BLOCK_OBJECT_ID(BLOCK)                    (*((size_t*)(BLOCK)))
#define ENTRY_BLOCK_DATUM(BLOCK)                        ((void*)(((char*)(BLOCK))+(sizeof(size_t))))
#define ENTRY_DISTANCE_TO_CHILD_MAP(BLOCK,HEADER)       ((size_t*)(((char*)(BLOCK))+(sizeof(size_t))+(GET_MAXIMUM_DATUM_OCCUPATION(HEADER))))

#include <functional>
#include "stxxl/deque"

class BKTreeDisk {

    unsigned long size;
    mmap_file fd;
    char* memory;

    unsigned long pidx_size;
    mmap_file primary_fd;
    PrimaryIndexInformation* primary;

    unsigned long roots_size;
    mmap_file roots_fd;
    size_t* roots;


public:
    BKTreeDisk(const std::string& filename) {
        memory = (char*)mmapFile(filename, &size, &fd);
        primary = (PrimaryIndexInformation*) mmapFile(filename+"_primary_index.bin", &pidx_size, &primary_fd);
        roots = (size_t*) mmapFile(filename+"_distinct_roots.bin", &roots_size, &roots_fd);
    }

    virtual ~BKTreeDisk() {
        if (memory) {
            mmapClose((void*)memory, &fd);
            memory = nullptr;
        }
        if (primary) {
            mmapClose((void*)primary, &primary_fd);
            primary = nullptr;
        }
        if (roots) {
            mmapClose((void*)roots, &roots_fd);
            roots = nullptr;
        }
    }

    static std::unique_ptr<BKTreeDisk> createNewDiskFile(const std::string& filename,
                                                         size_t maximum_node_size,
                                                         size_t maximum_data_storage,
                                                         size_t distance_method,
                                                         size_t maximum_discrete_distance);

    /**
     * Resolves the entry point block for each entry being stored as a node or as a child node for the current layer
     * @param id        VPTree node id that needs to be resolved into a node
     * @return          If the node has been inserted or associated to a node for duplication, the pointer to the entry, otherwise, nullptr
     */
    char* resolveObjectIdToBKTreeEntry(size_t id) const {
        if (!primary)
            return nullptr;
        if (!primary[id].BKTreeDiskOffset)
            return nullptr;
        return (memory+primary[id].BKTreeDiskOffset);
    }

    /**
     * Uses the pre-configured distance metric to compute the distance across objects
     * @param srcDatum
     * @param dstDatum
     * @return
     */
    size_t calculate_distance(void* srcDatum, void* dstDatum) const;

    inline size_t calculate_distance_with_srcId(size_t srcId, void* dstDatum) const {
        const auto block = ENTRY_NODE_BLOCK(memory, srcId);
        const auto datumOffset = ENTRY_BLOCK_DATUM(block);
        return calculate_distance(datumOffset, dstDatum);
    }

    inline size_t calculate_distance_with_srcOffset(size_t srcOffset, void* dstDatum) const {
        const auto block = memory+srcOffset;
        const auto datumOffset = ENTRY_BLOCK_DATUM(block);
        return calculate_distance(datumOffset, dstDatum);
    }

    inline size_t calculate_distance_with_Block(char* block, void* dstDatum) const {
        const auto datumOffset = ENTRY_BLOCK_DATUM(block);
        return calculate_distance(datumOffset, dstDatum);
    }

    size_t missingValues = 0;
    char* add(size_t id, void* datum_representation, int len) {
        auto ptr = add(id, datum_representation, len, Q);
        if (!ptr)
            missingValues++;
        return ptr;
    }

    /**
     * While attempting at writing the file for the first time, there might be some nodes that were not inserted.
     * For this, we might end adding other nodes, and adding new roots for this.
     */
    void finalise_insertion();

    void print(std::ostream& out, const std::function<std::string(void*)>& datum_serializer) {
        if (!memory) return;
        out << *((BKTReeHeader*)memory) << std::endl;
        out << "RECORD:" << std::endl
            << "-------" << std::endl;
        constexpr size_t h = sizeof(BKTReeHeader);
        auto ptr = memory+h;
        auto beg = ptr;
        auto end = memory+((BKTReeHeader*)memory)->last_free_offset_pointer;
        auto nChildren = ((BKTReeHeader*)memory)->maximum_discrete_distance;
        auto size_record = ENTRY_BLOCK_RECORD_SIZE(memory);
        while (ptr != end) {
            auto offset = ((size_t)(ptr-beg)) / size_record;
            out << std::endl << "- Record #" << offset << std::endl;
            auto datum = ENTRY_BLOCK_DATUM(ptr);
            out << "Data: " << datum_serializer(datum) << std::endl;
            auto children = ENTRY_DISTANCE_TO_CHILD_MAP(ptr,memory);
            for (size_t i = 0; i<nChildren; i++) {
                if (children[i]) {
                    out << " * distance = " << i << ", offset= "<< (children[i]-h)/size_record << std::endl;
                }
            }
            ptr+=size_record;
        }
    }

private:
    // Keeping track of the new offset, just in case that I need to insert a new element
    std::pair<char*, size_t> allocateNewNodeEntry(size_t id, void* datum_representation, int len) {
        size_t newNodeOffset = ((BKTReeHeader*)memory)->last_free_offset_pointer;
        char* ptr = (memory + ((BKTReeHeader*)memory)->last_free_offset_pointer);
        // 1. Determining the node ID
        *(size_t*)(ptr) = id;
        // 2. Serialising the datum information
        memcpy(ptr+sizeof(size_t), datum_representation, len);
        // 3. By default, the remaining bits are set up to zero, so we do not need to initialise this (assumption for Linux)

        // Setting up the pointer to the next free element in the header
        *(size_t*)(memory+offsetof(BKTReeHeader, last_free_offset_pointer)) = ENTRY_BLOCK_RECORD_SIZE(memory) + newNodeOffset;
        // Update the primary index to point to this new cell
        *(size_t*)(((char*)primary)+sizeof(PrimaryIndexInformation)*id) = newNodeOffset;
//        primary[id] = newNodeOffset;
        // TODO: secondary index inverted idx, mapping the current offset id to all the nodes associated to it.
        return {ptr, newNodeOffset};
    }

    // Adding an explicit new root
    inline char* addRoot(size_t id, void* datum_representation, int len) {
        *roots = *roots+1;
        auto cp = allocateNewNodeEntry(id, datum_representation, len);
        *(roots+*roots) = cp.second;
        return cp.first;
    }

    stxxl::deque<std::tuple<size_t, void*, int>> Q;

    /**
     * Attempts at writing some data on disk
     *
     * @param id                    ID of the object to be inserted
     * @param datum_representation  Value to be copied
     * @param len                   Length associated to the datum to be copied
     * @param Q                     Queue where to place missing insertions
     * @param doLastRoot            If this is set to true (false by default), it only attempts at inserting objects
     *                              from the last root. This works under the assumption that, if I have already tried
     *                              to add elements to no avail, then it makes less sense to add such elements from the
     *                              other previously-attempted ones!
     * @return                      Pointer to the inserted region
     */
    char* add(size_t id,
              void* datum_representation,
              int len,
              stxxl::deque<std::tuple<size_t, void*, int>>& Q,
              bool doLastRoot = false) {
        // if this is the first entry, then just use allocateNewNodeEntry;
        if ((((BKTReeHeader*)memory)->last_free_offset_pointer == sizeof(BKTReeHeader)) && (!(*roots))) {
            return addRoot(id, datum_representation, len);
        }
        // If I am inserting objects, it is actually likely that these objects should be tested from the lastly inserted root
        for (size_t i = *roots-1; i != (size_t)-1; i--) {
            // Otherwise, I need to perform a continuous recursive visit of the graph from secondary memory, starting from the root
            char* root = memory+roots[i+1];
            do {
                size_t distance = calculate_distance_with_Block(root, datum_representation);
                if (!distance) {
                    // If this has the zero distance with the current node, udpate the secondary index to add this
                    // current datum with the ID as its sibling
                    // TODO: secondary index update
                    return root;
                } else {
                    // Otherwise, I have a non-zero distance to the element
                    // First, I need to check whether this is within the scope of the maximum distance
                    if (distance < GET_MAXIMUM_DISCRETE_DISTANCE(memory)) {
                        auto root_map = ENTRY_DISTANCE_TO_CHILD_MAP(root, memory);
                        if (root_map[distance] == 0) {
                            // Inserting the element if this is missing
                            auto cp = allocateNewNodeEntry(id, datum_representation, len);
                            // Setting the newly-created node as a child of the current node
                            root_map[distance] = cp.second;
                            // Adding this information into the primary index, for remembering who the parent is,
                            // if we need later on to backtrack navigate
                            *(size_t*)(((char*)primary)+sizeof(PrimaryIndexInformation)*id+offsetof(PrimaryIndexInformation,ParentOffset)) = ENTRY_BLOCK_OBJECT_ID(root);
                            return cp.first;
                        } else {
                            // If there was an already existing element with the same distance, then recursively attempt
                            // to add this to this child
                            root = memory+root_map[distance];
                            // Now, keeping iterating
                        }
                    } else {
                        // Try with the next root, if any
                        break;
                    }
                }
            } while (root != nullptr);
            if (doLastRoot) break;
        }
        // TODO: what is the fall-back measure if I have no place to put the element within the map?
        //       If I have some nodes, I can try to add it to the first nearest child, and then continue,
        //       but what if there is no nearest child I can use as a support?
        //       Do I need an additional data structure for keeping all the additional roots?
//                        std::get<0>(Qrecord) = id;
//                        std::get<1>(Qrecord) = datum_representation;
//                        std::get<2>(Qrecord) = len;
        Q.push_back(std::make_tuple(id, datum_representation, len));
        return nullptr; // Remarking that now
    }

};



#endif //SIMMATCH_BKTREEDISK_H
