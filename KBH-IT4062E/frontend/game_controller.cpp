#include "game_controller.h"
#include <iostream>

// Constructor
GameController::GameController() {
    // TODO: Khởi tạo đối tượng socket_client_ để kết nối tới server
    // TODO: Khởi tạo đối tượng gui_ để hiển thị giao diện người chơi
}

// Destructor
GameController::~GameController() {
    // TODO: Giải phóng tài nguyên nếu cần thiết
}

// Hàm bắt đầu game
void GameController::start_game(const std::string& mode) {
    // TODO: Kết nối với server qua UNIX Socket
    socket_client_.connect_to_server();

    // TODO: Gửi yêu cầu đến server để bắt đầu chế độ chơi
    std::string message = "START GAME " + mode;
    socket_client_.send_message(message);

    // TODO: Khởi tạo giao diện cho chế độ chơi
    gui_.start_game();
}

// Hàm xử lý đầu vào từ người chơi
void GameController::process_input(const std::string& input) {
    // TODO: Gửi đầu vào từ người chơi đến server qua UNIX Socket
    socket_client_.send_message(input);

    // TODO: Cập nhật giao diện với thông tin nhập vào (ví dụ: WPM, độ chính xác)
    gui_.update_score(100);  // Ví dụ cập nhật điểm số tạm thời
}

// Hàm nhận kết quả từ server và cập nhật giao diện
void GameController::receive_results() {
    // TODO: Nhận kết quả từ server
    std::string results = socket_client_.receive_message();

    // TODO: Cập nhật giao diện với kết quả nhận được
    gui_.show_results(results);
}
