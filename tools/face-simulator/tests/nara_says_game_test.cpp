#include "nara_says_game.h"

#include <cassert>

int main() {
    {
        NaraSaysGame game;
        game.Start(1234, 1000, 3);
        assert(game.snapshot().active);
        assert(game.snapshot().round == 1);
        assert(game.snapshot().lives == 3);

        auto expected = game.snapshot().expected;
        assert(expected != NaraGameInput::None);
        assert(game.Input(expected, 1500) == NaraGameEvent::Correct);
        assert(game.snapshot().score == 1);
        assert(game.snapshot().round == 2);

        expected = game.snapshot().expected;
        assert(game.Input(expected, 2000) == NaraGameEvent::Correct);
        expected = game.snapshot().expected;
        assert(game.Input(expected, 2500) == NaraGameEvent::Won);
        assert(!game.snapshot().active);
        assert(game.snapshot().score == 3);
    }

    {
        NaraSaysGame game;
        game.Start(9, 100, 5);
        const auto expected = game.snapshot().expected;
        const auto wrong =
            expected == NaraGameInput::Tap
                ? NaraGameInput::Shake
                : NaraGameInput::Tap;
        assert(game.Input(wrong, 200) == NaraGameEvent::Wrong);
        assert(game.snapshot().lives == 2);
        assert(game.Tick(game.snapshot().deadline_ms) ==
               NaraGameEvent::Timeout);
        assert(game.snapshot().lives == 1);
        const auto wrong2 =
            game.snapshot().expected == NaraGameInput::Tap
                ? NaraGameInput::Shake
                : NaraGameInput::Tap;
        assert(game.Input(wrong2, game.snapshot().deadline_ms - 1) ==
               NaraGameEvent::Lost);
        assert(!game.snapshot().active);
    }

    return 0;
}
