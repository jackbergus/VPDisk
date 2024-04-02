/*
 * main.cpp
 * This file is part of DiskVP
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
//
// Created by giacomo on 02/04/24.
//

#include "bktree/BKTReeHeader.h"

BKTReeHeader::BKTReeHeader() : total_node_size{0}, data_value_storage_size{0}, distance_method{0}, maximum_discrete_distance{0}, last_free_offset_pointer{0}  {};
