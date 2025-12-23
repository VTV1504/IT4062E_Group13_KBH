#include "typing_engine.h"

TypingEngine::TypingEngine(const std::string& text)
    : target_text_(text) {}

void TypingEngine::add_player(int player_id) {
    sessions_[player_id] = std::make_unique<TypingSession>(target_text_);
}

void TypingEngine::remove_player(int player_id) {
    sessions_.erase(player_id);
}

void TypingEngine::on_key(int player_id, char c, double timestamp_seconds) {
    auto it = sessions_.find(player_id);
    if (it == sessions_.end()) return;
    it->second->on_key(c, timestamp_seconds);
}

void TypingEngine::on_batch(int player_id, const TypingSession::InputBatch& batch) {
    auto it = sessions_.find(player_id);
    if (it == sessions_.end()) return;
    it->second->on_batch(batch);
}

bool TypingEngine::all_finished() const {
    for (const auto& kv : sessions_) {
        if (!kv.second->finished()) return false;
    }
    return true;
}

const TypingSession* TypingEngine::get_session(int player_id) const {
    auto it = sessions_.find(player_id);
    if (it == sessions_.end()) return nullptr;
    return it->second.get();
}
