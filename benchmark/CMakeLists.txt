project(benchmark)


if (BENCHMARK_PERF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
endif ()


# download and setup celero
include(ExternalProject)

message(STATUS "Configuring Celero")
ExternalProject_Add(celero
        GIT_REPOSITORY https://github.com/DigitalInBlue/Celero.git
        UPDATE_COMMAND ""
        INSTALL_DIR ${CMAKE_BINARY_DIR}
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_BINARY_DIR}
        -DCELERO_COMPILE_DYNAMIC_LIBRARIES:BOOL=OFF
        -DCELERO_ENABLE_EXPERIMENTS:BOOL=OFF
        -DCELERO_ENABLE_FOLDERS:BOOL=OFF)
ExternalProject_Get_Property(celero SOURCE_DIR)
ExternalProject_Get_Property(celero INSTALL_DIR)
add_library(libcelero UNKNOWN IMPORTED)
set_property(TARGET libcelero
        PROPERTY
        IMPORTED_LOCATION_DEBUG
        "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}celerod${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_property(TARGET libcelero
        PROPERTY
        IMPORTED_LOCATION_RELEASE
        "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}celero${CMAKE_STATIC_LIBRARY_SUFFIX}")

INCLUDE_DIRECTORIES(${INSTALL_DIR}/include)
#file(MAKE_DIRECTORY "${INSTALL_DIR}/include")
#set_target_properties(libcelero PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${INSTALL_DIR}/include")

add_dependencies(libcelero celero)
# end celero

#### download and setup google benchmark ####
message(STATUS "Configuring google benchmark")
ExternalProject_Add(gbenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        UPDATE_COMMAND ""
        INSTALL_DIR ${CMAKE_BINARY_DIR}
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_BINARY_DIR}
        -DBENCHMARK_ENABLE_TESTING:BOOL=OFF
        -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS})
ExternalProject_Get_Property(gbenchmark SOURCE_DIR)
ExternalProject_Get_Property(gbenchmark INSTALL_DIR)
add_library(libgbenchmark UNKNOWN IMPORTED)
set_property(TARGET libgbenchmark
        PROPERTY
        IMPORTED_LOCATION_DEBUG
        "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}benchmark${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_property(TARGET libgbenchmark
        PROPERTY
        IMPORTED_LOCATION_RELEASE
        "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}benchmark${CMAKE_STATIC_LIBRARY_SUFFIX}")

INCLUDE_DIRECTORIES(${INSTALL_DIR}/include)
#file(MAKE_DIRECTORY "${INSTALL_DIR}/include")
#set_target_properties(libcelero PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${INSTALL_DIR}/include")

add_dependencies(libgbenchmark gbenchmark)
#### end google benchmark ####

#### Protocol buffers
find_package(Protobuf)

if (NOT PROTOBUF_FOUND)
    message(FATAL_ERROR "Protocol buffers not found")
endif ()

#### Threads
find_package(Threads)
### Cereal
set(BENCH_CEREAL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../include")

### C++11
#if (NOT MSVC)
#    #set(CMAKE_CXX_FLAGS "-Wall -Werror -Wno-gnu-statement-expression -g -Wextra -Wshadow -pedantic ${CMAKE_CXX_FLAGS}")
#    set(CMAKE_CXX_FLAGS "-Wall -Werror -g -Wextra -Wshadow -pedantic ${CMAKE_CXX_FLAGS}")
#    if (CMAKE_VERSION VERSION_LESS 3.1)
#        set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
#    else ()
#        if (NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD STREQUAL "98")
#            set(CMAKE_CXX_STANDARD 11)
#        endif ()
#        set(CMAKE_CXX_STANDARD_REQUIRED ON)
#    endif ()
#endif ()


macro(add_benchmark target_name)
    add_executable(${target_name} ${ARGN})

    target_link_libraries(${target_name} PUBLIC libgbenchmark)
    target_link_libraries(${target_name} PRIVATE common_benchmark)

    target_link_libraries(${target_name} PUBLIC ${PROTOBUF_LIBRARIES})
    target_include_directories(${target_name} PRIVATE ${PROTOBUF_INCLUDE_DIRS})

    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

    target_include_directories(${target_name} PUBLIC ${BENCH_CEREAL_INCLUDE_DIR})

    target_include_directories(${target_name} PRIVATE SYSTEM ${Boost_INCLUDE_DIRS})
    target_link_libraries(${target_name} PUBLIC ${Boost_LIBRARIES})
endmacro()

# source file properties are local for cmake file so it has to be macro, not function
macro(ignore_warnings_in_generated_files)
    foreach (generated_file ${ARGN})
        set_source_files_properties(${generated_file} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
    endforeach ()
endmacro()

#add_definitions(-DCEREAL_THREAD_SAFE=1)
#add_definitions(-DTHREADED_BENCHMARK)


#ADD_DEFINITIONS(-DCELERO_BENCHMARK)
set(libraries
        boost_serialization
        cereal
        protobuf)
set(time_benchmarks
        integer_class
        primitive_vector)

add_subdirectory(common)


add_subdirectory(integer_class)
add_subdirectory(vector)
add_subdirectory(map)
add_subdirectory(unique_ptr)
add_subdirectory(shared_ptr)
add_subdirectory(polymorphic)


ADD_CUSTOM_TARGET(benchmarks
        DEPENDS
        benchmark_integer_class
        benchmark_vector
        benchmark_map
        benchmark_unique_ptr
        benchmark_shared_ptr
        benchmark_polymorphic
        )