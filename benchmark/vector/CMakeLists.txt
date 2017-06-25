project(benchmark_vector)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS Vector.proto)
ignore_warnings_in_generated_files(${PROTO_SRCS})

add_benchmark(benchmark_vector
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_vector.cpp
        ${PROTO_SRCS}
        )

add_size_benchmark(size_benchmark_vector "cereal_binary;cereal_extendable;boost;protobuf"
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_vector_size.cpp
        #${PROTO_SRCS}
        )