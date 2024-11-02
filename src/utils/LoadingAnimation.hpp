/**
* @file LoadingAnimation.hpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_LOADING_ANIMATION_HPP
#define ATLAS_LOADING_ANIMATION_HPP

#include <thread>
#include <iostream>

#include <data/String.hpp>

namespace atlas {
  /**
   * @class LoadingAnimation
   *
   * A class that displays a loading animation and prints a message.
   * It uses an atomic boolean to control the running state, allowing for thread-safe usage.
   */
  class LoadingAnimation {
  private:
    static constexpr const char* DEFAULT_FRAMES[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    static constexpr const char* DONE_SYMBOL = "✓";
    static constexpr int FRAME_DELAY_MS = 80;

    mutable size_t m_lastLineLength = 0;
    std::atomic<bool> m_running{false};
    std::string m_message;
    std::vector<std::string> m_frames{std::begin(DEFAULT_FRAMES), std::end(DEFAULT_FRAMES)};
    std::thread m_animator;

  public:
    /**
     * Constructs a new LoadingAnimation instance with the given message.
     *
     * @param msg The message to be displayed alongside the loading animation.
     */
    explicit LoadingAnimation(const std::string& msg);

    /**
     * Destructs the LoadingAnimation instance, stopping the animation if it's still running.
     */
    ~LoadingAnimation();

    /**
     * Stops the loading animation and any associated threads.
     */
    void Stop();

  private:
    /**
     * Animates the loading process by printing frames to the console.
     *
     * @note This function is designed to be run in a separate thread for smooth animation.
     */
    void animate() const;
  };
}

#endif // ATLAS_LOADING_ANIMATION_HPP
