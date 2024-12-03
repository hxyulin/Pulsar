add_executable(PulsarEditor
    src/PulsarEditor/main.cpp
)

target_link_libraries(PulsarEditor
    PRIVATE
        Pulsar::LibCore
)

