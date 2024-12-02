add_executable(PulsarEditor
    Chrome/AppKit/AppKit.mm
)

target_link_libraries(PulsarEditor
    PRIVATE
        Pulsar::LibCore

        # AppKit
        "-framework AppKit"
        "-framework Foundation"
)
