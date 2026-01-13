#include "menu.h"
#include <iostream>

Menu::Menu() {}
void Menu::init(UIRenderer* ui_) { ui = ui_; options.clear(); selected = 0; }

void Menu::open(State s) {
    state = s; selected = 0;
    if (state == State::MAIN) {
        options = {"SINGLEPLAYER","CONNECT","START SERVER","QUIT"};
    } else if (state == State::PAUSE) {
        options = {"RESUME","MAIN MENU","QUIT"};
    }
}
void Menu::close(){ state = State::NONE; }
void Menu::togglePause(){ if (state == State::PAUSE) close(); else open(State::PAUSE); }

void Menu::update(GLFWwindow* window){
    if (state == State::NONE) return;
    // keyboard navigation (simple)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { selected = std::max(0, selected-1); }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { selected = std::min((int)options.size()-1, selected+1); }
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) {
        std::string opt = options[selected];
        if (opt == "SINGLEPLAYER" && onSingleplayer) onSingleplayer();
        if (opt == "START SERVER" && onStartServer) onStartServer();
        if (opt == "CONNECT" && onConnect) onConnect();
        if (opt == "QUIT" && onQuit) onQuit();
    }

    // mouse hover & click handling
    double mx, my; int winW, winH;
    glfwGetCursorPos(window, &mx, &my);
    glfwGetFramebufferSize(window, &winW, &winH);
    int startY = 160;
    int centerX = winW/2;
    for (int i=0;i<(int)options.size();++i){
        float y = (float)(startY + i*40);
        float top = y - 6;
        float bottom = top + 36;
        float left = (float)(centerX - 120);
        float right = (float)(centerX + 120);
        // GLFW's cursor Y is in pixels from top, matching our UI coords
        if (mx >= left && mx <= right && my >= top && my <= bottom) {
            selected = i; // hover
            int mb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (mb == GLFW_PRESS && !mouseWasDown) {
                // click: activate
                std::string opt = options[selected];
                if (opt == "SINGLEPLAYER" && onSingleplayer) onSingleplayer();
                if (opt == "START SERVER" && onStartServer) onStartServer();
                if (opt == "CONNECT" && onConnect) onConnect();
                if (opt == "QUIT" && onQuit) onQuit();
            }
            mouseWasDown = (mb == GLFW_PRESS);
            return; // don't process other items once one is hovered
        }
    }
    // reset mouse state when not over any item
    mouseWasDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
}

void Menu::draw(int windowW, int windowH){
    if (state == State::NONE) return;
    // dim background
    ui->drawRect(0,0,(float)windowW,(float)windowH, 0.0f,0.0f,0.0f,0.6f, windowW, windowH);
    // title: draw logo image if available, otherwise fallback to text
    ui->drawLogo((windowW/2)-120, 40, 240, 80, windowW, windowH);
    if (!ui->hasLogo()) {
        ui->drawText((windowW/2)-60, 80, 8, state==State::MAIN?"MAIN MENU":"PAUSE", 1.0f,1.0f,1.0f,1.0f, windowW, windowH);
    }
    // draw options
    int startY = 160;
    for (int i=0;i<(int)options.size();++i){
        float y = (float)(startY + i*40);
        if (i==selected) {
            // highlight
            ui->drawRect((windowW/2)-120, y-6, 240, 36, 0.2f,0.5f,0.9f,0.9f, windowW, windowH);
            ui->drawText((windowW/2)-100, y, 8, options[i], 1.0f,1.0f,1.0f,1.0f, windowW, windowH);
        } else {
            ui->drawText((windowW/2)-100, y, 8, options[i], 0.8f,0.8f,0.8f,1.0f, windowW, windowH);
        }
    }
}
