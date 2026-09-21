#pragma once

#include <cstdint>

enum NaraOfflineEvent : uint8_t {
    kNaraOfflineEventNone = 0,
    kNaraOfflineEventTimer = 1 << 0,
    kNaraOfflineEventAlarm = 1 << 1,
};

struct NaraOfflineUtilitySnapshot {
    int timezone_offset_minutes = 0;
    int64_t timer_due_epoch = 0;
    bool alarm_enabled = false;
    int alarm_hour = 0;
    int alarm_minute = 0;
    int64_t last_alarm_day = -1;
};

class NaraOfflineUtilityState {
public:
    explicit NaraOfflineUtilityState(
        NaraOfflineUtilitySnapshot snapshot = {});

    void SetTimezoneOffsetMinutes(int minutes);
    void SetTimerDueEpoch(int64_t epoch);
    void CancelTimer();
    void SetDailyAlarm(int hour, int minute);
    void CancelAlarm();
    void SetLastAlarmDay(int64_t day);

    uint8_t Poll(int64_t now_epoch);

    int timezone_offset_minutes() const {
        return snapshot_.timezone_offset_minutes;
    }
    int64_t timer_due_epoch() const { return snapshot_.timer_due_epoch; }
    bool alarm_enabled() const { return snapshot_.alarm_enabled; }
    int alarm_hour() const { return snapshot_.alarm_hour; }
    int alarm_minute() const { return snapshot_.alarm_minute; }
    int64_t last_alarm_day() const { return snapshot_.last_alarm_day; }

    int64_t LocalDay(int64_t now_epoch) const;
    int LocalHour(int64_t now_epoch) const;
    int LocalMinute(int64_t now_epoch) const;

private:
    NaraOfflineUtilitySnapshot snapshot_;

    static int64_t FloorDiv(int64_t numerator, int64_t denominator);
    int64_t LocalEpoch(int64_t now_epoch) const;
};
