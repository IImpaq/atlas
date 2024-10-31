/**
* @file JobSystem.hpp
* @author Marcus Gugacs
* @date 10/31/2024
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "JobSystem.hpp"

#include "core/Assert.hpp"
#include "os/ScopeLock.hpp"

using namespace ntl;

namespace atlas {
  void JobSystem::Initialize() {
    m_thread_count = std::thread::hardware_concurrency();
    m_running_jobs = 0;
    m_initialized = true;
    initThreadPool();
  }

  void JobSystem::Shutdown() {
    m_initialized = false;
  }

  void JobSystem::initThreadPool() {
    VERIFY(m_initialized && "JobSystem must be initialized prior to use")

    for(unsigned int i = 0; i < m_thread_count; ++i) {
      std::thread worker{
        [this]() {
          while(m_initialized) {
            auto job = JobSystem::Instance().GetJobOrWait();
            job();
            --m_running_jobs;
            m_jobs_changed.Broadcast();
          }
        }
      };
      worker.detach();
    }
  }

  void JobSystem::AddJob(const std::function<void()>& a_job) {
    VERIFY(m_initialized && "JobSystem must be initialized prior to use")

    ScopeLock lock(&m_jobs_lock);
    m_jobs.Put(a_job);
    m_jobs_changed.Broadcast();
  }

  std::function<void()> JobSystem::GetJobOrWait() {
    VERIFY(m_initialized && "JobSystem must be initialized prior to use")

    m_jobs_lock.Acquire();

    while(m_jobs.IsEmpty())
      m_jobs_changed.Wait();

    ++m_running_jobs;
    auto job = m_jobs.Get();

    m_jobs_changed.Broadcast();
    m_jobs_lock.Release();

    return job;
  }

  void JobSystem::WaitForJobsToFinish() {
    VERIFY(m_initialized && "JobSystem must be initialized prior to use")

    m_jobs_lock.Acquire();

    while(!m_jobs.IsEmpty() || (m_running_jobs > 0))
      m_jobs_changed.Wait();

    m_jobs_lock.Release();
  }

  Size JobSystem::GetPendingJobCount() {
    m_jobs_lock.Acquire();
    Size temp_count = m_jobs.GetSize();
    m_jobs_lock.Release();
    return temp_count;
  }
}
