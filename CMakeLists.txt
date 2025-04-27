cmake_minimum_required(VERSION 3.23.1 FATAL_ERROR)
project(see)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--copy-dt-needed-entries")

find_package(LAPACK REQUIRED)
find_package(BLAS REQUIRED)

# Include directories
add_subdirectory(third_party)
add_subdirectory(src)


# Add executable
add_executable(test_compass_ring ./tests/test_compass_ring.cpp ./src/cluster.cpp ./src/node.cpp ./src/graph.cpp ./tests/utils/config_parser.cpp)
target_link_libraries(test_compass_ring PRIVATE faiss oram_library argmap rapidjson)
target_link_libraries(test_compass_ring PRIVATE ${LAPACK_LIBRARIES} ${BLAS_LIBRARIES} )

add_executable(test_compass_tp ./tests/test_compass_ring_tp.cpp ./src/cluster.cpp ./src/node.cpp ./src/graph.cpp ./tests/utils/config_parser.cpp)
target_link_libraries(test_compass_tp PRIVATE faiss oram_library argmap rapidjson)
target_link_libraries(test_compass_tp PRIVATE ${LAPACK_LIBRARIES} ${BLAS_LIBRARIES} )

add_executable(compass_init ./tests/eval/compass_init.cpp ./src/cluster.cpp ./src/node.cpp ./src/graph.cpp ./tests/utils/config_parser.cpp)
target_link_libraries(compass_init PRIVATE faiss oram_library argmap rapidjson)
target_link_libraries(compass_init PRIVATE ${LAPACK_LIBRARIES} ${BLAS_LIBRARIES} )

add_executable(obi_init ./tests/baseline/obi_init.cpp ./tests/baseline/baseline_utils.cpp)
target_link_libraries(obi_init PRIVATE faiss oram_library argmap rapidjson)
target_link_libraries(obi_init PRIVATE ${LAPACK_LIBRARIES} ${BLAS_LIBRARIES} )

add_executable(test_obi ./tests/baseline/test_obi.cpp ./tests/baseline/baseline_utils.cpp)
target_link_libraries(test_obi PRIVATE faiss oram_library argmap rapidjson)
target_link_libraries(test_obi PRIVATE ${LAPACK_LIBRARIES} ${BLAS_LIBRARIES} )

add_executable(obi_accuracy ./tests/baseline/obi_accuracy.cpp ./tests/baseline/baseline_utils.cpp)
target_link_libraries(obi_accuracy PRIVATE argmap rapidjson evp)