#include "resourcepack.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include "../third_party/stb_image.h"

static bool existsFile(const std::string& p) {
    std::ifstream f(p);
    return f.good();
}

#include <sys/stat.h>
#include <unistd.h>
#include <regex>
#include <dirent.h>
#include <sstream>

static std::string readFileContents(const std::string &p) {
    std::ifstream f(p);
    if (!f) return std::string();
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

static int findMatchingBrace(const std::string &s, size_t start) {
    int depth = 0;
    for (size_t i = start; i < s.size(); ++i) {
        if (s[i] == '{') ++depth;
        else if (s[i] == '}') {
            --depth;
            if (depth == 0) return static_cast<int>(i);
        }
    }
    return -1;
}

// try to extract an entry from a zip into a temporary file and return its path, or empty on failure
static std::string extractZipEntryToTemp(const std::string &zipPath, const std::string &entry) {
    char tmpTemplate[] = "/tmp/cubica_rp_XXXXXX";
    int fd = mkstemp(tmpTemplate);
    if (fd == -1) return std::string();
    close(fd);
    std::string tmpPath(tmpTemplate);
    std::string cmd = "unzip -p '" + zipPath + "' '" + entry + "' > '" + tmpPath + "' 2>/dev/null";
    int res = system(cmd.c_str());
    if (res == 0 && existsFile(tmpPath)) return tmpPath;
    unlink(tmpPath.c_str());
    return std::string();
}

bool ResourcePack::loadFromDir(const std::string& dir) {
    // look for textures under <dir>/assets/minecraft/textures/block/
    std::string base = dir + "/assets/minecraft/textures/block/";

    // candidates we care about (blocks, items, and destroy stages)
    std::vector<std::pair<std::string,std::string>> candidates = {
        {"grass_top","grass_block_top.png"},
        {"grass_side","grass_block.png"},
        {"grass_block_side_overlay","grass_block_side_overlay.png"},
        {"grass_block_overlay","grass_block_overlay.png"},
        {"grass_overlay","grass_overlay.png"},
        {"dirt","dirt.png"},
        {"stone","stone.png"},
        {"grass_block","grass_block.png"},
        {"oak_log","oak_log.png"},
        {"oak_leaves","oak_leaves.png"},
        {"destroy_0","misc/destroy_stage_0.png"},
        {"destroy_1","misc/destroy_stage_1.png"},
        {"destroy_2","misc/destroy_stage_2.png"},
        {"destroy_3","misc/destroy_stage_3.png"},
        {"destroy_4","misc/destroy_stage_4.png"},
        {"destroy_5","misc/destroy_stage_5.png"},
        {"destroy_6","misc/destroy_stage_6.png"},
        {"destroy_7","misc/destroy_stage_7.png"},
        {"destroy_8","misc/destroy_stage_8.png"},
        {"destroy_9","misc/destroy_stage_9.png"}
    };

    std::vector<std::string> paths;
    std::vector<std::string> temps; // temporary extracted files to clean up
    nameToIndex.clear();

    // helper to test if base dir exists
    struct stat sb;
    bool dirExists = (stat(base.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));

    // check for a packaged resource zip at ~/.Cubica/resources/base.zip
    const char* homedir = getenv("HOME");
    std::string zipPath;
    bool zipExists = false;
    if (homedir) {
        zipPath = std::string(homedir) + "/.Cubica/resources/base.zip";
        zipExists = (stat(zipPath.c_str(), &sb) == 0);
    }

    for (auto &c : candidates) {
        std::string p = base + c.second;
        if (dirExists && existsFile(p)) {
            nameToIndex[c.first] = static_cast<int>(paths.size());
            paths.push_back(p);
            continue;
        }

        if (zipExists) {
            // try to extract single file from zip to a temporary file using `unzip -p`
            std::string zipEntry = "assets/minecraft/textures/block/" + c.second;
            char tmpTemplate[] = "/tmp/cubica_rp_XXXXXX";
            int fd = mkstemp(tmpTemplate);
            if (fd == -1) continue;
            close(fd);
            std::string tmpPath(tmpTemplate);

            std::string cmd = "unzip -p '" + zipPath + "' '" + zipEntry + "' > '" + tmpPath + "' 2>/dev/null";
            int res = system(cmd.c_str());
            if (res == 0 && existsFile(tmpPath)) {
                nameToIndex[c.first] = static_cast<int>(paths.size());
                paths.push_back(tmpPath);
                temps.push_back(tmpPath);
                continue;
            } else {
                // failed to extract, remove temp file
                unlink(tmpPath.c_str());
            }
        }
    }

    if (paths.empty()) return false;

    // let atlas load them
    bool ok = atlas.createFromFiles(paths);

    // cleanup temporary extracted files
    for (auto &t : temps) unlink(t.c_str());

    if (!ok) return false;

    // add filename aliases so model texture references can be resolved easily
    // e.g., if we added "grass_top" -> index for grass_block_top.png, also add "grass_block_top" -> same index
    for (auto &p : nameToIndex) {
        // we don't have paths here, but we can derive some common aliases from existing keys
        // add an alias with "_block_" variant: grass_top -> grass_block_top
        const std::string &k = p.first;
        int idx = p.second;
        if (k == "grass_top") {
            nameToIndex["grass_block_top"] = idx;
        }
        if (k == "grass_side") {
            nameToIndex["grass_block"] = idx; // allow referencing block/grass_block
        }
        // overlay aliases
        if (k == "grass_block_side_overlay") {
            nameToIndex["grass_side_overlay"] = idx;
            nameToIndex["grass_overlay"] = idx;
        }
        if (k == "grass_block_overlay") {
            nameToIndex["grass_overlay"] = idx;
        }
    }

    // load model jsons and blockstates if present
    loadModelsAndBlockstates(dir);

    // nameToIndex (and model/block mappings) filled at this point
    return true;
}

void ResourcePack::loadModelsAndBlockstates(const std::string &dir) {
    std::string modelsBase = dir + "/assets/minecraft/models/block/";
    std::string blockstatesBase = dir + "/assets/minecraft/blockstates/";

    struct stat sb;
    bool modelsDir = (stat(modelsBase.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
    bool blockstatesDir = (stat(blockstatesBase.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));

    const char* homedir = getenv("HOME");
    std::string zipPath;
    bool zipExists = false;
    if (homedir) {
        zipPath = std::string(homedir) + "/.Cubica/resources/base.zip";
        zipExists = (stat(zipPath.c_str(), &sb) == 0);
    }

    // parse model json files
    if (modelsDir) {
        DIR* d = opendir(modelsBase.c_str());
        if (d) {
            struct dirent* de;
            while ((de = readdir(d)) != nullptr) {
                std::string name(de->d_name);
                if (name.size() > 5 && name.substr(name.size()-5) == ".json") {
                    std::string full = modelsBase + name;
                    std::string contents = readFileContents(full);
                    if (contents.empty()) continue;
                    // find textures object
                    size_t tpos = contents.find("\"textures\"");
                    if (tpos == std::string::npos) continue;
                    size_t brace = contents.find('{', tpos);
                    if (brace == std::string::npos) continue;
                    int end = findMatchingBrace(contents, brace);
                    if (end < 0) continue;
                    std::string obj = contents.substr(brace+1, end-brace-1);
                    std::regex pairRe("\"([A-Za-z0-9_/]+)\"\\s*:\\s*\"([^\"]+)\"");
                    for (auto it = std::sregex_iterator(obj.begin(), obj.end(), pairRe); it != std::sregex_iterator(); ++it) {
                        std::smatch m = *it;
                        std::string key = m[1].str();
                        std::string val = m[2].str();
                        // model name is filename without extension, prefixed with "block/"
                        std::string modelName = "block/" + name.substr(0, name.size()-5);
                        modelTextures[modelName][key] = val;
                    }
                }
            }
            closedir(d);
        }
    } else if (zipExists) {
        // look for any model file entries we care about
        // this is rudimentary: try to extract block model files listed in zip
        // for performance we only attempt a small set of candidates (grass_block, dirt, stone)
        std::vector<std::string> candidates = {"grass_block.json","dirt.json","stone.json"};
        for (auto &c : candidates) {
            std::string entry = "assets/minecraft/models/block/" + c;
            std::string tmp = extractZipEntryToTemp(zipPath, entry);
            if (tmp.empty()) continue;
            std::string contents = readFileContents(tmp);
            unlink(tmp.c_str());
            if (contents.empty()) continue;
            size_t tpos = contents.find("\"textures\"");
            if (tpos == std::string::npos) continue;
            size_t brace = contents.find('{', tpos);
            if (brace == std::string::npos) continue;
            int end = findMatchingBrace(contents, brace);
            if (end < 0) continue;
            std::string obj = contents.substr(brace+1, end-brace-1);
            std::regex pairRe("\"([A-Za-z0-9_/]+)\"\\s*:\\s*\"([^\"]+)\"");
            for (auto it = std::sregex_iterator(obj.begin(), obj.end(), pairRe); it != std::sregex_iterator(); ++it) {
                std::smatch m = *it;
                std::string key = m[1].str();
                std::string val = m[2].str();
                std::string modelName = "block/" + c.substr(0, c.size()-5);
                modelTextures[modelName][key] = val;
            }
        }
    }

    // parse blockstates mapping (map block resource to model name)
    if (blockstatesDir) {
        DIR* d = opendir(blockstatesBase.c_str());
        if (d) {
            struct dirent* de;
            while ((de = readdir(d)) != nullptr) {
                std::string name(de->d_name);
                if (name.size() > 5 && name.substr(name.size()-5) == ".json") {
                    std::string full = blockstatesBase + name;
                    std::string contents = readFileContents(full);
                    if (contents.empty()) continue;
                    // find first occurrence of "model" and extract the value
                    std::regex modelRe("\"model\"\\s*:\\s*\"([^\"]+)\"");
                    std::smatch m;
                    if (std::regex_search(contents, m, modelRe)) {
                        std::string modelRef = m[1].str();
                        // modelRef may be like "block/grass_block" or "minecraft:block/grass_block"
                        // store as-is
                        blockToModel[name.substr(0, name.size()-5)] = modelRef;
                    }
                }
            }
            closedir(d);
        }
    } else if (zipExists) {
        std::vector<std::string> candidates = {"grass_block.json","dirt.json","stone.json"};
        for (auto &c : candidates) {
            std::string entry = "assets/minecraft/blockstates/" + c;
            std::string tmp = extractZipEntryToTemp(zipPath, entry);
            if (tmp.empty()) continue;
            std::string contents = readFileContents(tmp);
            unlink(tmp.c_str());
            if (contents.empty()) continue;
            std::regex modelRe("\"model\"\\s*:\\s*\"([^\"]+)\"");
            std::smatch m;
            if (std::regex_search(contents, m, modelRe)) {
                std::string modelRef = m[1].str();
                blockToModel[c.substr(0, c.size()-5)] = modelRef;
            }
        }
    }
}


int ResourcePack::findIndexForTextureRef(const std::string &ref) const {
    // try direct lookup
    auto it = nameToIndex.find(ref);
    if (it != nameToIndex.end()) return it->second;
    // try last path component
    auto pos = ref.find_last_of('/');
    std::string last = (pos==std::string::npos) ? ref : ref.substr(pos+1);
    // strip optional "minecraft:" prefix
    if (last.rfind("minecraft:", 0) == 0) last = last.substr(10);
    // strip optional "block/" prefix
    if (last.rfind("block/", 0) == 0) last = last.substr(6);
    // remove any leading namespace
    pos = last.find_last_of('/'); if (pos!=std::string::npos) last = last.substr(pos+1);
    // remove extension if present
    size_t dot = last.find_last_of('.'); if (dot!=std::string::npos) last = last.substr(0, dot);
    // try underscore variant
    std::string underscored = last;
    for (auto &c : underscored) if (c=='/') c = '_';

    auto it2 = nameToIndex.find(underscored);
    if (it2 != nameToIndex.end()) return it2->second;
    auto it3 = nameToIndex.find(last);
    if (it3 != nameToIndex.end()) return it3->second;
    // try replace "_block_" -> "_"
    std::string simplified = last;
    size_t posb = simplified.find("_block_");
    if (posb != std::string::npos) {
        simplified = simplified.substr(0,posb) + "_" + simplified.substr(posb+7);
        auto it4 = nameToIndex.find(simplified);
        if (it4 != nameToIndex.end()) return it4->second;
    }
    return -1;
}

int ResourcePack::getTileFor(BlockType t, Face face) const {
    // helper to map BlockType -> resource block name
    auto blockNameOf = [](BlockType bt)->std::string{
        switch(bt){
            case BlockType::GRASS: return std::string("grass_block");
            case BlockType::DIRT: return std::string("dirt");
            case BlockType::STONE: return std::string("stone");
            default: return std::string();
        }
    };

    std::string bn = blockNameOf(t);
    if (!bn.empty()) {
        // prefer model-specified texture if available
        auto itbm = blockToModel.find(bn);
        if (itbm != blockToModel.end()) {
            std::string modelRef = itbm->second;
            // normalize modelRef: strip "minecraft:" prefix if present
            if (modelRef.rfind("minecraft:",0) == 0) modelRef = modelRef.substr(10);
            auto mit = modelTextures.find(modelRef);
            if (mit != modelTextures.end()) {
                const auto &mmap = mit->second;
                // mapping from Face to texture key names to try
                std::vector<std::string> keys;
                if (face == TOP) keys = {"up","top"};
                else if (face == BOTTOM) keys = {"down","bottom"};
                else keys = {"side","north","east","south","west"};
                for (auto &k : keys) {
                    auto itk = mmap.find(k);
                    if (itk != mmap.end()) {
                        int idx = findIndexForTextureRef(itk->second);
                        if (idx >= 0) return idx;
                    }
                }
            }
        }
    }

    // fallback to previous behavior for known blocks
    if (t == BlockType::GRASS) {
        if (face == TOP) {
            auto it = nameToIndex.find("grass_top"); if (it!=nameToIndex.end()) return it->second;
        }
        if (face == SIDE) {
            // prefer named side texture
            auto it = nameToIndex.find("grass_side"); if (it!=nameToIndex.end()) return it->second;
            // fallback to common overlay names if side not present
            for (auto &k : {std::string("grass_block_side_overlay"), std::string("grass_block_overlay"), std::string("grass_overlay")}) {
                auto it2 = nameToIndex.find(k); if (it2!=nameToIndex.end()) return it2->second;
            }
        }
        // bottom falls back to dirt
        auto it = nameToIndex.find("dirt"); if (it!=nameToIndex.end()) return it->second;
        return -1;
    }
    if (t == BlockType::DIRT) {
        auto it = nameToIndex.find("dirt"); if (it!=nameToIndex.end()) return it->second;
        return -1;
    }
    if (t == BlockType::STONE) {
        auto it = nameToIndex.find("stone"); if (it!=nameToIndex.end()) return it->second;
        return -1;
    }
    if (t == BlockType::WOOD) {
        // try common names
        for (auto &k : {std::string("oak_log"), std::string("log"), std::string("wood")}) {
            auto it = nameToIndex.find(k); if (it!=nameToIndex.end()) return it->second;
        }
        return -1;
    }
    if (t == BlockType::LEAVES) {
        for (auto &k : {std::string("oak_leaves"), std::string("leaves")}) {
            auto it = nameToIndex.find(k); if (it!=nameToIndex.end()) return it->second;
        }
        return -1;
    }
    return -1;
}

int ResourcePack::getOverlayFor(BlockType t) const {
    auto blockNameOf = [](BlockType bt)->std::string{
        switch(bt){
            case BlockType::GRASS: return std::string("grass_block");
            case BlockType::DIRT: return std::string("dirt");
            case BlockType::STONE: return std::string("stone");
            default: return std::string();
        }
    };
    std::string bn = blockNameOf(t);
    if (!bn.empty()) {
        auto itbm = blockToModel.find(bn);
        if (itbm != blockToModel.end()) {
            std::string modelRef = itbm->second;
            if (modelRef.rfind("minecraft:",0) == 0) modelRef = modelRef.substr(10);
            auto mit = modelTextures.find(modelRef);
            if (mit != modelTextures.end()) {
                const auto &mmap = mit->second;
                // check for overlay keys
                std::vector<std::string> keys = {"overlay","side_overlay"};
                for (auto &k : keys) {
                    auto itk = mmap.find(k);
                    if (itk != mmap.end()) {
                        int idx = findIndexForTextureRef(itk->second);
                        if (idx >= 0) {
                            printf("ResourcePack: model specifies overlay '%s' -> tile %d for block %s\n", itk->second.c_str(), idx, bn.c_str());
                            return idx;
                        }
                    }
                }
            }
        }
    }
    // fallback: try common names
    if (t == BlockType::GRASS) {
        for (auto &n : {std::string("grass_block_side_overlay"), std::string("grass_side_overlay"), std::string("grass_overlay")}) {
            auto it = nameToIndex.find(n); if (it!=nameToIndex.end()) { printf("ResourcePack: fallback overlay '%s' -> tile %d for block %s\n", n.c_str(), it->second, bn.c_str()); return it->second; }
        }
    }
    return -1;
}
