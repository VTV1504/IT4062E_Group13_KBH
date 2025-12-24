#include "Client.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <sys/socket.h>   // recv
#include <unistd.h>

#include <queue>
#include <mutex>
#include <condition_variable>

// ---- MessageQueue (copy từ phần 1) ----
class MessageQueue {
public:
    void push(std::string msg) {
        {
            std::lock_guard<std::mutex> lk(m_);
            q_.push(std::move(msg));
        }
        cv_.notify_one();
    }

    bool waitPop(std::string& out, bool& running) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&] { return !q_.empty() || !running; });
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    void drainToStdout() {
        std::lock_guard<std::mutex> lk(m_);
        while (!q_.empty()) {
            std::cout << q_.front() << std::flush;
            q_.pop();
        }
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::string> q_;
};
// --------------------------------------

static void receiverLoop(int sockfd, std::atomic<bool>& running, MessageQueue& mq) {
    std::string buffer;
    buffer.reserve(4096);

    char tmp[2048];

    while (running) {
        ssize_t n = recv(sockfd, tmp, sizeof(tmp), 0);
        if (n <= 0) {
            running = false;
            break;
        }

        buffer.append(tmp, (size_t)n);

        // Tách và push ALL complete lines ngay lập tức
        for (;;) {
            auto pos = buffer.find('\n');
            if (pos == std::string::npos) break;

            std::string line = buffer.substr(0, pos + 1);
            buffer.erase(0, pos + 1);

            mq.push(std::move(line));
        }
    }

    // Nếu server close mà vẫn còn text chưa có '\n', vẫn in nốt (optional)
    if (!buffer.empty()) {
        mq.push(buffer);
    }
}

static void printerLoop(std::atomic<bool>& running, MessageQueue& mq) {
    std::string msg;
    bool runFlag = true; // bridge cho waitPop signature
    while (running) {
        runFlag = running.load();
        if (!mq.waitPop(msg, runFlag)) break;

        // In message vừa pop
        std::cout << msg << std::flush;

        // Drain thêm những message đã có sẵn để in “1 lượt”
        mq.drainToStdout();
    }

    // Drain cuối cùng
    mq.drainToStdout();
}

int main() {
    Client client;
    if (!client.connectTo("127.0.0.1", 5000)) {
        std::cerr << "Failed to connect\n";
        return 1;
    }

    std::atomic<bool> running{true};
    MessageQueue mq;

    // Thread nhận server -> queue
    std::thread tRecv(receiverLoop, client.fd(), std::ref(running), std::ref(mq));

    // Thread in queue -> stdout (giống nc: server nói gì in ngay)
    std::thread tPrint(printerLoop, std::ref(running), std::ref(mq));

    // Main thread: stdin -> server (giống nc)
    std::string input;
    while (running && std::getline(std::cin, input)) {
        if (!client.sendLine(input)) {
            running = false;
            break;
        }
    }

    running = false;
    client.close();

    if (tRecv.joinable()) tRecv.join();
    if (tPrint.joinable()) tPrint.join();

    return 0;
}
