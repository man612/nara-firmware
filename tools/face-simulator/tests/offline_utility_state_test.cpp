#include "offline_utility_state.h"

#include <cassert>

int main() {
    {
        NaraOfflineUtilityState state({
            .timezone_offset_minutes = 420,
            .timer_due_epoch = 1010,
        });
        assert(state.Poll(1009) == kNaraOfflineEventNone);
        assert((state.Poll(1010) & kNaraOfflineEventTimer) != 0);
        assert(state.timer_due_epoch() == 0);
        assert(state.Poll(1011) == kNaraOfflineEventNone);
    }

    {
        // 2026-09-21 00:30:00 UTC => 07:30 WIB.
        constexpr int64_t now = 1790037000;
        NaraOfflineUtilityState state({
            .timezone_offset_minutes = 420,
            .alarm_enabled = true,
            .alarm_hour = 7,
            .alarm_minute = 30,
        });
        assert(state.LocalHour(now) == 7);
        assert(state.LocalMinute(now) == 30);
        assert((state.Poll(now) & kNaraOfflineEventAlarm) != 0);
        assert(state.Poll(now + 20) == kNaraOfflineEventNone);
        assert(state.Poll(now + 86400) & kNaraOfflineEventAlarm);
    }

    {
        NaraOfflineUtilityState state;
        state.SetTimezoneOffsetMinutes(9999);
        assert(state.timezone_offset_minutes() == 14 * 60);
        state.SetDailyAlarm(23, 59);
        assert(state.alarm_enabled());
        state.CancelAlarm();
        assert(!state.alarm_enabled());
    }

    return 0;
}
