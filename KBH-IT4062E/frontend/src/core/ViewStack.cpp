#include "ViewStack.h"
#include "../app/App.h"

void ViewStack::clearAndPush(std::unique_ptr<View> v) {
    while (!stack.empty()) {
        stack.back()->onExit();
        stack.pop_back();
    }
    push(std::move(v));
}

void ViewStack::push(std::unique_ptr<View> v) {
    if (!v) return;
    v->setApp(app);
    v->onEnter();
    stack.push_back(std::move(v));
}

void ViewStack::pop() {
    if (stack.empty()) return;
    stack.back()->onExit();
    stack.pop_back();
}

void ViewStack::replaceTop(std::unique_ptr<View> v) {
    if (stack.empty()) {
        push(std::move(v));
        return;
    }
    stack.back()->onExit();
    stack.pop_back();
    push(std::move(v));
}

View* ViewStack::top() {
    if (stack.empty()) return nullptr;
    return stack.back().get();
}

View* ViewStack::belowTop() {
    if (stack.size() < 2) return nullptr;
    return stack[stack.size() - 2].get();
}

void ViewStack::handleEvent(const SDL_Event& e) {
    if (top()) top()->handleEvent(e);
}

void ViewStack::update(float dt) {
    if (top()) top()->update(dt);
}

void ViewStack::render(SDL_Renderer* r) {
    for (auto& v : stack) v->render(r);
}
