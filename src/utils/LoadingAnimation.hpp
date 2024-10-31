//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_LOADING_ANIMATION_HPP
#define ATLAS_LOADING_ANIMATION_HPP

#include <thread>
#include <iostream>

#include <data/String.hpp>

namespace atlas {
    class LoadingAnimation {
    private:
        mutable size_t m_lastLineLength = 0;
        std::atomic<bool> m_running{false};
        std::string m_message;
        std::vector<std::string> m_frames{std::begin(DEFAULT_FRAMES), std::end(DEFAULT_FRAMES)};
        std::thread m_animator;
        // const std::vector<ntl::String> m_frames = {"|", "/", "-", "\\"};

        static constexpr const char* DEFAULT_FRAMES[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
        static constexpr const char* DONE_SYMBOL = "✓";
        static constexpr int FRAME_DELAY_MS = 80;

    public:
        explicit LoadingAnimation(const std::string &a_msg);
        ~LoadingAnimation();

        void Stop();

    private:
        void animate() const;
    };
}

#endif // ATLAS_LOADING_ANIMATION_HPP
