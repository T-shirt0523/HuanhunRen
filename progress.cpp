// ============================================================
//  progress.cpp —— 命途 / 成就 / 配方图鉴 / 残影剧情（纯逻辑）
//
//  不依赖 raylib、不依赖 main.cpp 的任何类型。
//  main 每帧把状态打包成 ProgStat 喂进来，本模块解锁该解锁的东西，
//  把「该表现什么」丢进事件队列；main 取队列去播音效/飘字/卷轴。
// ============================================================
#include "progress.hpp"
#include "l10n.h"
#include <cstring>
#include <cstdio>

// ---------------- 内部状态 ----------------
static ProgSave  gS;
static ProgEvent gEvQ[PROG_EVQ];
static int       gEvHead = 0, gEvTail = 0;
static float     gTickT = 0;

// 章节是否已看过（不存档也无所谓，重进会重播一次；但存了更贴合直觉）
static bool gChapterSeen[CHAPTER_N];

namespace Prog {

// ---------------- 事件队列 ----------------
bool Pop(ProgEvent* out) {
    if (gEvHead == gEvTail) return false;
    *out = gEvQ[gEvHead];
    gEvHead = (gEvHead + 1) % PROG_EVQ;
    return true;
}
void Push(unsigned char t, int a, int b) {
    int nxt = (gEvTail + 1) % PROG_EVQ;
    if (nxt == gEvHead) return;                 // 队列满：丢弃（表现层丢一帧无所谓）
    gEvQ[gEvTail].t = t; gEvQ[gEvTail].a = a; gEvQ[gEvTail].b = b;
    gEvTail = nxt;
}

// ---------------- 位工具 ----------------
// 自带下标保护：越界一律当作「未置位 / 不操作」，绝不越界读写。
// 调用方虽已校验，这里再兜一层，防以后有人绕开校验直接调。
static inline bool BitGet(const unsigned char* arr, int nbyte, int i) {
    if (i < 0) return false;
    int b = i >> 3;
    if (b < 0 || b >= nbyte) return false;
    return (arr[b] >> (i & 7)) & 1;
}
static inline void BitSet(unsigned char* arr, int nbyte, int i) {
    if (i < 0) return;
    int b = i >> 3;
    if (b < 0 || b >= nbyte) return;
    arr[b] |= (unsigned char)(1u << (i & 7));
}

// ============================================================
//  命途
// ============================================================
static const char* PATH_NAME[PATH_N] = { "观", "伏", "驭", "营" };
static const char* PATH_FULL[PATH_N] = { "观微之道", "伏鬼之道", "驭鬼之道", "营寨之道" };

// 命途每级的具体效果（手册里逐行显示——写清楚"改变了什么玩法"，而不是"+10%"）
static const char* PATH_LINE[PATH_N][PATH_MAX_LV] = {
    // 观：看破规律
    { "鬼的规律二字可见距离 +70",
      "Q 可同时标记 2 只鬼",
      "领地再高，无面鬼仍留「脸是空的」这一丝破绽",
      "每日问询次数 +1",
      "鬼将触发规律前 1.5 秒，头顶朱砂预警" },
    // 伏：符咒
    { "每种符咒上限 8 -> 10",
      "全部符咒冷却 -15%",
      "符咒升级所需次数 3 -> 2",
      "贴符距离 +24",
      "黄符额外使鬼降 1 级" },
    // 驭：傀儡 / 鬼仆
    { "鬼仆阴气消耗 -20%（续航更久）",
      "傀儡同控上限 +1",
      "操控傀儡时本体可以半速移动",
      "鬼仆伤害 +25%",
      "收鬼成功率 +10%，收服的鬼自带 1 级" },
    // 营：领地 / 生计
    { "木/石/莓每次采集 +1",
      "饥饿衰减 -25%",
      "领地收容上限 +1",
      "睡眠回血 40->70，被袭概率 35%->20%",
      "领地镇宅光环增强，Lv1 起即可扰散追猎" },
};

const char* PathName(int k)  { return (k >= 0 && k < PATH_N) ? PATH_NAME[k] : ""; }
const char* PathFullName(int k) { return (k >= 0 && k < PATH_N) ? PATH_FULL[k] : ""; }
const char* PathLine(int k, int lv) {
    if (k < 0 || k >= PATH_N) return "";
    if (lv < 1 || lv > PATH_MAX_LV) return "";
    return PATH_LINE[k][lv - 1];
}

void GainInsight(int n) {
    if (n > 0) {
        gS.insight += n;
        if (gS.insight < 0) gS.insight = 0x7FFFFFFF;   // 极端累加溢出兜底
    }
}
int  Insight()      { return gS.insight; }
int  PathLv(int k)  { return (k >= 0 && k < PATH_N) ? (int)gS.pathLv[k] : 0; }
int  InsightSpent() { return gS.insightSpent; }

// 第 n 级（0-based）需要的悟点
static int PathCostAt(int lv) { return lv + 1; }

bool Invest(int k) {
    if (k < 0 || k >= PATH_N) return false;
    int lv = gS.pathLv[k];
    if (lv >= PATH_MAX_LV) return false;
    int cost = PathCostAt(lv);
    if (gS.insight < cost) return false;
    gS.insight -= cost;
    gS.insightSpent += cost;
    gS.pathLv[k] = (unsigned char)(lv + 1);
    return true;
}

int WipeCost() { return 10 * (1 + (int)gS.wipeCount); }   // 木 x10 / x20 / x30...

bool Wipe(int* costOut) {
    int spent = gS.insightSpent;
    if (spent <= 0) return false;
    if (costOut) *costOut = WipeCost();
    gS.insight += spent;
    gS.insightSpent = 0;
    for (int i = 0; i < PATH_N; i++) gS.pathLv[i] = 0;
    if (gS.wipeCount < 255) gS.wipeCount++;
    return true;
}

// ============================================================
//  成就（30 个，四品级）
// ============================================================
struct AchvDef { const char* name; const char* desc; unsigned char tier; };
static const AchvDef ACHV[ACHV_N] = {
    // ---- 凡：入门（0 悟）----
    { "撸树一时爽",   "一直撸树一直爽，万物始于树",       TIER_FAN },
    { "干饭人",       "干饭不积极，思想有问题",           TIER_FAN },
    { "火的力量",     "点亮第一堆篝火，文明 +1",          TIER_FAN },
    { "初见阴物",     "第一次看清一只鬼的规律",           TIER_FAN },
    { "落草为寇",     "在荒野立下第一块界碑",             TIER_FAN },
    { "符出有名",     "握起第一件符器（摄魂幡或收鬼葫），符出而名立", TIER_FAN },
    { "摸金校尉",     "打开第一个遗迹宝箱",               TIER_FAN },
    { "入土为安",     "掘开第一座坟冢",                   TIER_FAN },
    // ---- 奇：进阶（1 悟）----
    { "命悬一线",     "从鬼门关爬回来一次，贴脸对线",     TIER_QI },
    { "万物皆可盘",   "累计采集 50 次资源，盘学家认证",   TIER_QI },
    { "怪物清道夫",   "击败 10 只怪物，环保卫士",         TIER_QI },
    { "月下漫步",     "黑夜中裸奔 60 秒，最亮的仔",       TIER_QI },
    { "敕令收鬼",     "收服第一只鬼仆",                   TIER_QI },
    { "锻魂",         "把铁匠鬼喂到 2 级",                 TIER_QI },
    { "一箭封喉",     "用猎弓放倒一只活物",               TIER_QI },
    { "拓荒者",       "发现 2 处村庄遗迹",                 TIER_QI },
    { "开山立寨",     "领地升到 Lv2",                     TIER_QI },
    // ---- 秘：高难（2 悟）----
    { "六边形战士",   "等级达到 5 级，全属性进化",        TIER_MI },
    { "屠神者",       "净化鬼游戏核心，神挡杀神",         TIER_MI },
    { "摄魂使者",     "收服 5 只鬼仆",                    TIER_MI },
    { "三线同牵",     "傀儡操控同时牵住 3 只鬼",          TIER_MI },
    { "火眼金睛",     "揪出并赶走 3 只无面鬼",            TIER_MI },
    { "浑身是胆",     "被敲门鬼盯上一整夜，活到天亮",     TIER_MI },
    { "百宝箱",       "六件工具全部到手",                 TIER_MI },
    { "夜不闭户",     "领地升到 Lv3",                     TIER_MI },
    // ---- 绝：终局（3 悟）----
    { "一方之主",     "领地 Lv3 且名下 6 名同伴",         TIER_JUE },
    { "幽明两界",     "收服 10 只鬼，并让铁匠鬼认主",     TIER_JUE },
    { "破心之人",     "集齐十二片残影",                   TIER_JUE },
    { "轮回尽头",     "走到这条命的尽头（任一结局）",     TIER_JUE },
    { "百怪伏诛",     "荒野斩百怪，血煞盈身自成一道",     TIER_JUE },
};

int AchvCount() { return ACHV_N; }
const char* AchvName(int i) { return (i >= 0 && i < ACHV_N) ? ACHV[i].name : ""; }
const char* AchvDesc(int i) { return (i >= 0 && i < ACHV_N) ? ACHV[i].desc : ""; }
int  AchvTierOf(int i)      { return (i >= 0 && i < ACHV_N) ? (int)ACHV[i].tier : 0; }
bool AchvGot(int i)         { return (i >= 0 && i < ACHV_N) && BitGet(gS.achvGot, 8, i); }

// 解锁（内部）：发奖励 + 推事件
static void UnlockAchv(int i) {
    if (i < 0 || i >= ACHV_N) return;
    if (BitGet(gS.achvGot, 8, i)) return;
    BitSet(gS.achvGot, 8, i);
    int gain = TIER_INSIGHT[ACHV[i].tier];
    if (gain > 0) { gS.insight += gain; Push(PEV_INSIGHT, gain, 0); }
    Push(PEV_ACHV, i, 0);
}

// 进度显示：need<=0 表示无进度条（一次性成就）
void AchvProgress(int i, const ProgStat& st, int* cur, int* need) {
    int c = 0, n = 0;
    if (i >= 0 && i < ACHV_N) {           // 越界成就一律按「无进度」处理，绝不越界
        switch (i) {
            case 9:  c = st.nTree + st.nRock + st.nBerry; n = 50; break;
            case 10: c = st.kills;       n = 10; break;
            case 11: c = (int)st.nightOut; n = 60; break;
            case 15: c = st.ruinFound;   n = 2;  break;
            case 17: c = st.level;       n = 5;  break;
            case 19: c = st.ghostCaught; n = 5;  break;
            case 21: c = st.banished;    n = 3;  break;
            case 23: c = st.toolN;       n = 6;  break;
            case 26: c = st.ghostCaught; n = 10; break;
            case 27: c = ShardCount();   n = SHARD_N; break;
            case 29: c = st.kills;       n = 100; break;
            default: break;
        }
    }
    // 防御：need 绝不为负 / 0 时由调用方「need>0」守卫；cur 不为负避免怪异显示
    if (n < 0) n = 0;
    if (c < 0) c = 0;
    if (cur)  *cur  = c;
    if (need) *need = n;
}

// ============================================================
//  配方图鉴（23）
// ============================================================
// 悟得条件一句话：写「怎么做到的」，不写配方表
static const char* REC_LORE[RECIPE_N] = {
    /*0  桃木剑*/ "荒野求生第一课：削一根桃木",
    /*1  铁斧  */ "砍树砍到手酸，自然想换把快的",
    /*2  猎弓  */ "有些东西，隔远点收拾更稳妥",
    /*3  猎刀  */ "贴身短打，出手比铁剑快",
    /*4  铁锤  */ "石头认硬不认软",
    /*5  铁镐  */ "矿脉在土里，徒手刨不动",
    /*6  摄魂幡*/ "见过一次鬼如何散掉，你就懂了",
    /*7  铁甲  */ "被扑过两回，就想披点什么",
    /*8  萤晶药*/ "把萤晶磨了喝下去——你胆子不小",
    /*9  萤晶茶*/ "夜里看得见，才走得出去",
    /*10 血莓酱*/ "甜的能压住嘴里的土腥味",
    /*11 炖肉  */ "热食下肚，人才像个人",
    /*12 工具升*/ "趁手的东西，越用越趁手",
    /*13 铜铃幡*/ "铃一响，鬼就知道你不是好惹的",
    /*14 鎏金幡*/ "金能压阴，这是老说法，但管用",
    /*15 傀儡丝*/ "收服第一只鬼之后，你开始想牵住它",
    /*16 引魂灯*/ "长明灯照路，引魂灯照的是别的东西",
    /*17 镇魂钉*/ "领地立起来后，得有东西守着",
    /*18 萤晶甲*/ "铁甲挡爪牙，萤晶挡的是别的",
    /*19 辟邪匣*/ "符越来越多，总得有个地方放",
    /*20 摄魂铃*/ "铃声起时，鬼会想起自己还是人",
    /*21 血月剑*/ "血月之心在手里跳，像另一颗心脏",
    /*22 还魂香*/ "给自己留一条后路——只留一条",
};

static const char* REC_NAME[RECIPE_N] = {
    "辟邪桃木剑", "铁斧",     "猎弓",     "猎刀",     "铁锤",     "铁镐",
    "粗纸摄魂幡", "铁甲",     "萤晶药剂", "萤晶茶",   "血莓酱",   "炖肉",
    "工具升级",   "铜铃摄魂幡", "鎏金摄魂幡",
    "傀儡丝",     "引魂灯",   "镇魂钉",   "萤晶甲",   "辟邪符匣", "摄魂铃",
    "血月大剑",   "还魂香",
};
const char* RecipeName(int i) { return (i >= 0 && i < RECIPE_N) ? REC_NAME[i] : ""; }
const char* RecipeLore(int i) { return (i >= 0 && i < RECIPE_N) ? REC_LORE[i] : ""; }
bool RecipeKnown(int i) { return (i >= 0 && i < RECIPE_N) && BitGet(gS.recipeGot, 4, i); }
void RecipeUnlock(int i) {
    if (i < 0 || i >= RECIPE_N) return;
    if (BitGet(gS.recipeGot, 4, i)) return;
    BitSet(gS.recipeGot, 4, i);
    Push(PEV_RECIPE, i, 0);
}

// ============================================================
//  残影剧情
// ============================================================
static const char* CH_TITLE[CHAPTER_N] = {
    "序 · 破心", "一 · 无名", "二 · 立命", "三 · 识鬼",
    "四 · 问心", "五 · 残影", "六 · 明悟",
};

static const char* CH_TEXT[CHAPTER_N] = {
    // 0 序
    "你不记得自己是谁。\n"
    "只记得那天，满城的人同时捂住胸口，\n"
    "像被什么东西，从里面破开了。\n\n"
    "你也该那样死的。\n"
    "但你没有——有一只鬼，替你挡了一下。",

    // 1
    "你在一片荒野上醒来。\n"
    "身边没有活人，只有一只铁匠模样的鬼。\n"
    "它不说话，只是替你把火拢旺了些。\n\n"
    "你想问它自己是谁。\n"
    "它指了指自己的脸——那张脸，是空的。",

    // 2
    "界碑立起来的一刻，你忽然想起一件事：\n"
    "人心是会被破开的。\n"
    "可若把它分成许多份，分给活下来的人，\n"
    "是不是就破不完了？\n\n"
    "你决定收留他们。",

    // 3
    "第一只鬼被摄魂幡收进去的时候，它哭了。\n"
    "你说：你杀过人。\n"
    "它说：我只是太想活了。\n\n"
    "你这才明白——\n"
    "这些鬼，都是从破开的人心里跑出来的。",

    // 4
    "铁匠鬼终于开了口。\n"
    "它说：你身上有股味道，像……一个女人。\n\n"
    "你问：什么样的女人？\n"
    "它摇头：她的心破过三次，一次比一次亮。",

    // 5
    "十二片残影，你集齐了一半。\n"
    "每一片里都是同一个背影：\n"
    "凤冠霞帔，站在喜堂中央。\n"
    "而喜堂外，是整整一座城的死人。\n\n"
    "你开始怀疑——\n"
    "那一夜要破开的，不止是人心。",

    // 6
    "残影合拢的瞬间，你全想起来了。\n\n"
    "一四六九年，国王迎娶明朝公主。\n"
    "两国的人心，全压在一对新人身上。\n"
    "太重了。重到把心压破。\n\n"
    "而保下你的那只鬼——\n"
    "就是公主自己的心鬼。\n\n"
    "她一开始就是鬼。\n"
    "她爱上了一个人，于是化为人身。\n\n"
    "你就是那个人。",
};

static const char* END_TITLE[ENDING_N] = {
    "终 · 抹杀", "终 · 归处", "终 · 无名碑",
};
static const char* END_TEXT[ENDING_N] = {
    // 1 抹杀
    "你找到了她。\n"
    "她伸手，你却开始变淡。\n"
    "这世道容不下一个既属轮回、又属阴间的人。\n\n"
    "她抱着你渐渐透明的身体，\n"
    "像很多年前那样，\n"
    "替你挡了一次。\n\n"
    "——你被世界抹杀了。",

    // 2 拯救
    "你没有伸手。\n"
    "你把自己心里那只鬼，交给了她。\n"
    "两只相克的鬼，终于不再相克。\n\n"
    "满城破开的心，一点点合上。\n\n"
    "——世界得救了。\n"
    "而你成了一个普通人。\n"
    "她站在晨光里，脸上终于有了五官。",

    // 3 死
    "你倒在这片荒野上。\n"
    "心口空空的，什么也没剩下。\n\n"
    "很多年后，有人在这里立了一块碑。\n"
    "碑上没有名字。\n\n"
    "——轮回还在继续。",
};

// ---------------- 英文版剧情文本（与 CH_TEXT / END_TEXT 逐项一一对应）----------------
// 多行整段无法走 L10N 逐句查表（整串才唯一），因此这里给出对应的英文整段。
// 全部限定 ASCII：字形池不保证收录 em dash / 省略号等非 ASCII 符号。
static const char* CH_TEXT_EN[CHAPTER_N] = {
    // 0序
    "You do not remember who you are.\n"
    "Only that day: everyone in the city\n"
    "clutched their chest at once,\n"
    "as if something had torn them open\n"
    "from the inside.\n\n"
    "You were meant to die that way too.\n"
    "But you did not -- a ghost took the blow\n"
    "for you.",

    // 1
    "You wake in the wilds.\n"
    "No living soul beside you -- only a ghost\n"
    "shaped like a blacksmith.\n"
    "It does not speak. It only stirs the fire\n"
    "a little brighter for you.\n\n"
    "You try to ask it who you are.\n"
    "It points at its own face --\n"
    "and that face is blank.",

    // 2
    "The moment the boundary stone goes up,\n"
    "you remember something:\n"
    "a human heart can be torn open.\n"
    "But if it were divided into many shares,\n"
    "given to those still living,\n"
    "could it ever be torn again?\n\n"
    "You decide to take them in.",

    // 3
    "When the first ghost is drawn into the\n"
    "soul banner, it weeps.\n"
    "You say: you have killed people.\n"
    "It says: I only wanted to live.\n\n"
    "Only then do you understand --\n"
    "every one of these ghosts crawled out\n"
    "of a torn-open human heart.",

    // 4
    "At last the blacksmith ghost speaks.\n"
    "It says: there is a scent on you.\n"
    "Like... a woman.\n\n"
    "You ask: what kind of woman?\n"
    "It shakes its head: her heart was\n"
    "broken three times.\n"
    "Each time it shone brighter.",

    // 5
    "Twelve shards there are; you have half.\n"
    "Every one holds the same figure,\n"
    "seen from behind:\n"
    "a bride in phoenix crown and red robes,\n"
    "standing in the wedding hall.\n"
    "And outside that hall -- the dead of\n"
    "an entire city.\n\n"
    "You begin to suspect --\n"
    "that night meant to break more\n"
    "than human hearts.",

    // 6
    "The instant the shards close,\n"
    "you remember everything.\n\n"
    "In 1469, a king married a princess\n"
    "of the Ming.\n"
    "The hearts of two nations pressed\n"
    "down on one couple.\n"
    "Too heavy. Heavy enough to break hearts.\n\n"
    "And the ghost that saved you --\n"
    "was the princess's own heart-ghost.\n\n"
    "She was a ghost from the start.\n"
    "She loved a man, and so became flesh.\n\n"
    "You are that man.",
};

static const char* END_TEXT_EN[ENDING_N] = {
    // 1 抹杀
    "You found her.\n"
    "She reaches out -- and you begin to fade.\n"
    "This world has no room for one who\n"
    "belongs both to the wheel of rebirth\n"
    "and to the underworld.\n\n"
    "She holds you as you turn translucent,\n"
    "the way she did years ago,\n"
    "and takes the blow for you once more.\n\n"
    "-- You were erased by the world.",

    // 2 拯救
    "You do not reach out.\n"
    "You give her the ghost inside your heart.\n"
    "Two ghosts that could not coexist\n"
    "no longer war.\n\n"
    "Across the city, torn hearts close,\n"
    "one sliver at a time.\n\n"
    "-- The world is saved.\n"
    "And you become an ordinary man.\n"
    "She stands in the morning light,\n"
    "her face at last having features.",

    // 3 无名碑
    "You fall in these wilds.\n"
    "Your chest hollow, nothing left inside.\n\n"
    "Many years later, someone raises\n"
    "a stone here.\n"
    "Upon it, no name.\n\n"
    "-- The wheel keeps turning.",
};

bool ShardGot(int i)  { return (i >= 0 && i < SHARD_N) && BitGet(gS.shardBits, 2, i); }
int  ShardCount() {
    int n = 0;
    for (int i = 0; i < SHARD_N; i++) if (BitGet(gS.shardBits, 2, i)) n++;
    return n;
}
bool GiveShard(int i) {
    if (i < 0 || i >= SHARD_N) return false;
    if (BitGet(gS.shardBits, 2, i)) return false;
    BitSet(gS.shardBits, 2, i);
    Push(PEV_SHARD, i, 0);
    // 每 2 片推进一章（第 6 章需集齐 12 片）
    int n = ShardCount();
    int want = (n >= SHARD_N) ? 6 : (n / 2);
    if (want > (int)gS.chapter && want < CHAPTER_N) {
        gS.chapter = (unsigned char)want;
        Push(PEV_CHAPTER, want, 0);
    }
    return true;
}
int   Chapter() { return (int)gS.chapter; }
const char* ChapterTitle(int c) { return (c >= 0 && c < CHAPTER_N) ? CH_TITLE[c] : ""; }
const char* ChapterText(int c)  { return (c >= 0 && c < CHAPTER_N) ? (gL10nEn ? CH_TEXT_EN[c] : CH_TEXT[c]) : ""; }
bool  ChapterSeen(int c) { return (c >= 0 && c < CHAPTER_N) && gChapterSeen[c]; }
void  MarkChapterSeen(int c) { if (c >= 0 && c < CHAPTER_N) gChapterSeen[c] = true; }

int   Ending() { return (int)gS.ending; }
void  TriggerEnding(int e) {
    if (e < 1 || e > ENDING_N) return;
    if (gS.ending) return;                 // 结局只走一次
    gS.ending = (unsigned char)e;
    gS.chapter = (unsigned char)(CHAPTER_N - 1);
    Push(PEV_ENDING, e, 0);
    UnlockAchv(28);                        // 轮回尽头
}
const char* EndingTitle(int e) { return (e >= 1 && e <= ENDING_N) ? END_TITLE[e - 1] : ""; }
const char* EndingText(int e)  { return (e >= 1 && e <= ENDING_N) ? (gL10nEn ? END_TEXT_EN[e - 1] : END_TEXT[e - 1]) : ""; }

// ============================================================
//  每帧推进
// ============================================================
void Tick(const ProgStat& st, float dt) {
    if (!(dt >= 0.0f)) return;             // 丢弃 NaN / 负数 dt，避免计时器异常
    gTickT += dt;
    if (gTickT < 0.25f) return;            // 4Hz 轮询足够，别每帧刷
    gTickT = 0.0f;

    // ---- 成就 ----
    if (st.nTree > 0)                UnlockAchv(0);
    if (st.nEat > 0)                 UnlockAchv(1);
    if (st.nFire > 0)                UnlockAchv(2);
    if (st.kills > 0 || st.ghostCaught > 0) UnlockAchv(3);   // 初见阴物
    if (st.campLv > 0)               UnlockAchv(4);
    if (st.captureLv >= 1 || st.gourdLv >= 1) UnlockAchv(5);
    if (st.nChest > 0)               UnlockAchv(6);
    if (st.nGrave > 0)               UnlockAchv(7);
    if (st.nearDeath && st.hp >= 30) UnlockAchv(8);
    if (st.nTree + st.nRock + st.nBerry >= 50) UnlockAchv(9);
    if (st.kills >= 10)              UnlockAchv(10);
    if (st.nightOut >= 60.0f)        UnlockAchv(11);
    if (st.ghostCaught > 0)          UnlockAchv(12);
    if (st.smithLv >= 2)             UnlockAchv(13);
    if (st.bowKill)                  UnlockAchv(14);
    if (st.ruinFound >= 2)           UnlockAchv(15);
    if (st.campLv >= 2)              UnlockAchv(16);
    if (st.level >= 5)               UnlockAchv(17);
    if (st.domainPurged)             UnlockAchv(18);
    if (st.ghostCaught >= 5)         UnlockAchv(19);
    if (st.puppetMax >= 3)           UnlockAchv(20);
    if (st.banished >= 3)            UnlockAchv(21);
    if (st.stalkSurvived)            UnlockAchv(22);
    if (st.toolN >= 6)               UnlockAchv(23);
    if (st.campLv >= 3)              UnlockAchv(24);
    if (st.campLv >= 3 && st.allyN >= 6) UnlockAchv(25);
    if (st.ghostCaught >= 10 && st.smithOwned) UnlockAchv(26);
    if (ShardCount() >= SHARD_N)     UnlockAchv(27);
    if (st.bossSlain)                UnlockAchv(18);          // 兼容旧标记

    // ---- 配方图鉴：靠行为悟得 ----
    if (st.nTree >= 3)               RecipeUnlock(0);         // 桃木剑
    if (st.nTree >= 8)               RecipeUnlock(1);         // 铁斧
    if (st.kills >= 3 || st.nTree >= 12) RecipeUnlock(2);     // 猎弓
    if (st.nRock >= 3)               RecipeUnlock(3);         // 猎刀
    if (st.nRock >= 8)               RecipeUnlock(4);         // 铁锤
    if (st.iron > 0)                 RecipeUnlock(5);         // 铁镐
    if (st.crystal > 0)              { RecipeUnlock(8); RecipeUnlock(9); }
    if (st.nRock >= 12 || st.iron >= 3) RecipeUnlock(7);         // 铁甲
    if (st.nBerry >= 5)              RecipeUnlock(10);        // 血莓酱
    if (st.nFire > 0)                RecipeUnlock(11);        // 炖肉
    if (st.toolN >= 2)               RecipeUnlock(12);        // 工具升级
    if (st.ghostCaught >= 1)         { RecipeUnlock(6); RecipeUnlock(13); RecipeUnlock(15); }
    if (st.ghostCaught >= 3)         RecipeUnlock(14);
    if (st.campLv >= 2)              { RecipeUnlock(16); RecipeUnlock(17); }
    if (st.crystal >= 3)             RecipeUnlock(18);
    if (st.ghostCaught >= 5)         RecipeUnlock(20);
    if (st.heart > 0)                RecipeUnlock(21);
    if (ShardCount() >= 6)           { RecipeUnlock(19); RecipeUnlock(22); }

    // ---- 残影：散落各处，捡到才推剧情 ----
    if (st.campLv >= 1)              GiveShard(0);            // 立碑
    if (st.ghostCaught >= 1)         GiveShard(1);            // 初收鬼
    if (st.nChest >= 1)              GiveShard(2);            // 开箱
    if (st.nGrave >= 3)              GiveShard(3);            // 掘坟
    if (st.ruinFound >= 1)           GiveShard(4);            // 遗迹
    if (st.smithOwned)               GiveShard(5);            // 铁匠鬼认主
    if (st.campLv >= 2)              GiveShard(6);            // 领地 Lv2
    if (st.domainPurged)             GiveShard(7);            // 净化鬼域
    if (st.bossSlain)                GiveShard(8);            // 血月尸王
    if (st.campLv >= 3)              GiveShard(9);            // 领地 Lv3
    if (st.ghostCaught >= 8)         GiveShard(10);           // 八鬼
    if (st.level >= 8)               GiveShard(11);           // 八级

    // ---- 结局判定（仅集齐十二片残影后才可能触发）----
    if (ShardCount() >= SHARD_N && !gS.ending) {
        if (st.dead)                     TriggerEnding(3);    // 死在路上
        else if (st.facelessN > 0 && st.allyN > 0 &&
                 st.facelessN * 10 >= st.allyN * 3)  TriggerEnding(1);  // 被同化 >=30%
        else if (st.campLv >= 3)         TriggerEnding(2);    // 心已成城
    } else if (st.dead && gS.chapter >= 4) {
        TriggerEnding(3);                                     // 知道太多，死也是结局
    }
}

// ============================================================
//  存档
// ============================================================
void Reset() {
    gS = ProgSave();
    gEvHead = gEvTail = 0;
    gTickT = 0;
    for (int i = 0; i < CHAPTER_N; i++) gChapterSeen[i] = false;
    // 序章：一开局就该知道「你是谁（不知道）」
    gS.chapter = 0;
}
void Load(const ProgSave& s) {
    // 先判合法性：magic 不符 / 版本过新 → 视为损坏或旧档，直接全新开局，绝不加载垃圾
    if (s.magic != PROG_MAGIC || s.version > PROG_VERSION) {
        Reset();
        return;
    }
    gS = s;
    // 消毒：把任何越界/负值夹到合法域，挡住 0xFF 或未初始化的垃圾数据
    if (gS.insight < 0)      gS.insight = 0;
    if (gS.insightSpent < 0) gS.insightSpent = 0;
    for (int i = 0; i < PATH_N; i++)
        if (gS.pathLv[i] > PATH_MAX_LV) gS.pathLv[i] = (unsigned char)PATH_MAX_LV;
    for (int i = 0; i < ACHV_N; i++)
        if (gS.achvProg[i] < 0) gS.achvProg[i] = 0;
    if (gS.chapter >= CHAPTER_N) gS.chapter = (unsigned char)(CHAPTER_N - 1);
    if (gS.ending > ENDING_N)    gS.ending = 0;
    // 章节已看标记同步（chapter 已消毒，下标安全）
    gEvHead = gEvTail = 0;
    gTickT = 0;
    for (int i = 0; i < CHAPTER_N; i++) gChapterSeen[i] = (i <= gS.chapter);
}
const ProgSave& Save() { return gS; }

}  // namespace Prog
