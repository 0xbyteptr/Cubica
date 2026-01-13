#pragma once

#include <GLFW/glfw3.h>

class World; // forward

class Player {
public:
    float x, y, z;
    float yaw, pitch; // in degrees
    float yVel;
    const float eyeHeight = 2.8f;

    // breaking state
    int bx, by, bz; // targeted block coords
    bool hasTarget;
    float breakProgress; // 0..1

    Player(float px = 0.0f, float py = 0.0f, float pz = 0.0f);

    // update player based on input; uses world for ground queries and inventory for tools
    void update(GLFWwindow* window, float dt, World& world, class Inventory& inv);

    // run a raycast from eye, returns true if hit, fills bx,by,bz
    bool raycast(World& world, float reach = 5.0f);

    // previous target & selection tracking to reset break progress when changed
    int prev_bx = -9999, prev_by = -9999, prev_bz = -9999;
    int prev_selected_slot = -1;
};
