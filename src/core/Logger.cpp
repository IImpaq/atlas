/**
* @file Logger.hpp
* @author Marcus Gugacs
* @date 10/31/2024
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "Logger.hpp"

#include "utils/JobSystem.hpp"

namespace atlas {
  void Logger::Initialize() {
    m_file.ResetFile();

    m_initialized = true;
  }

  void Logger::Shutdown() {
    m_initialized = false;

    forceFlushBuffer();
  }

  void Logger::Msg(const ntl::String& a_message) {
    VERIFY(m_initialized && "Logger must be initialized prior to use")

    JobSystem::Instance().AddJob([a_message]() {
      Logger::Instance().log(Verbosity::MSG, a_message);
    });
  }

  void Logger::Debug(const ntl::String& a_message) {
    VERIFY(m_initialized && "Logger must be initialized prior to use")

    JobSystem::Instance().AddJob([a_message]() {
      Logger::Instance().log(Verbosity::DEBUG, a_message);
    });
  }

  void Logger::Info(const ntl::String& a_message) {
    VERIFY(m_initialized && "Logger must be initialized prior to use")

    JobSystem::Instance().AddJob([a_message]() {
      Logger::Instance().log(Verbosity::INFO, a_message);
    });
  }

  void Logger::Warn(const ntl::String& a_message) {
    VERIFY(m_initialized && "Logger must be initialized prior to use")

    JobSystem::Instance().AddJob([a_message]() {
      Logger::Instance().log(Verbosity::WARN, a_message);
    });
  }

  void Logger::Error(const ntl::String& a_message) {
    VERIFY(m_initialized && "Logger must be initialized prior to use")

    JobSystem::Instance().AddJob([a_message]() {
      Logger::Instance().log(Verbosity::ERROR, a_message);
    });
  }

  void Logger::Fatal(const ntl::String& a_message) {
    VERIFY(m_initialized && "Logger must be initialized prior to use")

    JobSystem::Instance().AddJob([a_message]() {
      Logger::Instance().log(Verbosity::FATAL, a_message);
      std::terminate();
    });
  }

  void Logger::SetMinVerbosity(Verbosity a_verbosity) {
    m_configuration_lock.StartWrite();
    m_min_verbosity = a_verbosity;
    m_configuration_lock.EndWrite();
  }

  void Logger::SetBufferThreshold(ntl::Size a_threshold) {
    m_configuration_lock.StartWrite();
    m_buffer_threshold = a_threshold;
    m_configuration_lock.EndWrite();
  }

  Verbosity Logger::GetMinVerbosity() {
    m_configuration_lock.StartRead();
    auto verbosity = m_min_verbosity;
    m_configuration_lock.EndRead();
    return verbosity;
  }

  ntl::Size Logger::GetBufferThreshold() {
    m_configuration_lock.StartRead();
    auto threshold = m_buffer_threshold;
    m_configuration_lock.EndRead();
    return threshold;
  }

  Logger::Logger()
    : m_min_verbosity{DEFAULT_MIN_VERBOSITY}, m_buffer_threshold{DEFAULT_BUFFER_THRESHOLD},
      m_configuration_lock{}, m_logs{1024, false}, m_logs_lock{},
      m_file{DEFAULT_PATH}, m_initialized{false} {}

  void Logger::log(Verbosity a_verbosity, const ntl::String& a_message) {
    m_configuration_lock.StartRead();

    if(a_verbosity < m_min_verbosity) {
      m_configuration_lock.EndRead();
      return;
    }

    ntl::String log = getVerbosityPrefix(a_verbosity) + a_message + "\n";
    auto threshold = m_buffer_threshold;
    m_configuration_lock.EndRead();

    std::cout << log;

    JobSystem::Instance().AddJob([log, threshold]() {
      Instance().flushBuffer(log, threshold);
    });
  }

  void Logger::forceFlushBuffer() {
    ntl::ScopeLock lock(&m_logs_lock);
    m_file.WriteFile(m_logs);
    m_logs.Clear();
  }

  void Logger::flushBuffer(const ntl::String& a_log, ntl::Size a_threshold) {
    ntl::ScopeLock lock(&m_logs_lock);
    m_logs.Insert(a_log);
    if(m_logs.GetSize() > a_threshold) {
      m_file.WriteFile(m_logs);
      m_logs.Clear();
    }
  }

  ntl::String Logger::getVerbosityPrefix(Verbosity a_verbosity) {
    ntl::String prefix = "";

    switch(a_verbosity) {
      case Verbosity::MSG:
        break;
      case Verbosity::DEBUG:
        prefix = "[DEBUG] ";
        break;
      case Verbosity::INFO:
        prefix = "[INFO] ";
        break;
      case Verbosity::WARN:
        prefix = "[WARN] ";
        break;
      case Verbosity::ERROR:
        prefix = "[ERROR] ";
        break;
      case Verbosity::FATAL:
        prefix = "[FATAL] ";
        break;
    }

    return prefix;
  }
}
