//
// Created by Marcus Gugacs on 28.10.24.
//

#include "LoadingAnimation.hpp"

LoadingAnimation::LoadingAnimation(const std::string &a_msg): m_running(true), m_message(a_msg) {
  m_animator = std::thread(&LoadingAnimation::animate, this);
}

LoadingAnimation::~LoadingAnimation() { Stop(); }

void LoadingAnimation::Stop() {
  if (m_running) {
    m_running = false;
    if (m_animator.joinable()) {
      m_animator.join();
    }
  }
}

void LoadingAnimation::animate() const {
  int frame = 0;
  while (m_running) {
    std::cout << "\r" << m_message << m_frames[frame] << std::flush;
    frame = (frame + 1) % m_frames.size();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "\r" << m_message << " ✓" << std::endl;
}
