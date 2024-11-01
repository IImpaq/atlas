/**
* @file Config.hpp
* @author Marcus Gugacs
* @date 30.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_CONFIG_HPP
#define ATLAS_CONFIG_HPP

#include <toml++/toml.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace atlas {
    class Config {
    public:
        struct Core {
            bool verbose;
        };

        struct Paths {
            fs::path install_dir;
            fs::path cache_dir;
            fs::path shortcut_dir;
        };

        struct Network {
            int timeout;
            int retries;
            int max_parallel_downloads;
        };

    private:
        fs::path m_config_path;
        toml::table m_config;
        Core m_core;
        Paths m_paths;
        Network m_network;

    public:
        Config();

        explicit Config(const fs::path& a_config_path);

        void Load();

        bool Save();

        // Getters
        const Core& GetCore() const { return m_core; }
        const Paths& GetPaths() const { return m_paths; }
        const Network& GetNetwork() const { return m_network; }

        // Core setters
        void SetVerbose(bool a_verbose);

        // Path setters
        void SetInstallDir(const fs::path& a_path);

        void SetCacheDir(const fs::path& a_path);

        void SetShortcutDir(const fs::path& a_path);

        // Network setters
        void SetTimeout(int a_seconds);

        void SetRetries(int a_count);

        void SetMaxParallelDownloads(int a_count);

    private:
        static fs::path expandPath(const std::string& a_path);

        static std::string compressPath(const fs::path& a_path);

        void setDefaults();

        void loadFromTable();

        void updateTable();
    };
}

#endif // ATLAS_CONFIG_HPP
