#ifndef SELF_TRAINING_MODE_H
#define SELF_TRAINING_MODE_H

#include "game_mode.h"
#include <iostream>

class SelfTrainingMode : public GameMode {
public:
    SelfTrainingMode();
    explicit SelfTrainingMode(const std::string& text);
    ~SelfTrainingMode();

    void start() override;
    void process_input(const std::string& input) override;
    void end() override;
    void display_results() override;

    Result get_result() const { return last_result_; }

private:
    Result last_result_;
};

#endif
