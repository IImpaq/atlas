/**
* @file MultiLoadingAnimation.cpp
* @author Marcus Gugacs
* @date 01.11.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "MultiLoadingAnimation.hpp"

namespace atlas {
  MultiLoadingAnimation::MultiLoadingAnimation(): m_running(true) {
    m_animator = std::thread(&MultiLoadingAnimation::animate, this);
  }

  MultiLoadingAnimation::~MultiLoadingAnimation() {
    Stop(); // Ensure thread is stopped and joined
  }

  void MultiLoadingAnimation::ForceClean() {
    for (size_t i = 0; i < m_last_line_count; ++i) {
      std::cout << "\033[A";
    }

    for (size_t i = 0; i < m_last_line_count; ++i) {
      std::cout << "\r" << "\033[K";
      if (i < m_last_line_count - 1) {
        std::cout << "\n";
      }
    }

    std::cout.flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    m_last_line_count = 0;
  }

  void MultiLoadingAnimation::UpdateStatus(const ntl::String& package_name, const ntl::String& status) {
    ntl::ScopeLock lock(&m_mutex);
    m_package_status[package_name] = status;
    m_package_frames[package_name] = 0;
  }

  void MultiLoadingAnimation::RemovePackage(const ntl::String& package_name) {
    ntl::ScopeLock lock(&m_mutex);
    m_package_status.Remove(package_name);
    m_package_frames.Remove(package_name);
    if (m_package_frames.GetSize() == 0 && m_package_status.GetSize() == 0) {
      ForceClean();
    }
  }

  void MultiLoadingAnimation::Stop() {
    if (m_running) {
      m_running = false;
      if (m_animator.joinable()) {
        m_animator.join();
      }
      ForceClean();
      Console::GetInstance().PrintLine("");
    }
  }

  void MultiLoadingAnimation::animate() {
    while (m_running) {
      ntl::String status;
      size_t current_line_count = 0; {
        ntl::ScopeLock lock(&m_mutex);
        for (const auto& [pkg, state] : m_package_status) {
          status += ntl::String{CYAN} + pkg + ": " + YELLOW + state +
              RESET + " " + DEFAULT_FRAMES[m_package_frames[pkg]] + "\n";
          m_package_frames[pkg] = (m_package_frames[pkg] + 1) % std::size(DEFAULT_FRAMES);
          current_line_count++;
        }
      }

      for (size_t i = 0; i < m_last_line_count; ++i) {
        std::cout << "\033[A" << "\033[2K";
      }

      if (!status.IsEmpty()) {
        Console::GetInstance().UpdateProgress(status);
      }

      m_last_line_count = current_line_count;

      std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DELAY_MS));
    }
  }
}
