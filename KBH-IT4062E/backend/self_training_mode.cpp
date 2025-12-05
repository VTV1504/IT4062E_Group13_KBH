#include "self_training_mode.h"

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
    typed_text_ = input;
    record_end();
    last_result_ = compute_result(typed_text_);
}

void SelfTrainingMode::end() {
    std::cout << "Ending Self-training Mode...\n";
}

void SelfTrainingMode::display_results() {
    std::cout << "Self-training results:\n";
    std::cout << "WPM: " << last_result_.wpm 
              << " | Accuracy: " << last_result_.accuracy << "%\n";
}
