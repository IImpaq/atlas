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

    void animate() {
        int frame = 0;
        while (running) {
            std::cout << "\r" << message << frames[frame] << std::flush;
            frame = (frame + 1) % frames.size();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "\r" << message << " ✓" << std::endl;
    }

public:
    explicit LoadingAnimation(const std::string &msg)
            : running(true), message(msg) {
        animator = std::thread(&LoadingAnimation::animate, this);
    }

    ~LoadingAnimation() { stop(); }

    void stop() {
        if (running) {
            running = false;
            if (animator.joinable()) {
                animator.join();
            }
        }
    }
};

#endif // ATLAS_LOADING_ANIMATION_HPP
