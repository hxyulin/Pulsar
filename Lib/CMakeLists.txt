add_subdirectory(Core)
add_subdirectory(Window)

add_library(Pulsar::LibCore ALIAS PulsarLibCore)
add_library(Pulsar::LibWindow ALIAS PulsarLibWindow)
