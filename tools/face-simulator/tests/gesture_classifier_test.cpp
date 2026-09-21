#include "gesture_classifier.h"

#include <cassert>

static NaraMotionSample Sample(uint32_t ms, float ax, float ay, float az,
                               float gx = 0, float gy = 0, float gz = 0) {
    return {.ax_g = ax, .ay_g = ay, .az_g = az,
            .gx_dps = gx, .gy_dps = gy, .gz_dps = gz,
            .timestamp_ms = ms};
}

int main() {
    {
        NaraGestureClassifier classifier;
        assert(classifier.Update(Sample(100, 0, 0, 1)) == NaraMotionGesture::None);
        assert(classifier.Update(Sample(500, 0, 0, -1)) == NaraMotionGesture::Flip);
    }

    {
        NaraGestureClassifier classifier;
        assert(classifier.Update(Sample(100, 2.2f, 0, 0)) == NaraMotionGesture::None);
        classifier.Update(Sample(150, 1.0f, 0, 0));
        classifier.Update(Sample(220, -2.1f, 0, 0));
        classifier.Update(Sample(280, -1.0f, 0, 0));
        assert(classifier.Update(Sample(350, 0, 2.0f, 0)) == NaraMotionGesture::Shake);
    }

    {
        NaraGestureClassifier classifier;
        assert(classifier.Update(Sample(100, 0, 0, 1, 0, 0, 330)) == NaraMotionGesture::None);
        assert(classifier.Update(Sample(420, 0, 0, 1, 0, 0, 330)) == NaraMotionGesture::Spin);
    }

    {
        NaraGestureClassifier classifier;
        classifier.Update(Sample(100, 0, 0, 1));
        assert(classifier.Update(Sample(500, 0, 0, -1)) == NaraMotionGesture::Flip);
        assert(classifier.Update(Sample(700, 0, 0, 1)) == NaraMotionGesture::None);
    }

    return 0;
}
