project(benchmark_polymorphic)

add_benchmark(benchmark_polymorphic
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_polymorphic.cpp
        )
add_size_benchmark(size_benchmark_polymorphic "cereal_binary;cereal_extendable;boost"
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_polymorphic_size.cpp
        )