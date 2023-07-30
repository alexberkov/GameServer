#include "ticker.h"

namespace ticker {
void Ticker::Start() {
    last_tick_ = std::chrono::steady_clock::now();
    auto handle = [self = shared_from_this()] {
        self->ScheduleTick();
    };
    net::dispatch(strand_, handle);
}

void Ticker::ScheduleTick() {
    timer_.expires_after(period_);
    timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
        self->OnTick(ec);
    });
}

void Ticker::OnTick(sys::error_code ec) {
    auto current_tick = std::chrono::steady_clock::now();
    auto period = std::chrono::duration_cast<duration>(current_tick - last_tick_);
    handler_(period);
    last_tick_ = current_tick;
    ScheduleTick();
}
} // namespace ticker
