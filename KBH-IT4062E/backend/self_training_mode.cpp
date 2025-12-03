#include "self_training_mode.h"

// Implementations
SelfTrainingMode::SelfTrainingMode() {}
SelfTrainingMode::SelfTrainingMode(const std::string& text) {
    set_target_text(text);
}
SelfTrainingMode::~SelfTrainingMode() {}

void SelfTrainingMode::start() {
    std::cout << "Starting Self-training Mode...\n";
    record_start();
}

void SelfTrainingMode::process_input(const std::string& input) {
    // record typed text and end time, compute result
    typed_text_ = input;
    record_end();
    last_result_ = compute_result(typed_text_);
}

void SelfTrainingMode::end() {
    std::cout << "Ending Self-training Mode...\n";
    // In a full implementation, persist stats for logged-in users here
}

void SelfTrainingMode::display_results() {
    std::cout << "Self-training results:\n";
    std::cout << "WPM: " << last_result_.wpm << " | Accuracy: " << last_result_.accuracy << "%\n";
}
