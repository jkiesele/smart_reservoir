#include "ReservoirReporter.h"
#include "LoggingBase.h"

ReservoirReporter::ReservoirReporter(const tcpmsg::formats::ReservoirInfo& state,
                                     const ReservoirSettings& settings)
    : state_(state), settings_(settings), encryptionHandler_(&secret::encryption_keys) {}

bool ReservoirReporter::begin(const Config& cfg) {
    end();

    cfg_ = cfg;
    lazySend_ = false;
    awaitingSendResult_ = false;
    lastReportAttemptMs_ = 0;
    lastReportSuccessMs_ = 0;
    lastSendResult_.rc = tcpmsg::TCPMessenger::Result::NotRunning;

    tcpmsg::MACAddress localMac;
    if (!tcpmsg::defineLocalMACAddress(localMac)) {
        gLogger->println("Reporter: Failed to define local MAC address, cannot start reporter");
        return false;
    }
    char macstr[tcpmsg::MACAddress::kStringSize];
    localMac.toCString(macstr);
    gLogger->println("Reporter: Local MAC address defined as " + String(macstr));

    return messenger_.begin(settings_.tcpPort, localMac, &encryptionHandler_);
}

void ReservoirReporter::enableImmediateSend() {
    lazySend_ = false;
}

void ReservoirReporter::end() {
    messenger_.end();
    awaitingSendResult_ = false;
}

bool ReservoirReporter::isRunning() const {
    return messenger_.isRunning();
}

void ReservoirReporter::loop() {
    const uint32_t nowMs = millis();

    pollSendResult(nowMs);

    if (handleSendResultStallIfNeeded(nowMs)) {
        return;
    }

    if (!canAttemptSend(nowMs)) {
        return;
    }

    if (!shouldSendReport(nowMs)) {
        return;
    }

    trySendReport(nowMs);
}

const tcpmsg::TCPMessenger::SendResult& ReservoirReporter::lastSendResult() const {
    return lastSendResult_;
}

uint32_t ReservoirReporter::lastReportSuccessMs() const {
    return lastReportSuccessMs_;
}

bool ReservoirReporter::levelChanged(const tcpmsg::formats::ReservoirInfo& current) const {
    if (!lazySend_) {
        return true;
    }
    return current.level() != lastReportedState_.level();
}

bool ReservoirReporter::heartbeatExpired(uint32_t nowMs) const {
    if (!lazySend_) {
        return true;
    }
    return (nowMs - lastReportSuccessMs_) >= cfg_.heartbeatIntervalMs;
}

void ReservoirReporter::pollSendResult(uint32_t nowMs) {
    tcpmsg::TCPMessenger::SendResult sr;
    if (!messenger_.popSendResult(sr)) {
        return;
    }

    lastSendResult_ = sr;

    if (!awaitingSendResult_) {
        return;
    }

    awaitingSendResult_ = false;

    if (sr.rc == tcpmsg::TCPMessenger::Result::Ok) {
        lastReportedState_ = pendingState_;
        lazySend_ = true;
        lastReportSuccessMs_ = nowMs;
    }
}

bool ReservoirReporter::canAttemptSend(uint32_t nowMs) const {
    if (!settings_.sendActive) {
        return false;
    }

    if (!messenger_.isRunning()) {
        return false;
    }

    if (awaitingSendResult_) {
        return false;
    }

    if ((nowMs - lastReportAttemptMs_) < cfg_.minAttemptIntervalMs) {
        return false;
    }

    return true;
}

bool ReservoirReporter::shouldSendReport(uint32_t nowMs) const {
        return levelChanged(state_) || heartbeatExpired(nowMs);
}

void ReservoirReporter::trySendReport(uint32_t nowMs) {
    if (REPORTER_VERBOSE_LOGGING) {
        gLogger->print("Reporter: Sending update: level=" + String(state_.level()) +
                       "%, emptyLevel=" + String(state_.emptyLevel()) +
                       "%, capacity=" + String(state_.capacity()) +
                       "L, temp=" + String(state_.temperature()) + "C\n");
        gLogger->println("to " + settings_.injIP.toString() + ":" + String(settings_.tcpPort) +
                         " chanId=" + String(settings_.channelID));
    }

    const auto rc = messenger_.sendToIP(state_,
                                        settings_.channelID,
                                        settings_.injIP,
                                        settings_.tcpPort,
                                        settings_.injMAC,
                                        cfg_.sendOptions);

    if (rc == tcpmsg::TCPMessenger::Result::Ok) {
        awaitingSendResult_ = true;
        pendingState_ = state_;
        lastReportAttemptMs_ = nowMs;
    } else {
        lastSendResult_.rc = rc;
        lastSendResult_.ip = settings_.injIP;
        lastSendResult_.port = settings_.tcpPort;
        lastReportAttemptMs_ = nowMs;
    }
}

uint32_t ReservoirReporter::expectedSendResultTimeoutMs() const {
    const tcpmsg::TCPMessenger::SendOptions& opt = cfg_.sendOptions;

    const uint64_t attempts =
        static_cast<uint64_t>(opt.maxRetries) + 1ULL;

    uint64_t timeoutMs =
        attempts * static_cast<uint64_t>(opt.timeoutMs) +
        static_cast<uint64_t>(opt.maxRetries) * static_cast<uint64_t>(opt.retryDelayMs);

    // Grace margin for scheduling jitter, TCP cleanup, logging delays, etc.
    timeoutMs += 5000ULL;

    // Avoid false positives for very short configured send timeouts.
    if (timeoutMs < 10000ULL) {
        timeoutMs = 10000ULL;
    }

    // Avoid waiting forever if someone configures silly send options.
    if (timeoutMs > 60000ULL) {
        timeoutMs = 60000ULL;
    }

    return static_cast<uint32_t>(timeoutMs);
}

bool ReservoirReporter::sendResultStalled(uint32_t nowMs) const {
    if (!awaitingSendResult_) {
        return false;
    }

    const uint32_t elapsedMs =
        static_cast<uint32_t>(nowMs - lastReportAttemptMs_);

    return elapsedMs > expectedSendResultTimeoutMs();
}

bool ReservoirReporter::handleSendResultStallIfNeeded(uint32_t nowMs) {
    if (!sendResultStalled(nowMs)) {
        return false;
    }

    gLogger->println("Reporter: send result stalled; restarting TCP messenger.");

    awaitingSendResult_ = false;

    // Do not mark the report successful. The pending state was never confirmed.
    // Force a future send attempt once minAttemptIntervalMs has elapsed.
    lazySend_ = false;

    lastSendResult_.rc = tcpmsg::TCPMessenger::Result::TransportError;
    lastSendResult_.ip = settings_.injIP;
    lastSendResult_.port = settings_.tcpPort;

    // Prevent immediate retry/restart churn in the same loop cycle.
    lastReportAttemptMs_ = nowMs;

    if (!restartConnection()) {
        gLogger->println("Reporter: TCP messenger restart failed after send-result stall.");
    }

    return true;
}