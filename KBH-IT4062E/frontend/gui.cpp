#include "gui.h"
#include <iostream>

// TODO: Cài đặt giao diện người chơi (có thể dùng Qt, SDL2, v.v.)

GUI::GUI() {
    // TODO: Khởi tạo giao diện, các widget (button, textfield, v.v.)
}

void GUI::start_game() {
    std::cout << "Game Started\n";
    // TODO: Bắt đầu trò chơi (hiển thị chế độ, các thông tin game)
}

void GUI::update_score(int score) {
    std::cout << "Score: " << score << std::endl;
    // TODO: Cập nhật điểm số trong giao diện
}

void GUI::show_results(const std::string& results) {
    std::cout << "Results: " << results << std::endl;
    // TODO: Hiển thị kết quả trên giao diện
}
