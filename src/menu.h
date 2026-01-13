#pragma once
#include <vector>
#include <string>
#include "ui.h"
#include <GLFW/glfw3.h>
#include <functional>

class Menu {
public:
    enum class State { MAIN, PAUSE, NONE };
    Menu();
    void init(UIRenderer* ui);
    void open(State s);
    void close();
    void togglePause();
    State getState() const { return state; }
    void update(GLFWwindow* window);
    void draw(int windowW, int windowH);
    bool isOpen() const { return state != State::NONE; }
    // action callbacks
    std::function<void()> onSingleplayer;
    std::function<void()> onStartServer;
    std::function<void()> onConnect;
    std::function<void()> onQuit;
private:
    UIRenderer* ui = nullptr;
    State state = State::MAIN;
    std::vector<std::string> options;
    int selected = 0;
    // mouse click debouncing
    bool mouseWasDown = false;
};