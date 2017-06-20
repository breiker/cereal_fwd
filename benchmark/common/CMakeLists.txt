
add_library(common_benchmark STATIC
        ${CMAKE_CURRENT_LIST_DIR}/integer_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/utils.cpp)
target_include_directories(common_benchmark PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(common_benchmark PUBLIC libgbenchmark)
