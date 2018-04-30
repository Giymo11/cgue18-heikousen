#include <GLFW/glfw3.h>
#include <debug_trap.h>
#include <algorithm>
#include "jojo_replay.hpp"

#define SET_BIT(s,n,b) (s ^= (-b ^ s) & (1UL << n));
#define GET_BIT(s,n) (s >> n & 1)

namespace Replay {

static const float PHYSICS_FRAMETIME = 16.0f;
static const size_t PHYSICS_FPS = 60;
static const size_t MAX_RECORD_TIME = 600;
static const size_t MAX_SLICES = PHYSICS_FPS * MAX_RECORD_TIME;

Recorder::Recorder (GLFWwindow *window) :
    mState(RecorderState::Passthrough),
    mStorage(MAX_SLICES),
    mWindow(window),
    mCurrentTick(0),
    mTicksRecorded(0),
    mLastMeasurement (std::chrono::high_resolution_clock::now ()),
    mResetFunc ([]() {}) {}

void Recorder::setResetFunc (const std::function<void ()> &func) {
    mResetFunc = func;
}

int Recorder::getKey (int key) {
    auto &buttonState = mStorage[mCurrentTick].buttonState;
    auto keyState = static_cast<int64_t>(glfwGetKey (mWindow, key)) & 1;

    switch (mState) {
    case RecorderState::Recording:
        switch (key) {
        case GLFW_KEY_W:
            SET_BIT (buttonState, 0, keyState);
            break;
        case GLFW_KEY_S:
            SET_BIT (buttonState, 1, keyState);
            break;
        case GLFW_KEY_A:
            SET_BIT (buttonState, 2, keyState);
            break;
        case GLFW_KEY_D:
            SET_BIT (buttonState, 3, keyState);
            break;
        case GLFW_KEY_R:
            SET_BIT (buttonState, 4, keyState);
            break;
        case GLFW_KEY_F:
            SET_BIT (buttonState, 5, keyState);
            break;
        case GLFW_KEY_X:
            SET_BIT (buttonState, 6, keyState);
            break;
        case GLFW_KEY_Z:
            SET_BIT (buttonState, 7, keyState);
            break;
        case GLFW_KEY_Q:
            SET_BIT (buttonState, 8, keyState);
            break;
        case GLFW_KEY_E:
            SET_BIT (buttonState, 9, keyState);
            break;
        default:
            break;
        }
        return static_cast<int>(keyState);
    case RecorderState::Replaying:
    case RecorderState::ReplayFinished:
        switch (key) {
        case GLFW_KEY_W:
            return GET_BIT (buttonState, 0);
        case GLFW_KEY_S:
            return GET_BIT (buttonState, 1);
        case GLFW_KEY_A:
            return GET_BIT (buttonState, 2);
        case GLFW_KEY_D:
            return GET_BIT (buttonState, 3);
        case GLFW_KEY_R:
            return GET_BIT (buttonState, 4);
        case GLFW_KEY_F:
            return GET_BIT (buttonState, 5);
        case GLFW_KEY_X:
            return GET_BIT (buttonState, 6);
        case GLFW_KEY_Z:
            return GET_BIT (buttonState, 7);
        case GLFW_KEY_Q:
            return GET_BIT (buttonState, 8);
        case GLFW_KEY_E:
            return GET_BIT (buttonState, 9);
        default:
            return 0;
        }
    default:
        return static_cast<int>(keyState);
    }
}

void Recorder::getCursorPos (double *x, double *y) {
    auto &buttonState = mStorage[mCurrentTick];

    switch (mState) {
    case RecorderState::Recording:
        glfwGetCursorPos (mWindow, x, y);
        buttonState.mouseX = static_cast<float>(*x);
        buttonState.mouseY = static_cast<float>(*y);
        break;
    case RecorderState::Replaying:
    case RecorderState::ReplayFinished:
        *x = buttonState.mouseX;
        *y = buttonState.mouseY;
        break;
    default:
        glfwGetCursorPos (mWindow, x, y);
    }
}

void Recorder::startRecording () {
    mCurrentTick = 0;
    mTicksRecorded = 0;
    mLastMeasurement = std::chrono::high_resolution_clock::now ();
    mState = RecorderState::Recording;
}

void Recorder::startReplay () {
    mCurrentTick = 0;
    mLastMeasurement = std::chrono::high_resolution_clock::now ();
    mState = RecorderState::Replaying;
    mResetFunc ();
}

bool Recorder::nextTickReady () {
    mLastMeasurement = std::chrono::high_resolution_clock::now ();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds> (
        mLastMeasurement - mLastTickTime
    ).count ();
    return delta >= PHYSICS_FRAMETIME;
}

void Recorder::nextTick () {
    mLastTickTime = mLastMeasurement;

    switch (mState) {
    case RecorderState::Recording:
        if (mCurrentTick + 1 < MAX_SLICES) {
            mCurrentTick += 1;
            mTicksRecorded += 1;
            return;
        }

        mState = RecorderState::Passthrough;
        return;
    case RecorderState::Replaying:
        if (mCurrentTick + 1 == std::min (MAX_SLICES, mTicksRecorded))
            mState = RecorderState::ReplayFinished;
        else
            mCurrentTick += 1;
        return;
    case RecorderState::ReplayFinished:
        mState = RecorderState::ReplayFinished;
        return;
    default:
        mState = RecorderState::Passthrough;
        return;
    }    
}

RecorderState Recorder::state () {
    return mState;
}

}