/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_ALWAYS_IF(value, string)    \
{                                       \
    if (value)                          \
        ALOGE("%s", string);            \
}                                       \

#include <chrono>
#include "OneShotTimer.h"
#include <utils/Timers.h>
namespace {
using namespace std::chrono_literals;
constexpr int64_t kNsToSeconds = std::chrono::duration_cast<std::chrono::nanoseconds>(1s).count();

// The syscall interface uses a pair of integers for the timestamp. The first
// (tv_sec) is the whole count of seconds. The second (tv_nsec) is the
// nanosecond part of the count. This function takes care of translation.
void calculateTimeoutTime(std::chrono::nanoseconds timestamp, timespec* spec) {
    auto timeout = systemTime(CLOCK_MONOTONIC) + timestamp.count();
    spec->tv_sec = static_cast<__kernel_time_t>(timeout / kNsToSeconds);
    spec->tv_nsec = timeout % kNsToSeconds;
}
} // namespace

OneShotTimer::OneShotTimer(const Interval &interval, const ResetCallback &resetCallback,
                           const TimeoutCallback &timeoutCallback)
    : mInterval(interval), mResetCallback(resetCallback), mTimeoutCallback(timeoutCallback) {}

OneShotTimer::~OneShotTimer() {
    stop();
}

void OneShotTimer::start() {
    int result = sem_init(&mSemaphore, 0, 0);
    LOG_ALWAYS_IF(result, "OneShotTimer::start(), sem_init failed");

    if (!mThread.joinable()) {
        mThread = std::thread(&OneShotTimer::loop, this);
    } else {
    ALOGI("OneShotTimer::the thread is already started!");
    }
}

void OneShotTimer::stop() {
    mStopTriggered = true;
    int result = sem_post(&mSemaphore);
    LOG_ALWAYS_IF(result, "OneShotTimer::stop(), sem_post_failed");

    if (mThread.joinable()) {
        mThread.join();
        result = sem_destroy(&mSemaphore);
        LOG_ALWAYS_IF(result, "OneShotTimer::stop(), sem_destroy failed");
    } else {
        mStopTriggered = false;
    }
}

void OneShotTimer::setInterval(const Interval &interval) {
    stop();
    mInterval = interval;
    start();
}

void OneShotTimer::loop() {
    TimerState state = TimerState::RESET;
    while (true) {
        bool triggerReset = false;
        bool triggerTimeout = false;

        state = checkForResetAndStop(state);
        if (state == TimerState::STOPPED)
            break;

        if (state == TimerState::IDLE) {
            int result = sem_wait(&mSemaphore);
            if (result && errno != EINTR)
                ALOGE("OneShotTimer::loop(), sem_wait failed %d", errno);
            continue;
        }

        if (state == TimerState::RESET)
            triggerReset = true;

        if (triggerReset && mResetCallback)
            mResetCallback();

        state = checkForResetAndStop(state);
        if (state == TimerState::STOPPED)
            break;

        auto triggerTime = std::chrono::steady_clock::now() + mInterval;
        state = TimerState::WAITING;
        while (true) {
            // Wait until triggerTime time to check if we need to reset or drop into the idle state.
            if (const auto triggerInterval = triggerTime - std::chrono::steady_clock::now(); triggerInterval > 0ns) {
                mWaiting = true;
                struct timespec ts;
                calculateTimeoutTime(triggerInterval, &ts);
                int result = sem_clockwait(&mSemaphore, CLOCK_MONOTONIC, &ts);
                if (result && errno != ETIMEDOUT && errno != EINTR)
                    ALOGE("OneShotTimer::loop(), sem_clockwait failed");
            }

            mWaiting = false;
            state = checkForResetAndStop(state);
            if (state == TimerState::STOPPED)
                break;

            if (state == TimerState::WAITING && (triggerTime - std::chrono::steady_clock::now()) <= 0ns) {
                triggerTimeout = true;
                state = TimerState::IDLE;
                break;
            }

            if (state == TimerState::RESET) {
                triggerTime = mLastResetTime.load() + mInterval;
                state = TimerState::WAITING;
            }
        }

        if (triggerTimeout && mTimeoutCallback)
            mTimeoutCallback();
    }
}

OneShotTimer::TimerState OneShotTimer::checkForResetAndStop(TimerState state) {
    // Stop takes precedence of the reset.
    if (mStopTriggered.exchange(false))
        return TimerState::STOPPED;

    // If the state was stopped, the thread was joined, and we cannot reset
    // the timer anymore.
    if (state != TimerState::STOPPED && mResetTriggered.exchange(false))
        return TimerState::RESET;

    return state;
}

void OneShotTimer::reset() {
    mLastResetTime = std::chrono::steady_clock::now();
    mResetTriggered = true;
    // If mWaiting is true, then we are guaranteed to be in a block where we are waiting on
    // mSemaphore for a timeout, rather than idling. So we can avoid a sem_post call since we can
    // just check that we triggered a reset on timeout.
    if (!mWaiting)
        LOG_ALWAYS_IF(sem_post(&mSemaphore), "OneShotTimer::reset(), sem_post failed");
}

bool OneShotTimer::isTimerRunning() {
    return mWaiting;
}
