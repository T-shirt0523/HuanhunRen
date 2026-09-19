// ============================================================
//  progress.hpp —— 元系统：命途 / 成就 / 配方图鉴 / 残影剧情
//
//  设计原则（不做成烂大街的天赋树 + 数值膨胀）：
//    · 命途不给「伤害 +10%」，只给「看待与处置鬼的方式」——
//      观(规律) / 伏(符咒) / 驭(傀儡) / 营(领地)，各自改写一条既有机制。
//    · 成就奖励「悟点」，与命途同源；成就是玩法的旁证，不是数值来源。
//    · 配方不是列表，是图鉴：未悟得的显示「？？？」，靠行为解锁。
//    · 剧情不做强制任务链：残影散落各处，捡到才推一章，不捡也能通关。
//
//  本文件只做数据与纯逻辑，不碰渲染、不碰 raylib；
//  对外通过事件队列（ProgEvent）把「该弹什么」交回 main 处理。
// ============================================================
#pragma once
#include <cstdint>
#include <type_traits>

// ---------------- 命途（四道）----------------
enum PathKind : unsigned char {
    PATH_SEE = 0,   // 观：看破规律
    PATH_BIND,      // 伏：符咒
    PATH_RIDE,      // 驭：傀儡 / 鬼仆
    PATH_HOME,      // 营：领地 / 生计
    PATH_N
};
constexpr int PATH_MAX_LV    = 5;    // 每条命途最高 5 级
constexpr int PATH_COST_BASE = 1;    // 第 n 级耗悟点 = n（1+2+3+4+5=15 点满一条）

// ---------------- 成就 ----------------
enum AchvTier : unsigned char { TIER_FAN = 0, TIER_QI, TIER_MI, TIER_JUE, TIER_N };
constexpr int ACHV_N = 30;
// 成就奖励悟点：凡 0 / 奇 1 / 秘 2 / 绝 3
constexpr int TIER_INSIGHT[4] = { 0, 1, 2, 3 };

// ---------------- 配方图鉴 ----------------
constexpr int RECIPE_N = 23;         // 15 原有 + 8 新增

// ---------------- 残影剧情 ----------------
constexpr int SHARD_N    = 12;       // 残影总数
constexpr int CHAPTER_N  = 7;        // 章节数（含序章与终章）
constexpr int ENDING_N   = 3;        // 三种结局

// ---------------- 事件（main 每帧取走并表现）----------------
enum ProgEv : unsigned char {
    PEV_NONE = 0,
    PEV_ACHV,        // a=成就下标
    PEV_RECIPE,      // a=配方下标（悟得新配方）
    PEV_SHARD,       // a=残影编号（拾取）
    PEV_CHAPTER,     // a=章节号（推进）
    PEV_LEVELUP,     // a=新等级（升级；b=本次获得悟点）
    PEV_ENDING,      // a=结局号 1/2/3
    PEV_INSIGHT,     // a=获得悟点数
};
struct ProgEvent { unsigned char t; int a; int b; };
constexpr int PROG_EVQ = 24;

// ---------------- 每帧由 main 填入的快照 ----------------
struct ProgStat {
    int   level = 1;
    int   kills = 0, ghostCaught = 0, deaths = 0;
    int   day = 1;                 // 存活天数
    float nightOut = 0;            // 夜游累计秒
    int   nTree = 0, nRock = 0, nBerry = 0, nEat = 0, nFire = 0, nChest = 0, nGrave = 0;
    int   wood = 0, stone = 0, iron = 0, crystal = 0, shard = 0, heart = 0;
    int   campLv = 0;              // 0=未立碑
    int   allyN = 0;               // 名下同伴数
    int   facelessN = 0;           // 身边无面鬼数
    int   banished = 0;            // 揪出并赶走的无面鬼数
    int   toolN = 0;               // 已拥有的工具件数
    int   armorLv = 0, captureLv = 0, swordLv = 0, gourdLv = 0;
    int   smithLv = 1;
    int   ruinFound = 0;
    int   puppetMax = 0;           // 曾同时操控鬼仆的最大数
    bool  bossSlain = false, domainPurged = false, smithOwned = false;
    bool  boat = false, nearDeath = false, stalkSurvived = false;
    bool  bowKill = false;         // 用猎弓击杀过
    int   hp = 100, maxHp = 100;
    bool  dead = false;
};

// ---------------- 存档数据（全部 POD，顺序读写）----------------
// 警告：本结构会被 fwrite/fread 整块读写，严禁加入 std::string / std::vector /
// 虚函数 / 非平凡构造，否则跨版本存档会读成乱码。下面 static_assert 兜底。
constexpr unsigned int PROG_MAGIC  = 0x50574C53u;  // "PWLS"
constexpr int          PROG_VERSION = 1;

struct ProgSave {
    unsigned int magic = PROG_MAGIC;      // 必须排在最前，用于识别存档合法性
    int          version = PROG_VERSION;
    int          insight = 0;
    unsigned char pathLv[PATH_N] = {};
    unsigned char achvGot[8] = {};        // 64 位
    unsigned char recipeGot[4] = {};      // 32 位
    unsigned char shardBits[2] = {};      // 16 位
    unsigned char chapter = 0;
    unsigned char ending = 0;
    unsigned char wipeCount = 0;
    int          insightSpent = 0;        // 累计花掉的悟点（洗点成本用）
    int          achvProg[ACHV_N] = {};   // 进度缓存（UI 显示用，可重算）
};
static_assert(std::is_trivially_copyable_v<ProgSave>,
              "ProgSave 必须是可平凡拷贝的 POD，否则 fwrite/fread 存档会坏");

// ---------------- 对外接口 ----------------
namespace Prog {

void  Reset();                              // NewGame：全清
void  Load(const ProgSave& s);
const ProgSave& Save();

// 事件队列
bool  Pop(ProgEvent* out);
void  Push(unsigned char t, int a, int b);

// --- 命途 ---
int   Insight();                            // 当前可用悟点
int   PathLv(int k);
const char* PathName(int k);                // 单字：观/伏/驭/营
const char* PathFullName(int k);            // 观微之道…
const char* PathLine(int k, int lv);        // 第 lv 级的效果描述（1-based）
void  GainInsight(int n);                   // 直接加悟点（升级等；不走事件，避免丢点）
bool  Invest(int k);                        // 投 1 点（自动扣本级所需悟点）
bool  Wipe(int* costOut);                   // 洗点（成本随次数递增），返回是否可洗
int   WipeCost();
int   InsightSpent();

// --- 每帧推进：传入快照，内部解锁成就/配方/残影/章节 ---
void  Tick(const ProgStat& st, float dt);

// --- 成就 ---
bool  AchvGot(int i);
const char* AchvName(int i);
const char* AchvDesc(int i);
int   AchvTierOf(int i);
void  AchvProgress(int i, const ProgStat& st, int* cur, int* need);  // UI 进度
int   AchvCount();

// --- 配方图鉴 ---
bool  RecipeKnown(int i);
void  RecipeUnlock(int i);
const char* RecipeLore(int i);              // 悟得条件的一句话
const char* RecipeName(int i);              // 配方名（图鉴未解锁时 UI 显示 ？？？）

// --- 残影剧情 ---
bool  ShardGot(int i);
int   ShardCount();
bool  GiveShard(int i);                     // 拾取（重复拾取返回 false）
int   Chapter();
const char* ChapterTitle(int c);
const char* ChapterText(int c);             // 章节正文（多段，\n 分隔）
bool  ChapterSeen(int c);
void  MarkChapterSeen(int c);
int   Ending();
void  TriggerEnding(int e);                 // e=1/2/3
const char* EndingTitle(int e);
const char* EndingText(int e);

}  // namespace Prog
