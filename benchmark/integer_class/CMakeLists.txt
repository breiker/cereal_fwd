project(benchmark_integer_class)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS IntegerClass.proto)
ignore_warnings_in_generated_files(${PROTO_SRCS})

add_benchmark(benchmark_integer_class
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_integer_class.cpp
        ${PROTO_SRCS}
        )

