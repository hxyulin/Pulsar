# If we are macos, we can use AppKit
if(APPLE)
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

else()
    message(WARN "Non AppKit platform, not building PulsarEditor")
endif()
