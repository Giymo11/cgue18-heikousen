#pragma once
#include <vector>
#include <chrono>
#include <functional>

// Forward declarations
struct GLFWwindow;

namespace Replay {

struct State {
    int64_t buttonState;
    float mouseX;
    float mouseY;
};

enum RecorderState : uint8_t {
    Recording,
    Replaying,
    ReplayFinished,
    Passthrough
};

class Recorder {
public:
    Recorder (GLFWwindow *window);
    void setResetFunc (const std::function<void ()> &func);
    int getKey (int key);
    void getCursorPos (double *x, double *y);
    bool nextTickReady ();
    void nextTick ();
    void startRecording ();
    void startReplay ();
    RecorderState state ();

private:
    GLFWwindow                    *mWindow;
    std::vector<State>             mStorage;
    RecorderState                  mState;
    size_t                         mCurrentTick;
    size_t                         mTicksRecorded;
    std::chrono::time_point<
        std::chrono::steady_clock
    >                              mLastTickTime;
    std::chrono::time_point<
        std::chrono::steady_clock
    >                              mLastMeasurement;
    std::function<void ()>         mResetFunc;
};

}