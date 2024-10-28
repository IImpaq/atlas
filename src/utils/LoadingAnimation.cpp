//
// Created by Marcus Gugacs on 28.10.24.
//

#include "LoadingAnimation.hpp"

LoadingAnimation::LoadingAnimation(const std::string &msg): running(true), message(msg) {
  animator = std::thread(&LoadingAnimation::animate, this);
}

LoadingAnimation::~LoadingAnimation() { stop(); }

void LoadingAnimation::stop() {
  if (running) {
    running = false;
    if (animator.joinable()) {
      animator.join();
    }
  }
}

void LoadingAnimation::animate() const {
  int frame = 0;
  while (running) {
    std::cout << "\r" << message << frames[frame] << std::flush;
    frame = (frame + 1) % frames.size();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "\r" << message << " ✓" << std::endl;
}
