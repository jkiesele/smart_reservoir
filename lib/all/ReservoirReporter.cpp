#include "ReservoirReporter.h"

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

    if (!messenger_.isRunning()) {
        return;
    }

    if (awaitingSendResult_) {
        return;
    }

    // min interval check
    if ((nowMs - lastReportAttemptMs_) < cfg_.minAttemptIntervalMs) {
        return;
    }

    tcpmsg::formats::ReservoirInfo current = state_;

    const bool shouldSend =
        /* !lazySend_ || redundant */
        levelChanged(current) ||
        heartbeatExpired(nowMs);

    if (!shouldSend) {
        return;
    }
    // settings are sanity checked so can be used directly here
    if(REPORTER_VERBOSE_LOGGING) {
        gLogger->print("Reporter: Sending update: level=" + String(current.level()) +
                         "%, emptyLevel=" + String(current.emptyLevel()) +
                         "%, capacity=" + String(current.capacity()) +
                         "L, temp=" + String(current.temperature()) + "C\n");
        gLogger->println("to " + settings_.injIP.toString() + ":" + String(settings_.tcpPort) +
                         " chanId=" + String(settings_.channelID));
    }

    const auto rc = messenger_.sendToIP(current,
                                        settings_.channelID, // this can change in the web settings so read it here
                                        settings_.injIP,
                                        settings_.tcpPort,
                                        settings_.injMAC,
                                        cfg_.sendOptions);

    if (rc == tcpmsg::TCPMessenger::Result::Ok) {
        awaitingSendResult_ = true;
        pendingState_ = current;
        lastReportAttemptMs_ = nowMs;
    } else {
        lastSendResult_.rc = rc;
        lastSendResult_.ip = settings_.injIP;
        lastSendResult_.port = settings_.tcpPort;
        lastReportAttemptMs_ = nowMs;
    }
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
    awaitingSendResult_ = false;

    if (sr.rc == tcpmsg::TCPMessenger::Result::Ok) {
        lastReportedState_ = pendingState_;
        lazySend_ = true;
        lastReportSuccessMs_ = nowMs;
    }
}