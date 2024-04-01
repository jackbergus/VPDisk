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











#include "include/Builder.h"


#include <regex>
#include <string>
#include <iostream>

int main() {
    {
        Builder b1{3, "dataset/vp.bin", -1, true};
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
    b1.printSortedFile(std::cout);
    DiskVP::TopKSearch mds(&b1, 4, 2);
   auto r = mds.run();
    unlink("dataset/vp.bin");
    unlink("dataset/vp.bin_idx");
}