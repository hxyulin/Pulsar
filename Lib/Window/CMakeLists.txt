add_library(PulsarLibWindow STATIC
    src/PulsarWindow/Window.hpp
    src/PulsarWindow/Window.cpp
)

find_package(glfw3 REQUIRED)

target_link_libraries(PulsarLibWindow PUBLIC
    Pulsar::LibCore
    glfw
)
