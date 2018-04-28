#pragma once
#include <vector>

// Forward declarations
struct GLFWwindow;

namespace Replay {

static const size_t PHYSICS_FPS     = 60;
static const size_t MAX_RECORD_TIME = 600;

struct State {
    uint64_t buttonState;
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
    bool nextTick ();

private:
    GLFWwindow         *mWindow;
    std::vector<State>  mStorage;
    RecorderState       mState;
    size_t              mCurrentTick;
};

}