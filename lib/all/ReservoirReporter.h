#pragma once

#include <Arduino.h>

#include "TCPMessenger.h"
#include "EncryptionHandler.h"
#include "DataFormats.h"
#include "ReservoirSettings.h"
#include "passwords.h"
#include "LocalIdentity.h"

#define REPORTER_VERBOSE_LOGGING 0

class ReservoirReporter {
public:
    struct Config {
        uint32_t heartbeatIntervalMs = 60000;
        uint32_t minAttemptIntervalMs = 1000;

        tcpmsg::TCPMessenger::SendOptions sendOptions;
    };

    ReservoirReporter(const tcpmsg::formats::ReservoirInfo& state,
                      const ReservoirSettings& settings);

    bool begin(const Config& cfg);

    // Force eager reporting until one send completes successfully.
    void enableImmediateSend();

    void end();

    bool isRunning() const;

    void loop();

    const tcpmsg::TCPMessenger::SendResult& lastSendResult() const;

    uint32_t lastReportSuccessMs() const;

private:
    bool levelChanged(const tcpmsg::formats::ReservoirInfo& current) const;
    bool heartbeatExpired(uint32_t nowMs) const;
    void pollSendResult(uint32_t nowMs);

private:
    const tcpmsg::formats::ReservoirInfo& state_;
    const ReservoirSettings& settings_;
    tcpmsg::EncryptionHandler encryptionHandler_;
    tcpmsg::TCPMessenger messenger_;

    Config cfg_{};

    tcpmsg::formats::ReservoirInfo lastReportedState_{};
    tcpmsg::formats::ReservoirInfo pendingState_{};

    bool lazySend_ = false;
    bool awaitingSendResult_ = false;

    uint32_t lastReportAttemptMs_ = 0;
    uint32_t lastReportSuccessMs_ = 0;

    tcpmsg::TCPMessenger::SendResult lastSendResult_{};
};