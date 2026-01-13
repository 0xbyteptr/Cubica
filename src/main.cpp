#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include "world.h"
#include "player.h"
#include "shader.h"
#include "math.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"
#include "resourcepack.h"
#include "ui.h"
#include "inventory.h"
#include "player_renderer.h"
#include "net_server.h"
#include "net_client.h"
#include <thread>
#include <chrono>
#include <string>
#include "menu.h"

int main(int argc, char** argv) {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // make the window resizable
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Cubica", NULL, NULL);

    // set a framebuffer size callback to update viewport when window resizes
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height){
        glViewport(0, 0, width, height);
    });
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // load voxel shader
    Shader voxelShader;
    if (!voxelShader.loadFromFiles("shaders/voxel.vert", "shaders/voxel.frag")) {
        std::cerr << "Failed to load voxel shader\n";
        return -1;
    }

    // init UI renderer
    UIRenderer ui; ui.init();
    // try to load embedded font if present
    if (!ui.loadFont("assets/font.otf", 24)) {
        ui.loadBuiltInFont();
    }
    // try to load a logo image for the main menu
    ui.loadLogo("assets/logo.png");

    // player renderer (skins)
    PlayerRenderer playerRenderer; playerRenderer.loadSkin(nullptr);

    // set up player and world
    World world;

    // try to load a resource pack from assets/resourcepack or ~/.Cubica/resources/base.zip
    ResourcePack rp;
    bool rpLoaded = rp.loadFromDir("assets/resourcepack");
    TextureAtlas fallbackAtlas;
    TextureAtlas* activeAtlas = nullptr;

    // if resource pack loaded, use that atlas; otherwise fallback to procedural atlas
    if (rpLoaded) {
        world.setResourcePack(&rp);
        activeAtlas = &rp.atlas;
        playerRenderer.loadSkin(&rp);
    } else {
        if (!fallbackAtlas.create(16, 5)) {
            std::cerr << "Failed to create texture atlas\n";
            return -1;
        }
        activeAtlas = &fallbackAtlas;
    }

    // bind atlas to texture unit 0 and inform shader
    activeAtlas->bind(0);
    voxelShader.use();
    voxelShader.setInt("atlas", 0);
    voxelShader.setInt("atlasTiles", activeAtlas->tiles);

    // command-line flags: --server, --port <port>, --connect <host:port>
    bool runServer = false; int serverPort = 69696; std::string connectHost;
    for (int i=1;i<argc;i++) {
        std::string a = argv[i];
        if (a == "--server") runServer = true;
        else if (a == "--port" && i+1<argc) { serverPort = std::stoi(argv[++i]); }
        else if (a == "--connect" && i+1<argc) { connectHost = argv[++i]; }
    }

    NetServer *server = nullptr; NetClient *client = nullptr;
    if (runServer) {
        server = new NetServer(serverPort);
        if (!server->start(&world)) { std::cerr<<"Failed to start server\n"; delete server; server=nullptr; }
        else {
            // dedicated server mode: don't start rendering loop; run simple loop
            std::cout << "Running dedicated server. Press Ctrl-C to stop.\n";
            while (true) std::this_thread::sleep_for(std::chrono::seconds(60));
            // unreachable
        }
    }

    if (!connectHost.empty()) {
        // parse host:port
        std::string h = connectHost; int p = 25565;
        auto pos = h.find(':'); if (pos != std::string::npos) { p = std::stoi(h.substr(pos+1)); h = h.substr(0,pos); }
        client = new NetClient();
        if (client->start(h,p)) g_netClient = client; else { delete client; client = nullptr; }
    }

    // start background pregeneration (only block data) for a small radius
    world.pregenerateAsync(4);

    Player player(0.0f, 0.0f, 0.0f);

    // initialize player height
    player.y = world.getHeightAt(player.x, player.z) + player.eyeHeight;

    // inventory
    Inventory inv;

    // menu
    Menu menu; menu.init(&ui);
    // start with main menu open
    menu.open(Menu::State::MAIN);

    // set cursor mode according to menu state (show cursor while in menu)
    if (menu.isOpen()) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // FPS / debug overlay state
    int frames = 0;
    double fpsTimer = 0.0;
    double fps = 0.0;
    bool showDebug = false;
    int prevF3State = GLFW_RELEASE;
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    // wire up menu actions
    menu.onSingleplayer = [&](){ menu.close(); /* when starting singleplayer, capture mouse for look */ glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); glfwSetCursorPos(window, 400, 300); };
    menu.onStartServer = [&](){ std::cout<<"Menu: starting local server..."<<std::endl; NetServer *s = new NetServer(25565); if (!s->start(&world)) { std::cerr<<"Failed to start server from menu\n"; delete s; } };
    menu.onConnect = [&](){ std::cout<<"Menu: to connect, run with --connect host:port or use console for now"<<std::endl; };
    menu.onQuit = [&](){ glfwSetWindowShouldClose(window, true); };


    // capture mouse for look (if menu is not open)
    if (!menu.isOpen()){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        // set initial cursor position to center to avoid large initial jump
        glfwSetCursorPos(window, 400, 300);
    } else {
        // show cursor when a menu is open
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        // update
        if (!menu.isOpen()) {
            player.update(window, dt, world, inv);
            // update world meshes on GL thread
            world.processMeshQueue(2);
        } else {
            // when menu is open, update menu input
            menu.update(window);
        }

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // sky color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // prepare camera
        int width, height; glfwGetFramebufferSize(window, &width, &height);
        float aspect = (height > 0) ? (float)width / (float)height : 4.0f/3.0f;
        Math::Mat4 proj = Math::perspective(70.0f, aspect, 0.1f, 1000.0f);
        Math::Vec3 eye{player.x, player.y, player.z};
        float radYaw = player.yaw * 3.14159265f / 180.0f;
        float radPitch = player.pitch * 3.14159265f / 180.0f;
        Math::Vec3 dir{std::cos(radPitch) * std::cos(radYaw), std::sin(radPitch), std::cos(radPitch) * std::sin(radYaw)};
        Math::Vec3 center{player.x + dir.x, player.y + dir.y, player.z + dir.z};
        Math::Vec3 up{0.0f,1.0f,0.0f};
        Math::Mat4 view = Math::lookAt(eye, center, up);

        // render all chunk meshes
        activeAtlas->bind(0);
        voxelShader.use();
        voxelShader.setMat4("projection", proj.data());
        voxelShader.setMat4("view", view.data());
        Math::Mat4 model = Math::identity();
        voxelShader.setMat4("model", model.data());
        // set snow line relative to world (could be dynamic)
        voxelShader.setFloat("snowLine", 80.0f);
        voxelShader.setFloat("snowBlendRange", 8.0f);

        // process a couple mesh rebuilds per frame on the GL thread
        world.processMeshQueue(2);

        for (auto &p : world.chunks) {
            Chunk* c = p.second;
            if (c && c->mesh) c->mesh->draw();
        }

        // FPS counting and F3 debug overlay toggle
        frames++;
        fpsTimer += dt;
        int f3State = glfwGetKey(window, GLFW_KEY_F3);
        if (f3State == GLFW_PRESS && prevF3State == GLFW_RELEASE) {
            showDebug = !showDebug;
            if (showDebug) {
                std::cout << "--- DEBUG ON ---\n";
            } else {
                std::cout << "--- DEBUG OFF ---\n";
            }
        }
        prevF3State = f3State;

        // toggle player model HUD with F5
        static int prevF5 = GLFW_RELEASE;
        int f5State = glfwGetKey(window, GLFW_KEY_F5);
        if (f5State == GLFW_PRESS && prevF5 == GLFW_RELEASE) {
            playerRenderer.toggleVisible();
        }
        prevF5 = f5State;

        // toggle pause menu with ESC
        static int prevEsc = GLFW_RELEASE;
        int escState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        if (escState == GLFW_PRESS && prevEsc == GLFW_RELEASE) {
            if (menu.getState() == Menu::State::NONE) {
                menu.open(Menu::State::PAUSE);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else if (menu.getState() == Menu::State::PAUSE) {
                menu.close();
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        prevEsc = escState;
        if (fpsTimer >= 0.5) {
            fps = frames / fpsTimer;
            frames = 0;
            fpsTimer = 0.0;

            // build title
            std::ostringstream title;
            title.setf(std::ios::fixed); title.precision(1);
            title << "Cubica - FPS: " << fps << " | Pos: (" << player.x << ", " << player.y << ", " << player.z << ")";
            title << " | Chunks: " << world.getChunkCount();
            if (showDebug) {
                title << " | PendingMesh: " << world.getPendingMeshCount();
                title << " | RP: " << (world.resourcePack ? "yes" : "none");
                title << " | Renderer: " << (renderer ? renderer : "unknown");
            }
            glfwSetWindowTitle(window, title.str().c_str());
        }

        // draw UI
        int winW, winH; glfwGetFramebufferSize(window, &winW, &winH);
        inv.draw(ui, winW, winH, world.resourcePack);

        // draw breaking overlay progress at center if breaking
        if (player.hasTarget && player.breakProgress > 0.0f) {
            // choose destroy stage 0..9
            int stage = static_cast<int>(player.breakProgress * 10.0f);
            if (stage < 0) stage = 0; if (stage > 9) stage = 9;
            int tile = 0;
            if (world.resourcePack) {
                auto it = world.resourcePack->nameToIndex.find(std::string("destroy_") + std::to_string(stage));
                if (it != world.resourcePack->nameToIndex.end()) tile = it->second;
            } else {
                // when no RP, fallback: assume destroy stages appended after tiles, compute index
                tile = 5 + stage; // because procedural atlas now has 5 tiles
            }
            ui.drawSprite((winW/2)-32, (winH/2)-32, 64, 64, tile, world.resourcePack ? world.resourcePack->atlas.tiles : 15, winW, winH);
        }

        // draw player model HUD if visible
        playerRenderer.drawHUD(winW, winH);

        // draw menu overlay if open
        if (menu.isOpen()) {
            menu.draw(winW, winH);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}