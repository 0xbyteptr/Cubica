#include "player.h"
#include "world.h"
#include "inventory.h"
#include "net_client.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static float baseBreakTimeForBlock(BlockType t){
    switch(t){
        case BlockType::DIRT: return 1.0f;
        case BlockType::GRASS: return 1.0f;
        case BlockType::STONE: return 3.0f;
        default: return 1.0f;
    }
}

static float toolSpeedMultiplier(Inventory::ToolType tool, BlockType block){
    // higher multiplier => faster breaking (time = base / multiplier)
    switch(tool){
        case Inventory::TOOL_WOODEN_PICKAXE:
            return (block==BlockType::STONE) ? 1.5f : 1.0f;
        case Inventory::TOOL_STONE_PICKAXE:
            return (block==BlockType::STONE) ? 2.0f : 1.0f;
        case Inventory::TOOL_IRON_PICKAXE:
            return (block==BlockType::STONE) ? 3.0f : 1.0f;
        case Inventory::TOOL_WOODEN_SHOVEL:
            return (block==BlockType::DIRT || block==BlockType::GRASS) ? 1.8f : 1.0f;
        case Inventory::TOOL_STONE_SHOVEL:
            return (block==BlockType::DIRT || block==BlockType::GRASS) ? 2.5f : 1.0f;
        case Inventory::TOOL_IRON_SHOVEL:
            return (block==BlockType::DIRT || block==BlockType::GRASS) ? 3.5f : 1.0f;
        case Inventory::TOOL_HAND:
        default: return 1.0f;
    }
}

Player::Player(float px, float py, float pz)
    : x(px), y(py), z(pz), yaw(0.0f), pitch(0.0f), yVel(0.0f), hasTarget(false), breakProgress(0.0f) {}

bool Player::raycast(World& world, float reach) {
    // compute ray origin and direction
    float radYaw = yaw * 3.14159265f / 180.0f;
    float radPitch = pitch * 3.14159265f / 180.0f;
    float dx = std::cos(radPitch) * std::cos(radYaw);
    float dy = std::sin(radPitch);
    float dz = std::cos(radPitch) * std::sin(radYaw);
    float ox = x;
    float oy = y;
    float oz = z;

    float step = 0.1f;
    for (float t = 0.0f; t <= reach; t += step) {
        float rx = ox + dx * t;
        float ry = oy + dy * t;
        float rz = oz + dz * t;
        int bx0 = static_cast<int>(std::floor(rx));
        int by0 = static_cast<int>(std::floor(ry));
        int bz0 = static_cast<int>(std::floor(rz));
        // ensure chunk exists
        int cx = bx0 / CHUNK_SIZE; if (bx0<0 && bx0%CHUNK_SIZE) cx -= 1;
        int cz = bz0 / CHUNK_SIZE; if (bz0<0 && bz0%CHUNK_SIZE) cz -= 1;
        world.generateChunk(cx, cz);
        auto c = world.getChunk(cx, cz);
        if (!c) continue;
        int lx = bx0 - cx * CHUNK_SIZE;
        int lz = bz0 - cz * CHUNK_SIZE;
        if (lx>=0 && lx<CHUNK_SIZE && lz>=0 && lz<CHUNK_SIZE && by0>=0 && by0<CHUNK_HEIGHT) {
            if (c->getBlock(lx,by0,lz).isSolid()) {
                bx = bx0; by = by0; bz = bz0;
                hasTarget = true;
                return true;
            }
        }
    }
    hasTarget = false;
    return false;
}

void Player::update(GLFWwindow* window, float dt, World& world, Inventory& inv) {
    // mouse look
    static double lastX = 400.0, lastY = 300.0;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float dx = static_cast<float>(xpos - lastX);
    float dy = static_cast<float>(ypos - lastY);
    lastX = xpos; lastY = ypos;

    const float mouseSensitivity = 0.1f;
    yaw += dx * mouseSensitivity;
    pitch -= dy * mouseSensitivity;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // movement
    float forward = 0.0f;
    float right = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) right += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) right -= 1.0f;

    float rad = yaw * 3.14159265f / 180.0f;
    float fx = std::cos(rad);
    float fz = std::sin(rad);
    // right vector (-fz, fx)
    float rx = -fz;
    float rz = fx;

    float len = std::sqrt(forward*forward + right*right);
    if (len > 0.0f) {
        forward /= len; right /= len;
    }

    const float speed = 6.0f; // m/s
    x += (fx * forward + rx * right) * speed * dt;
    z += (fz * forward + rz * right) * speed * dt;

    // switching hotbar with number keys
    for (int i=0;i<9;++i){
        int key = GLFW_KEY_1 + i; if (key>GLFW_KEY_9) break;
        if (glfwGetKey(window, key) == GLFW_PRESS) inv.selected = i;
    }

    // gravity and jumping
    bool grounded = false;
    float groundY = world.getHeightAt(x, z) + eyeHeight;
    if (y <= groundY + 1e-4f) {
        grounded = true;
        y = groundY;
        yVel = 0.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && grounded) {
        yVel = 5.0f; // jump impulse
        grounded = false;
    }

    if (!grounded) {
        yVel -= 9.8f * dt; // gravity
        y += yVel * dt;
    }

    // clamp y within world bounds
    if (y < 0.0f) { y = 0.0f; yVel = 0.0f; }

    // breaking logic: reset progress when target or selected slot changes
    bool hit = raycast(world, 6.0f);
    if (!hit) { breakProgress = 0.0f; }
    else {
        if (bx != prev_bx || by != prev_by || bz != prev_bz || inv.selected != prev_selected_slot) {
            breakProgress = 0.0f;
            prev_bx = bx; prev_by = by; prev_bz = bz; prev_selected_slot = inv.selected;
        }
        int mb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        // compute break time based on selected tool and block
        auto bt = world.getBlockAt(bx, by, bz).type;
        float base = baseBreakTimeForBlock(bt);
        Inventory::ToolType tool = inv.getSelectedTool();
        float mult = toolSpeedMultiplier(tool, bt);
        float breakTime = base / mult;

        if (mb == GLFW_PRESS) {
            breakProgress += dt / breakTime;
            if (breakProgress >= 1.0f) {
                // break the block
                // if a network client is present, send SET command to server; otherwise apply locally
                if (g_netClient) {
                    char buf[128]; snprintf(buf, sizeof(buf), "SET %d %d %d %d", bx,by,bz, static_cast<int>(BlockType::AIR));
                    g_netClient->sendLine(buf);
                } else {
                    world.setBlockAt(bx, by, bz, {BlockType::AIR});
                }
                breakProgress = 0.0f;
            }
        } else if (mb == GLFW_RELEASE) {
            // reset progress on release
            breakProgress = 0.0f;
        }
    }
}
