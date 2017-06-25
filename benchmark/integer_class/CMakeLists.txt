project(benchmark_integer_class)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS IntegerClass.proto)
ignore_warnings_in_generated_files(${PROTO_SRCS})

add_benchmark(benchmark_integer_class
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_integer_class.cpp
        ${PROTO_SRCS}
        )
add_size_benchmark(size_benchmark_integer_class "cereal_binary;cereal_extendable;boost;protobuf"
        ${CMAKE_CURRENT_LIST_DIR}/benchmark_integer_class_size.cpp
        #${PROTO_SRCS}
        )
