#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include <string>
#include "unix_socket_client.h"
#include "gui.h"

class GameController {
public:
    GameController();  // Constructor
    ~GameController(); // Destructor

    // TODO: Hàm để khởi tạo game, kết nối với server, và bắt đầu chế độ chơi
    void start_game(const std::string& mode);

    // TODO: Hàm gửi thông điệp đầu vào từ người chơi (ví dụ: gõ phím, bấm nút)
    void process_input(const std::string& input);

    // TODO: Hàm để nhận kết quả từ server và cập nhật giao diện
    void receive_results();

private:
    UNIXSocketClient socket_client_;  // Đối tượng client để giao tiếp với server qua UNIX socket
    GUI gui_;                         // Đối tượng giao diện người chơi
};

#endif
