/**
* @file LoadingAnimation.cpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "LoadingAnimation.hpp"

#include "Misc.hpp"

namespace atlas {
  LoadingAnimation::LoadingAnimation(const std::string& a_msg) : m_running(true), m_message(a_msg) {
    m_animator = std::thread(&LoadingAnimation::animate, this);
  }

  LoadingAnimation::~LoadingAnimation() { Stop(); }

  void LoadingAnimation::Stop() {
    if (m_running) {
      m_running = false;
      if (m_animator.joinable()) {
        m_animator.join();
      }
      // Clear the line and print completion message
      std::cout << "\r" << std::string(m_lastLineLength, ' ') << "\r";
      std::cout << CYAN << m_message << GREEN << " " << DONE_SYMBOL << RESET << std::endl;
    }
  }

  void LoadingAnimation::animate() const {
    int frame = 0;
    while (m_running) {
      std::string currentLine = CYAN + m_message + YELLOW + " " + m_frames[frame] + RESET;

      // Clear previous line and print new one
      std::cout << "\r" << std::string(m_lastLineLength, ' ') << "\r";
      std::cout << currentLine << std::flush;

      // Update last line length
      m_lastLineLength = currentLine.length();

      frame = (frame + 1) % m_frames.size();
      std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS));
    }
  }
}
