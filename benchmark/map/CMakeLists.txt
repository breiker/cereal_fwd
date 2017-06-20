project(benchmark_map)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS Map.proto)
ignore_warnings_in_generated_files(${PROTO_SRCS})

add_benchmark(benchmark_map
        "${CMAKE_CURRENT_LIST_DIR}/benchmark_map .cpp"
        ${PROTO_SRCS}
        )
