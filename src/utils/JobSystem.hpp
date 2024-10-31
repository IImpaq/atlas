/**
* @file JobSystem.hpp
* @author Marcus Gugacs
* @date 10/31/2024
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_JOB_SYSTEM_HPP
#define ATLAS_JOB_SYSTEM_HPP

#include <functional>
#include <thread>

#include "data/Array.hpp"
#include "data/Bool.hpp"
#include "data/Queue.hpp"
#include "data/Singleton.hpp"
#include "os/Lock.hpp"
#include "os/Condition.hpp"

namespace atlas {
  /**
   * @brief JobSystem class used to distribute jobs in a pool of threads.
   */
  class JobSystem : public ntl::Singleton<JobSystem> {
    SINGLETON_IMPL(JobSystem)

  private:
    ntl::Size m_thread_count;
    ntl::Array<std::thread> m_workers;
    ntl::Queue<std::function<void()>> m_jobs;
    ntl::Lock m_jobs_lock;
    ntl::Condition m_jobs_changed;
    std::atomic<ntl::Bool> m_initialized;
    std::atomic<int> m_running_jobs;

  public:
    /**
     * @brief Initializes the job system (singleton).
     */
    void Initialize();
    /**
     * @brief Shuts down the job system (singleton).
     */
    void Shutdown();

    /**
     * @brief Adds the given job to the job queue.
     * @param a_job the job to add to the queu
     */
    void AddJob(const std::function<void()>& a_job);

    /**
     * @brief Gets the next job or waits until a job becomes available.
     * @return the next job from the queue
     */
    std::function<void()> GetJobOrWait();
    /**
     * @brief Waits for all jobs to finish.
     */
    void WaitForJobsToFinish();

    ntl::Size GetPendingJobCount();

  private:
    /**
     * @brief Default Constructor.
     */
    JobSystem()
      : Singleton{}, m_thread_count{0}, m_workers{}, m_jobs{}, m_jobs_lock{}, m_jobs_changed{&m_jobs_lock},
        m_initialized{false} {}

    /**
     * @brief Default Destructor.
     */
    ~JobSystem() {}

    /**
     * @brief Initializes the thread pool.
     */
    void initThreadPool();
  };
}

#endif // ATLAS_JOB_SYSTEM_HPP
