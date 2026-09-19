#pragma once
#include "raylib.h"
#include <vector>

class World;

// ============================================================
// 生物模块（神秘复苏 · 十大规则鬼）
// 只保留 2 种凡兽 + 12 种规则鬼：
//   鬼不会无缘无故杀人 —— 除非你触碰了它的规则。
//   每种鬼有专属触发规则，未触发时蛰伏无害。
// 统一状态机：idle/wander/flee/chase/windup/attack/hurt/dead
// ============================================================

enum class CreatureKind : unsigned char {
    // ---- 凡兽（非鬼：可驯服、可猎杀取肉）----
    Rabbit, Deer,
    // ---- 十大规则鬼 ----
    GhostChild,    // 1.鬼种·鬼童（S级·成长进化型）：吞魂进化，打它才反击
    GhostTeen,     // 1.鬼种·少年（进化第二形态）：快速近战
    GhostAdult,    // 1.鬼种·成年（最终形态）：转化鬼奴（召唤）
    LoneGhost,     // 2.单鬼（S级·概念压制型）：落单必死 + 时间重启
    NineFace,      // 3.9面鬼（十层鬼域·时空系）：掉血逐层解锁权能
    MimicAll,      // 4.所有人（模仿性鬼）：蛰伏时与幸存者完全同款（无破绽），贴脸交谈即触发
    Faceless,      // 5.无脸鬼（智慧篡改型）：蛰伏时伪装常人潜伏接近；命中以染血人脸覆盖玩家——篡改记忆（假血条）+窃取身份（偷经验），吞食其他厉鬼完善自身
    GameGhost,     // 6.游戏鬼（规则博弈型）：猜拳，输/平局才杀人
    DealGhost,     // 7.交易鬼（交易许愿型）：等价交换，永不主动攻击
    KnockGhost,    // 8.敲门鬼（声源传播型）：入夜屋内被盯上，实体化来袭
    ManyFaces,     // 9.多面人（身份剥夺型）：盯上你，命中剥夺面容
    GhostDomain,   // 10.鬼游戏（场景同化型）：鬼域核心，区域即鬼
    // ---- 特殊 NPC 鬼 ----
    SmithGhost,    // 铁匠鬼：驻守废弃铁匠铺，收服后解锁鬼界合成（3级工作台）
};

enum class AState : unsigned char {
    Idle, Wander, Flee, Chase, Windup, Attack, Hurt, Dead
};

struct Creature {
    CreatureKind kind = CreatureKind::Rabbit;
    AState state = AState::Idle;
    float x = 0, y = 0;
    float kvx = 0, kvy = 0;      // 击退速度（衰减）
    float wx = 0, wy = 0;        // 游荡方向
    int hp = 2;
    float timer = 0;             // 状态剩余时间
    float animT = 0;             // 动画累计
    int dir = 0;                 // 朝向 0下 1上 2右 3左
    float hurtFlash = 0;         // 白闪计时
    float deadFade = 0;          // 死亡淡出（3 秒移除）
    float atkCd = 0;             // 攻击冷却
    float hopT = 0;              // 兔子蹦跳相位
    unsigned char tier = 0;      // 强度层级（0..2，鬼仆复刻/收服难度基准）
    float dmgMul = 1.0f;         // 强度系数（生成时按天数确定）
    float dashT = 0;             // 突进剩余时间

    // ---- 招式（鬼种·成年 召唤 / 9面鬼 权能）----
    unsigned char move = 0;      // 招式 0=冲锋 1=召唤 2=震地 3=普攻
    float chargeVx = 0, chargeVy = 0;   // 冲锋方向（前摇锁定）
    float mCd1 = 0, mCd2 = 0, mCd3 = 0; // 冲锋/召唤/震地 冷却

    // ---- 规则系统 ----
    bool triggered = false;              // 规律是否已被玩家触发
    unsigned short ruleMask = 0;         // 规律位掩码（标记显示用）
    unsigned char  tintSeed = 0;         // 外观随机种子（色块位置与色调）
    bool marked = false;                 // 玩家是否标记了它（常亮规律 + 朱砂标记符）
    bool wailer = false;                 // 声音系：定期发声，听到即被它盯上
    float wailT = 0;                     // 距下一次发声的倒计时
    unsigned char phase = 0;             // 鬼种：已吞魂数；9面鬼：当前权能层；单鬼：已重启次数
    float ruleT = 0;                     // 规则计时（单鬼落单 / 无脸潜伏贴近 / 多面人盯上）
    float lifeT = -1.0f;                 // 限时存在（敲门鬼实体；<=0 永久）
    float confuseT = 0;                  // 记忆错乱（无脸鬼鬼仆撕咬触发）：错乱中忘了要杀玩家，停止敌对
    // ---- 五符作用态 ----
    float lowerT = 0;                    // 黄符降级剩余秒数（>0 时等级按 tier-1 算：可被低一级器物收容）
    float fightT = 0;                    // 仇符互斗剩余秒数（>0 时与 fightWith 互殴，不追玩家）
    int   fightWith = -1;                // 互斗对象（mobs 下标）
    float fightHitT = 0;                 // 互斗出手节拍
    float dotT = 0, dotDps = 0;          // 持续失血（鬼仆「鬼域」技能；由主程序逐帧结算）
    // ---- 伴生鬼（铁匠鬼）：跟随玩家悬浮，永不敌对、永不消散 ----
    bool  companion = false;             // true = 伴生随行（跳过一切敌对/触发逻辑）
    float hoverT = 0;                    // 悬浮起伏相位（纯渲染用）
    // ---- 无面鬼·混入同伴（同化渗透）----
    bool  infiltrated = false;           // 已混入同伴队伍：伪装成随行人，永不敌对、永不露形
    float infilT = 0;                    // 蛰伏时贴近玩家的累计秒数（够久即自行混入）
    float convertT = 0;                  // 距下一次同化身边同伴的倒计时
    // ---- 联机同步：快照用的稳定网络 ID（单机恒 -1；由房主按生成序分配）----
    int   netId = -1;
    float netTx = 0, netTy = 0;          // 客人：快照目标位置（本地朝它平滑插值）
};

// 玩家状态上下文（规则判定用：鬼只对"符合规律"的玩家起反应）
struct PlayerCtx {
    float hpPct = 1.0f;          // 生命百分比 0..1
    bool  running = false;       // 是否在奔跑（Shift）
    bool  moving = false;        // 是否在移动
    int   dir = 0;               // 朝向 0下 1上 2右 3左
    bool  nearWater = false;     // 是否贴近水域
    bool  usedItem = false;      // 本帧是否使用了道具（进食/药剂/合成）
    bool  mining = false;        // 本帧是否在动土（砍/挖/采）
    bool  hiding = false;        // 隐身符生效中：期间不会触发任何规律
    bool  alone = true;          // 玩家是否落单（120px 内无宠物/鬼仆/幸存者）
    bool  hasAlly = false;       // 玩家身边已有同伴（无面鬼据此混入队伍：空手一个人不值得它伪装）
    float slowK = 1.0f;          // 减速系数（被 9面鬼 现实修改 / 多面人 面容剥夺）
};

// 联机多目标条目：每个可成为鬼的猎杀目标的玩家
struct PlayerTarget {
    float x = 0, y = 0;
    bool  dead = false;
    PlayerCtx pc;
};

// 查询某只鬼的名字与"杀人规律"文案（UI 提示用）
const char* GhostName(CreatureKind k);
const char* GhostRuleText(CreatureKind k);
// 鬼的威胁评级（S/A/B，显示在鬼条上）
const char* GhostRank(CreatureKind k);
// 按规律位掩码取文案（标记显示用）
const char* GhostRuleTextMask(unsigned short m);
// 生成时给一只鬼随机外观种子 + 声音系判定
void RollGhostRule(Creature& c);

// ============================================================
// 规律位（保留给标记显示 / 敲门鬼事件内部分类用）
// ============================================================
enum GhostRule : unsigned short {
    RULE_NIGHT   = 1 << 0,    // 入夜方行：夜晚才敌对
    RULE_RUN     = 1 << 1,    // 逐快不逐慢：玩家奔跑时
    RULE_BLOOD   = 1 << 2,    // 嗅血而来：玩家残血 / 血月
    RULE_NEAR    = 1 << 3,    // 近身方扑：距离极近
    RULE_WEB     = 1 << 4,    // 触网即至：踏入其网
    RULE_GRAVE   = 1 << 5,    // 擅入者死：擅闯坟地
    RULE_MOVE    = 1 << 6,    // 见动而袭：玩家移动时
    RULE_HITBACK = 1 << 7,    // 犯之则反：挨打后
    RULE_BACK    = 1 << 8,    // 莫要回头：玩家背对它
    RULE_WATER   = 1 << 9,    // 近水现身：玩家贴近水域
    RULE_ITEM    = 1 << 10,   // 窥人用物：玩家使用道具
    RULE_ALWAYS  = 1 << 11,   // 见即杀：无规律（最凶）
};

// 盟友信息（creature.cpp 只读，用于仇恨吸引目标选择：宠物 + 鬼仆）
struct PetInfo {
    bool on = false;             // 是否有宠物存活
    float x = 0, y = 0;          // 宠物位置
    int   ghostN = 0;            // 鬼仆数量 0..3
    float ghostX[3] = {};        // 鬼仆位置
    float ghostY[3] = {};
};

// 事件通知（由 main.cpp 实现）
void MobHitPlayer(float dmg, float dirX, float dirY);          // 怪物命中玩家
void WailHeard(float x, float y);                              // 声音系鬼发声（听到即被盯上）
void AllyGhostHit(int idx, float dmg, float dirX, float dirY); // 怪物命中鬼仆（收鬼系统）
void MobDied(CreatureKind kind, float x, float y,
             unsigned char tier = 0, float mul = 1.0f);        // 生物死亡（掉落+击杀数+残魂）
void SpawnHitParticles(float x, float y, unsigned char r, unsigned char g, unsigned char b, int n);
void SpawnSpit(float x, float y, float vx, float vy);          // 远程魂息（main.cpp 实现投射物池）
void SpawnFireball(float x, float y, float vx, float vy);      // 9面鬼 红光弹（复用投射物池）
void MobPoisonPlayer(float dps);                               // 中毒 DoT（main.cpp 实现）
void PetHit(float dmg, float dirX, float dirY);                // 怪物命中宠物（main.cpp 实现）
void BossFx(int kind);                                         // 招式特效（0冲锋啸叫 1召唤 2震地）
void GhostTeleportFx(float x, float y);                        // 9面鬼 空间位移特效（main.cpp 演出）
void FaceCoverPlayer();                                        // 无脸鬼命中：染血人脸覆盖（遮屏窒息演出，main.cpp）
void MemoryHackPlayer();                                       // 无脸鬼命中：记忆篡改（假血条）+ 窃取身份（偷经验）
// 单鬼「时间重启」：第一次被击杀时原地满血复活（返回 true = 已重启，本帧不算击杀）
bool  MobPreDeath(Creature& c);
// 铁匠鬼死亡登记（3 游戏日后重生）
void NoteSmithGhostDied(float gameTime);

// 初始散布生物（动物 + 固定 POI 鬼由 main.cpp 补充放置）
void InitCreatures(std::vector<Creature>& mobs, const World& w, float px, float py, float gameTime);
// 每帧更新全部生物 AI（规则触发 gate + 各鬼专属行为）
void UpdateCreatures(std::vector<Creature>& mobs, World& w, float px, float py,
                     bool playerDead, float gameTime, float dt, const PetInfo& pet,
                     const PlayerCtx& pc);
// 联机多目标版：每只鬼在更新开始时挑"最近的存活玩家"作为自己的目标。
// tgts[0] 恒为房主/单机玩家；伴生鬼与混入同伴永远跟随 tgts[0]。
void UpdateCreaturesT(std::vector<Creature>& mobs, World& w,
                      const PlayerTarget* tgts, int nTgts,
                      float gameTime, float dt, const PetInfo& pet);
// 当前正在更新的那只鬼选中的目标下标（-1 无 / 0=房主 / >0=联机同伴），伤害路由用
int  CreatureTargetIndex();
// 重开新局时清零模块内的跨帧维持状态
void ResetCreatureSpawnState();

// ---- 属性查询导出（收鬼系统：鬼仆属性按原怪复刻）----
int   MobBaseHp(CreatureKind k);      // 基础生命
float MobMeleeDmg(CreatureKind k);    // 近战伤害
float MobChaseSpd(CreatureKind k);    // 追击速度
float MobRadius(CreatureKind k);      // 碰撞半径
bool  MobFlying(CreatureKind k);      // 飞行种（无视地形）
bool  MobHostile(CreatureKind k);     // 敌对种（可收魂）
// 不死单位：不可被攻击（任何来源都不掉血）且不会死亡（铁匠鬼）
bool  MobUndying(CreatureKind k);
// 选中/警戒半径：玩家进入该圈即被这只鬼盯上（UI 直接以鬼为圆心画圈显示）
float MobSightRadius(CreatureKind k);
