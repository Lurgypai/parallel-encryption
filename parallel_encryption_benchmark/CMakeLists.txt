cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(paralle_encryption_benchmark)

add_subdirectory("./encryption_wrapper")

find_package(MPI REQUIRED)
# Add encryption library headers
find_package(GCrypt REQUIRED)
find_package(Nettle REQUIRED)
find_package(GPGError REQUIRED)

add_executable(bench "main.cpp")
target_link_libraries(bench enc-lib MPI::MPI_C GCrypt::GCrypt Nettle::Nettle GPGError::GPGError)
target_include_directories(bench PUBLIC "./encryption_wrapper")

