#pragma once
#include <vector>
#include <chrono>

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
    Passthrough
};

class Recorder {
public:
    Recorder (GLFWwindow *window);
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
};

}