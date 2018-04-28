#include <GLFW/glfw3.h>
#include <debug_trap.h>
#include <algorithm>
#include "jojo_replay.hpp"

#define SET_BIT(s,n,b) (s ^= (-b ^ s) & (1UL << n));
#define GET_BIT(s,n) (s >> n & 1)

namespace Replay {

Recorder::Recorder (GLFWwindow *window) :
    mState(RecorderState::Recording),
    mStorage(PHYSICS_FPS * MAX_RECORD_TIME),
    mWindow(window),
    mCurrentTick(0),
    mTicksRecorded(0) {}

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
        *x = buttonState.mouseX;
        *y = buttonState.mouseY;
        break;
    default:
        glfwGetCursorPos (mWindow, x, y);
    }
}

void Recorder::startReplay () {
    mCurrentTick = 0;
    mState = RecorderState::Replaying;
}

bool Recorder::nextTick () {
    switch (mState) {
    case RecorderState::Recording:
        if (mCurrentTick + 1 < PHYSICS_FPS * MAX_RECORD_TIME) {
            mCurrentTick += 1;
            mTicksRecorded += 1;
            return true;
        }

        mState = RecorderState::Passthrough;
        return false;
    case RecorderState::Replaying:
        mCurrentTick = (mCurrentTick + 1) % std::min (PHYSICS_FPS * MAX_RECORD_TIME, mTicksRecorded);
        return true;
    default:
        mState = RecorderState::Passthrough;
        return false;
    }    
}

}