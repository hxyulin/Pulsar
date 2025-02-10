add_library(PulsarEngine INTERFACE)

target_include_directories(PulsarEngine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(PulsarEngine INTERFACE
    Pulsar::LibCore
)

add_library(Pulsar::Engine ALIAS PulsarEngine)

