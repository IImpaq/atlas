//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_LOADING_ANIMATION_HPP
#define ATLAS_LOADING_ANIMATION_HPP

#include <thread>
#include <iostream>
#include <string>

class LoadingAnimation {
private:
    bool m_running;
    std::thread m_animator;
    std::string m_message;
    const std::vector<std::string> m_frames = {"|", "/", "-", "\\"};

public:
    explicit LoadingAnimation(const std::string &a_msg);
    ~LoadingAnimation();

    void Stop();

private:
    void animate() const;
};

#endif // ATLAS_LOADING_ANIMATION_HPP
