#include "creature.hpp"
#include "world.hpp"
#include <cmath>
#include <cstdlib>

// ============================================================
// 生物 AI 实现（神秘复苏 · 十大规则鬼）
// 每种鬼有专属触发规则；未触发时蛰伏无害（不进入敌对状态）。
// ============================================================

namespace {

float FRnd01() { return (float)rand() / (float)RAND_MAX; }

// 随机单位向量
void RandDir(float& dx, float& dy) {
    float a = FRnd01() * 6.283185f;
    dx = cosf(a); dy = sinf(a);
}

// 数值参数表（按种类）
struct MobStat {
    int hp; float wanderSpd, fleeSpd, chaseSpd;
    float radius;
};
const MobStat& Stat(CreatureKind k) {
    static const MobStat RABBIT  = { 2,  60.0f, 112.0f,   0.0f, 5.0f };
    static const MobStat DEER    = { 4,  34.0f, 128.0f,   0.0f, 6.5f };
    static const MobStat GCHILD  = { 8,  30.0f,  92.0f,  70.0f, 6.0f };  // 鬼童：远程魂息风筝
    static const MobStat GTEEN   = { 16, 45.0f,   0.0f, 158.0f, 5.0f };  // 少年：快速近战
    static const MobStat GADULT  = { 60, 20.0f,   0.0f,  58.0f, 9.0f };  // 成年：坦克 + 召唤鬼奴
    static const MobStat LONE    = { 30, 16.0f,   0.0f, 150.0f, 6.0f };  // 单鬼：穿墙必死追击
    static const MobStat NINE    = { 40, 26.0f,  70.0f,  64.0f, 6.0f };  // 9面鬼：远程红光弹
    static const MobStat MIMIC   = { 14, 10.0f,   0.0f, 142.0f, 6.0f };  // 所有人：站立伪装，触发后暴起
    static const MobStat FACE    = { 26, 12.0f,   0.0f,  96.0f, 6.5f };  // 无脸鬼：智慧走位包抄（背对加速/正面绕行）
    static const MobStat GAME    = { 12, 10.0f,   0.0f,  88.0f, 6.0f };  // 游戏鬼：猜拳输了才动
    static const MobStat DEAL    = { 20, 10.0f,   0.0f,   0.0f, 6.0f };  // 交易鬼：永不攻击
    static const MobStat KNOCK   = { 10, 30.0f,   0.0f, 185.0f, 5.0f };  // 敲门鬼：飞行突袭
    static const MobStat MANY    = { 18, 22.0f,   0.0f, 148.0f, 5.5f };  // 多面人：高速毒咬式
    static const MobStat DOMAIN  = { 120, 0.0f,   0.0f,   0.0f, 10.0f }; // 鬼域核心：钉死原地
    static const MobStat SMITH   = { 80, 14.0f,   0.0f,  52.0f, 9.0f };  // 铁匠鬼：重甲反击
    switch (k) {
    case CreatureKind::Rabbit:      return RABBIT;
    case CreatureKind::Deer:        return DEER;
    case CreatureKind::GhostChild:  return GCHILD;
    case CreatureKind::GhostTeen:   return GTEEN;
    case CreatureKind::GhostAdult:  return GADULT;
    case CreatureKind::LoneGhost:   return LONE;
    case CreatureKind::NineFace:    return NINE;
    case CreatureKind::MimicAll:    return MIMIC;
    case CreatureKind::Faceless:    return FACE;
    case CreatureKind::GameGhost:   return GAME;
    case CreatureKind::DealGhost:   return DEAL;
    case CreatureKind::KnockGhost:  return KNOCK;
    case CreatureKind::ManyFaces:   return MANY;
    case CreatureKind::GhostDomain: return DOMAIN;
    case CreatureKind::SmithGhost:  return SMITH;
    default:                        return RABBIT;
    }
}

// 近战参数表
struct MeleeCfg {
    float dmg;        // 单次伤害
    float windup;     // 前摇秒数（红闪警示）
    float atkRange;   // 进入前摇的距离
    float hitRange;   // 前摇结束时判定命中距离
    float cd;         // 攻击冷却
};
const MeleeCfg& Melee(CreatureKind k) {
    // 威胁分级（玩家初始 100 血）：
    //   S 级（成年鬼种 / 单鬼 / 鬼域核心 / 铁匠鬼）= 重创级 55~70
    //   普通 = 无护甲时 2~3 击重创（30~40）
    static const MeleeCfg GCHILDM = { 12.0f, 0.55f, 26.0f, 34.0f, 1.20f };
    static const MeleeCfg GTEENM  = { 30.0f, 0.22f, 24.0f, 30.0f, 0.85f };
    static const MeleeCfg GADULTM = { 46.0f, 0.85f, 30.0f, 40.0f, 1.60f };
    static const MeleeCfg LONEM   = { 55.0f, 0.30f, 26.0f, 34.0f, 0.90f };   // 单鬼：必死级高速
    static const MeleeCfg NINEM   = { 20.0f, 0.45f, 26.0f, 34.0f, 1.10f };
    static const MeleeCfg MIMICM  = { 34.0f, 0.26f, 24.0f, 30.0f, 0.95f };
    static const MeleeCfg FACEM   = { 38.0f, 0.30f, 26.0f, 36.0f, 1.20f };
    static const MeleeCfg GAMEM   = { 30.0f, 0.45f, 26.0f, 34.0f, 1.10f };
    static const MeleeCfg KNOCKM  = { 36.0f, 0.12f, 22.0f, 28.0f, 0.70f };
    static const MeleeCfg MANYM   = { 22.0f, 0.18f, 24.0f, 30.0f, 0.90f };
    static const MeleeCfg DOMAINM = { 50.0f, 1.00f, 30.0f, 44.0f, 1.80f };
    static const MeleeCfg SMITHM  = { 60.0f, 1.00f, 30.0f, 40.0f, 1.80f };
    static const MeleeCfg ZERO    = { 0.0f,  0.40f, 26.0f, 34.0f, 1.00f };   // 交易鬼：无攻击
    switch (k) {
    case CreatureKind::GhostChild:  return GCHILDM;
    case CreatureKind::GhostTeen:   return GTEENM;
    case CreatureKind::GhostAdult:  return GADULTM;
    case CreatureKind::LoneGhost:   return LONEM;
    case CreatureKind::NineFace:    return NINEM;
    case CreatureKind::MimicAll:    return MIMICM;
    case CreatureKind::Faceless:    return FACEM;
    case CreatureKind::GameGhost:   return GAMEM;
    case CreatureKind::DealGhost:   return ZERO;
    case CreatureKind::KnockGhost:  return KNOCKM;
    case CreatureKind::ManyFaces:   return MANYM;
    case CreatureKind::GhostDomain: return DOMAINM;
    case CreatureKind::SmithGhost:  return SMITHM;
    default:                        return ZERO;
    }
}

inline bool Hostile(CreatureKind k) {
    return k != CreatureKind::Rabbit && k != CreatureKind::Deer;
}

// 飞行种（单鬼/敲门鬼）：无视地形碰撞
inline bool Flying(CreatureKind k) {
    return k == CreatureKind::LoneGhost || k == CreatureKind::KnockGhost;
}

// 鬼种·成年 召唤落地（循环外统一补怪，避免遍历 mobs 中 push_back 导致引用失效）
static int ghostSummon = 0;
static float ghostSummonX = 0, ghostSummonY = 0;

// 数量维持的跨帧状态（NewGame 重开时必须清零）
static float sMaintCheck = 0.0f;
static int   sSmithDeadDay = -999;    // 铁匠鬼死亡日（3 天后重生）

// 盟友仇恨吸引目标解算：宠物/鬼仆中最近且明显更近者成为攻击目标
// 返回 -1=无（打玩家） 0=宠物 1..3=鬼仆
inline int AllyTaunt(const PetInfo& pet, const Creature& c, float distP,
                     float& tgtX, float& tgtY, float& distT) {
    if (distP <= 55.0f) return -1;
    float bestQ = distP * 0.8f;
    int best = -1; float bx = 0, by = 0, bq = 0;
    auto Check = [&](float ax, float ay, int idx) {
        float dx = ax - c.x, dy = ay - c.y;
        float dq = sqrtf(dx * dx + dy * dy);
        if (dq < 110.0f && dq < bestQ) { bestQ = dq; best = idx; bx = ax; by = ay; bq = dq; }
    };
    if (pet.on) Check(pet.x, pet.y, 0);
    for (int i = 0; i < pet.ghostN && i < 3; i++)
        Check(pet.ghostX[i], pet.ghostY[i], i + 1);
    if (best < 0) return -1;
    tgtX = bx; tgtY = by; distT = bq;
    return best;
}

// 盟友坐标（-1=玩家 0=宠物 1..3=鬼仆）
inline void AllyPos(const PetInfo& pet, int allyIdx, float px, float py,
                    float& hx, float& hy) {
    hx = px; hy = py;
    if (allyIdx == 0) { hx = pet.x; hy = pet.y; }
    else if (allyIdx > 0 && allyIdx <= pet.ghostN) {
        hx = pet.ghostX[allyIdx - 1]; hy = pet.ghostY[allyIdx - 1];
    }
}

// 盟友命中路由（-1=玩家 0=宠物 1..3=鬼仆）
inline void AllyHit(int allyIdx, float dmg, float dx, float dy) {
    if (allyIdx == 0) PetHit(dmg, dx, dy);
    else if (allyIdx > 0) AllyGhostHit(allyIdx - 1, dmg, dx, dy);
    else MobHitPlayer(dmg, dx, dy);
}

// 出生一只地表生物（距玩家 minDist 以外、非水、非固体）
static bool FarFromOthers(const std::vector<Creature>& mobs, float x, float y, float md) {
    float md2 = md * md;
    for (const Creature& c : mobs) {
        float dx = c.x - x, dy = c.y - y;
        if (dx * dx + dy * dy < md2) return false;
    }
    return true;
}

bool SpawnOne(std::vector<Creature>& mobs, const World& w, CreatureKind kind,
              float px, float py, float minDist) {
    for (int tryN = 0; tryN < 40; tryN++) {
        float x = FRnd01() * (MAP_W * TILE - 64) + 32;
        float y = FRnd01() * (MAP_H * TILE - 64) + 32;
        float dx = x - px, dy = y - py;
        if (dx * dx + dy * dy < minDist * minDist) continue;
        if (!w.CircleFree(x, y, 8.0f)) continue;
        float spread = (tryN < 24) ? 160.0f : (tryN < 34 ? 90.0f : 0.0f);
        if (spread > 0.0f && !FarFromOthers(mobs, x, y, spread)) continue;
        Creature c;
        c.kind = kind; c.x = x; c.y = y; c.hp = Stat(kind).hp;
        c.state = AState::Idle; c.timer = FRnd01() * 2.0f;
        mobs.push_back(c);
        return true;
    }
    return false;
}

// 朝目标方向移动（带碰撞滑动）
float MoveToward(Creature& c, const World& w, float tx, float ty, float spd, float dt) {
    float dx = tx - c.x, dy = ty - c.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return 0;
    dx /= len; dy /= len;
    if (fabsf(dx) > fabsf(dy)) c.dir = dx > 0 ? 2 : 3;
    else c.dir = dy > 0 ? 0 : 1;
    float r = Stat(c.kind).radius;
    Vector2 np = w.MoveCircle(c.x, c.y, r, dx * spd * dt, dy * spd * dt);
    float moved = sqrtf((np.x - c.x) * (np.x - c.x) + (np.y - c.y) * (np.y - c.y));
    c.x = np.x; c.y = np.y;
    return moved;
}

// 飞行移动：无视地形，直接位移 + 世界边界钳制
void MoveFly(Creature& c, float tx, float ty, float spd, float dt) {
    float dx = tx - c.x, dy = ty - c.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    dx /= len; dy /= len;
    if (fabsf(dx) > fabsf(dy)) c.dir = dx > 0 ? 2 : 3;
    else c.dir = dy > 0 ? 0 : 1;
    c.x += dx * spd * dt; c.y += dy * spd * dt;
    float maxX = MAP_W * 16.0f - 10.0f, maxY = MAP_H * 16.0f - 10.0f;
    if (c.x < 10.0f) c.x = 10.0f; else if (c.x > maxX) c.x = maxX;
    if (c.y < 10.0f) c.y = 10.0f; else if (c.y > maxY) c.y = maxY;
}

// ============================================================
// 各鬼专属触发规则（"鬼不会无缘无故杀人，除非你触碰了规则"）
// 返回 true = 本帧触发。部分规则（计时类）由 ruleT 在调用侧累计。
// ============================================================
bool GhostTriggered(const Creature& c, const World& w, float px, float py,
                    const PlayerCtx& pc, bool night, float gameTime, float distP) {
    (void)w; (void)px; (void)py; (void)night; (void)gameTime;
    if (pc.hiding) return false;                    // 隐身符：敛息期间鬼察觉不到你
    switch (c.kind) {
    case CreatureKind::GhostChild:
    case CreatureKind::GhostTeen:
    case CreatureKind::GhostAdult:
    case CreatureKind::NineFace:
    case CreatureKind::SmithGhost:
        // 犯之则反：只有先动手打它，它才会还手
        return c.hp < Stat(c.kind).hp;
    case CreatureKind::LoneGhost:
        // 落单必死：玩家孤身（无宠物/鬼仆/幸存者）靠近即开始倒数
        return pc.alone;
    case CreatureKind::MimicAll:     // 所有人：贴脸交谈即触发（蛰伏时与幸存者完全同款，无破绽）
        return distP < 42.0f || c.hp < Stat(c.kind).hp;
    case CreatureKind::Faceless:
        // 无脸鬼：蛰伏伪装常人漂移接近；先动手或被靠近即触发
        return c.hp < Stat(c.kind).hp;   // 犯之则反（打它才反击）
    case CreatureKind::GameGhost:
        // 猜拳定生死：永不主动（猜拳输/平局时由 main 置 triggered）
        return false;
    case CreatureKind::DealGhost:
        // 等价交换：永不攻击
        return false;
    case CreatureKind::KnockGhost:
        // 闻声开门：实体化时已被惊动（生成即 triggered）
        return true;
    case CreatureKind::ManyFaces:
        // 对视即被盯上：靠得太近被它注意到
        return distP < 70.0f || c.hp < Stat(c.kind).hp;
    case CreatureKind::GhostDomain:
        // 鬼游戏核心：坐镇鬼域当"庄家"（真正的威胁是域内代理游戏鬼与它设的局），
        // 只有你动手打它，它才还手。
        return c.hp < Stat(c.kind).hp;
    default:
        return false;
    }
}

// 单鬼与多面人这类"可解除"的触发：条件消失后回到蛰伏
bool TriggerRelaxable(CreatureKind k) {
    return k == CreatureKind::LoneGhost || k == CreatureKind::GhostDomain;
}

// 游荡鬼随机池（5% 遇鬼事件的抽签表；不含驻点鬼/交易鬼）
CreatureKind RollWanderGhost() {
    static const CreatureKind pool[9] = {
        CreatureKind::GhostChild, CreatureKind::GhostChild,      // 鬼种最常见
        CreatureKind::GhostTeen,
        CreatureKind::LoneGhost,
        CreatureKind::MimicAll,
        CreatureKind::Faceless,
        CreatureKind::GameGhost,
        CreatureKind::DealGhost,
        CreatureKind::ManyFaces,
    };
    return pool[(int)(FRnd01() * 9.0f) % 9];
}

} // namespace

// 重开新局：清零跨帧维持状态
void ResetCreatureSpawnState() { sMaintCheck = 0.0f; sSmithDeadDay = -999; }

// ---- 属性查询导出（收鬼系统：鬼仆属性按原怪复刻）----
int   MobBaseHp(CreatureKind k)   { return Stat(k).hp; }
float MobMeleeDmg(CreatureKind k) { return Melee(k).dmg; }
float MobChaseSpd(CreatureKind k) { return Stat(k).chaseSpd; }
float MobRadius(CreatureKind k)   { return Stat(k).radius; }
bool  MobFlying(CreatureKind k)   { return Flying(k); }
bool  MobHostile(CreatureKind k)  { return Hostile(k); }
// 不死单位（铁匠鬼）：玩家/箭矢/伙伴/鬼仆都伤不了它，它也永远不会进入死亡态
bool  MobUndying(CreatureKind k)  { return k == CreatureKind::SmithGhost; }

// 选中/警戒半径（UI 直接以鬼为圆心画圈显示：踏入即被这只鬼盯上）
float MobSightRadius(CreatureKind k) {
    switch (k) {
    case CreatureKind::GhostChild:  return 150.0f;
    case CreatureKind::GhostTeen:   return 165.0f;
    case CreatureKind::GhostAdult:  return 185.0f;
    case CreatureKind::LoneGhost:   return 210.0f;   // 落单必死：200 判定 + 缓冲
    case CreatureKind::NineFace:    return 235.0f;
    case CreatureKind::MimicAll:    return 62.0f;    // 贴脸才亮獠牙（42 判定）
    case CreatureKind::Faceless:    return 140.0f;
    case CreatureKind::GameGhost:   return 280.0f;   // 见面即开鬼域
    case CreatureKind::DealGhost:   return 72.0f;    // 交易圈
    case CreatureKind::KnockGhost:  return 320.0f;   // 声源传播
    case CreatureKind::ManyFaces:   return 220.0f;
    case CreatureKind::GhostDomain: return 170.0f;
    case CreatureKind::SmithGhost:  return 130.0f;
    default:                        return 0.0f;     // 凡兽不画
    }
}

// 鬼名 / 规则文案 / 威胁评级（UI 鬼条与蛰伏提示共用）
const char* GhostName(CreatureKind k) {
    switch (k) {
    case CreatureKind::GhostChild:  return "鬼童";
    case CreatureKind::GhostTeen:   return "鬼种·少年";
    case CreatureKind::GhostAdult:  return "鬼种·成年";
    case CreatureKind::LoneGhost:   return "单鬼";
    case CreatureKind::NineFace:    return "9面鬼";
    case CreatureKind::MimicAll:    return "所有人";
    case CreatureKind::Faceless:    return "无脸鬼";
    case CreatureKind::GameGhost:   return "游戏鬼";
    case CreatureKind::DealGhost:   return "交易鬼";
    case CreatureKind::KnockGhost:  return "敲门鬼";
    case CreatureKind::ManyFaces:   return "多面人";
    case CreatureKind::GhostDomain: return "鬼游戏·核心";
    case CreatureKind::SmithGhost:  return "铁匠鬼";
    default:                        return "凡兽";
    }
}
const char* GhostRank(CreatureKind k) {
    switch (k) {
    case CreatureKind::GhostAdult:
    case CreatureKind::LoneGhost:
    case CreatureKind::GhostDomain:  return "S";
    case CreatureKind::GhostTeen:
    case CreatureKind::NineFace:
    case CreatureKind::MimicAll:
    case CreatureKind::Faceless:
    case CreatureKind::SmithGhost:   return "A";
    default:                         return "B";
    }
}
const char* GhostRuleText(CreatureKind k) {
    switch (k) {
    case CreatureKind::GhostChild:
    case CreatureKind::GhostTeen:
    case CreatureKind::GhostAdult:
    case CreatureKind::NineFace:
    case CreatureKind::SmithGhost:   return "犯之则反";
    case CreatureKind::LoneGhost:    return "落单必死";
    case CreatureKind::MimicAll:     return "莫与交谈";
    case CreatureKind::Faceless:     return "莫久视之";
    case CreatureKind::GameGhost:    return "猜拳定生死";
    case CreatureKind::DealGhost:    return "等价交换";
    case CreatureKind::KnockGhost:   return "闻声勿开门";
    case CreatureKind::ManyFaces:    return "莫要靠近";
    case CreatureKind::GhostDomain:  return "擅入鬼域";
    default:                         return "";
    }
}
const char* GhostRuleTextMask(unsigned short m) {
    if (m & RULE_ALWAYS)  return "见即杀";
    if (m & RULE_NIGHT)   return "入夜方行";
    if (m & RULE_RUN)     return "逐快不逐慢";
    if (m & RULE_BLOOD)   return "嗅血而来";
    if (m & RULE_NEAR)    return "近身方扑";
    if (m & RULE_WEB)     return "触网即至";
    if (m & RULE_GRAVE)   return "擅入者死";
    if (m & RULE_MOVE)    return "见动而袭";
    if (m & RULE_HITBACK) return "犯之则反";
    if (m & RULE_BACK)    return "莫要回头";
    if (m & RULE_WATER)   return "近水现身";
    if (m & RULE_ITEM)    return "窥人用物";
    return "见机而动";
}

// 生成时给一只鬼随机外观种子 + 声音系判定（敲门鬼专属：会敲门）
void RollGhostRule(Creature& c) {
    c.tintSeed = (unsigned char)(FRnd01() * 256.0f);
    c.wailer = (c.kind == CreatureKind::KnockGhost) ||
               (Hostile(c.kind) && FRnd01() < 0.10f);   // 10% 声音系
    c.wailT = 2.0f + FRnd01() * 5.0f;
    c.ruleMask = 0;
}

void InitCreatures(std::vector<Creature>& mobs, const World& w, float px, float py, float gameTime) {
    (void)w; (void)px; (void)py; (void)gameTime;
    mobs.clear();
    mobs.reserve(64);
    // ---- 只放凡兽：遇鬼概率 5%，鬼极少在野外游荡（POI 鬼由 main.cpp 放置）----
    for (int i = 0; i < 12; i++) SpawnOne(mobs, w, CreatureKind::Rabbit, px, py, 120);
    for (int i = 0; i < 8; i++)  SpawnOne(mobs, w, CreatureKind::Deer,   px, py, 160);
}

// 单鬼「时间重启」：第一次被击杀时原地满血复活（返回 true = 已重启）
bool MobPreDeath(Creature& c) {
    if (c.kind != CreatureKind::LoneGhost) return false;
    if (c.phase >= 1) return false;          // 已重启过一次：第二次真死
    c.phase = 1;
    c.hp = Stat(c.kind).hp;
    c.hurtFlash = 0.6f;
    c.state = AState::Hurt;
    c.timer = 0.5f;
    return true;
}

// 通用：近战流程（前摇 -> 判定 -> 冷却）。盟友仇恨吸引由 ally 决定目标
static void MeleeFlow(Creature& c, const World& w, float px, float py,
                      const PetInfo& pet, float distP, float sight,
                      float dt, bool darkCtx, bool& moving, int ally,
                      float tgtX, float tgtY, float distT) {
    const MobStat& st = Stat(c.kind);
    const MeleeCfg& mc = Melee(c.kind);
    if (c.state == AState::Idle) {
        c.timer -= dt;
        if (c.timer <= 0) { c.state = AState::Wander; c.timer = 1.2f + FRnd01() * 1.8f; RandDir(c.wx, c.wy); }
    } else if (c.state == AState::Wander) {
        c.timer -= dt;
        MoveToward(c, w, c.x + c.wx * 10, c.y + c.wy * 10, st.wanderSpd, dt);
        moving = true;
        if (c.timer <= 0) { c.state = AState::Idle; c.timer = 0.8f + FRnd01() * 2.0f; }
    } else if (c.state == AState::Chase) {
        // 篝火威慑：低阶鬼怕火，S 级悍不畏火
        int fire = w.NearCampfire(c.x, c.y, 60.0f);
        bool fearless = GhostRank(c.kind)[0] == 'S';
        if (fire >= 0 && !fearless) {
            const WorldObj& fo = w.objs[(size_t)fire];
            float fx = c.x - (fo.tx * 16.0f + 8.0f), fy = c.y - (fo.ty * 16.0f + 8.0f);
            MoveToward(c, w, c.x + fx, c.y + fy, st.chaseSpd * 0.8f, dt);
        } else {
            float spd = darkCtx ? st.chaseSpd + 8.0f : st.chaseSpd;
            MoveToward(c, w, tgtX, tgtY, spd, dt);
        }
        moving = true;
        if (distT < mc.atkRange && c.atkCd <= 0) { c.state = AState::Windup; c.timer = mc.windup; }
        (void)distP; (void)sight;
    } else if (c.state == AState::Windup) {
        c.timer -= dt;   // 前摇定格（红闪由渲染层处理）
        if (c.timer <= 0) {
            c.state = AState::Attack;
            c.timer = 0.18f;
            float hx, hy;
            AllyPos(pet, ally, px, py, hx, hy);
            if (distT < mc.hitRange) {
                float dx = hx - c.x, dy = hy - c.y;
                float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                AllyHit(ally, mc.dmg * c.dmgMul, dx / l, dy / l);
            }
            c.atkCd = mc.cd;
        }
    } else if (c.state == AState::Attack) {
        c.timer -= dt;
        if (c.timer <= 0) c.state = AState::Chase;
    } else if (c.state == AState::Hurt) {
        c.timer -= dt;
        if (c.timer <= 0) c.state = AState::Chase;   // 已触发：硬直后继续追
    }
}

// 联机：当前生物选中的目标下标（伤害路由：MobHitPlayer 按它决定打谁）
static int sCurTargetIdx = 0;
int CreatureTargetIndex() { return sCurTargetIdx; }

void UpdateCreatures(std::vector<Creature>& mobs, World& w, float px, float py,
                     bool playerDead, float gameTime, float dt, const PetInfo& pet,
                     const PlayerCtx& pc) {
    PlayerTarget solo;
    solo.x = px; solo.y = py; solo.dead = playerDead; solo.pc = pc;
    UpdateCreaturesT(mobs, w, &solo, 1, gameTime, dt, pet);
}

void UpdateCreaturesT(std::vector<Creature>& mobs, World& w,
                      const PlayerTarget* tgts, int nTgts,
                      float gameTime, float dt, const PetInfo& pet) {
    const bool night = IsNight(gameTime);
    const bool darkCtx = night;
    const int curDay = (int)(gameTime / DAY_LEN);

    // 目标上下文（每只鬼在循环顶部按"最近的存活玩家"重选，后续逻辑全部沿用这两个局部量）
    float px = tgts[0].x, py = tgts[0].y;
    PlayerCtx pc = tgts[0].pc;
    bool playerDead = tgts[0].dead;

    for (size_t i = 0; i < mobs.size(); ) {
        Creature& c = mobs[i];
        const MobStat& st = Stat(c.kind);

        // ---- 目标重选：伴生鬼/混入同伴永远跟着 0 号（房主）；其余挑最近的存活玩家 ----
        {
            int pick = 0;
            if (!c.companion && !c.infiltrated) {
                float bd = 1e18f; int bi = -1;
                for (int k = 0; k < nTgts; k++) {
                    if (tgts[k].dead) continue;
                    float dx = tgts[k].x - c.x, dy = tgts[k].y - c.y;
                    float d2 = dx * dx + dy * dy;
                    if (d2 < bd) { bd = d2; bi = k; }
                }
                if (bi >= 0) pick = bi;
            }
            sCurTargetIdx = pick;
            px = tgts[pick].x; py = tgts[pick].y;
            pc = tgts[pick].pc;
            playerDead = tgts[pick].dead;
        }

        // ---- 通用计时 ----
        if (c.hurtFlash > 0) c.hurtFlash -= dt;
        if (c.atkCd > 0) c.atkCd -= dt;
        if (c.mCd1 > 0) c.mCd1 -= dt;
        if (c.mCd2 > 0) c.mCd2 -= dt;
        if (c.mCd3 > 0) c.mCd3 -= dt;
        c.animT += dt;

        // ---- 限时存在（敲门鬼实体）：时间到自行消散，不掉落 ----
        if (c.lifeT > 0.0f) {
            c.lifeT -= dt;
            if (c.lifeT <= 0.0f) {
                SpawnHitParticles(c.x, c.y - 8, 120, 130, 200, 10);
                c = mobs.back(); mobs.pop_back(); continue;
            }
        }

        // ---- 死亡：淡出 3 秒后移除 ----
        if (c.state == AState::Dead) {
            c.deadFade += dt;
            if (c.deadFade > 3.0f) { c = mobs.back(); mobs.pop_back(); continue; }
            i++; continue;
        }

        // ---- 伴生鬼（铁匠鬼）：跟随玩家悬浮，永不敌对、永不消散、不可被击伤 ----
        if (c.companion) {
            c.hoverT += dt;
            c.hp = Stat(c.kind).hp;              // 常驻满血：伴生鬼不会死
            c.triggered = false;
            c.hurtFlash = 0;
            c.state = AState::Idle;
            c.kvx = 0; c.kvy = 0;
            // 目标点：玩家侧上方（右肩斜上方，带呼吸起伏）——不挡正前方视野，也绝不在攻击扇形里
            float bob = sinf(c.hoverT * 1.7f) * 4.0f;
            float tx = px + 26.0f, ty = py - 34.0f + bob;
            float dx = tx - c.x, dy = ty - c.y;
            float dl = sqrtf(dx * dx + dy * dy);
            if (dl > 2.0f) {
                float step = fminf(dl, 260.0f * dt);   // 匀速追随（不瞬移，有跟随感）
                c.x += dx / dl * step;
                c.y += dy / dl * step;
                c.dir = (dx > 0.0f) ? 2 : 3;
            }
            i++; continue;
        }
        // ---- 混入同伴的无面鬼：伪装成随行人，跟着队伍走，永不敌对 ----
        if (c.infiltrated) {
            c.hoverT += dt;
            c.hp = Stat(c.kind).hp;
            c.triggered = false;
            c.state = AState::Idle;
            c.kvx = 0; c.kvy = 0;
            // 站位：按外观种子散开成一圈，跟真同伴混在一起（看不出谁是谁）
            float ang = (float)c.tintSeed * 0.7f + c.hoverT * 0.15f;
            float rad = 30.0f + (float)(c.tintSeed % 3) * 9.0f;
            float tx = px + cosf(ang) * rad;
            float ty = py + sinf(ang) * rad * 0.72f + 6.0f;
            float dx = tx - c.x, dy = ty - c.y;
            float dl = sqrtf(dx * dx + dy * dy);
            if (dl > 400.0f) { c.x = tx; c.y = ty; }        // 走丢保护：直接归位
            else if (dl > 3.0f) {
                float step = fminf(dl, 150.0f * dt);
                Vector2 np = w.MoveCircle(c.x, c.y, st.radius, dx / dl * step, dy / dl * step);
                c.x = np.x; c.y = np.y;
                c.dir = fabsf(dx) > fabsf(dy) ? (dx > 0 ? 2 : 3) : (dy > 0 ? 0 : 1);
            }
            c.animT += dt;
            i++; continue;
        }

        // ---- 击退滑移（衰减；飞行种直接位移，重甲高抗性）----
        if (fabsf(c.kvx) + fabsf(c.kvy) > 0.5f) {
            if (Flying(c.kind)) {
                c.x += c.kvx * dt; c.y += c.kvy * dt;
            } else {
                Vector2 np = w.MoveCircle(c.x, c.y, st.radius, c.kvx * dt, c.kvy * dt);
                c.x = np.x; c.y = np.y;
            }
            bool heavy = (c.kind == CreatureKind::GhostAdult ||
                          c.kind == CreatureKind::GhostDomain ||
                          c.kind == CreatureKind::SmithGhost);
            float decay = 1.0f - (heavy ? 26.0f : 8.0f) * dt;
            if (decay < 0) decay = 0;
            c.kvx *= decay; c.kvy *= decay;
        }

        float distP = sqrtf((px - c.x) * (px - c.x) + (py - c.y) * (py - c.y));
        bool moving = false;

        // ---- 无面鬼·混入同伴：蛰伏时贴着玩家够久，就悄悄顶替进队伍 ----
        // （前提：玩家身边本来就有同伴——一个人走时它没必要伪装）
        if (c.kind == CreatureKind::Faceless && !c.infiltrated && !c.triggered &&
            !playerDead && !pc.hiding && pc.hasAlly && distP < 96.0f) {
            c.infilT += dt;
            if (c.infilT >= 6.0f) {
                c.infiltrated = true;                // 混入：此后由主程序接管（计数/同化/提问）
                c.convertT = 24.0f;                  // 刚混进来先蛰伏一阵，别立刻动手
            }
        } else if (c.kind == CreatureKind::Faceless && !c.infiltrated) {
            c.infilT = 0.0f;
        }

        // ---- 声音系鬼：定期发声，玩家在听觉范围内听到就会被盯上 ----
        if (c.wailer && !c.triggered && c.state != AState::Dead && !playerDead) {
            c.wailT -= dt;
            if (c.wailT <= 0.0f) {
                c.wailT = 7.0f;
                if (distP < 220.0f) {
                    c.triggered = true;
                    c.hurtFlash = 0.3f;
                    WailHeard(c.x, c.y);
                }
            }
        }

        // ---- 规则触发判定（未触发的鬼蛰伏无害）----
        if (!c.triggered && !playerDead && Hostile(c.kind)) {
            // 计时类规则：单鬼（落单 8 秒）/ 无脸鬼（贴近 2.5 秒）
            bool timing = false;
            if (c.kind == CreatureKind::LoneGhost && pc.alone && distP < 200.0f) {
                c.ruleT += dt;
                timing = (c.ruleT >= 8.0f);
            } else if (c.kind == CreatureKind::Faceless && distP < 34.0f) {
                c.ruleT += dt;
                timing = (c.ruleT >= 2.5f);
            } else {
                c.ruleT = 0.0f;    // 条件中断：计时清零
            }
            if (timing || GhostTriggered(c, w, px, py, pc, night, gameTime, distP)) {
                c.triggered = true;
                c.hurtFlash = 0.35f;
                c.ruleT = 0.0f;
            }
        }
        // ---- 可解除规则：条件消失后回到蛰伏（单鬼找到同伴 / 离开鬼域）----
        if (c.triggered && !playerDead && TriggerRelaxable(c.kind)) {
            bool relax = false;
            if (c.kind == CreatureKind::LoneGhost && !pc.alone) relax = true;        // 你不再落单
            if (c.kind == CreatureKind::GhostDomain && distP > 260.0f) relax = true; // 你退出了鬼域
            if (relax) {
                c.triggered = false;
                c.state = AState::Idle; c.timer = 1.5f;
                c.ruleT = 0.0f;
            }
        }

        float tgtX = px, tgtY = py, distT = distP;
        int ally = AllyTaunt(pet, c, distP, tgtX, tgtY, distT);

        switch (c.kind) {

        case CreatureKind::Rabbit: {
            // 被动：靠近 80px 逃跑
            if (c.state != AState::Hurt && c.state != AState::Flee && distP < 80.0f) {
                c.state = AState::Flee; c.timer = 1.2f;
            }
            if (c.state == AState::Idle) {
                c.timer -= dt;
                if (c.timer <= 0) { c.state = AState::Wander; c.timer = 0.8f + FRnd01() * 1.2f; RandDir(c.wx, c.wy); }
            } else if (c.state == AState::Wander) {
                c.timer -= dt;
                c.hopT += dt;
                if (fmodf(c.hopT, 0.44f) < 0.24f) {
                    MoveToward(c, w, c.x + c.wx * 10, c.y + c.wy * 10, st.wanderSpd, dt);
                    moving = true;
                }
                if (c.timer <= 0) { c.state = AState::Idle; c.timer = 0.8f + FRnd01() * 1.6f; c.hopT = 0; }
            } else if (c.state == AState::Flee) {
                c.timer -= dt;
                MoveToward(c, w, c.x + (c.x - px), c.y + (c.y - py), st.fleeSpd, dt);
                moving = true;
                if (c.timer <= 0) {
                    if (distP > 150.0f) { c.state = AState::Idle; c.timer = 1.0f; }
                    else c.timer = 0.8f;
                }
            }
            break;
        }

        case CreatureKind::Deer: {
            // 中性：慢速游荡，受击后逃跑
            if (c.state == AState::Idle) {
                c.timer -= dt;
                if (c.timer <= 0) { c.state = AState::Wander; c.timer = 1.5f + FRnd01() * 2.0f; RandDir(c.wx, c.wy); }
            } else if (c.state == AState::Wander) {
                c.timer -= dt;
                MoveToward(c, w, c.x + c.wx * 10, c.y + c.wy * 10, st.wanderSpd, dt);
                moving = true;
                if (c.timer <= 0) { c.state = AState::Idle; c.timer = 1.5f + FRnd01() * 2.5f; }
            } else if (c.state == AState::Flee) {
                c.timer -= dt;
                MoveToward(c, w, c.x + (c.x - px), c.y + (c.y - py), st.fleeSpd, dt);
                moving = true;
                if (c.timer <= 0) { c.state = AState::Idle; c.timer = 2.0f; }
            }
            break;
        }

        case CreatureKind::GhostChild: {
            // 鬼童：远程魂息风筝（未触发时也在缓慢吞魂游荡，靠近不打不闹）
            float sight = 260.0f;
            if (c.state == AState::Idle || c.state == AState::Wander) {
                if (c.state == AState::Idle) {
                    c.timer -= dt;
                    if (c.timer <= 0) { c.state = AState::Wander; c.timer = 1.2f + FRnd01() * 1.6f; RandDir(c.wx, c.wy); }
                } else {
                    c.timer -= dt;
                    MoveToward(c, w, c.x + c.wx * 10, c.y + c.wy * 10, st.wanderSpd, dt);
                    moving = true;
                    if (c.timer <= 0) { c.state = AState::Idle; c.timer = 1.0f + FRnd01() * 1.8f; }
                }
                if (!playerDead && c.triggered && distP < sight) { c.state = AState::Chase; c.timer = 0; }
            } else if (c.state == AState::Chase) {
                if (playerDead || distP > sight + 140.0f) { c.state = AState::Idle; c.timer = 1.0f; break; }
                if (distP < 90.0f) {
                    MoveToward(c, w, c.x + (c.x - px), c.y + (c.y - py), st.fleeSpd, dt);
                } else if (distP > 150.0f) {
                    MoveToward(c, w, px, py, st.chaseSpd, dt);
                }
                moving = true;
                c.timer += dt;
                if (distP < 210.0f && c.timer > 2.2f) { c.state = AState::Windup; c.timer = 0.55f; }
            } else if (c.state == AState::Windup) {
                c.timer -= dt;   // 魂息蓄力
                if (c.timer <= 0) {
                    c.state = AState::Attack;
                    c.timer = 0.22f;
                    if (!playerDead && distP < 260.0f) {
                        float dx = px - c.x, dy = (py - 8.0f) - (c.y - 6.0f);
                        float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                        SpawnSpit(c.x, c.y - 6.0f, dx / l * 140.0f, dy / l * 140.0f);
                    }
                    c.timer = -2.2f;
                }
            } else if (c.state == AState::Attack) {
                c.timer -= dt;
                if (c.timer <= 0) { c.state = (distP < sight + 60.0f && !playerDead) ? AState::Chase : AState::Idle; c.timer = 0; }
            } else if (c.state == AState::Hurt) {
                c.timer -= dt;
                if (c.timer <= 0) { c.state = distP < sight ? AState::Chase : AState::Wander; c.timer = 0; }
            }
            break;
        }

        case CreatureKind::GhostTeen:
            // 鬼种·少年：快速近战
            MeleeFlow(c, w, px, py, pet, distP, 240.0f, dt, darkCtx, moving,
                      ally, tgtX, tgtY, distT);
            break;

        case CreatureKind::GhostAdult: {
            // 鬼种·成年：坦克 + 转化鬼奴（召唤）+ 震地
            if (c.state == AState::Idle || c.state == AState::Wander) {
                c.state = AState::Chase;                       // 成年体：触发即永久仇恨
            } else if (c.state == AState::Chase) {
                if (c.atkCd <= 0 && distP < 34.0f) {           // 贴身普攻
                    c.state = AState::Windup; c.timer = 0.60f; c.move = 3;
                } else if (c.mCd1 <= 0 && distP > 90.0f && distP < 300.0f) {   // 冲锋
                    c.state = AState::Windup; c.timer = 0.55f; c.move = 0;
                    float dx = px - c.x, dy = py - c.y;
                    float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                    c.chargeVx = dx / l; c.chargeVy = dy / l;
                } else if (c.mCd2 <= 0 && distP > 80.0f && distP < 260.0f) {   // 转化鬼奴
                    c.state = AState::Windup; c.timer = 0.90f; c.move = 1;
                } else if (c.mCd3 <= 0 && distP < 70.0f) {     // 震地 AOE
                    c.state = AState::Windup; c.timer = 0.70f; c.move = 2;
                } else {
                    MoveToward(c, w, px, py, st.chaseSpd, dt);
                    moving = true;
                }
            } else if (c.state == AState::Windup) {
                c.timer -= dt;
                if (c.timer <= 0) {
                    if (c.move == 0) {                          // 冲锋启动
                        c.state = AState::Attack; c.timer = 0.50f;
                        c.chargeVx *= 300.0f; c.chargeVy *= 300.0f;
                        BossFx(0);
                    } else if (c.move == 1) {                   // 召唤鬼奴
                        ghostSummon = 2; ghostSummonX = c.x; ghostSummonY = c.y;
                        BossFx(1);
                        c.state = AState::Attack; c.timer = 0.40f;
                    } else if (c.move == 2) {                   // 震地
                        BossFx(2);
                        if (!playerDead && distP < 88.0f) {
                            float dx = px - c.x, dy = py - c.y;
                            float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                            MobHitPlayer(24.0f * c.dmgMul, dx / l, dy / l);
                        }
                        c.state = AState::Attack; c.timer = 0.50f;
                    } else {                                    // 贴身普攻
                        c.state = AState::Attack; c.timer = 0.18f;
                        const MeleeCfg& mc = Melee(c.kind);
                        float hx, hy;
                        AllyPos(pet, ally, px, py, hx, hy);
                        if (distT < mc.hitRange) {
                            float dx = hx - c.x, dy = hy - c.y;
                            float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                            AllyHit(ally, mc.dmg * c.dmgMul, dx / l, dy / l);
                        }
                    }
                    if (c.move == 0) c.mCd1 = 5.0f;
                    else if (c.move == 1) c.mCd2 = 12.0f;
                    else if (c.move == 2) c.mCd3 = 7.0f;
                    else c.atkCd = 1.4f;
                }
            } else if (c.state == AState::Attack) {
                if (c.move == 0) {                              // 冲锋中
                    c.timer -= dt;
                    Vector2 np = w.MoveCircle(c.x, c.y, st.radius, c.chargeVx * dt, c.chargeVy * dt);
                    c.x = np.x; c.y = np.y;
                    moving = true;
                    if (fabsf(c.chargeVx) > fabsf(c.chargeVy)) c.dir = c.chargeVx > 0 ? 2 : 3;
                    else c.dir = c.chargeVy > 0 ? 0 : 1;
                    if (c.atkCd <= 0 && distP < 24.0f) {
                        float dx = px - c.x, dy = py - c.y;
                        float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                        MobHitPlayer(18.0f * c.dmgMul, dx / l, dy / l);
                        c.atkCd = 0.8f;
                    }
                    if (c.timer <= 0) c.state = AState::Chase;
                } else {
                    c.timer -= dt;
                    if (c.timer <= 0) c.state = AState::Chase;
                }
            } else if (c.state == AState::Hurt) {
                c.timer -= dt;
                if (c.timer <= 0) c.state = AState::Chase;
            }
            break;
        }

        case CreatureKind::LoneGhost: {
            // 单鬼：穿墙飞行，落单必死的高速追杀；玩家不再落单时它会退散（触发解除）
            float sight = 320.0f;
            if (c.state == AState::Idle || c.state == AState::Wander) {
                c.timer -= dt;
                if (c.state == AState::Idle && c.timer <= 0) { c.state = AState::Wander; c.timer = 1.4f + FRnd01() * 1.6f; RandDir(c.wx, c.wy); }
                if (c.state == AState::Wander) {
                    MoveFly(c, c.x + c.wx * 10, c.y + c.wy * 10, st.wanderSpd, dt);
                    moving = true;
                    if (c.timer <= 0) { c.state = AState::Idle; c.timer = 1.0f + FRnd01() * 1.5f; }
                }
            } else if (c.state == AState::Chase) {
                MoveFly(c, tgtX, tgtY, st.chaseSpd, dt);
                moving = true;
                const MeleeCfg& mc = Melee(c.kind);
                if (distT < mc.atkRange && c.atkCd <= 0) { c.state = AState::Windup; c.timer = mc.windup; }
            } else if (c.state == AState::Windup) {
                c.timer -= dt;
                if (c.timer <= 0) {
                    c.state = AState::Attack; c.timer = 0.14f;
                    const MeleeCfg& mc = Melee(c.kind);
                    if (distT < mc.hitRange) {
                        float hx, hy;
                        AllyPos(pet, ally, px, py, hx, hy);
                        float dx = hx - c.x, dy = hy - c.y;
                        float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                        AllyHit(ally, mc.dmg * c.dmgMul, dx / l, dy / l);
                    }
                    c.atkCd = mc.cd;
                }
            } else if (c.state == AState::Attack) {
                c.timer -= dt;
                if (c.timer <= 0) c.state = AState::Chase;
            } else if (c.state == AState::Hurt) {
                c.timer -= dt;
                if (c.timer <= 0) c.state = AState::Chase;
            }
            (void)sight;
            break;
        }

        case CreatureKind::NineFace: {
            // 9面鬼：远程红光弹；掉血逐层解锁权能
            //   层0 红光覆盖（远程）-> 层1 空间位移（瞬移逼近）-> 层2 修改现实（命中减速）
            //   -> 层3 回溯时间（一次性回血）+ 狂暴加速
            int maxHp = Stat(c.kind).hp;
            int layer = 3;
            if (c.hp > maxHp * 3 / 4) layer = 0;
            else if (c.hp > maxHp / 2) layer = 1;
            else if (c.hp > maxHp / 4) layer = 2;
            if ((int)c.phase != layer) {
                c.phase = (unsigned char)layer;
                GhostTeleportFx(c.x, c.y);                     // 层切换演出
            }
            // 层3：一次性时间回溯（回血 30%）—— mCd3 兼作"已回溯"锁（>=0 未回溯）
            if (layer == 3 && c.mCd3 >= 0.0f) {
                c.mCd3 = -1.0f;                                // 锁死，只回溯一次
                c.hp += maxHp * 3 / 10;
                if (c.hp > maxHp) c.hp = maxHp;
                BossFx(2);
            }
            // 层1+：空间位移（每 4 秒闪现到玩家近旁；计时由顶部通用计时递减）
            if (layer >= 1 && c.mCd1 <= 0.0f) {
                c.mCd1 = 4.0f;
                float a = FRnd01() * 6.2831853f;
                float nx = px + cosf(a) * 70.0f, ny = py + sinf(a) * 70.0f;
                GhostTeleportFx(c.x, c.y);
                c.x = nx; c.y = ny;
                GhostTeleportFx(c.x, c.y);
            }
            float spdMul = (layer >= 3) ? 1.5f : 1.0f;         // 层3 狂暴
            float sight = 280.0f;
            if (c.state == AState::Idle || c.state == AState::Wander) {
                if (c.state == AState::Idle) {
                    c.timer -= dt;
                    if (c.timer <= 0) { c.state = AState::Wander; c.timer = 1.2f + FRnd01() * 1.6f; RandDir(c.wx, c.wy); }
                } else {
                    c.timer -= dt;
                    MoveToward(c, w, c.x + c.wx * 10, c.y + c.wy * 10, st.wanderSpd, dt);
                    moving = true;
                    if (c.timer <= 0) { c.state = AState::Idle; c.timer = 1.0f + FRnd01() * 1.8f; }
                }
                if (!playerDead && c.triggered && distP < sight) { c.state = AState::Chase; c.timer = 0; }
            } else if (c.state == AState::Chase) {
                if (playerDead || distP > sight + 140.0f) { c.state = AState::Idle; c.timer = 1.0f; break; }
                if (distP < 120.0f) {
                    MoveToward(c, w, c.x + (c.x - px), c.y + (c.y - py), st.fleeSpd * spdMul, dt);
                } else if (distP > 190.0f) {
                    MoveToward(c, w, px, py, st.chaseSpd * spdMul, dt);
                }
                moving = true;
                c.timer += dt;
                if (distP < 240.0f && c.timer > 2.0f) { c.state = AState::Windup; c.timer = 0.60f; }
            } else if (c.state == AState::Windup) {
                c.timer -= dt;   // 红光弹蓄力
                if (c.timer <= 0) {
                    c.state = AState::Attack;
                    c.timer = 0.22f;
                    if (!playerDead && distP < 280.0f) {
                        float dx = px - c.x, dy = (py - 8.0f) - (c.y - 8.0f);
                        float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                        SpawnFireball(c.x, c.y - 8.0f, dx / l * 160.0f, dy / l * 160.0f);
                    }
                    c.timer = -2.0f;
                }
            } else if (c.state == AState::Attack) {
                c.timer -= dt;
                if (c.timer <= 0) { c.state = (distP < sight + 60.0f && !playerDead) ? AState::Chase : AState::Idle; c.timer = 0; }
            } else if (c.state == AState::Hurt) {
                c.timer -= dt;
                if (c.timer <= 0) { c.state = distP < sight ? AState::Chase : AState::Wander; c.timer = 0; }
            }
            break;
        }

        case CreatureKind::MimicAll:
            // 所有人：蛰伏时与幸存者完全同款的小幅游荡（无破绽）；触发后暴起追杀
            if (!c.triggered) {
                if (c.state == AState::Idle) {
                    c.timer -= dt;
                    if (c.timer <= 0) { c.state = AState::Wander; c.timer = 2.0f + FRnd01() * 3.0f; RandDir(c.wx, c.wy); }
                } else if (c.state == AState::Wander) {
                    c.timer -= dt;
                    MoveToward(c, w, c.x + c.wx * 10, c.y + c.wy * 10, 14.0f, dt);
                    moving = true;
                    if (c.timer <= 0) { c.state = AState::Idle; c.timer = 2.0f + FRnd01() * 4.0f; }
                }
            } else {
                MeleeFlow(c, w, px, py, pet, distP, 260.0f, dt, darkCtx, moving,
                          ally, tgtX, tgtY, distT);
            }
            break;

        case CreatureKind::Faceless: {
            // 无脸鬼（智慧篡改型）：
            //   蛰伏 = 伪装成常人小步游荡，同时向玩家缓慢漂移（潜伏接近——它想覆盖你的脸）
            //   触发 = 长前摇近战（把染血人脸慢慢盖上来），命中：遮屏 + 记忆篡改 + 窃取经验
            if (c.triggered) {
                const MeleeCfg& mc = Melee(c.kind);
                if (c.state == AState::Idle || c.state == AState::Wander) {
                    c.state = AState::Chase;                       // 触发即追
                } else if (c.state == AState::Chase) {
                    // 智慧型走位：玩家背对它时加速贴脸，正对它时绕行侧翼（不硬冲正面）
                    static const float FACE[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
                    float fx = FACE[pc.dir][0], fy = FACE[pc.dir][1];
                    float lx = c.x - px, ly = c.y - py;
                    float l = sqrtf(lx * lx + ly * ly); if (l < 0.001f) l = 1;
                    bool facing = (fx * -lx + fy * -ly) / l > 0.4f;    // 玩家正面对它
                    float spd = facing ? st.chaseSpd * 0.8f : st.chaseSpd * 1.3f;  // 背对时更快
                    if (facing && distP > 60.0f) {
                        // 正面时侧向包抄（垂直方向绕）
                        float nx = -ly / l, ny = lx / l;
                        MoveToward(c, w, c.x + nx * 40.0f + lx, c.y + ny * 40.0f + ly, spd, dt);
                    } else {
                        MoveToward(c, w, tgtX, tgtY, spd, dt);
                    }
                    moving = true;
                    if (distT < mc.atkRange && c.atkCd <= 0) { c.state = AState::Windup; c.timer = mc.windup; }
                } else if (c.state == AState::Windup) {
                    c.timer -= dt;   // 长前摇：它把那张人脸慢慢举起来
                    if (c.timer <= 0) {
                        c.state = AState::Attack; c.timer = 0.22f;
                        if (distT < mc.hitRange) {
                            float hx, hy;
                            AllyPos(pet, ally, px, py, hx, hy);
                            float dx = hx - c.x, dy = hy - c.y;
                            float ll = sqrtf(dx * dx + dy * dy); if (ll < 0.001f) ll = 1;
                            AllyHit(ally, mc.dmg * c.dmgMul, dx / ll, dy / ll);
                            if (ally < 0) {                          // 命中玩家：染血人脸覆盖
                                FaceCoverPlayer();                  // 遮屏窒息演出
                                MemoryHackPlayer();                  // 记忆篡改 + 窃取身份
                            }
                        }
                        c.atkCd = mc.cd;
                    }
                } else if (c.state == AState::Attack) {
                    c.timer -= dt;
                    if (c.timer <= 0) c.state = AState::Chase;
                } else if (c.state == AState::Hurt) {
                    c.timer -= dt;
                    if (c.timer <= 0) c.state = AState::Chase;
                }
            } else {
                // 蛰伏：与幸存者同款小步游荡 + 每秒向玩家漂移 ~10px（潜伏接近，玩家难以察觉）
                if (c.state == AState::Idle) {
                    c.timer -= dt;
                    if (c.timer <= 0) { c.state = AState::Wander; c.timer = 2.5f + FRnd01() * 3.0f; RandDir(c.wx, c.wy); }
                } else if (c.state == AState::Wander) {
                    c.timer -= dt;
                    MoveToward(c, w, c.x + c.wx * 10, c.y + c.wy * 10, 12.0f, dt);
                    moving = true;
                    if (c.timer <= 0) { c.state = AState::Idle; c.timer = 2.5f + FRnd01() * 3.5f; }
                }
                // 漂移潜伏：无脸鬼主动想贴近玩家（限 260px 视野外不追）
                if (distP < 260.0f && distP > 30.0f) {
                    float dx2 = px - c.x, dy2 = py - c.y;
                    float l2 = sqrtf(dx2 * dx2 + dy2 * dy2); if (l2 < 0.001f) l2 = 1;
                    Vector2 np = w.MoveCircle(c.x, c.y, st.radius, dx2 / l2 * 10.0f * dt, dy2 / l2 * 10.0f * dt);
                    c.x = np.x; c.y = np.y;
                }
            }
            // 吞食其他鬼：靠近同类鬼时吸取其生命（每 2 秒一次）——吞食厉鬼完善自身
            c.mCd2 -= dt;
            if (c.mCd2 <= 0.0f) {
                c.mCd2 = 2.0f;
                for (size_t oi = 0; oi < mobs.size(); oi++) {
                    if (oi == i) continue;
                    Creature& o = mobs[oi];
                    if (o.state == AState::Dead || !Hostile(o.kind)) continue;
                    if (o.kind == CreatureKind::Faceless) continue;
                    float dx = o.x - c.x, dy = o.y - c.y;
                    if (dx * dx + dy * dy > 40.0f * 40.0f) continue;
                    int steal = o.hp / 6 + 1;
                    o.hp -= steal;
                    c.hp += steal;
                    if (c.hp > Stat(c.kind).hp) c.hp = Stat(c.kind).hp;
                    SpawnHitParticles((c.x + o.x) / 2, (c.y + o.y) / 2 - 8, 160, 130, 220, 6);
                    if (o.hp <= 0) {                           // 吞噬致死
                        o.state = AState::Dead; o.deadFade = 0;
                        MobDied(o.kind, o.x, o.y, o.tier, o.dmgMul);
                    }
                    break;
                }
            }
            break;
        }

        case CreatureKind::GameGhost:
            // 游戏鬼：猜拳定生死。未触发/未输时永远站着不动（等玩家来猜拳）
            if (c.triggered) {
                MeleeFlow(c, w, px, py, pet, distP, 240.0f, dt, darkCtx, moving,
                          ally, tgtX, tgtY, distT);
            } else {
                if (FRnd01() < dt * 0.4f) c.dir = (int)(FRnd01() * 4.0f) % 4;
            }
            break;

        case CreatureKind::DealGhost:
            // 交易鬼：等价交换。永不攻击，永远站着（等玩家来交易）
            if (FRnd01() < dt * 0.4f) c.dir = (int)(FRnd01() * 4.0f) % 4;
            break;

        case CreatureKind::KnockGhost: {
            // 敲门鬼：飞行突袭（限时实体，由敲门事件生成）
            float sight = 320.0f;
            if (c.state == AState::Idle || c.state == AState::Wander) {
                c.state = AState::Chase;                       // 实体化即扑向玩家
            } else if (c.state == AState::Chase) {
                float wob = sinf(c.animT * 9.0f) * 50.0f;
                float dx = tgtX - c.x, dy = tgtY - c.y;
                float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                MoveFly(c, tgtX + (-dy / l) * wob, tgtY + (dx / l) * wob, st.chaseSpd, dt);
                moving = true;
                const MeleeCfg& mc = Melee(c.kind);
                if (distT < mc.atkRange && c.atkCd <= 0) { c.state = AState::Windup; c.timer = mc.windup; }
            } else if (c.state == AState::Windup) {
                c.timer -= dt;
                if (c.timer <= 0) {
                    c.state = AState::Attack; c.timer = 0.12f;
                    const MeleeCfg& mc = Melee(c.kind);
                    if (distT < mc.hitRange) {
                        float hx, hy;
                        AllyPos(pet, ally, px, py, hx, hy);
                        float dx = hx - c.x, dy = hy - c.y;
                        float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                        AllyHit(ally, mc.dmg * c.dmgMul, dx / l, dy / l);
                    }
                    c.atkCd = mc.cd;
                }
            } else if (c.state == AState::Attack) {
                c.timer -= dt;
                if (c.timer <= 0) c.state = AState::Chase;
            } else if (c.state == AState::Hurt) {
                c.timer -= dt;
                if (c.timer <= 0) c.state = AState::Chase;
            }
            (void)sight;
            break;
        }

        case CreatureKind::ManyFaces:
            // 多面人：慢速游荡；被盯上后高速追击，命中剥夺面容（main 侧诅咒叠加）
            MeleeFlow(c, w, px, py, pet, distP, 260.0f, dt, darkCtx, moving,
                      ally, tgtX, tgtY, distT);
            break;

        case CreatureKind::GhostDomain: {
            // 鬼游戏·核心：钉死原地（场景同化，区域即鬼）。触发时攻击近身者
            if (c.triggered) {
                const MeleeCfg& mc = Melee(c.kind);
                if (c.state == AState::Idle || c.state == AState::Wander) {
                    c.state = AState::Chase;
                } else if (c.state == AState::Chase) {
                    if (distT < mc.atkRange && c.atkCd <= 0) { c.state = AState::Windup; c.timer = mc.windup; }
                } else if (c.state == AState::Windup) {
                    c.timer -= dt;
                    if (c.timer <= 0) {
                        c.state = AState::Attack; c.timer = 0.30f;
                        BossFx(2);                              // 震地波
                        if (distT < mc.hitRange) {
                            float hx, hy;
                            AllyPos(pet, ally, px, py, hx, hy);
                            float dx = hx - c.x, dy = hy - c.y;
                            float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
                            AllyHit(ally, mc.dmg * c.dmgMul, dx / l, dy / l);
                        }
                        c.atkCd = mc.cd;
                    }
                } else if (c.state == AState::Attack) {
                    c.timer -= dt;
                    if (c.timer <= 0) c.state = AState::Chase;
                } else if (c.state == AState::Hurt) {
                    c.timer -= dt;
                    if (c.timer <= 0) c.state = AState::Chase;
                }
            }
            break;
        }

        case CreatureKind::SmithGhost:
            // 铁匠鬼：驻守废弃铁匠铺，重甲缓步反击（收服可解锁鬼界合成）
            MeleeFlow(c, w, px, py, pet, distP, 160.0f, dt, darkCtx, moving,
                      ally, tgtX, tgtY, distT);
            break;
        default: break;
        } // switch kind

        // 被动生物受击恢复（Hurt 结束 -> Flee）
        if ((c.kind == CreatureKind::Rabbit || c.kind == CreatureKind::Deer)
            && c.state == AState::Hurt) {
            c.timer -= dt;
            if (c.timer <= 0) { c.state = AState::Flee; c.timer = 2.5f; }
        }

        // ---- 拆墙：玩家的木墙挡住去路时，已触发的鬼改为拆墙 ----
        if (Hostile(c.kind) && c.triggered && c.state == AState::Chase &&
            c.atkCd <= 0 && distP > 40.0f) {
            int wi = -1;
            float wd = 44.0f * 44.0f;
            for (size_t oi = 0; oi < w.objs.size(); oi++) {
                const WorldObj& o = w.objs[oi];
                if (o.kind != ObjKind::Wall || o.hp <= 0) continue;
                float wdx = o.tx * 16.0f + 8.0f - c.x, wdy = o.ty * 16.0f + 8.0f - c.y;
                float q = wdx * wdx + wdy * wdy;
                if (q < wd) { wd = q; wi = (int)oi; }
            }
            if (wi >= 0) {
                WorldObj& o = w.objs[(size_t)wi];
                if (o.harvested) {
                    o.harvested = false;                    // 封门符替墙受一次
                } else {
                    o.hp--;
                    o.shake = 0.22f;
                    if (o.hp <= 0)
                        w.objAt[(size_t)o.ty * MAP_W + o.tx] = -1;
                }
                c.atkCd = 1.3f;
                c.state = AState::Attack;
                c.timer = 0.16f;
            }
        }

        // ---- 记忆错乱（无脸鬼鬼仆撕咬所致）：错乱中它忘了要杀谁 ----
        // 计时递减；错乱期间强制脱离敌对状态（不追不打，原地打转），但仇恨不丢（错乱结束继续追）
        // 集中 gate 在各 AI 分支之后：无论哪个分支转入敌对，错乱中一律打回游荡
        if (c.confuseT > 0.0f) {
            c.confuseT -= dt;
            if (c.state == AState::Chase || c.state == AState::Windup || c.state == AState::Attack) {
                c.state = AState::Wander;
                c.timer = 0.8f;
                RandDir(c.wx, c.wy);
            }
        }

        // ---- 五符作用态：黄符降级 / 仇符互斗 ----
        if (c.lowerT > 0.0f) c.lowerT -= dt;
        if (c.fightT > 0.0f) {
            c.fightT -= dt;
            bool foeOk = false;
            if (c.fightWith >= 0 && c.fightWith < (int)mobs.size()
                && c.fightWith != (int)i) {
                Creature& o = mobs[(size_t)c.fightWith];
                // 双向校验：对方也必须仍在与我互斗（mobs 池会复用下标，防错配）
                foeOk = (o.state != AState::Dead && o.fightT > 0.0f && o.fightWith == (int)i);
                if (foeOk) {
                    // 互殴节拍：打到对方剩 1 血为止（鬼杀不死，两败俱伤后是收容窗口）
                    c.fightHitT -= dt;
                    if (c.fightHitT <= 0.0f) {
                        c.fightHitT = 0.9f;
                        if (o.hp > 1) {
                            o.hp -= 2 + (int)(FRnd01() * 3.0f);
                            if (o.hp < 1) o.hp = 1;
                            o.hurtFlash = 0.22f;
                            SpawnHitParticles(o.x, o.y - 10, 200, 80, 70, 8);
                        }
                    }
                    // 扑向对方（借游荡方向逼近）
                    float ddx = o.x - c.x, ddy = o.y - c.y;
                    float dl = sqrtf(ddx * ddx + ddy * ddy);
                    if (dl > 0.001f) { c.wx = ddx / dl; c.wy = ddy / dl; }
                }
            }
            if (!foeOk) { c.fightT = 0.0f; c.fightWith = -1; }
            // 互斗中眼里没有玩家：一律打回游荡（游荡分支会按 wx/wy 扑向对方）
            if (c.state == AState::Chase || c.state == AState::Windup || c.state == AState::Attack) {
                c.state = AState::Wander;
                c.timer = 0.4f;
            }
        }

        // ---- 规则 gate：未触发的鬼不得进入敌对状态（集中一处，防漏）----
        if (Hostile(c.kind) && !c.triggered &&
            (c.state == AState::Chase || c.state == AState::Windup || c.state == AState::Attack)) {
            c.state = AState::Wander;
            c.timer = 1.2f;
            RandDir(c.wx, c.wy);
        }

        if (moving) c.animT += dt * 0.7f;
        i++;
    }

    // ---- 鬼种·成年 召唤鬼奴落地（循环外补怪）----
    while (ghostSummon > 0) {
        ghostSummon--;
        for (int tryN = 0; tryN < 24; tryN++) {
            float a = FRnd01() * 6.283185f;
            float sx2 = ghostSummonX + cosf(a) * (28.0f + FRnd01() * 34.0f);
            float sy2 = ghostSummonY + sinf(a) * (28.0f + FRnd01() * 34.0f);
            if (!w.CircleFree(sx2, sy2, 6.0f)) continue;
            Creature m;
            m.kind = CreatureKind::GhostChild; m.x = sx2; m.y = sy2;
            m.hp = 4;                                          // 鬼奴：弱化鬼童
            m.state = AState::Chase;
            m.triggered = true;                                // 已被驱使
            m.dmgMul = 0.8f;
            m.lifeT = 20.0f;                                   // 鬼奴 20 秒后自行消散
            mobs.push_back(m);
            break;
        }
    }

    // ---- 数量维持：动物补充 + 5% 游荡遇鬼 + 铁匠鬼重生 ----
    sMaintCheck += dt;
    if (sMaintCheck > 4.0f) {
        sMaintCheck = 0;
        // 动物维持
        int nr = 0, nd = 0;
        for (const Creature& c : mobs)
            if (c.state != AState::Dead) {
                if (c.kind == CreatureKind::Rabbit) nr++;
                else if (c.kind == CreatureKind::Deer) nd++;
            }
        if (nr < 12) SpawnOne(mobs, w, CreatureKind::Rabbit, px, py, 240);
        if (nd < 8)  SpawnOne(mobs, w, CreatureKind::Deer,   px, py, 260);

        // ---- 5% 遇鬼：每 30 秒掷一次骰（场上游荡鬼 ≤2 时）----
        static float sGhostRoll = 0.0f;
        sGhostRoll += 4.0f;                                    // 每次检查累计 4 秒
        if (sGhostRoll >= 30.0f) {
            sGhostRoll = 0.0f;
            int nWander = 0;
            for (const Creature& c : mobs)
                if (c.state != AState::Dead && Hostile(c.kind) && c.kind != CreatureKind::SmithGhost)
                    nWander++;
            if (nWander <= 2 && FRnd01() < 0.05f) {
                // 5% 概率：远处游来一只随机规则鬼（游荡鬼上限自然受控）
                CreatureKind k = RollWanderGhost();
                for (int tryN = 0; tryN < 30; tryN++) {
                    float x = FRnd01() * (MAP_W * TILE - 64) + 32;
                    float y = FRnd01() * (MAP_H * TILE - 64) + 32;
                    float dx = x - px, dy = y - py;
                    float d = sqrtf(dx * dx + dy * dy);
                    if (d < 420.0f || d > 700.0f) continue;    // 视野外生成（不惊扰玩家）
                    if (!w.CircleFree(x, y, 8.0f)) continue;
                    Creature c;
                    c.kind = k; c.x = x; c.y = y; c.hp = Stat(k).hp;
                    c.state = AState::Idle; c.timer = FRnd01() * 2.0f;
                    c.tier = (FRnd01() < 0.3f) ? 1 : 0;
                    c.dmgMul = 1.0f + 0.1f * c.tier;
                    mobs.push_back(c);
                    break;
                }
            }
        }

        // ---- 铁匠鬼重生：死亡 3 游戏日后回到废弃铁匠铺 ----
        if (sSmithDeadDay != -999 && curDay - sSmithDeadDay >= 3) {
            sSmithDeadDay = -999;
            if (w.smithGhostPos.x > 0.0f) {
                Creature c;
                c.kind = CreatureKind::SmithGhost;
                c.x = w.smithGhostPos.x; c.y = w.smithGhostPos.y;
                c.hp = Stat(CreatureKind::SmithGhost).hp;
                c.state = AState::Idle;
                mobs.push_back(c);
            }
        }
    }
}

// 铁匠鬼死亡登记（main.cpp 击杀时调用：3 日后重生）
void NoteSmithGhostDied(float gameTime) {
    // 记录在匿名 namespace 的静态里（本函数与 UpdateCreatures 同一翻译单元）
    sSmithDeadDay = (int)(gameTime / DAY_LEN);
}
