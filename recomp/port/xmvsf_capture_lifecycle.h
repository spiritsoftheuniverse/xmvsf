// XMVSF 87.451 - input capture transition, independent of SDL and renderer.
#pragma once
#include <cstdint>

namespace port::xmvsf_home {

class CaptureLifecycle {
public:
    enum class Phase : unsigned char { Navigate, AwaitActivatorRelease, Listening, AwaitMenuRelease };
    static constexpr std::uint64_t kTimeoutMs = 12000;
    static constexpr std::uint64_t kMaxRearmMs = 1500;
private:
    Phase phase_ = Phase::Navigate;
    std::uint64_t started_ = 0;
public:
    void Reset() { phase_ = Phase::Navigate; started_ = 0; }
    void Arm(std::uint64_t now, bool activatorHeld) {
        started_ = now;
        phase_ = activatorHeld ? Phase::AwaitActivatorRelease : Phase::Listening;
    }
    bool Capturing() const {
        return phase_ == Phase::AwaitActivatorRelease || phase_ == Phase::Listening;
    }
    bool AwaitingActivatorRelease() const { return phase_ == Phase::AwaitActivatorRelease; }
    bool MayAccept(bool sameActivator) const {
        return Capturing() && !(AwaitingActivatorRelease() && sameActivator);
    }
    void ActivatorReleased() {
        if (phase_ == Phase::AwaitActivatorRelease) phase_ = Phase::Listening;
    }
    void Finish(std::uint64_t now) {
        if (Capturing()) { phase_ = Phase::AwaitMenuRelease; started_ = now; }
    }
    void Cancel(std::uint64_t now) { Finish(now); }
    bool Expired(std::uint64_t now) const {
        return Capturing() && now - started_ >= kTimeoutMs;
    }
    void MenuButtons(bool confirmOrBackHeld, std::uint64_t now) {
        // A driver reporting A stuck must not lock the Options page forever.
        if (phase_ == Phase::AwaitMenuRelease &&
            (!confirmOrBackHeld || now - started_ >= kMaxRearmMs))
            phase_ = Phase::Navigate;
    }
    bool MayNavigate() const { return phase_ == Phase::Navigate; }
    Phase State() const { return phase_; }
};

} // namespace port::xmvsf_home
