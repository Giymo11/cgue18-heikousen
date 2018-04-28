#include <GLFW/glfw3.h>
#include <debug_trap.h>
#include "jojo_replay.hpp"

namespace Replay {

Recorder::Recorder (GLFWwindow *window) :
    mState(RecorderState::Recording),
    mStorage (PHYSICS_FPS * MAX_RECORD_TIME),
    mWindow(window),
    mCurrentTick(0) {}

int Recorder::getKey (int key) {

}

void Recorder::getCursorPos (double *x, double *y) {
    glfwGetCursorPos (mWindow, x, y);
}

bool Recorder::nextTick () {
    if (mCurrentTick + 1 < PHYSICS_FPS * MAX_RECORD_TIME) {
        mCurrentTick += 1;
        return true;
    }

    mState = RecorderState::Passthrough;
    return false;
}

}