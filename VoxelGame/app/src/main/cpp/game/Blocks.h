#pragma once
#include <string>
#include <array>
#include <unordered_map>

// Face indices: +X, -X, +Y, -Y, +Z, -Z
struct BlockDef {
    std::string name;
    int         price       = 0;
    int         sellPrice   = 0;
    float       hardness    = 1.0f;
    bool        transparent = false;
    bool        isFluid     = false;
    float       opacity     = 1.0f;
    bool        indestructible = false;
    bool        isTorch     = false;
    bool        isDoor      = false;
    bool        isDoorTop   = false;
    bool        isDoorBottom= false;
    bool        isChest     = false;
    bool        isSlab      = false;
    bool        isStair     = false;
    bool        isGlowing   = false;   // emits light
    // Texture file names for each face: +X,-X,+Y,-Y,+Z,-Z
    std::array<std::string,6> files;
    // Atlas tile indices (filled at runtime)
    std::array<int,6> tiles = {0,0,0,0,0,0};
};

// ────────────────────────────────────────────
//  All 60+ block definitions, exactly matching
//  the JS source.
// ────────────────────────────────────────────
inline std::unordered_map<int, BlockDef> makeBlockDefs() {
    std::unordered_map<int, BlockDef> B;

    auto mk = [](const std::string& name, int price, int sell, float hard,
                  const std::array<std::string,6>& f,
                  bool transp=false, float op=1.0f,
                  bool indestr=false) -> BlockDef {
        BlockDef d;
        d.name=name; d.price=price; d.sellPrice=sell;
        d.hardness=hard; d.files=f; d.transparent=transp;
        d.opacity=op; d.indestructible=indestr;
        return d;
    };

    B[1]  = mk("Grass",       4,  1, 0.6f, {"grass_side.png","grass_side.png","grass_top.png","dirt.png","grass_side.png","grass_side.png"});
    B[2]  = mk("Dirt",        2,  1, 0.5f, {"dirt.png","dirt.png","dirt.png","dirt.png","dirt.png","dirt.png"});
    B[3]  = mk("Stone",       5,  2, 1.5f, {"stone.png","stone.png","stone.png","stone.png","stone.png","stone.png"});
    B[4]  = mk("Log",         5,  2, 2.0f, {"log_side.png","log_side.png","log_top.png","log_top.png","log_side.png","log_side.png"});
    B[5]  = mk("Leaves",      2,  1, 0.2f, {"leaves.png","leaves.png","leaves.png","leaves.png","leaves.png","leaves.png"},true);
    B[6]  = mk("Sand",        2,  1, 0.5f, {"sand.png","sand.png","sand.png","sand.png","sand.png","sand.png"});
    B[7]  = mk("Planks",      4,  1, 2.0f, {"planks.png","planks.png","planks.png","planks.png","planks.png","planks.png"});
    B[8]  = mk("Glass",       6,  2, 0.3f, {"glass.png","glass.png","glass.png","glass.png","glass.png","glass.png"},true);
    B[9]  = mk("Bedrock",     0,  0, 1e9f, {"bedrock.png","bedrock.png","bedrock.png","bedrock.png","bedrock.png","bedrock.png"},false,1.0f,true);
    { auto& t=B[10]; t=mk("Torch",5,1,0.1f,{"torch.png","torch.png","torch.png","torch.png","torch.png","torch.png"},true); t.isTorch=true; t.isGlowing=true; }
    B[11] = mk("Water",       0,  0, 1e9f, {"water.png","water.png","water.png","water.png","water.png","water.png"},true,0.7f);
    B[12] = mk("Coal Ore",    10, 3, 3.0f, {"coal_ore.png","coal_ore.png","coal_ore.png","coal_ore.png","coal_ore.png","coal_ore.png"});
    B[13] = mk("Iron Ore",    15, 4, 3.0f, {"iron_ore.png","iron_ore.png","iron_ore.png","iron_ore.png","iron_ore.png","iron_ore.png"});
    B[14] = mk("Gold Ore",    20, 5, 3.0f, {"gold_ore.png","gold_ore.png","gold_ore.png","gold_ore.png","gold_ore.png","gold_ore.png"});
    B[15] = mk("Diamond Ore", 30, 6, 4.0f, {"diamond_ore.png","diamond_ore.png","diamond_ore.png","diamond_ore.png","diamond_ore.png","diamond_ore.png"});
    B[16] = mk("Emerald Ore", 30, 8, 3.0f, {"emerald_ore.png","emerald_ore.png","emerald_ore.png","emerald_ore.png","emerald_ore.png","emerald_ore.png"});
    B[17] = mk("Cobblestone", 3,  1, 2.0f, {"cobblestone.png","cobblestone.png","cobblestone.png","cobblestone.png","cobblestone.png","cobblestone.png"});
    B[18] = mk("Mossy Cobble",4,  1, 2.0f, {"mossy_cobblestone.png","mossy_cobblestone.png","mossy_cobblestone.png","mossy_cobblestone.png","mossy_cobblestone.png","mossy_cobblestone.png"});
    B[19] = mk("Stone Bricks",4,  1, 1.5f, {"stone_bricks.png","stone_bricks.png","stone_bricks.png","stone_bricks.png","stone_bricks.png","stone_bricks.png"});
    B[20] = mk("Obsidian",    40, 10,10.0f,{"obsidian.png","obsidian.png","obsidian.png","obsidian.png","obsidian.png","obsidian.png"});
    B[21] = mk("Gravel",      2,  1, 0.6f, {"gravel.png","gravel.png","gravel.png","gravel.png","gravel.png","gravel.png"});
    B[22] = mk("Clay",        4,  2, 0.6f, {"clay.png","clay.png","clay.png","clay.png","clay.png","clay.png"});
    B[23] = mk("Birch Log",   5,  2, 2.0f, {"birch_log.png","birch_log.png","birch_log_top.png","birch_log_top.png","birch_log.png","birch_log.png"});
    B[24] = mk("Birch Planks",4,  1, 2.0f, {"birch_planks.png","birch_planks.png","birch_planks.png","birch_planks.png","birch_planks.png","birch_planks.png"});
    B[25] = mk("Spruce Log",  5,  2, 2.0f, {"spruce_log.png","spruce_log.png","spruce_log_top.png","spruce_log_top.png","spruce_log.png","spruce_log.png"});
    B[26] = mk("Spruce Planks",4, 1, 2.0f, {"spruce_planks.png","spruce_planks.png","spruce_planks.png","spruce_planks.png","spruce_planks.png","spruce_planks.png"});
    B[27] = mk("White Wool",  6,  2, 0.8f, {"wool_white.png","wool_white.png","wool_white.png","wool_white.png","wool_white.png","wool_white.png"});
    B[28] = mk("Red Wool",    6,  2, 0.8f, {"wool_red.png","wool_red.png","wool_red.png","wool_red.png","wool_red.png","wool_red.png"});
    B[29] = mk("Green Wool",  6,  2, 0.8f, {"wool_green.png","wool_green.png","wool_green.png","wool_green.png","wool_green.png","wool_green.png"});
    B[30] = mk("Blue Wool",   6,  2, 0.8f, {"wool_blue.png","wool_blue.png","wool_blue.png","wool_blue.png","wool_blue.png","wool_blue.png"});
    B[31] = mk("Yellow Wool", 6,  2, 0.8f, {"wool_yellow.png","wool_yellow.png","wool_yellow.png","wool_yellow.png","wool_yellow.png","wool_yellow.png"});
    B[32] = mk("Black Wool",  6,  2, 0.8f, {"wool_black.png","wool_black.png","wool_black.png","wool_black.png","wool_black.png","wool_black.png"});
    B[33] = mk("Orange Wool", 6,  2, 0.8f, {"wool_orange.png","wool_orange.png","wool_orange.png","wool_orange.png","wool_orange.png","wool_orange.png"});
    B[34] = mk("Furnace",    16,  4, 3.5f, {"furnace_side.png","furnace_side.png","furnace_top.png","furnace_top.png","furnace_front.png","furnace_side.png"});
    B[35] = mk("Crafting Table",10,3,2.5f, {"crafting_side.png","crafting_side.png","crafting_top.png","planks.png","crafting_side.png","crafting_side.png"});
    B[36] = mk("Bookshelf",  15,  4, 1.5f, {"bookshelf.png","bookshelf.png","planks.png","planks.png","bookshelf.png","bookshelf.png"});
    B[37] = mk("TNT",        50, 10, 0.0f, {"tnt_side.png","tnt_side.png","tnt_top.png","tnt_bottom.png","tnt_side.png","tnt_side.png"});
    B[38] = mk("Pumpkin",     8,  2, 1.0f, {"pumpkin_side.png","pumpkin_side.png","pumpkin_top.png","pumpkin_top.png","pumpkin_face.png","pumpkin_side.png"});
    B[39] = mk("Melon",       8,  2, 1.0f, {"melon_side.png","melon_side.png","melon_top.png","melon_top.png","melon_side.png","melon_side.png"});
    B[40] = mk("Cyan Wool",   6,  2, 0.8f, {"wool_cyan.png","wool_cyan.png","wool_cyan.png","wool_cyan.png","wool_cyan.png","wool_cyan.png"});
    B[41] = mk("Purple Wool", 6,  2, 0.8f, {"wool_purple.png","wool_purple.png","wool_purple.png","wool_purple.png","wool_purple.png","wool_purple.png"});
    B[42] = mk("Pink Wool",   6,  2, 0.8f, {"wool_pink.png","wool_pink.png","wool_pink.png","wool_pink.png","wool_pink.png","wool_pink.png"});
    B[43] = mk("Lime Wool",   6,  2, 0.8f, {"wool_lime.png","wool_lime.png","wool_lime.png","wool_lime.png","wool_lime.png","wool_lime.png"});
    B[44] = mk("Snow Block",  2,  1, 0.2f, {"snow.png","snow.png","snow.png","snow.png","snow.png","snow.png"});
    B[45] = mk("Ice",         4,  1, 0.5f, {"ice.png","ice.png","ice.png","ice.png","ice.png","ice.png"},true,0.6f);
    B[46] = mk("Cactus",      4,  1, 0.4f, {"cactus_side.png","cactus_side.png","cactus_top.png","cactus_bottom.png","cactus_side.png","cactus_side.png"},true);
    B[47] = mk("Mycelium",    4,  1, 0.6f, {"mycelium_side.png","mycelium_side.png","mycelium_top.png","dirt.png","mycelium_side.png","mycelium_side.png"});
    B[48] = mk("Netherrack",  2,  1, 0.4f, {"netherrack.png","netherrack.png","netherrack.png","netherrack.png","netherrack.png","netherrack.png"});
    B[49] = mk("Soul Sand",   4,  1, 0.5f, {"soul_sand.png","soul_sand.png","soul_sand.png","soul_sand.png","soul_sand.png","soul_sand.png"});
    { auto& t=B[50]; t=mk("Glowstone",16,4,0.3f,{"glowstone.png","glowstone.png","glowstone.png","glowstone.png","glowstone.png","glowstone.png"}); t.isGlowing=true; }
    B[51] = mk("End Stone",  10,  3, 3.0f, {"end_stone.png","end_stone.png","end_stone.png","end_stone.png","end_stone.png","end_stone.png"});
    B[52] = mk("Iron Block", 100, 15, 5.0f,{"iron_block.png","iron_block.png","iron_block.png","iron_block.png","iron_block.png","iron_block.png"});
    B[53] = mk("Gold Block", 150, 25, 3.0f,{"gold_block.png","gold_block.png","gold_block.png","gold_block.png","gold_block.png","gold_block.png"});
    B[54] = mk("Diamond Block",500,20,5.0f,{"diamond_block.png","diamond_block.png","diamond_block.png","diamond_block.png","diamond_block.png","diamond_block.png"});
    B[55] = mk("Brick",       6,  2, 2.0f, {"brick.png","brick.png","brick.png","brick.png","brick.png","brick.png"});
    B[56] = mk("Sandstone",   4,  1, 0.8f, {"sandstone_side.png","sandstone_side.png","sandstone_top.png","sandstone_bottom.png","sandstone_side.png","sandstone_side.png"});
    B[57] = mk("Quartz",     10,  3, 0.8f, {"quartz_side.png","quartz_side.png","quartz_top.png","quartz_bottom.png","quartz_side.png","quartz_side.png"});
    B[58] = mk("Grey Wool",   6,  2, 0.8f, {"wool_grey.png","wool_grey.png","wool_grey.png","wool_grey.png","wool_grey.png","wool_grey.png"});
    B[59] = mk("Lt Grey Wool",6,  2, 0.8f, {"wool_light_grey.png","wool_light_grey.png","wool_light_grey.png","wool_light_grey.png","wool_light_grey.png","wool_light_grey.png"});
    B[60] = mk("Magenta Wool",6,  2, 0.8f, {"wool_magenta.png","wool_magenta.png","wool_magenta.png","wool_magenta.png","wool_magenta.png","wool_magenta.png"});
    B[61] = mk("Lt Blue Wool",6,  2, 0.8f, {"wool_light_blue.png","wool_light_blue.png","wool_light_blue.png","wool_light_blue.png","wool_light_blue.png","wool_light_blue.png"});
    // Door (bottom half = 101, top half = 102)
    { auto& d=B[101]; d.name="Door"; d.price=8; d.sellPrice=2; d.hardness=3.0f;
      d.files={"__door_side","__door_side","__door_top","__door_top","__door_front","__door_side"};
      d.isDoor=true; d.isDoorBottom=true; d.transparent=true; }
    { auto& d=B[102]; d.name="Door Top"; d.hardness=3.0f;
      d.files={"__door_side","__door_side","__door_top","__door_top","__door_front","__door_side"};
      d.isDoor=true; d.isDoorTop=true; d.transparent=true; }
    // Chest = 103
    { auto& d=B[103]; d.name="Chest"; d.price=10; d.sellPrice=3; d.hardness=2.5f;
      d.files={"__chest_side","__chest_side","__chest_top","__chest_top","__chest_front","__chest_side"};
      d.isChest=true; }
    // Land claim flag = 100
    B[100] = mk("Claim Flag",50,0,1.0f,{"planks.png","planks.png","planks.png","planks.png","planks.png","planks.png"});

    return B;
}
