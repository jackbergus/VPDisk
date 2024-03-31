#cmake_minimum_required(VERSION 3.23.1)
project(simmatch)
set(FAISS_ENABLE_GPU OFF)
set(BUILD_TESTING OFF)
#add_subdirectory(submodules/faiss)
#include_directories(submodules/faiss/faiss)
set(CMAKE_CXX_STANDARD 23)
include_directories(include)
#
#add_library(vptree OBJECT submodules/vptree/inc/vptree.h
#        submodules/vptree/src/vptree_sequential.cpp)

add_executable(simmatch main.cpp src/FAISSBatch.cpp include/FAISSBatch.h include/disk_vp_node_header.h src/DiskVP.cpp include/DiskVP.h src/mmapFile.cpp include/mmapFile.h src/Builder.cpp include/Builder.h)
target_link_libraries(simmatch  stdc++fs)
