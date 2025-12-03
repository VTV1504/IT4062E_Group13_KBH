#ifndef GUI_H
#define GUI_H

#include <string>

class GUI {
public:
    GUI();  // Constructor
    void start_game();  // Bắt đầu trò chơi
    void update_score(int score);  // Cập nhật điểm số
    void show_results(const std::string& results);  // Hiển thị kết quả

private:
    // TODO: Các đối tượng giao diện, như button, textbox, v.v.
};

#endif
