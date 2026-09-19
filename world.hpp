#pragma once
#include "raylib.h"
#include <vector>

class Assets;

// ============================================================
// 世界模块：200x200 瓦片（16px/格）值噪声地形、静态物体、
// 自然生成屋子、掉落物、圆形碰撞（平滑滑动）、地面预渲染
// ============================================================

constexpr int MAP_W = 200;
constexpr int MAP_H = 200;
constexpr int TILE = 16;
constexpr float DAY_LEN = 180.0f;   // 1 天 = 3 分钟（昼 2 分钟 + 夜 1 分钟）

enum class Tile : unsigned char { Grass, Forest, Water, Sand };

enum class ObjKind : unsigned char {
    Tree, Rock, Berry, Stump, TallGrass, Flower, Campfire,
    RuinWall,           // 村庄遗迹残墙（固体）
    Chest,              // 遗迹/地牢宝箱（harvested=true 表示已开启）
    TreePine,           // 松树（针叶锥形，森林）
    TreeBirch,          // 白桦（白干黑纹，森林/草原）
    TreePalm,           // 棕榈（沙滩近水）
    OreRock,            // 矿脉（散落乱葬岗/山岩，需镐采集）
    Workbench,          // 工作台（hp 字段复用为等级 1/2/3；合成站）
    GraveMound,         // 坟冢（乱葬岗地标，产出萤晶/符文碎片，靠近会惊动阴物）
    Wall,               // 木墙（固体，耐久 6；自然屋子墙体，鬼会攻击它）
    Bed,                // 草席床（睡眠点；夜间入睡推进时间并恢复状态）
    CampStone,          // 营地石碑（领地系统：按 E 建庇护屋收容幸存者）
};

// 掉落物种类
enum class ItemKind : unsigned char {
    Wood, Stone, Berry, RawMeat, CookedMeat, RottenMeat,
    IronOre,            // 铁矿石（地牢矿脉/宝箱产出，高级配方）
    Crystal,            // 萤晶（"复苏时刻"关键素材，地牢深层产出）
    GemShard,           // 符文碎片（深层地牢稀有掉落，全工具矿石升级材料）
    Heart               // 血月之心（血月尸王专属掉落，终极武器材料）
};

struct WorldObj {
    ObjKind kind;
    unsigned char tx, ty;    // 所在瓦片
    unsigned char hp;        // 树/石被采次数（3 次）
    float shake;             // 被击晃动计时
    bool harvested;          // 浆果丛已采空
    float regrow;            // 浆果丛再生计时（60 秒）
};

struct Drop {
    ItemKind kind;
    float x, y;
    float vx, vy;
    float age;
    unsigned netId = 0;   // 联机：掉落网络 id（房主按生成序分配；单机恒 0）
};

class World {
public:
    std::vector<Tile> tiles;                  // MAP_W * MAP_H
    std::vector<WorldObj> objs;               // 全部静态物体
    std::vector<int> objAt;                   // 瓦片 -> objs 下标（-1 无），加速视口裁剪与碰撞
    std::vector<int> campfires;               // 篝火下标（光照/AI 高频查询）
    std::vector<Drop> drops;                  // 掉落物
    std::vector<unsigned char> waterEdge;     // 水瓦片贴陆方向位 N=1 E=2 S=4 W=8（岸边泡沫）
    std::vector<unsigned char> shoreDist;     // 水瓦离岸距离：1=贴岸 2=近岸 3=远水 0=非水（近岸倒影范围）
    std::vector<unsigned char> landShore;     // 陆地瓦是否贴水（2 格内，水面 Shader 倒影候选源）
    std::vector<Vector2> ruins;               // 村庄遗迹中心点（探索 POI，发现判定由主程序做）
    Texture2D ground = {};                    // 预渲染地面大纹理
    unsigned seed = 0;
    Vector2 spawn = { 0, 0 };

    std::vector<Vector2> graveyards;                 // 乱葬岗中心点（矿脉/坟冢聚集，探索 POI）

    // ---- 自然生成屋子（取代玩家建造：世界里长出来的房子）----
    struct House {
        int tx, ty;           // 中心瓦片
        int w, h;             // 半宽半高（瓦片）
        int kind;             // 0=普通民居 1=铁匠屋
        bool haunted;         // 闹鬼屋（生成潜伏鬼）
        float roofA = 255.0f; // 屋顶当前不透明度（玩家进屋渐隐到 0，出屋恢复 255）
        Vector2 Center() const { return { tx * 16.0f + 8.0f, ty * 16.0f + 8.0f }; }
    };
    std::vector<House> houses;                       // 全部屋子
    Vector2 smithPos = {};                           // 铁匠 NPC 位置（铁匠屋内）
    Vector2 smithGhostPos = {};                      // 铁匠鬼位置（废弃铁匠铺）
    Vector2 campsite = {};                           // 营地石碑位置（领地系统）
    Vector2 domainPos = {};                          // 鬼游戏区域中心（场景同化型鬼域）

    void Generate(unsigned newSeed, const Assets& a);
    void Unload();
    // 砍倒树后把该瓦片地面换成草地（清除森林深色残留）
    void ClearTreeGround(int tx, int ty, const Assets& a);

    // ---- 查询 ----
    const Texture2D& CurGround() const { return ground; }
    Tile TileAt(int tx, int ty) const;
    bool WalkableTile(int tx, int ty) const;               // 非水
    int  ObjIndexAt(int tx, int ty) const;                 // 该瓦片物体下标
    bool ObjSolid(ObjKind k) const;                        // 树/石/篝火/墙类有碰撞
    // 圆是否与固体（水瓦片 + 固体物体）重叠；ignoreWater=true 时水域可通行（泛舟）
    bool CircleFree(float x, float y, float r, bool ignoreWater = false) const;
    // 圆形移动（X/Y 轴分离，撞墙平滑滑动）；ignoreWater=true 时水域可通行（泛舟）
    Vector2 MoveCircle(float x, float y, float r, float dx, float dy,
                       bool ignoreWater = false) const;
    // 距离 (x,y) 半径 rad 内最近的篝火下标（无则 -1）
    int NearCampfire(float x, float y, float rad) const;
    int NearWorkbench(float x, float y, float rad) const;   // 工作台（hp 字段 = 等级 1..3）
    bool PlaceWall(int tx, int ty);                         // 放置木墙（自然屋子/营地建造用）
    bool PlaceBed(int tx, int ty);                          // 放置草席床
    bool PlaceCampfire(int tx, int ty);                     // 放置长明灯
    bool PlaceCampStone(int tx, int ty);                    // 放置营地石碑
    bool PlaceWorkbench(int tx, int ty, int lv);            // 放置/授予工作台（lv = 等级 1..3）
    bool MarkWallRoof(int tx, int ty);                      // 把木墙标记为屋顶沿（立体屋顶渲染用）
    bool RemoveObjAt(int tx, int ty);                       // 拆除该瓦片上的物体（领地房屋重建用；swap-pop 并修索引）
    int  CountWallAround(float x, float y, int rad) const;  // 屋内判定：邻域墙体数量
    int  NearBed(float x, float y, float rad) const;        // 睡眠：找最近的床
    int  NearObj(ObjKind k, float x, float y, float rad) const; // 最近指定物体（石碑等）

    // 掉落物生成与更新（磁吸拾取回调；路由到当前上下文）
    void SpawnDrop(ItemKind k, float x, float y, float vx, float vy);
    void UpdateDrops(float px, float py, float dt,
                     void (*onPickup)(const Drop&, float, float));
};

// 联机：掉落网络 id 分配回调（main.cpp 在创建联机房间时设置为递增分配器；单机为 nullptr）
extern unsigned (*WDropIdCb)();
// 联机客人：本地采集演出时抑制掉落生成（掉落由房主生成后经快照下发）
extern bool WDropSuppress;
void WSuppressDrops(bool on);

// 值噪声工具
float VNoise(float fx, float fy, unsigned seed);
float FBM(float x, float y, unsigned seed, int octaves);

// 昼夜：返回 0..1 相位
float DayPhase(float gameTime);
bool  IsNight(float gameTime);
// 黑暗遮罩强度 0..205
float NightDarkness(float gameTime);
// 黄昏暖色调系数 0..1（用于渲染层暖色叠加）
float DayWarm(float gameTime);
// 血月夜判定：从第 2 晚起每晚约 25% 概率（按天数确定性哈希）
bool IsBloodMoon(float gameTime);
// 怪物属性强度系数：每天 +15%（20 天封顶）+ 每层地牢深度 +35%
float MobScaleMul(float gameTime, int tier);
// 怪物数量倍率：每天 +50%，4 倍封顶
float MobCountMul(float gameTime);
