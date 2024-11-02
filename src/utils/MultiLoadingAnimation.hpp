/**
* @file MultiLoadingAnimation.hpp
* @author Marcus Gugacs
* @date 01.11.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_MULTI_LOADING_ANIMATION_HPP
#define ATLAS_MULTI_LOADING_ANIMATION_HPP

#include <thread>
#include <data/Map.hpp>
#include <os/Lock.hpp>
#include <os/ScopeLock.hpp>

#include "Console.hpp"
#include "Misc.hpp"

namespace atlas {
  /**
   * @class MultiLoadingAnimation
   * @brief A multi-loading animation class that can be used to display a loading animation on the console.
   *
   * This class is designed to be used in conjunction with other classes and functions that update package statuses,
   * allowing for a smooth and interactive loading animation experience. It uses multiple threads and synchronization primitives
   * to ensure thread safety and minimize performance impact.
   */
  class MultiLoadingAnimation {
  private:
    static constexpr const char* DEFAULT_FRAMES[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    static constexpr int FRAME_DELAY_MS = 100;

    std::atomic<bool> m_running;
    std::thread m_animator;
    ntl::Lock m_mutex;
    ntl::Map<ntl::String, ntl::String> m_package_status;
    ntl::Map<ntl::String, int> m_package_frames;
    size_t m_last_line_count{0};

  public:
    /**
     * @brief Default constructor.
     *
     * Initializes the animation and starts it running.
     */
    MultiLoadingAnimation();

    /**
     * @brief Destructor.
     *
     * Stops the animation and cleans up any resources used by the class.
     */
    ~MultiLoadingAnimation();

    /**
     * @brief Delete copy constructor.
     *
     * This class is not designed to be copied or assigned.
     */
    MultiLoadingAnimation(const MultiLoadingAnimation&) = delete;

    /**
     * @brief Delete assignment operator.
     *
     * This class is not designed to be copied or assigned.
     */
    MultiLoadingAnimation& operator=(const MultiLoadingAnimation&) = delete;

    /**
     * @brief Forces the animation to clean up and stop running.
     */
    void ForceClean();

    /**
     * @brief Updates the status of a package in the animation.
     *
     * @param package_name The name of the package being updated.
     * @param status The new status of the package.
     */
    void UpdateStatus(const ntl::String& package_name, const ntl::String& status);

    /**
     * @brief Removes a package from the animation.
     *
     * @param package_name The name of the package to remove.
     */
    void RemovePackage(const ntl::String& package_name);

    /**
     * @brief Stops the animation and clean up any resources used by the class.
     */
    void Stop();

  private:
    /**
     * @brief Animates the loading process.
     *
     * This function is responsible for displaying the loading animation on the console and updating the status of packages.
     */
    void animate();
  };
}

#endif // ATLAS_MULTI_LOADING_ANIMATION_HPP
