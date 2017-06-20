project(benchmark_vector)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS Vector.proto)
ignore_warnings_in_generated_files(${PROTO_SRCS})

add_benchmark(benchmark_vector
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_vector.cpp
        ${PROTO_SRCS}
        )
