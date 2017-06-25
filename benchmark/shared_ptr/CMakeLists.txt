project(benchmark_shared_ptr)

add_benchmark(benchmark_shared_ptr
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_shared_ptr.cpp
        )
add_size_benchmark(size_benchmark_shared_ptr "cereal_binary;cereal_extendable;boost"
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_shared_ptr_size.cpp
        )