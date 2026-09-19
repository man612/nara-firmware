#include "nara_face_controller.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace {
void TestSleepingEyesClose() {
    NaraFaceController face(1);
    face.SetInteraction(NaraInteractionState::Sleeping);
    face.Tick(100);
    assert(face.state().left_eye_open <= 0.07f);
    assert(face.state().right_eye_open <= 0.07f);
}

void TestSpeakingMouthTracksLevel() {
    NaraFaceController face(2);
    face.SetInteraction(NaraInteractionState::Speaking);
    face.SetSpeechLevel(1.0f);
    face.Tick(100);
    assert(face.state().mouth_open > 0.9f);

    face.SetSpeechLevel(0.0f);
    face.Tick(200);
    assert(face.state().mouth_open < 0.2f);
}

void TestManualGazeClamps() {
    NaraFaceController face(3);
    face.SetGaze(9.0f, -9.0f);
    for (uint32_t now = 100; now <= 700; now += 100) {
        face.Tick(now);
    }
    assert(face.state().gaze_x <= 1.0f);
    assert(face.state().gaze_x > 0.95f);
    assert(face.state().gaze_y >= -1.0f);
    assert(face.state().gaze_y < -0.95f);
}

void TestBlinkEventuallyRuns() {
    NaraFaceController face(4);
    bool observed_closed = false;

    for (uint32_t now = 10; now <= 7000; now += 10) {
        face.Tick(now);
        if (face.state().left_eye_open < 0.35f) {
            observed_closed = true;
            break;
        }
    }

    assert(observed_closed);
}

void TestEmotionAndInteractionAreIndependent() {
    NaraFaceController face(5);
    face.SetInteraction(NaraInteractionState::Speaking);
    face.SetEmotion(NaraEmotion::Shy, 0.8f);
    face.Tick(100);

    assert(face.state().interaction == NaraInteractionState::Speaking);
    assert(face.state().emotion == NaraEmotion::Shy);
    assert(std::abs(face.state().emotion_intensity - 0.8f) < 0.001f);
}
}  // namespace

int main() {
    TestSleepingEyesClose();
    TestSpeakingMouthTracksLevel();
    TestManualGazeClamps();
    TestBlinkEventuallyRuns();
    TestEmotionAndInteractionAreIndependent();

    std::cout << "Nara face controller tests passed\n";
    return 0;
}
