project(benchmark_unique_ptr)

add_benchmark(benchmark_unique_ptr
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_unique_ptr.cpp
        )

add_size_benchmark(size_benchmark_unique_ptr "cereal_binary;cereal_extendable;boost"
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_unique_ptr_size.cpp
        )