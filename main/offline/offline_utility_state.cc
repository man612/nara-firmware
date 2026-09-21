#include "offline_utility_state.h"

#include <algorithm>

NaraOfflineUtilityState::NaraOfflineUtilityState(
    NaraOfflineUtilitySnapshot snapshot)
    : snapshot_(snapshot) {
    SetTimezoneOffsetMinutes(snapshot.timezone_offset_minutes);
    if (snapshot.alarm_hour < 0 || snapshot.alarm_hour > 23 ||
        snapshot.alarm_minute < 0 || snapshot.alarm_minute > 59) {
        snapshot_.alarm_enabled = false;
        snapshot_.alarm_hour = 0;
        snapshot_.alarm_minute = 0;
    }
    if (snapshot_.timer_due_epoch < 0) {
        snapshot_.timer_due_epoch = 0;
    }
}

void NaraOfflineUtilityState::SetTimezoneOffsetMinutes(int minutes) {
    snapshot_.timezone_offset_minutes =
        std::clamp(minutes, -12 * 60, 14 * 60);
}

void NaraOfflineUtilityState::SetTimerDueEpoch(int64_t epoch) {
    snapshot_.timer_due_epoch = std::max<int64_t>(0, epoch);
}

void NaraOfflineUtilityState::CancelTimer() {
    snapshot_.timer_due_epoch = 0;
}

void NaraOfflineUtilityState::SetDailyAlarm(int hour, int minute) {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return;
    }
    snapshot_.alarm_enabled = true;
    snapshot_.alarm_hour = hour;
    snapshot_.alarm_minute = minute;
}

void NaraOfflineUtilityState::CancelAlarm() {
    snapshot_.alarm_enabled = false;
}

void NaraOfflineUtilityState::SetLastAlarmDay(int64_t day) {
    snapshot_.last_alarm_day = day;
}

int64_t NaraOfflineUtilityState::FloorDiv(
    int64_t numerator, int64_t denominator) {
    int64_t quotient = numerator / denominator;
    const int64_t remainder = numerator % denominator;
    if (remainder != 0 &&
        ((remainder < 0) != (denominator < 0))) {
        --quotient;
    }
    return quotient;
}

int64_t NaraOfflineUtilityState::LocalEpoch(int64_t now_epoch) const {
    return now_epoch +
           static_cast<int64_t>(snapshot_.timezone_offset_minutes) * 60;
}

int64_t NaraOfflineUtilityState::LocalDay(int64_t now_epoch) const {
    return FloorDiv(LocalEpoch(now_epoch), 86400);
}

int NaraOfflineUtilityState::LocalHour(int64_t now_epoch) const {
    const int64_t local = LocalEpoch(now_epoch);
    const int64_t day = FloorDiv(local, 86400);
    const int64_t seconds = local - day * 86400;
    return static_cast<int>(seconds / 3600);
}

int NaraOfflineUtilityState::LocalMinute(int64_t now_epoch) const {
    const int64_t local = LocalEpoch(now_epoch);
    const int64_t day = FloorDiv(local, 86400);
    const int64_t seconds = local - day * 86400;
    return static_cast<int>((seconds % 3600) / 60);
}

uint8_t NaraOfflineUtilityState::Poll(int64_t now_epoch) {
    if (now_epoch <= 0) {
        return kNaraOfflineEventNone;
    }

    uint8_t events = kNaraOfflineEventNone;

    if (snapshot_.timer_due_epoch > 0 &&
        now_epoch >= snapshot_.timer_due_epoch) {
        snapshot_.timer_due_epoch = 0;
        events |= kNaraOfflineEventTimer;
    }

    if (snapshot_.alarm_enabled) {
        const int64_t day = LocalDay(now_epoch);
        if (day != snapshot_.last_alarm_day &&
            LocalHour(now_epoch) == snapshot_.alarm_hour &&
            LocalMinute(now_epoch) == snapshot_.alarm_minute) {
            snapshot_.last_alarm_day = day;
            events |= kNaraOfflineEventAlarm;
        }
    }

    return events;
}
