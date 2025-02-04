// Copyright (c) 2025-2025 The Pocketcoin Core developers
// Licensed under the Apache License, Version 2.0;
// http://www.apache.org/licenses/LICENSE-2.0

#ifndef I2P_DAEMON_WRAPPER_H
#define I2P_DAEMON_WRAPPER_H

#include <string>
#include <cstdint>

namespace i2pdw {

    class DaemonWrapper {
    public:
        DaemonWrapper();
        ~DaemonWrapper();

        bool Init();
        bool Start();
        bool Stop();
        
        static DaemonWrapper& GetInstance() {
            static DaemonWrapper instance;
            return instance;
        }

        std::string GetSAMAddress();
        uint16_t GetSAMPort();

    private:
        bool m_runned;
        // Запрещаем копирование и присваивание для паттерна Singleton
        DaemonWrapper(const DaemonWrapper&) = delete;
        DaemonWrapper& operator=(const DaemonWrapper&) = delete;
    };

}

#endif // I2P_DAEMON_WRAPPER_H