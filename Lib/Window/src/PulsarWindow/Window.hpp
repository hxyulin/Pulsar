#pragma once

#include "PulsarCore/Types.hpp"

namespace Pulsar {
    /// A cross-platform window API, with some inspration from winit
    /// # Performance
    /// The window functions themselves are not performance critical, but the
    /// underlying windowing systemad and event system can be. Therefore, we can
    /// afford to use inheritance to implement the windowing system, instead of
    /// using a virtual function table structure.
    class Window {
    public:
        Window() = default;
        virtual ~Window() = default;
        Window(const Window &) = delete;
        Window(Window &&) = delete;
        Window &operator=(const Window &) = delete;
        Window &operator=(Window &&) = delete;
    
        [[nodiscard]] virtual bool should_close() const = 0;
        virtual void poll_events() = 0;
    };

    // NOLINTBEGIN(readability-identifier-naming)
    struct WindowSettings_t {
        UVec2 inner_size = {1280, 720};
        bool visible      = true;
        bool resizable    = true;
        bool fullscreen   = false;
    };

    // NOLINTEND(readability-identifier-naming)
} // namespace Pulsar
