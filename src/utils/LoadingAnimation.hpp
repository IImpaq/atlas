//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_LOADING_ANIMATION_HPP
#define ATLAS_LOADING_ANIMATION_HPP

#include <thread>
#include <iostream>

#include <data/String.hpp>

class LoadingAnimation {
private:
    bool m_running;
    std::thread m_animator;
    ntl::String m_message;
    const std::vector<ntl::String> m_frames = {"|", "/", "-", "\\"};

public:
    explicit LoadingAnimation(const ntl::String &a_msg);
    ~LoadingAnimation();

    void Stop();

private:
    void animate() const;
};

#endif // ATLAS_LOADING_ANIMATION_HPP
