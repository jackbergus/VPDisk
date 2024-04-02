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











#include "vptree/Builder.h"


#include <regex>
#include <string>
#include <iostream>

void vp_tree_example() {
    {
        Builder b1{3, "dataset/vp.bin", 3, BALANCED_ROOT_IS_HALFWAY_BLOCKADE};
        b1.write_entry_to_disk( {0,0,0});
        b1.write_entry_to_disk( {1,2,3});
        b1.write_entry_to_disk( {1.1,2.1,2.9});
        b1.write_entry_to_disk( {.9,1.9,3});
        b1.write_entry_to_disk( {3,3,3});
        b1.write_entry_to_disk( {.001,.001,.001});
        b1.write_entry_to_disk( {.2,.2,.1});
        b1.write_entry_to_disk( {0.2,0.2,0.2});
        b1.write_entry_to_disk( {0.1,0,0.3});
        b1.build();
    }

    DiskVP b1(3, "dataset/vp.bin", squared_distance);
    b1.openSortedFile();
    b1.printSortedFile(std::cout);
    DiskVP::TopKSearch mds(&b1, 4, 2);
    auto r = mds.run();
    unlink("dataset/vp.bin");
    unlink("dataset/vp.bin_idx");
}

#include <bktree/BKTreeDisk.h>

void bktree_test() {
    auto tree = BKTreeDisk::createNewDiskFile("test.bin", 20, 10, (size_t)distance_method::STRING_DISTANCE, 5);
    auto fptr = [](void* ptr) {
        std::string s{(char*)ptr};
        return s;
    };
    tree->add(0, (void*)"help", 5);
    tree->add(1, (void*)"hell", 5);
    tree->add(2, (void*)"hello", 6);
    tree->add(3, (void*)"loop", 5);
    tree->add(4, (void*)"helps", 6);
    tree->add(5, (void*)"shell", 6);
    tree->add(6, (void*)"helper", 7);
    tree->add(7, (void*)"troop", 6);
    tree->add(8, (void*)"genoveffo", 10);
    tree->add(9, (void*)"genova", 7);
    tree->add(10, (void*)"genoa", 6);
    tree->add(11, (void*)"genoveffa", 10);
    tree->finalise_insertion();
    tree->print(std::cout, fptr);
}

int main() {
    bktree_test();
}