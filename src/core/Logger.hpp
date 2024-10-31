/**
* @file Logger.hpp
* @author Marcus Gugacs
* @date 10/31/2024
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_LOGGER_HPP
#define ATLAS_LOGGER_HPP

#include <iostream>

#include "data/Array.hpp"
#include "data/Bool.hpp"
#include "data/Map.hpp"
#include "data/String.hpp"
#include "data/Singleton.hpp"
#include "os/Lock.hpp"
#include "os/ScopeLock.hpp"
#include "os/SharedLock.hpp"
#include "utils/File.hpp"

namespace atlas {
  /**
   * @brief Verbosity enum class storing the different available verbosities.
   */
  enum class Verbosity {
    DEBUG,
    INFO,
    MSG,
    WARN,
    ERROR,
    FATAL
  };

  /**
   * @brief Channel struct representing the state of a single channel.
   */
  struct Channel {
    ntl::Bool enabled{true};
  };

  /**
   * @brief Logger class used to handle logging.
   */
  class Logger : public ntl::Singleton<Logger> {
    SINGLETON_IMPL(Logger)
    static constexpr ntl::Size DEFAULT_BUFFER_THRESHOLD = 64;
    static constexpr Verbosity DEFAULT_MIN_VERBOSITY = Verbosity::INFO;
    static constexpr const char* DEFAULT_PATH = "output.log";

  private:
    Verbosity m_min_verbosity;
    ntl::Size m_buffer_threshold;
    ntl::SharedLock m_configuration_lock;
    ntl::Array<ntl::String> m_logs;
    File m_file;
    ntl::Lock m_logs_lock;
    std::atomic<ntl::Bool> m_initialized;

  public:
    /**
     * @brief Deletes Copy Constructor.
     */
    Logger(const Logger&) = delete;
    /**
     * @brief Deletes copy assignment operator.
     * @return the reference to the current logger object
     */
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Initializes the logger.
     */
    void Initialize();
    /**
     * @brief Shuts down the logger (makes it unusable until initialized again).
     */
    void Shutdown();

    /**
     * @brief Logs a trace message on a given channel.
     * @param a_message a log message
     */
    void Msg(const ntl::String& a_message);
    /**
     * @brief Logs a debug message on a given channel.
     * @param a_message a log message
     */
    void Debug(const ntl::String& a_message);
    /**
     * @brief Logs a info message on a given channel.
     * @param a_message a log message
     */
    void Info(const ntl::String& a_message);
    /**
     * @brief Logs a warning message on a given channel.
     * @param a_message a log message
     */
    void Warn(const ntl::String& a_message);
    /**
     * @brief Logs a error message on a given channel.
     * @param a_message a log message
     */
    void Error( const ntl::String& a_message);
    /**
     * @brief Logs a fatal message on a given channel.
     * @param a_message a log message
     */
    void Fatal(const ntl::String& a_message);

    /**
     * @brief Sets the minimum verbosity to log.
     * @param a_verbosity a minimum verbosity
     */
    void SetMinVerbosity(Verbosity a_verbosity);
    /**
     * @brief Sets the threshold of number of logs when the buffer will be flushed.
     * @param a_threshold the threshold when to flush the buffer
     */
    void SetBufferThreshold(ntl::Size a_threshold);

    /**
     * @brief Gets the minimum verbosity to log.
     */
    Verbosity GetMinVerbosity();
    /**
     * @brief Gets the current threshold of number of logs when the buffer will be flushed.
     */
    ntl::Size GetBufferThreshold();

  private:
    /**
     * @brief Default Constructor
     */
    Logger();
    /**
     * @brief Default Destructor
     */
    ~Logger() = default;

    /**
     * @brief Logs a message using the given arguments.
     * @param a_verbosity a log verbosity
     * @param a_message a message to log
     */
    void log(Verbosity a_verbosity, const ntl::String& a_message);
    /**
     * @brief Force the buffer to be flushed wheb the given threshold is reached.
     */
    void forceFlushBuffer();
    /**
     * @brief Inserts the next log to the buffer and flushed when the given threshold is reached.
     * @param a_log the log to insert
     * @param a_threshold the threshold to check for
     */
    void flushBuffer(const ntl::String& a_log, ntl::Size a_threshold);
    /**
     * @brief Gets the string prefix for a given verbosity enum.
     * @param a_verbosity a log verbosity
     * @return the verbosity prefix string
     */
    ntl::String getVerbosityPrefix(Verbosity a_verbosity);
  };
}

#define LOG_MSG(a_message)(atlas::Logger::Instance().Msg(a_message))
#define LOG_DEBUG(a_message)(atlas::Logger::Instance().Debug(a_message))
#define LOG_INFO(a_message)(atlas::Logger::Instance().Info(a_message))
#define LOG_WARN(a_message)(atlas::Logger::Instance().Warn(a_message))
#define LOG_ERROR(a_message)(atlas::Logger::Instance().Error(a_message))
#define LOG_FATAL(a_message)(atlas::Logger::Instance().Fatal(a_message))

#endif // ATLAS_LOGGER_HPP
