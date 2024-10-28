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
    bool running;
    std::thread animator;
    std::string message;
    const std::vector<std::string> frames = {"|", "/", "-", "\\"};

public:
    explicit LoadingAnimation(const std::string &msg);
    ~LoadingAnimation();

    void stop();

private:
    void animate() const;
};

#endif // ATLAS_LOADING_ANIMATION_HPP
