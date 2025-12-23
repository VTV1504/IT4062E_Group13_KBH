#include <iostream>
#include <vector>
#include <string>

#include "typing_engine/typing_session.h"

int main() {
    std::string text = "hello world typing test";
    TypingSession session(text);

    using KeyType = TypingSession::KeyType;
    using InputEvent = TypingSession::InputEvent;
    using InputBatch = TypingSession::InputBatch;

    InputBatch batch;
    batch.start_ts = 0.0;

    batch.events = {
        {KeyType::CHAR, 'h', 0},
        {KeyType::CHAR, 'e', 50},
        {KeyType::CHAR, 'l', 100},
        {KeyType::CHAR, 'l', 150},
        {KeyType::CHAR, 'x', 200},
        {KeyType::SPACE, 0, 250},

        {KeyType::CHAR, 'w', 300},
        {KeyType::CHAR, 'o', 350},
        {KeyType::CHAR, 'r', 400},
        {KeyType::CHAR, 'l', 450},
        {KeyType::CHAR, 'd', 500},
        {KeyType::SPACE, 0, 550},

        {KeyType::CHAR, 't', 600},
        {KeyType::CHAR, 'y', 650},
        {KeyType::CHAR, 'p', 700},
        {KeyType::CHAR, 'x', 750},     // typo
        {KeyType::BACKSPACE, 0, 800},  // fix
        {KeyType::CHAR, 'i', 850},
        {KeyType::CHAR, 'n', 900},
        {KeyType::CHAR, 'g', 950},
        {KeyType::SPACE, 0, 1000},

        {KeyType::CHAR, 't', 300},
        {KeyType::CHAR, 'e', 350},
        {KeyType::CHAR, 's', 400},
        {KeyType::CHAR, 't', 450},
        {KeyType::SPACE, 0, 550},
    };

    session.on_batch(batch);

    const TypingMetrics& m = session.metrics();

    std::cout << "===== Typing Engine Test =====\n";
    std::cout << "Target text: " << text << "\n\n";

    std::cout << "Accuracy (%): " << m.accuracy() << "\n";
    std::cout << "WPM: " << m.wpm() << "\n";

    std::cout << "================================\n";

    return 0;
}
