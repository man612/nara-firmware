#include "touch_classifier.h"

#include <cassert>

static NaraTouchSample S(bool pressed, float x, float y, uint32_t ms) {
    return {.pressed = pressed, .x = x, .y = y, .timestamp_ms = ms};
}

int main() {
    {
        NaraTouchClassifier c;
        assert(c.Update(S(true, 100, 100, 0)) == NaraTouchGesture::None);
        assert(c.Update(S(false, 100, 100, 100)) == NaraTouchGesture::None);
        assert(c.Update(S(false, 100, 100, 500)) == NaraTouchGesture::Tap);
    }

    {
        NaraTouchClassifier c;
        c.Update(S(true, 100, 100, 0));
        c.Update(S(false, 100, 100, 100));
        c.Update(S(true, 108, 104, 220));
        assert(c.Update(S(false, 108, 104, 300)) ==
               NaraTouchGesture::DoubleTap);
    }

    {
        NaraTouchClassifier c;
        c.Update(S(true, 100, 100, 0));
        assert(c.Update(S(true, 103, 102, 700)) ==
               NaraTouchGesture::Hold);
        assert(c.Update(S(false, 103, 102, 800)) ==
               NaraTouchGesture::None);
    }

    {
        NaraTouchClassifier c;
        c.Update(S(true, 30, 100, 0));
        c.Update(S(true, 100, 100, 180));
        assert(c.Update(S(false, 150, 100, 300)) ==
               NaraTouchGesture::Stroke);
    }

    return 0;
}
