#include "world.hpp"
#include "assets.hpp"
#include <cmath>
#include <cstdlib>

// ============================================================
// 世界实现：值噪声地形（岛形）、物体分布、地面预渲染、碰撞
// ============================================================

namespace {

// 整数格点哈希 -> [0,1]
float Hash2(int x, int y, unsigned seed) {
    unsigned h = seed ^ ((unsigned)x * 374761393u) ^ ((unsigned)y * 668265263u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFFu) / 16777215.0f;
}

float SmoothT(float t) { return t * t * (3.0f - 2.0f * t); }

float Lerp(float a, float b, float t) { return a + (b - a) * t; }

unsigned Frnd = 777u;
unsigned FRnd() { Frnd = Frnd * 1664525u + 1013904223u; return Frnd >> 8; }

} // namespace

// ---- 值噪声 ----
float VNoise(float fx, float fy, unsigned seed) {
    int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
    float tx = SmoothT(fx - x0), ty = SmoothT(fy - y0);
    float a = Hash2(x0, y0, seed), b = Hash2(x0 + 1, y0, seed);
    float c = Hash2(x0, y0 + 1, seed), d = Hash2(x0 + 1, y0 + 1, seed);
    return Lerp(Lerp(a, b, tx), Lerp(c, d, tx), ty);
}

float FBM(float x, float y, unsigned seed, int octaves) {
    float sum = 0, amp = 0.5f, freq = 1;
    for (int i = 0; i < octaves; i++) {
        sum += VNoise(x * freq, y * freq, seed + (unsigned)i * 131u) * amp;
        amp *= 0.5f; freq *= 2;
    }
    return sum;
}

// ---- 昼夜 ----
float DayPhase(float gameTime) { return fmodf(gameTime, DAY_LEN) / DAY_LEN; }

bool IsNight(float gameTime) { return DayPhase(gameTime) >= 0.6667f; }

// smoothstep 平滑过渡
static float Sm01(float x) {
    float c = x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
    return c * c * (3.0f - 2.0f * c);
}

float NightDarkness(float gameTime) {
    const float maxDark = 228.0f;      // 夜晚非照明处更深（衬托手电）
    float p = DayPhase(gameTime);
    // 黄昏渐入：p 0.60 -> 0.68，用 smoothstep 平滑爬升（约 14 秒慢慢变暗）
    float duskIn = Sm01((p - 0.60f) / 0.08f);
    // 黎明渐出：p 0.95 -> 1.0
    float dawnOut = Sm01((p - 0.95f) / 0.05f);
    float dark = maxDark * duskIn * (1.0f - dawnOut);
    return dark;
}

// 黄昏暖色调系数：p 0.54 -> 0.63 渐强 -> 0.63 -> 0.70 渐弱
float DayWarm(float gameTime) {
    float p = DayPhase(gameTime);
    float rise = Sm01((p - 0.54f) / 0.045f);
    float fall = 1.0f - Sm01((p - 0.63f) / 0.035f);
    float warm = rise * fall;
    if (warm < 0) warm = 0;
    return warm;
}

// 血月夜：天数确定性哈希，第 1 晚必不血月，其后每晚 25% 概率
bool IsBloodMoon(float gameTime) {
    int day = (int)(gameTime / DAY_LEN);
    if (day < 1) return false;
    unsigned h = (unsigned)day * 2654435761u;
    h ^= h >> 13;
    return (h & 3) == 0;
}

// 怪物属性强度：每天 +15%（20 天封顶 4x）+ 地牢每层 +35%
float MobScaleMul(float gameTime, int tier) {
    int day = (int)(gameTime / DAY_LEN);
    if (day > 20) day = 20;
    return 1.0f + 0.15f * day + 0.35f * (tier > 0 ? tier : 0);
}

// 怪物数量倍率：每天 +50%，4 倍封顶
float MobCountMul(float gameTime) {
    int day = (int)(gameTime / DAY_LEN);
    float m = 1.0f + 0.5f * day;
    return m > 4.0f ? 4.0f : m;
}

// ============================================================

void World::Generate(unsigned newSeed, const Assets& a) {
    seed = newSeed;
    Unload();
    tiles.assign((size_t)MAP_W * MAP_H, Tile::Grass);
    objAt.assign((size_t)MAP_W * MAP_H, -1);
    objs.clear(); objs.reserve(3000);
    campfires.clear();
    drops.clear(); drops.reserve(256);

    // ---- 地形：双层 FBM（高度 + 湿度），边缘下沉成岛 ----
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            float h = FBM(x * 0.045f, y * 0.045f, seed, 4);
            float m = FBM(x * 0.08f + 37.0f, y * 0.08f + 91.0f, seed ^ 0x9E37u, 3);
            float dx = (x - MAP_W * 0.5f) / (MAP_W * 0.5f);
            float dy = (y - MAP_H * 0.5f) / (MAP_H * 0.5f);
            float d = sqrtf(dx * dx + dy * dy);
            h -= (d > 0.72f) ? (d - 0.72f) * 2.4f : 0.0f;   // 四周强制水域
            Tile t = Tile::Grass;
            if (h < 0.34f) t = Tile::Water;
            else if (h < 0.40f) t = Tile::Sand;
            else if (m > 0.55f) t = Tile::Forest;
            tiles[(size_t)y * MAP_W + x] = t;
        }

    // ---- 大湖区：整片大湖（椭圆 + 沙滩环），湖面连成一片大区域 ----
    {
        int lx = MAP_W * 13 / 40, ly = MAP_H * 2 / 5;      // 湖心（避开中心出生点）
        float rx = MAP_W * 0.15f, ry = MAP_H * 0.20f;
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++) {
                float ddx = (x - lx) / rx, ddy = (y - ly) / ry;
                float dd = ddx * ddx + ddy * ddy;
                size_t i = (size_t)y * MAP_W + x;
                if (dd < 1.0f) tiles[i] = Tile::Water;
                else if (dd < 1.17f && tiles[i] != Tile::Water) tiles[i] = Tile::Sand;
            }
    }

    // ---- 岸线自然侵蚀：噪声扰动水陆边界（基于快照互不干扰），制造凹凸曲折有机岸线 ----
    {
        std::vector<Tile> snap = tiles;
        auto Land = [&](int x, int y) { return snap[(size_t)y * MAP_W + x] != Tile::Water; };
        auto NearLand = [&](int x, int y) {
            return (x > 0 && Land(x - 1, y)) || (x < MAP_W - 1 && Land(x + 1, y)) ||
                   (y > 0 && Land(x, y - 1)) || (y < MAP_H - 1 && Land(x, y + 1));
        };
        auto NearWater = [&](int x, int y) {
            return (x > 0 && !Land(x - 1, y)) || (x < MAP_W - 1 && !Land(x + 1, y)) ||
                   (y > 0 && !Land(x, y - 1)) || (y < MAP_H - 1 && !Land(x, y + 1));
        };
        for (int y = 1; y < MAP_H - 1; y++)
            for (int x = 1; x < MAP_W - 1; x++) {
                float n = Hash2(x, y, seed ^ 0xE10Eu);
                size_t i = (size_t)y * MAP_W + x;
                if (snap[i] == Tile::Water && NearLand(x, y) && n < 0.10f)
                    tiles[i] = Tile::Sand;                     // 岸向水推进（凸）
                else if (snap[i] == Tile::Sand && NearWater(x, y) && n > 0.92f)
                    tiles[i] = Tile::Water;                    // 水向岸凹进（凹）
            }
    }

    // ---- 离岸距离场：1=贴岸 2=近岸 3=远水（两轮传播，近岸倒影/浅滩用）----
    shoreDist.assign((size_t)MAP_W * MAP_H, 0);
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            size_t i = (size_t)y * MAP_W + x;
            if (tiles[i] != Tile::Water) continue;
            bool land = (y > 0 && tiles[i - MAP_W] != Tile::Water) ||
                        (x < MAP_W - 1 && tiles[i + 1] != Tile::Water) ||
                        (y < MAP_H - 1 && tiles[i + MAP_W] != Tile::Water) ||
                        (x > 0 && tiles[i - 1] != Tile::Water);
            shoreDist[i] = land ? 1 : 3;
        }
    for (int it = 0; it < 2; it++)
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++) {
                size_t i = (size_t)y * MAP_W + x;
                if (shoreDist[i] != 3) continue;
                unsigned char m = 250;
                if (y > 0          && shoreDist[i - MAP_W] > 0 && shoreDist[i - MAP_W] < m) m = shoreDist[i - MAP_W];
                if (x < MAP_W - 1  && shoreDist[i + 1] > 0     && shoreDist[i + 1] < m)     m = shoreDist[i + 1];
                if (y < MAP_H - 1  && shoreDist[i + MAP_W] > 0 && shoreDist[i + MAP_W] < m) m = shoreDist[i + MAP_W];
                if (x > 0          && shoreDist[i - 1] > 0     && shoreDist[i - 1] < m)     m = shoreDist[i - 1];
                if (m < 250) shoreDist[i] = (unsigned char)(m + 1);
            }

    // ---- 近岸陆地标记（陆地在水 2 格内 -> 水面 Shader 倒影候选源）----
    landShore.assign((size_t)MAP_W * MAP_H, 0);
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            size_t i = (size_t)y * MAP_W + x;
            if (tiles[i] == Tile::Water) continue;
            bool nearW = false;
            for (int oy = -2; oy <= 2 && !nearW; oy++)
                for (int ox = -2; ox <= 2 && !nearW; ox++) {
                    int nx = x + ox, ny = y + oy;
                    if (nx < 0 || ny < 0 || nx >= MAP_W || ny >= MAP_H) continue;
                    if (tiles[(size_t)ny * MAP_W + nx] == Tile::Water) nearW = true;
                }
            landShore[i] = nearW ? 1 : 0;
        }

    // ---- 岸线掩码：水瓦片贴陆方向位（N=1 E=2 S=4 W=8），渲染岸边泡沫用 ----
    waterEdge.assign((size_t)MAP_W * MAP_H, 0);
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            if (tiles[(size_t)y * MAP_W + x] != Tile::Water) continue;
            unsigned char b = 0;
            if (y > 0 && tiles[(size_t)(y - 1) * MAP_W + x] != Tile::Water) b |= 1;
            if (x < MAP_W - 1 && tiles[(size_t)y * MAP_W + x + 1] != Tile::Water) b |= 2;
            if (y < MAP_H - 1 && tiles[(size_t)(y + 1) * MAP_W + x] != Tile::Water) b |= 4;
            if (x > 0 && tiles[(size_t)y * MAP_W + x - 1] != Tile::Water) b |= 8;
            waterEdge[(size_t)y * MAP_W + x] = b;
        }

    // ---- 物体分布（固定种子拒绝采样）----
    Frnd = seed | 1u;
    auto TryPlace = [&](ObjKind k, int x, int y) {
        if (x < 1 || y < 1 || x >= MAP_W - 1 || y >= MAP_H - 1) return;
        if (objAt[(size_t)y * MAP_W + x] >= 0) return;
        WorldObj o;
        o.kind = k; o.tx = (unsigned char)x; o.ty = (unsigned char)y;
        o.hp = 3; o.shake = 0; o.harvested = false; o.regrow = 0;
        objAt[(size_t)y * MAP_W + x] = (int)objs.size();
        objs.push_back(o);
    };
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            Tile t = tiles[(size_t)y * MAP_W + x];
            float r = FRnd() / 16777216.0f;
            unsigned hv = (unsigned)(Hash2(x, y, seed ^ 0x7EE7u) * 100.0f);
            if (t == Tile::Forest) {
                if (r < 0.14f)      TryPlace(ObjKind::Tree, x, y);
                else if (r < 0.24f) TryPlace(hv % 2 ? ObjKind::TreePine : ObjKind::Tree, x, y);  // 松树混交
                else if (r < 0.30f) TryPlace(ObjKind::TreeBirch, x, y);                          // 白桦小片
                else if (r < 0.40f) TryPlace(ObjKind::TallGrass, x, y);
                else if (r < 0.415f) TryPlace(ObjKind::Rock, x, y);
            } else if (t == Tile::Grass) {
                if (r < 0.018f) TryPlace(ObjKind::Tree, x, y);
                else if (r < 0.026f) TryPlace(ObjKind::TreeBirch, x, y);   // 草原散生白桦
                else if (r < 0.046f) TryPlace(ObjKind::Rock, x, y);
                else if (r < 0.068f) TryPlace(ObjKind::Berry, x, y);
                else if (r < 0.168f) TryPlace(ObjKind::TallGrass, x, y);
                else if (r < 0.208f) TryPlace(ObjKind::Flower, x, y);
            } else if (t == Tile::Sand) {
                bool nearWater = waterEdge.empty() ? false :
                    (y > 0 && tiles[(size_t)(y - 1) * MAP_W + x] == Tile::Water) ||
                    (y < MAP_H - 1 && tiles[(size_t)(y + 1) * MAP_W + x] == Tile::Water) ||
                    (x > 0 && tiles[(size_t)y * MAP_W + x - 1] == Tile::Water) ||
                    (x < MAP_W - 1 && tiles[(size_t)y * MAP_W + x + 1] == Tile::Water);
                if (nearWater && r < 0.045f) TryPlace(ObjKind::TreePalm, x, y);   // 贴水沙滩棕榈
                else if (waterEdge[(size_t)y * MAP_W + x]) {      // 贴水沙滩：岩石/芦苇点缀（侵蚀岸缘的碎石植被）
                    if      (r < 0.085f) TryPlace(ObjKind::Rock, x, y);
                    else if (r < 0.150f) TryPlace(ObjKind::TallGrass, x, y);
                } else if (r < 0.028f) TryPlace(ObjKind::Rock, x, y);
            }
        }

    // ---- 村庄遗迹：2 处（矩形残墙 + 中央宝箱；距岛心 ≥45 瓦避免压出生点）----
    ruins.clear();
    {
        int placed = 0;
        for (int attempt = 0; attempt < 600 && placed < 2; attempt++) {
            int cx = 10 + (int)(FRnd() % (MAP_W - 20));
            int cy = 10 + (int)(FRnd() % (MAP_H - 20));
            float ddx = cx - MAP_W * 0.5f, ddy = cy - MAP_H * 0.5f;
            if (ddx * ddx + ddy * ddy < 45.0f * 45.0f) continue;        // 离出生区足够远
            // 足印 11x9 全部可站立且无水
            bool ok = true;
            for (int oy = -4; oy <= 4 && ok; oy++)
                for (int ox = -5; ox <= 5 && ok; ox++) {
                    if (tiles[(size_t)(cy + oy) * MAP_W + cx + ox] == Tile::Water) ok = false;
                }
            if (!ok) continue;
            // 清空足印内已有物体（原 objs 残留项不再被 objAt 引用即可）
            for (int oy = -4; oy <= 4; oy++)
                for (int ox = -5; ox <= 5; ox++)
                    objAt[(size_t)(cy + oy) * MAP_W + cx + ox] = -1;
            // 四周残墙（随机开 2~3 个豁口，破败感）
            int gaps[3] = { (int)(FRnd() % 30), (int)(FRnd() % 30), (int)(FRnd() % 30) };
            auto Wall = [&](int x, int y, int id) {
                for (int g = 0; g < 3; g++) if (gaps[g] == id) return;   // 豁口
                TryPlace(ObjKind::RuinWall, x, y);
            };
            int id = 0;
            for (int x = cx - 5; x <= cx + 5; x++) { Wall(x, cy - 4, id++); Wall(x, cy + 4, id++); }
            for (int y = cy - 3; y <= cy + 3; y++) { Wall(cx - 5, y, id++); Wall(cx + 5, y, id++); }
            // 中央宝箱 + 周围碎石点缀
            TryPlace(ObjKind::Chest, cx, cy);
            ruins.push_back({ cx * 16.0f + 8.0f, cy * 16.0f + 8.0f });
            placed++;
        }
    }

    // ---- 乱葬岗（2~3 处）：义庄祭台 + 坟冢 + 矿脉 ----
    // 取代原地牢，成为地表资源（矿石/萤晶）与阴物的核心聚集地
    graveyards.clear();
    {
        int want = 2 + (int)(FRnd() % 2);
        int placed = 0;
        for (int attempt = 0; attempt < 1200 && placed < want; attempt++) {
            int cx = 10 + (int)(FRnd() % (MAP_W - 20));
            int cy = 10 + (int)(FRnd() % (MAP_H - 20));
            if (tiles[(size_t)cy * MAP_W + cx] == Tile::Water) continue;
            float ddx = cx - MAP_W * 0.5f, ddy = cy - MAP_H * 0.5f;
            if (sqrtf(ddx * ddx + ddy * ddy) < 18.0f) continue;      // 别贴出生点
            bool far = true;
            for (const Vector2& g : graveyards) {                    // 彼此拉开距离
                float gx = g.x / 16.0f - cx, gy = g.y / 16.0f - cy;
                if (gx * gx + gy * gy < 28.0f * 28.0f) { far = false; break; }
            }
            if (!far) continue;
            TryPlace(ObjKind::Workbench, cx, cy);                      // 中心：义庄祭台（2 级工作台）
            if (objAt[(size_t)cy * MAP_W + cx] >= 0)
                objs[(size_t)objAt[(size_t)cy * MAP_W + cx]].hp = 2;   // hp = 工作台等级
            if (objAt[(size_t)cy * MAP_W + cx] < 0) continue;          // 该格被占，换个地方
            for (int k = 0; k < 6; k++) {                             // 环绕：坟冢 x4 + 矿脉 x2
                float ang = (FRnd() / 16777216.0f) * 6.2831853f;
                float rr = 2.0f + (FRnd() / 16777216.0f) * 4.0f;
                int ox = cx + (int)(cosf(ang) * rr);
                int oy = cy + (int)(sinf(ang) * rr);
                TryPlace(k < 4 ? ObjKind::GraveMound : ObjKind::OreRock, ox, oy);
            }
            graveyards.push_back({ cx * 16.0f + 8.0f, cy * 16.0f + 8.0f });
            placed++;
        }
    }

    // ---- 自然生成屋子（取代玩家建造：世界里长出来的房子）----
    // 民居 8~12 座（闹鬼屋概率潜伏鬼）+ 铁匠屋 + 废弃铁匠铺（铁匠鬼）+ 营地石碑 + 鬼域
    houses.clear();
    smithPos = {}; smithGhostPos = {}; campsite = {}; domainPos = {};
    {
        auto FootprintOk = [&](int cx, int cy, int hw, int hh) {
            for (int oy = -hh - 1; oy <= hh + 1; oy++)
                for (int ox = -hw - 1; ox <= hw + 1; ox++) {
                    int x = cx + ox, y = cy + oy;
                    if (x < 2 || y < 2 || x >= MAP_W - 2 || y >= MAP_H - 2) return false;
                    if (tiles[(size_t)y * MAP_W + x] == Tile::Water) return false;
                }
            return true;
        };
        auto ClearFoot = [&](int cx, int cy, int hw, int hh) {
            for (int oy = -hh; oy <= hh; oy++)
                for (int ox = -hw; ox <= hw; ox++)
                    objAt[(size_t)(cy + oy) * MAP_W + cx + ox] = -1;
        };
        // 砌一座屋子：木墙围一圈（南面正中留 2 格门豁口）+ 屋内家具
        auto BuildHouse = [&](int cx, int cy, int hw, int hh, int kind) -> bool {
            if (!FootprintOk(cx, cy, hw, hh)) return false;
            ClearFoot(cx, cy, hw, hh);
            for (int x = cx - hw; x <= cx + hw; x++) {          // 北墙（屋顶沿）/ 南墙（南面留门）
                if (x < cx - 1 || x > cx) TryPlace(ObjKind::Wall, x, cy + hh);
                TryPlace(ObjKind::Wall, x, cy - hh);
            }
            for (int y = cy - hh + 1; y <= cy + hh - 1; y++) {  // 东西墙
                TryPlace(ObjKind::Wall, cx - hw, y);
                TryPlace(ObjKind::Wall, cx + hw, y);
            }
            // 北沿墙标记为屋顶（立体屋顶；regrow 字段复用作标记，仅 Berry 使用它）
            for (int x = cx - hw; x <= cx + hw; x++) {
                int oi = objAt[(size_t)(cy - hh) * MAP_W + x];
                if (oi >= 0 && objs[(size_t)oi].kind == ObjKind::Wall)
                    objs[(size_t)oi].regrow = 1.0f;
            }
            House h;
            h.tx = cx; h.ty = cy; h.w = hw; h.h = hh; h.kind = kind;
            h.haunted = false;
            if (kind == 1) {                                    // ---- 铁匠屋：大屋 + 长明灯 ----
                TryPlace(ObjKind::Campfire, cx - hw + 2, cy);
                smithPos = { cx * 16.0f + 8.0f, cy * 16.0f + 20.0f };
            } else {                                            // ---- 普通民居：家具 + 闹鬼 ----
                if (FRnd() % 100 < 60) TryPlace(ObjKind::Bed, cx - hw + 1, cy - 1);
                if (FRnd() % 100 < 30) TryPlace(ObjKind::Chest, cx + hw - 1, cy - 1);
                if (FRnd() % 100 < 18) {                        // 少数民居自带一级工作台
                    TryPlace(ObjKind::Workbench, cx, cy);
                    if (objAt[(size_t)cy * MAP_W + cx] >= 0)
                        objs[(size_t)objAt[(size_t)cy * MAP_W + cx]].hp = 1;   // hp = 工作台等级
                }
                h.haunted = (FRnd() % 100) < 18;                // 18% 闹鬼屋（潜伏鬼）
            }
            houses.push_back(h);
            return true;
        };
        // 民居：远离出生点 22 瓦、彼此间隔 ≥18 瓦
        int placed = 0;
        for (int attempt = 0; attempt < 900 && placed < 10; attempt++) {
            int cx = 8 + (int)(FRnd() % (MAP_W - 16));
            int cy = 8 + (int)(FRnd() % (MAP_H - 16));
            float ddx = cx - MAP_W * 0.5f, ddy = cy - MAP_H * 0.5f;
            if (ddx * ddx + ddy * ddy < 22.0f * 22.0f) continue;
            bool far = true;
            for (const House& h : houses) {
                float hx = h.tx - cx, hy = h.ty - cy;
                if (hx * hx + hy * hy < 18.0f * 18.0f) { far = false; break; }
            }
            if (!far) continue;
            if (BuildHouse(cx, cy, 2 + (int)(FRnd() % 2), 2, 0)) placed++;
        }
        // 铁匠屋：全图随机（远离出生点 30 瓦，探索驱动）
        {
            bool ok = false;
            for (int attempt = 0; attempt < 700 && !ok; attempt++) {
                int cx = 10 + (int)(FRnd() % (MAP_W - 20));
                int cy = 10 + (int)(FRnd() % (MAP_H - 20));
                float ddx = cx - MAP_W * 0.5f, ddy = cy - MAP_H * 0.5f;
                if (ddx * ddx + ddy * ddy < 30.0f * 30.0f) continue;   // 别贴出生点
                bool far = true;                                       // 离已有屋子远些
                for (const House& h : houses) {
                    float hx = h.tx - cx, hy = h.ty - cy;
                    if (hx * hx + hy * hy < 14.0f * 14.0f) { far = false; break; }
                }
                if (!far) continue;
                ok = BuildHouse(cx, cy, 4, 3, 1);
            }
        }
        // 废弃铁匠铺（铁匠鬼驻守 + 矿脉）：离铁匠屋至少 50 瓦
        {
            bool ok = false;
            for (int attempt = 0; attempt < 800 && !ok; attempt++) {
                int cx = 8 + (int)(FRnd() % (MAP_W - 16));
                int cy = 8 + (int)(FRnd() % (MAP_H - 16));
                if (smithPos.x > 0.0f) {
                    float dxs = smithPos.x / 16.0f - cx, dys = smithPos.y / 16.0f - cy;
                    if (dxs * dxs + dys * dys < 50.0f * 50.0f) continue;
                }
                bool far = true;
                for (const House& h : houses) {
                    float hx = h.tx - cx, hy = h.ty - cy;
                    if (hx * hx + hy * hy < 14.0f * 14.0f) { far = false; break; }
                }
                if (!far) continue;
                if (BuildHouse(cx, cy, 3, 2, 0)) {
                    // 铁匠鬼站到屋外门口（南墙门豁口正下方）：不再被屋顶盖住，始终看得见
                    smithGhostPos = { cx * 16.0f + 8.0f, (cy + 3) * 16.0f + 8.0f };
                    objAt[(size_t)(cy + 3) * MAP_W + cx] = -1;            // 清空站位（免得树石压着他）
                    TryPlace(ObjKind::OreRock, cx + 3, cy + 2);
                    TryPlace(ObjKind::OreRock, cx - 3, cy - 2);
                    ok = true;
                }
            }
        }
        // 营地石碑：出生点附近 18~30 瓦的空地（领地系统锚点）
        {
            for (int attempt = 0; attempt < 400; attempt++) {
                float a = (FRnd() / 16777216.0f) * 6.2831853f;
                float r = 18.0f + (FRnd() / 16777216.0f) * 12.0f;
                int cx = MAP_W / 2 + (int)(cosf(a) * r);
                int cy = MAP_H / 2 + (int)(sinf(a) * r);
                if (cx < 3 || cy < 3 || cx >= MAP_W - 3 || cy >= MAP_H - 3) continue;
                if (tiles[(size_t)cy * MAP_W + cx] == Tile::Water) continue;
                if (objAt[(size_t)cy * MAP_W + cx] >= 0) continue;
                if (PlaceCampStone(cx, cy)) { campsite = { cx * 16.0f + 8.0f, cy * 16.0f + 8.0f }; break; }
            }
        }
        // 鬼域（鬼游戏·核心）：第一个乱葬岗的中心
        if (!graveyards.empty()) domainPos = graveyards[0];
    }

    // ---- 出生点：从中心螺旋搜索空草原 ----
    spawn = { MAP_W * 8.0f, MAP_H * 8.0f };
    for (int rad = 0; rad < 40 && spawn.x == MAP_W * 8.0f && spawn.y == MAP_H * 8.0f; rad++) {
        for (int dy = -rad; dy <= rad; dy++)
            for (int dx = -rad; dx <= rad; dx++) {
                if (abs(dx) != rad && abs(dy) != rad) continue;
                int x = MAP_W / 2 + dx, y = MAP_H / 2 + dy;
                if (x < 2 || y < 2 || x >= MAP_W - 2 || y >= MAP_H - 2) continue;
                bool ok = tiles[(size_t)y * MAP_W + x] == Tile::Grass;
                for (int oy = -1; oy <= 1 && ok; oy++)
                    for (int ox = -1; ox <= 1 && ok; ox++) {
                        if (tiles[(size_t)(y + oy) * MAP_W + x + ox] == Tile::Water) ok = false;
                        if (objAt[(size_t)(y + oy) * MAP_W + x + ox] >= 0) ok = false;
                    }
                if (ok) { spawn = { x * 16.0f + 8.0f, y * 16.0f + 8.0f }; break; }
            }
    }

    // ---- 地面预渲染（2560x2560 大纹理，一次绘制）----
    // 先把瓦片纹理转成 Image（ImageDraw 需要 Image 源）
    Image imgGrass[4], imgForest[3], imgSand[3];
    for (int i = 0; i < 4; i++) imgGrass[i] = LoadImageFromTexture(a.grass[i]);
    for (int i = 0; i < 3; i++) { imgForest[i] = LoadImageFromTexture(a.forest[i]); imgSand[i] = LoadImageFromTexture(a.sand[i]); }
    Image imgWater = LoadImageFromTexture(a.water[0]);   // 地面底色仅需 1 帧（冗余清理）

    Image img = GenImageColor(MAP_W * TILE, MAP_H * TILE, BLACK);
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            Tile t = tiles[(size_t)y * MAP_W + x];
            unsigned hv = (unsigned)(Hash2(x, y, seed ^ 0xBEEFu) * 4.0f);
            Image src;
            if (t == Tile::Grass) src = imgGrass[hv & 3];
            else if (t == Tile::Forest) src = imgForest[hv % 3];
            else if (t == Tile::Sand) src = imgSand[hv % 3];
            else src = imgWater;
            Rectangle s = { 0, 0, TILE, TILE };
            Rectangle d = { (float)(x * TILE), (float)(y * TILE), TILE, TILE };
            ImageDraw(&img, src, s, d, WHITE);
            if (t == Tile::Water) {              // 湖心渐深：水邻居越多越深（烘焙零运行时开销）
                int wn = 0;
                if (y > 0 && tiles[(size_t)(y - 1) * MAP_W + x] == Tile::Water) wn++;
                if (x < MAP_W - 1 && tiles[(size_t)y * MAP_W + x + 1] == Tile::Water) wn++;
                if (y < MAP_H - 1 && tiles[(size_t)(y + 1) * MAP_W + x] == Tile::Water) wn++;
                if (x > 0 && tiles[(size_t)y * MAP_W + x - 1] == Tile::Water) wn++;
                if (wn > 0) ImageDrawRectangle(&img, x * TILE, y * TILE, TILE, TILE,
                                               (Color) { 22, 40, 34, (unsigned char)(17 * wn) });
                unsigned char sd = shoreDist[(size_t)y * MAP_W + x];
                if (sd == 1)        // 贴岸浅滩：透出沙色（水陆自然过渡，无几何断层）
                    ImageDrawRectangle(&img, x * TILE, y * TILE, TILE, TILE, (Color) { 214, 200, 158, 66 });
                else if (sd == 2)   // 近岸弱浅滩
                    ImageDrawRectangle(&img, x * TILE, y * TILE, TILE, TILE, (Color) { 214, 200, 158, 28 });
            }
        }
    ground = LoadTextureFromImage(img);
    UnloadImage(img);
    for (int i = 0; i < 4; i++) UnloadImage(imgGrass[i]);
    for (int i = 0; i < 3; i++) { UnloadImage(imgForest[i]); UnloadImage(imgSand[i]); }
    UnloadImage(imgWater);
    SetTextureFilter(ground, TEXTURE_FILTER_POINT);
}

void World::Unload() {
    if (ground.id != 0) { UnloadTexture(ground); ground = {}; }
}

// 建造：放置木墙（可通行地形 + 该格无物体）
bool World::PlaceWall(int tx, int ty) {
    if (tx < 1 || ty < 1 || tx >= MAP_W - 1 || ty >= MAP_H - 1) return false;
    if (!WalkableTile(tx, ty)) return false;
    if (objAt[(size_t)ty * MAP_W + tx] >= 0) return false;
    WorldObj o;
    o.kind = ObjKind::Wall; o.tx = (unsigned char)tx; o.ty = (unsigned char)ty;
    o.hp = 6; o.shake = 0; o.harvested = false; o.regrow = 0;   // 耐久 6
    objAt[(size_t)ty * MAP_W + tx] = (int)objs.size();
    objs.push_back(o);
    return true;
}

// 建造：放置草席床
bool World::PlaceBed(int tx, int ty) {
    if (tx < 1 || ty < 1 || tx >= MAP_W - 1 || ty >= MAP_H - 1) return false;
    if (!WalkableTile(tx, ty)) return false;
    if (objAt[(size_t)ty * MAP_W + tx] >= 0) return false;
    WorldObj o;
    o.kind = ObjKind::Bed; o.tx = (unsigned char)tx; o.ty = (unsigned char)ty;
    o.hp = 1; o.shake = 0; o.harvested = false; o.regrow = 0;
    objAt[(size_t)ty * MAP_W + tx] = (int)objs.size();
    objs.push_back(o);
    return true;
}

// 放置营地石碑（领地系统锚点）
bool World::PlaceCampStone(int tx, int ty) {
    if (tx < 1 || ty < 1 || tx >= MAP_W - 1 || ty >= MAP_H - 1) return false;
    if (!WalkableTile(tx, ty)) return false;
    if (objAt[(size_t)ty * MAP_W + tx] >= 0) return false;
    WorldObj o;
    o.kind = ObjKind::CampStone; o.tx = (unsigned char)tx; o.ty = (unsigned char)ty;
    o.hp = 1; o.shake = 0; o.harvested = false; o.regrow = 0;
    objAt[(size_t)ty * MAP_W + tx] = (int)objs.size();
    objs.push_back(o);
    return true;
}

// 放置/授予工作台（lv = 等级 1..3，存于 hp 字段）
bool World::PlaceWorkbench(int tx, int ty, int lv) {
    if (tx < 1 || ty < 1 || tx >= MAP_W - 1 || ty >= MAP_H - 1) return false;
    if (!WalkableTile(tx, ty)) return false;
    if (objAt[(size_t)ty * MAP_W + tx] >= 0) return false;
    WorldObj o;
    o.kind = ObjKind::Workbench; o.tx = (unsigned char)tx; o.ty = (unsigned char)ty;
    o.hp = (unsigned char)lv; o.shake = 0; o.harvested = false; o.regrow = 0;
    objAt[(size_t)ty * MAP_W + tx] = (int)objs.size();
    objs.push_back(o);
    return true;
}

// 把已放置的木墙标记为屋顶沿（营地庇护屋的北沿；regrow 复用作标记）
bool World::MarkWallRoof(int tx, int ty) {
    int oi = ObjIndexAt(tx, ty);
    if (oi < 0 || objs[(size_t)oi].kind != ObjKind::Wall) return false;
    objs[(size_t)oi].regrow = 1.0f;
    return true;
}

// 拆除该瓦片上的物体（领地房屋升级重建用）：swap-pop 并同步修 objAt / campfires 索引
bool World::RemoveObjAt(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return false;
    int idx = objAt[(size_t)ty * MAP_W + tx];
    if (idx < 0 || idx >= (int)objs.size()) { objAt[(size_t)ty * MAP_W + tx] = -1; return false; }
    int last = (int)objs.size() - 1;
    objs[(size_t)idx] = objs[(size_t)last];
    objs.pop_back();
    objAt[(size_t)ty * MAP_W + tx] = -1;
    if (idx != last) {
        const WorldObj& mv = objs[(size_t)idx];
        objAt[(size_t)mv.ty * MAP_W + mv.tx] = idx;
    }
    // campfires 是 objs 下标缓存：swap-pop 后全部失效，直接重扫重建（数量小，代价可忽略）
    campfires.clear();
    for (size_t i = 0; i < objs.size(); i++)
        if (objs[i].kind == ObjKind::Campfire) campfires.push_back((int)i);
    return true;
}

// 最近指定物体下标（石碑/床等通用查询）
int World::NearObj(ObjKind k, float x, float y, float rad) const {
    int best = -1;
    float bd = rad * rad;
    for (size_t i = 0; i < objs.size(); i++) {
        if (objs[i].kind != k) continue;
        float dx = objs[i].tx * 16.0f + 8.0f - x, dy = objs[i].ty * 16.0f + 8.0f - y;
        float q = dx * dx + dy * dy;
        if (q < bd) { bd = q; best = (int)i; }
    }
    return best;
}

// 屋内判定：以玩家为中心 rad 格范围内有多少面墙（敲门鬼只在《屋内》找上门）
int World::CountWallAround(float x, float y, int rad) const {
    int ptx = (int)(x / TILE), pty = (int)(y / TILE);
    int cnt = 0;
    for (int oy = -rad; oy <= rad; oy++)
        for (int ox = -rad; ox <= rad; ox++) {
            int idx = ObjIndexAt(ptx + ox, pty + oy);
            if (idx >= 0 && objs[(size_t)idx].kind == ObjKind::Wall) cnt++;
        }
    return cnt;
}

int World::NearBed(float x, float y, float rad) const {
    for (size_t i = 0; i < objs.size(); i++) {
        if (objs[i].kind != ObjKind::Bed) continue;
        float dx = objs[i].tx * 16.0f + 8.0f - x, dy = objs[i].ty * 16.0f + 8.0f - y;
        if (dx * dx + dy * dy < rad * rad) return (int)i;
    }
    return -1;
}

// 砍倒树木后：该瓦片地面按地类即时还原（草地/沙滩，清除森林深色残留，阴影随之消失）
void World::ClearTreeGround(int tx, int ty, const Assets& a) {
    if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return;
    if (ground.id == 0) return;
    unsigned hv = (unsigned)(Hash2(tx, ty, seed ^ 0xBEEFu) * 4.0f);
    // 棕榈长在沙滩：还沙地；其余树（阔叶/松/桦）还草地
    bool onSand = tiles[(size_t)ty * MAP_W + tx] == Tile::Sand;
    Image g = LoadImageFromTexture(onSand ? a.sand[hv % 3] : a.grass[hv & 3]);
    UpdateTextureRec(ground, { (float)(tx * TILE), (float)(ty * TILE), (float)TILE, (float)TILE }, g.data);
    UnloadImage(g);
}

Tile World::TileAt(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return Tile::Water;
    return tiles[(size_t)ty * MAP_W + tx];
}

bool World::WalkableTile(int tx, int ty) const { return TileAt(tx, ty) != Tile::Water; }

int World::ObjIndexAt(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return -1;
    return objAt[(size_t)ty * MAP_W + tx];
}

bool World::ObjSolid(ObjKind k) const {
    return k == ObjKind::Tree || k == ObjKind::TreePine || k == ObjKind::TreeBirch ||
           k == ObjKind::TreePalm || k == ObjKind::Rock || k == ObjKind::OreRock ||
           k == ObjKind::Campfire || k == ObjKind::RuinWall || k == ObjKind::Chest ||
           k == ObjKind::Workbench || k == ObjKind::Wall;   // 床不做固体：可踩上去
}

// 圆 vs AABB 相交
static bool CircleAABB(float cx, float cy, float r, float x0, float y0, float w, float h) {
    float nx = cx < x0 ? x0 : (cx > x0 + w ? x0 + w : cx);
    float ny = cy < y0 ? y0 : (cy > y0 + h ? y0 + h : cy);
    float dx = cx - nx, dy = cy - ny;
    return dx * dx + dy * dy < r * r;
}

bool World::CircleFree(float x, float y, float r, bool ignoreWater) const {
    float maxX = MAP_W * (float)TILE;
    float maxY = MAP_H * (float)TILE;
    if (x < r || y < r || x > maxX - r || y > maxY - r) return false;
    // 查询邻域外扩 1 格：物体的碰撞圆常超出其注册瓦片（树心偏下、半径 >8px 等），
    // 只查圆覆盖的瓦片会漏检"擦边"情况，让玩家贴得比碰撞体积允许得更近，
    // 也会使各树种差异化的半径在正面接近时全部退化成同一个瓦片边界值。
    constexpr int MARGIN = 1;
    int tx0 = (int)floorf((x - r) / TILE) - MARGIN, tx1 = (int)floorf((x + r) / TILE) + MARGIN;
    int ty0 = (int)floorf((y - r) / TILE) - MARGIN, ty1 = (int)floorf((y + r) / TILE) + MARGIN;
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (!ignoreWater && TileAt(tx, ty) == Tile::Water &&
                CircleAABB(x, y, r, tx * 16.0f, ty * 16.0f, 16, 16)) return false;
            int idx = ObjIndexAt(tx, ty);
            if (idx < 0) continue;
            const WorldObj& o = objs[(size_t)idx];
            if (!ObjSolid(o.kind)) continue;
            // 物体碰撞圆
            // 每种树的碰撞体积各不相同：阔叶粗壮、松树锥形细干且下盘宽、
            // 白桦细高、棕榈斜干重心偏移（外观与碰撞一一对应）
            float ox = o.tx * 16.0f + 8.0f;
            float oy = o.ty * 16.0f + 8.0f;
            float orad = 5.0f;
            switch (o.kind) {
            case ObjKind::Tree:                                   // 阔叶：粗壮树干
                oy = o.ty * 16.0f + 13.0f; orad = 6.0f;  break;
            case ObjKind::TreePine:                               // 松：锥形，干细而下盘宽
                oy = o.ty * 16.0f + 14.0f; orad = 4.0f;  break;
            case ObjKind::TreeBirch:                              // 白桦：细高，碰撞最小
                oy = o.ty * 16.0f + 12.0f; orad = 3.5f;  break;
            case ObjKind::TreePalm:                               // 棕榈：斜干，碰撞圆心右偏
                ox = o.tx * 16.0f + 10.0f;
                oy = o.ty * 16.0f + 13.0f; orad = 4.5f;  break;
            case ObjKind::Rock:
            case ObjKind::OreRock:
                orad = 7.5f; break;
            default:
                break;                                            // 篝火/残墙/宝箱/工作台：默认 5.0f
            }
            float dx = x - ox, dy = y - oy;
            if (dx * dx + dy * dy < (r + orad) * (r + orad)) return false;
        }
    return true;
}

Vector2 World::MoveCircle(float x, float y, float r, float dx, float dy, bool ignoreWater) const {
    // X 轴先动：被挡则归零（轴分离天然产生沿墙滑动）
    if (dx != 0) { if (CircleFree(x + dx, y, r, ignoreWater)) x += dx; else { if (CircleFree(x + dx * 0.25f, y, r, ignoreWater)) x += dx * 0.25f; } }
    if (dy != 0) { if (CircleFree(x, y + dy, r, ignoreWater)) y += dy; else { if (CircleFree(x, y + dy * 0.25f, r, ignoreWater)) y += dy * 0.25f; } }
    return { x, y };
}

int World::NearCampfire(float x, float y, float rad) const {
    for (int idx : campfires) {
        const WorldObj& o = objs[(size_t)idx];
        float dx = o.tx * 16.0f + 8.0f - x, dy = o.ty * 16.0f + 8.0f - y;
        if (dx * dx + dy * dy < rad * rad) return idx;
    }
    return -1;
}

int World::NearWorkbench(float x, float y, float rad) const {
    const std::vector<WorldObj>& ov = objs;
    for (size_t i = 0; i < ov.size(); i++) {
        if (ov[i].kind != ObjKind::Workbench) continue;
        float dx = ov[i].tx * 16.0f + 8.0f - x, dy = ov[i].ty * 16.0f + 8.0f - y;
        if (dx * dx + dy * dy < rad * rad) return (int)i;
    }
    return -1;
}

bool World::PlaceCampfire(int tx, int ty) {
    if (tx < 1 || ty < 1 || tx >= MAP_W - 1 || ty >= MAP_H - 1) return false;
    if (!WalkableTile(tx, ty)) return false;
    if (objAt[(size_t)ty * MAP_W + tx] >= 0) return false;
    WorldObj o;
    o.kind = ObjKind::Campfire; o.tx = (unsigned char)tx; o.ty = (unsigned char)ty;
    o.hp = 0; o.shake = 0; o.harvested = false; o.regrow = 0;
    objAt[(size_t)ty * MAP_W + tx] = (int)objs.size();
    campfires.push_back((int)objs.size());
    objs.push_back(o);
    return true;
}

unsigned (*WDropIdCb)() = nullptr;   // 联机掉落 id 分配（main.cpp 设置）
bool WDropSuppress = false;          // 联机客人：抑制本地掉落生成
void WSuppressDrops(bool on) { WDropSuppress = on; }

void World::SpawnDrop(ItemKind k, float x, float y, float vx, float vy) {
    std::vector<Drop>& dv = drops;
    if (dv.size() >= 200) dv.erase(dv.begin());   // 上限保护（极少触发）
    Drop d{ k, x, y, vx, vy, 0, 0 };
    d.netId = WDropIdCb ? WDropIdCb() : 0;        // 联机：分配稳定网络 id
    dv.push_back(d);
}

void World::UpdateDrops(float px, float py, float dt,
                        void (*onPickup)(const Drop&, float, float)) {
    std::vector<Drop>& dv = drops;
    for (size_t i = 0; i < dv.size(); ) {
        Drop& d = dv[i];
        d.age += dt;
        float dx = px - d.x, dy = py - d.y;
        float dist2 = dx * dx + dy * dy;
        if (dist2 < 48.0f * 48.0f && d.age > 0.35f) {   // 磁吸
            float dist = sqrtf(dist2);
            if (dist > 0.001f) {   // dist==0（掉落物与玩家重合）时 dx/dist 为 NaN，会永久污染坐标
                d.vx += dx / dist * 900.0f * dt;
                d.vy += dy / dist * 900.0f * dt;
            }
        } else {
            d.vx *= (1.0f - 6.0f * dt);   // 摩擦
            d.vy *= (1.0f - 6.0f * dt);
        }
        d.x += d.vx * dt; d.y += d.vy * dt;
        if (dist2 < 10.0f * 10.0f && d.age > 0.35f) {   // 拾取
            if (onPickup) onPickup(d, d.x, d.y);
            d = dv.back(); dv.pop_back();
            continue;
        }
        if (d.age > 120.0f) {   // 超时消失
            d = dv.back(); dv.pop_back();
            continue;
        }
        i++;
    }
}
