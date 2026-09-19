#include "raylib.h"
#include "assets.hpp"
#include "audio.hpp"
#include "world.hpp"
#include "creature.hpp"
#include "water.hpp"
#include "progress.hpp"
#include "net.hpp"
#include "l10n.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#undef IN
#undef OUT
#endif

// ============================================================
// 还魂人 —— 主程序
// 内部 640x360 渲染 -> 整数 2 倍放大到 1280x720
// 主循环零 new/delete（全部容器预分配 + swap-remove 池）
// ============================================================

constexpr int VW = 640, VH = 360;   // 内部分辨率
constexpr int SCALE = 2;            // 整数放大倍数

// ---------------- 中文字体 ----------------
// 零外部素材：直接读取 Windows 系统自带中文字体；回退则默认字体
static Font ZhFonts[5] = {};              // 5 档精确字号字体（1:1 绘制，零缩放 = 零模糊）
static const int ZhSizes[5] = { 10, 12, 14, 26, 32 };
static std::vector<int> gZhCp;            // 字体码点缓存
// ---- 缺字自愈：任何未在码点表里的字，首次出现时登记，帧末重建一次图集 ----
// （旧实现靠手写 ZHTEXT 清单，漏一个字 raylib 就回退画 '?' —— 现改为运行时自动补齐）
static std::vector<int> gZhPending;                 // 待补码点
static std::unordered_set<int> gZhPendingSet;       // 待补去重（各档补齐前可重复登记）
static std::unordered_map<int, int> gZhGlyph[5];    // 字号档 -> (码点 -> 字形下标)
static unsigned char gZhHave[0x10000];              // BMP 直查：该码点是否在预载集
static bool gZhOwned[5] = {};                       // 该档字体是否真实加载（default 兜底档绝不可 Unload）
static bool gZhDirty = false;                       // 有待补字，需要重建

// 游戏内用到的所有中文（去重，驱动字体码点加载）
// 由 gen_zhtext.ps1 从全部字符串字面量自动提取 —— 新增文案后重跑脚本即可
static const char* ZHTEXT =
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    "·—…←↑→↓、。「」一万三上下不与且世东丝两个中临为主久么"
    "之乐九也买乱了事二于互五些亡交亮人什仆仇今仍从仔他付代令以们"
    "件价任份伍伏伙会传伤伪伴伸但位低住体何作你使保俱倍倒候值做停"
    "傀储像僵儡充先光克兔入全公六关兵其具养兽内册再农冠冢冥冲决冷"
    "净减凡凤出击刀分切列则创初刨别到制刻剂削前剑剥剩剪力功加动助"
    "劲势勿包化匠匣区医十升半单博占卫印即却卷压原去又友反发叔取变"
    "口另只召可台右吃各吆合同名后向吞否吧听启吴吸周味命咒咚咬响哨"
    "哭唤喂喉喜喝嗅嘴器噬四回困围国图圆土在地场坏块坟垣城域基堂堆"
    "塌墙增士声处备复外多夜够大天太夫央失头夺奇契奔女奴她好如妥始"
    "娘娶婆婶子字存孙孤学它宅守安完官定宝客害家容寇寝寡寨对寿封射"
    "将尉小少尚就尸尽局层屋屏属屠山岛崩工左差己已布师帔带席幕幡干"
    "平年并幸幽庄庇床序应废度座建开弃弈式弓引张弱弹强归当形彩影径"
    "待很律徒得微德心必忆快忽怀态怒思性怪总恢息悟悬想惹愈意愿慢懂"
    "戏成我或战户房所手才扑打扩扫扯扰找技把投护披抱抹拉拓招拢拥拨"
    "择拳拷拾持指按挑挡挣捂换掉掌掘接控提握揪搜摄摇摸撕撞撸擅操收"
    "改攻放效敌救敕敛散数敲整文斗料斧斩断新方施旁旅无既日时旺明易"
    "是晨普晶暂暗更替最月有朋服朝木未本朱机杀杂李材村束条来板极果"
    "标栏树校样核根格桃案档棵概槽模横次欢欺止此步死残每毒比气水永"
    "求没沿法泛洗活浆测浑海涂消涨淡深混清渐渡游溃源溯滚滞满滩演漫"
    "火灭灯灵炉炖点炼烤热然煞照熟燃爪爬爱爷爽片牙牢物牵犯猎猜献率"
    "王玩环现理甜生用甩由甲界留疑疾痊白百的皆盈盖盘盯直相看眠眼着"
    "睁睛睡瞬知短石矿砂砌砍破础硬确碎碑磁磨磷示礼神票祸禁福离秀种"
    "秒秘积移程稳空突窃窄窒窥立站章童端符第等答管箭箱篝篡类粗精系"
    "素累约级纸线终经结给绝继绪续绽缓缺网置群翻老者而耗联聚肉肚股"
    "肯胆背胸能脉脏脱脸腥自至舟航色艺芦花若范茶草荒荧药莓莫萤营落"
    "葫蓝藏蛮蛰蜕血行表衰被袭装裸西要覆见观规视觉角解触警计认让议"
    "记许设证识试诛话询该说请读课谁调谈谢贝负败货贴资赌赏赢走赵赶"
    "起趁越足跑距跟路跳踏身躺车轮软输辟边达过迎近返还这进远违迟迹"
    "追退送逃选透逐途通速造遍道遗遮邀那邪郎部都配酱酸醒采里重野量"
    "金鉴鎏针钉铁铃铜铺锁锋错锤键锻镇镐长门闭问间闻阈队阳阴阵阿附"
    "陈降限隐隔难集雇雨雷需震霞靠面音页顶项顺须预领颗题额飘飞食饥"
    "饭饱饿首香驭驯驱驶驾验高鬼魂魄鹿黄黑鼠齐！（），：？";
// 注：资源栏 "x%d"、配方成本 "铁矿3/3" 等含 ASCII，基础码点 32..126 已覆盖

// UTF-8 解码：返回 codepoint 并把 *off 推进对应字节数
static int decUTF8(const char* s, int* off, int* adv) {
    int o = off ? *off : 0; unsigned char b0 = (unsigned char)s[o];
    int cp, bytes;
    if      (b0 < 0x80) { cp = b0; bytes = 1; }
    else if ((b0 & 0xE0) == 0xC0) { cp = (b0 & 0x1F) << 6;  cp |= ((unsigned char)s[o + 1] & 0x3F); bytes = 2; }
    else if ((b0 & 0xF0) == 0xE0) { cp = (b0 & 0x0F) << 12; cp |= ((unsigned char)s[o + 1] & 0x3F) << 6; cp |= ((unsigned char)s[o + 2] & 0x3F); bytes = 3; }
    else if ((b0 & 0xF8) == 0xF0) { cp = (b0 & 0x07) << 18; cp |= ((unsigned char)s[o + 1] & 0x3F) << 12; cp |= ((unsigned char)s[o + 2] & 0x3F) << 6; cp |= ((unsigned char)s[o + 3] & 0x3F); bytes = 4; }
    else                            { cp = 0; bytes = 1; }
    if (off) *off = o + bytes;
    if (adv) *adv = bytes;
    return cp;
}



// 按指定像素尺寸加载字体（图集尺寸 = 绘制尺寸，1:1 点对点采样，杜绝缩放模糊）
static Font Zh_LoadFontAt(int px) {
    Font f = {};
    // 注意：raylib 5.5 无法从 .ttc 字体集合加载（stbtt 只认单字体 ttf），.ttf 优先
    const char* cand[] = {
        "C:/Windows/Fonts/simhei.ttf", "C:/Windows/Fonts/simfang.ttf",
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/msyh.ttc", "C:/Windows/Fonts/simsun.ttc", nullptr };
    for (int i = 0; cand[i]; i++)
        if (FileExists(cand[i])) {
            Font t = LoadFontEx(cand[i], px, gZhCp.data(), (int)gZhCp.size());
            if (t.texture.id != 0 && t.glyphCount > 0) { f = t; break; }   // 真正成功才采用
            if (t.texture.id != 0 || t.glyphCount > 0) UnloadFont(t);      // 失败则释放重试下一个
        }
    if (f.texture.id == 0 || f.glyphCount <= 0) f = GetFontDefault();      // 全部失败才退回默认
    SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);                     // 点采样：1:1 像素级锐利
    return f;
}

// 预加载全部 5 档字号（10/12/14/26/32，覆盖游戏内所有文本尺寸）
static void Zh_LoadAll() {
    gZhCp.clear();
    for (int c = 32; c <= 126; c++) gZhCp.push_back(c);           // ascii 基础
    { int n = 0; int* cpArr = LoadCodepoints(ZHTEXT, &n); for (int i = 0; i < n; i++) gZhCp.push_back(cpArr[i]); UnloadCodepoints(cpArr); }
    // 常用标点/符号兜底（文本里随时可能用到，先备好，避免首帧缺字抖动）
    static const int extra[] = { 0x00B7,0x2014,0x2026,0x3001,0x3002,0x201C,0x201D,0xFF08,0xFF09,
                                 0xFF1A,0xFF0C,0x25A0,0x25B2,0x25BC,0x2605,0x2716,0x2190,0x2192 };
    for (int c : extra) gZhCp.push_back(c);
    for (int c : gZhCp) if (c >= 0 && c < 0x10000) gZhHave[c] = 1;
    for (int i = 0; i < 5; i++) ZhFonts[i] = Zh_LoadFontAt(ZhSizes[i]);
    for (int i = 0; i < 5; i++)
        gZhOwned[i] = (ZhFonts[i].texture.id != 0 && ZhFonts[i].glyphCount > 0 && ZhFonts[i].glyphs != nullptr);
}

// 重建「码点 -> 字形下标」映射：替代 raylib GetGlyphIndex 的线性扫描（每字 O(n) -> O(1)）
static void Zh_Reindex() {
    for (int i = 0; i < 5; i++) {
        gZhGlyph[i].clear();
        if (ZhFonts[i].glyphCount <= 0 || !ZhFonts[i].glyphs) continue;
        gZhGlyph[i].reserve(ZhFonts[i].glyphCount * 2);
        for (int k = 0; k < ZhFonts[i].glyphCount; k++)
            gZhGlyph[i][ZhFonts[i].glyphs[k].value] = k;
    }
}
// 帧末调用：把本帧遇到的缺字补进图集。
// 关键性能设计：每帧最多重载 1 个档位的图集（分帧摊开销）。
// 旧实现一次重建 5 档 = 5 次 LoadFontEx（几千字形/档），单帧卡几十上百 ms —— 这就是 V/U 按键卡顿的根因。
// 映射表（内存哈希）每帧全档重建 —— 微秒级；图集重载只做 1 档。
// 收敛保证：某档图集尚未含新字时 ZhGlyphOf 会再次登记 → 逐档补齐后 gZhDirty 归零。
static void Zh_Flush() {
    if (!gZhDirty || gZhPending.empty()) return;
    gZhDirty = false;
    for (int cp : gZhPending) gZhCp.push_back(cp);
    gZhPending.clear();
    gZhPendingSet.clear();                      // 清去重集：未补档的 miss 下一帧重新登记
    static int sSlot = 0;                       // 分帧游标：每帧推进一档
    for (int k = 0; k < 5; k++, sSlot = (sSlot + 1) % 5) {
        int i = sSlot;
        if (!gZhOwned[i]) continue;             // GetFontDefault 兜底档：绝不可 Unload（非堆内存，卸载即崩）
        UnloadFont(ZhFonts[i]);
        ZhFonts[i] = Zh_LoadFontAt(ZhSizes[i]);
        gZhOwned[i] = (ZhFonts[i].texture.id != 0 && ZhFonts[i].glyphCount > 0 && ZhFonts[i].glyphs != nullptr);
        break;                                  // 一帧只重载一档，剩余档下一帧继续
    }
    Zh_Reindex();                               // 全档映射重建（纯内存操作，微秒级）
}

// ============================================================
// ---- 字体系统 v3：多色风格 / 绝对清晰 / 分帧补字 ----
// ============================================================
// 清晰三原则：图集尺寸=绘制尺寸(1:1 零缩放) + POINT 点采样 + 全整数坐标。
// 多色：TxtStyle 风格套件 —— 每种场景一套「主色 + 衬底方案」，统一视觉语言。
// 补字：分帧重建（每帧最多重载 1 档图集），且只有真实加载的字体才允许 Unload。
// ============================================================

// ---- 多色风格套件：场景 -> (主色, 衬底色, 衬底模式) ----
enum TxtStyle {
    TXT_BODY = 0,   // 正文：米白 + 1px 黑投影（最通用）
    TXT_TITLE,      // 标题：鎏金 + 4向黑描边（面板题字）
    TXT_GOLD,       // 强调：淡金 + 4向黑描边（奖励/升级）
    TXT_WARN,       // 警告：朱红 + 4向黑描边（危险/拒绝）
    TXT_GHOST,      // 阴物：幽绿 + 深青描边（鬼仆/阴气）
    TXT_SYS,        // 系统：天青 + 1px 黑投影（提示/键位）
    TXT_MUT,        // 弱化：暖灰 + 1px 黑投影（次要说明）
    TXT_BLOOD,      // 血字：血红 + 4向黑描边（幡/封印主题）
    TXT_N
};
struct TxtLook { Color col; Color edge; bool outline; };   // outline=true 4向描边 / false 1px投影
static const TxtLook TXT_LOOK[TXT_N] = {
    { { 240, 236, 224, 255 }, { 0, 0, 0, 255 },       false },  // BODY
    { { 255, 224, 138, 255 }, { 0, 0, 0, 255 },       true  },  // TITLE
    { { 255, 240, 180, 255 }, { 30, 22, 8, 255 },     true  },  // GOLD
    { { 255, 96, 74, 255 },   { 0, 0, 0, 255 },       true  },  // WARN
    { { 140, 255, 210, 255 }, { 8, 30, 26, 255 },     true  },  // GHOST
    { { 120, 220, 255, 255 }, { 0, 0, 0, 255 },       false },  // SYS
    { { 152, 148, 140, 255 }, { 0, 0, 0, 255 },       false },  // MUT
    { { 226, 40, 46, 255 },   { 12, 2, 4, 255 },      true  },  // BLOOD
};

// 字号对齐到预载字体档位（图集尺寸 = 绘制尺寸 -> 1:1 点对点零缩放）
static int ZhPick(int& size) {
    int best = 0, bd = 1 << 30;
    for (int i = 0; i < 5; i++) {
        int d = size - ZhSizes[i]; if (d < 0) d = -d;
        if (d < bd) { bd = d; best = i; }
    }
    size = ZhSizes[best];
    return best;
}
// 取字形：命中缓存返回下标；未收录则登记待补并返回 -1（调用方跳过绘制，绝不画 '?'）
// 缺字按「帧」去重（gZhPendingSet，声明在文件首部）：某档补齐前其余档可重复登记，各档最终都会补上
static inline int ZhGlyphOf(int slot, int cp) {
    auto it = gZhGlyph[slot].find(cp);
    if (it != gZhGlyph[slot].end()) return it->second;
    if (cp > 31 && cp != 127 && cp < 0x10000) {
        if (gZhPendingSet.insert(cp).second) {
            gZhPending.push_back(cp);
            gZhDirty = true;
        }
    }
    return -1;
}
static inline int ZhStep(const Font& f, int gi, int cp, int size) {
    if (cp == 32) return size / 3 + 1;                 // 空格呼吸位
    if (gi >= 0 && gi < f.glyphCount) return (int)ceilf(f.recs[gi].width + 1);
    return size;                                       // 缺字：等宽占位（补齐后自动对齐）
}
// 核心绘制：风格 + 全整数坐标（浮点坐标是像素字发糊的第一元凶）
static void ZhTextStyled(const TxtLook& lk, const char* s, int x, int y, int size) {
    s = L10N(s);                                   // 本地化：中文模式下原样返回
    int slot = ZhPick(size);
    const Font& f = ZhFonts[slot];
    int off = 0, cx = x, cy = y;
    while (s[off]) {
        int cp = decUTF8(s, &off, nullptr);
        int gi = ZhGlyphOf(slot, cp);
        int step = ZhStep(f, gi, cp, size);
        if (cp != 32 && gi >= 0) {                     // 空格/缺字：只推进光标
            if (lk.outline || size >= 20) {            // 4 向描边
                DrawTextCodepoint(f, cp, { (float)(cx + 1), (float)cy       }, (float)size, lk.edge);
                DrawTextCodepoint(f, cp, { (float)(cx - 1), (float)cy       }, (float)size, lk.edge);
                DrawTextCodepoint(f, cp, { (float)cx,       (float)(cy + 1) }, (float)size, lk.edge);
                DrawTextCodepoint(f, cp, { (float)cx,       (float)(cy - 1) }, (float)size, lk.edge);
            } else {                                   // 1px 投影
                DrawTextCodepoint(f, cp, { (float)(cx + 1), (float)(cy + 1) }, (float)size, lk.edge);
            }
            DrawTextCodepoint(f, cp, { (float)cx,       (float)cy       }, (float)size, lk.col);
        }
        cx += step;
    }
}
// 风格入口：多色文字统一走这里
static void ZhTextS(TxtStyle st, const char* s, int x, int y, int size) {
    if (st < 0 || st >= TXT_N) st = TXT_BODY;
    ZhTextStyled(TXT_LOOK[st], s, x, y, size);
}
// 兼容入口：自定义色（默认正文衬底）
static void ZhText(const char* s, int x, int y, int size, Color c) {
    TxtLook lk = TXT_LOOK[TXT_BODY];
    lk.col = c;
    ZhTextStyled(lk, s, x, y, size);
}
static int ZhWidth(const char* s, int size) {
    s = L10N(s);                                   // 与绘制同一份译文，保证居中/换行宽度一致
    int slot = ZhPick(size);
    const Font& f = ZhFonts[slot];
    int w = 0, off = 0;
    while (s[off]) {
        int cp = decUTF8(s, &off, nullptr);
        int gi = ZhGlyphOf(slot, cp);
        w += ZhStep(f, gi, cp, size);
    }
    return w;
}

// ---------------- 数据结构 ----------------

struct Player {
    float x = 0, y = 0;
    int hp = 100, maxHp = 100, hunger = 100;
    int dir = 0;                    // 0下 1上 2右 3左
    float animT = 0;
    bool walking = false, running = false;
    float atkCd = 0, atkTime = 0;   // 攻击冷却 / 攻击动画剩余
    float invuln = 0, hurtFlash = 0;
    float kx = 0, ky = 0;           // 受击击退速度
    bool dead = false;
    float deadT = 0;
    int wood = 0, stone = 0, berry = 0, rawMeat = 0, cookedMeat = 0, rottenMeat = 0;
    int iron = 0, crystal = 0;      // 铁矿石 / 萤晶（遗迹产出，高级配方）
    int gemShard = 0;               // 符文碎片（地牢深层稀有掉落，工具矿石升级材料）
    int kills = 0;

    // ---- 符咒（五符：困/速/离/黄/仇，用得越多符力越深——自动升级）----
    int   talN[5] = {};             // 各类符咒持有数（上限 8）
    float talCd[5] = {};            // 各类符咒冷却剩余
    int   talSel = 0;               // 当前选中的符咒
    int   talLv[5] = { 1, 1, 1, 1, 1 };   // 符等级 1..3（每用 3 次升 1 级）
    int   talUse[5] = {};           // 升级计数（每级需 3 次）

    // ---- 状态 ----
    float hideT = 0;                // 隐身符：剩余秒数（期间不会触发任何规律）
    bool  stalked = false;          // 被敲门鬼盯上（撑到天亮才解除）
    int   faceStolen = 0;           // 面容剥夺层数（多面人命中叠加；3 层重创；随时间消退）
    float faceStolenT = 0;          // 距离下一层消退的计时
    float faceCoverT = 0;           // 无脸鬼：染血人脸覆盖剩余秒数（视野收拢遮屏）
    float shieldT = 0;              // 鬼仆「锻炉」护体剩余秒数（期间受伤减半）
    // ---- 记忆篡改（无脸鬼命中）：假血条显示 + 经验窃取 ----
    int   fakeHp = 0;               // 篡改后显示的假血量（玩家以为的血量）
    bool  hacked = false;           // 当前是否被篡改记忆
    float hackT = 0;                // 距离下次篡改消退的计时
    int   stolenXp = 0;             // 被无脸鬼窃走的经验（收服它时全额追回）

    // ---- 中毒（穴蛛毒咬 DoT）----
    float poisonT = 0, poisonDps = 0, poisonTick = 0;

    // ---- 等级经验 ----
    int level = 1, xp = 0;

    // ---- 装备/工具（环形物品栏槽 0..5；0=未拥有 1=基础 2=矿石升级 3=血月大剑）----
    // 槽位：0 铁剑 1 猎弓 2 猎刀 3 铁斧 4 铁锤 5 铁镐
    unsigned char toolLv[6] = {};
    unsigned char armorLv = 0;      // 护甲 0=无 1=铁甲(-38%) 2=萤晶甲(-55%)
    int potion = 0;                 // 萤火药剂（槽 6，使用回血 50）
    int stew = 0;                   // 炖肉（槽 9，键 0，饱食+60+回血buff）
    int heart = 0;                  // 血月之心（血月尸王专属掉落，终极武器材料）
    bool boat = false;              // 小舟（木×5 免工作台合成；拥有后可踏水泛舟渡海）

    // ---- 收鬼系统 ----
    unsigned char captureLv = 0;    // 镇鬼幡等级 0=无 1=粗纸 2=铜铃 3=鎏金（可收鬼类/成功率/复刻度）
    unsigned char swordLv = 0;      // 桃木剑等级 0=无 1..3（击退同级及以下鬼）
    unsigned char gourdLv = 0;      // 收鬼葫芦等级 0=无 1..3（暂储同级及以下鬼，容量=等级x3）
    int ghostCaught = 0;            // 累计收服鬼仆数（成就统计）

    // ---- Buff 计时（果酱加速 / 炖肉回血 / 萤晶茶夜视）----
    float buffSpdT = 0;             // 加速 buff 剩余秒数
    float talSpdT = 0;              // 速符疾行剩余秒数（1.6x，独立于食物加速）
    float buffRegenT = 0;           // 回血 buff 剩余秒数
    float buffNightT = 0;           // 夜视 buff 剩余秒数
    float buffRegenTick = 0;        // 回血节拍器

    // ---- 移动平滑（加减速插值）----
    float vx = 0, vy = 0;

    // ---- 成就计数 ----
    int nTree = 0, nRock = 0, nBerry = 0;   // 采集分项
    int nEat = 0, nFire = 0, nChest = 0;    // 进食/篝火/开箱
    float nightOut = 0;                     // 夜间无照明游荡秒数
    bool nearDeath = false;                 // 血量曾 ≤10（回血到 30+ 解锁成就）
};

// 实际攻击伤害 / 升级所需经验：定义在全局状态之后（见 PlayerDmg/XpNext 声明处）

struct Particle { float x, y, vx, vy, life, maxLife; unsigned char r, g, b; };
struct DmgText  { float x, y, life; char txt[48]; unsigned char r, g, b; };

enum class GS { Title, Play, Pause, Dead };

// ---------------- 遗迹探索 POI ----------------
static bool ruinFound[4] = {};      // 已发现的遗迹（世界最多 4 处）
static float arrowPh = 0;           // 指引箭头摆动相位

// ---------------- 全局状态 ----------------

static Assets A;
static AudioBank AU;
static World W;
static WaterRenderer WATER;   // 水面渲染器 v4.1（哑光青绿 + 云斑 + 倒影 + 雨涟漪）
static std::vector<Creature> mobs;
static std::vector<Particle> parts;      // 粒子池（上限 512）
static std::vector<DmgText> dmgs;        // 伤害/提示飘字池（上限 32）
static Player P;
static GS gs = GS::Play;
static float gameTime = 0, deathTime = 0;
static float hitStop = 0;                // 全局打击停顿
static float shakeT = 0, shakeDur = 0.15f;
static float hurtVin = 0;                // 受击红边闪
static float hungerT = 0, starveT = 0, regenT = 0, healT = 0;
static bool craftOpen = false;            // 合成面板（TAB；需靠近工作台）
static int craftSel = 0;                  // 当前选中配方（CapsLock/Enter 确认）
constexpr int CRAFT_CNT = 23;             // 配方总数（按工作台 1/2/3 级分档解锁；图鉴制，未悟得显示为 ???）
static int craftPage = 0;                 // 合成面板分页（每页 12 项）
static bool craftAnimOn = false;          // 合成动画播放中（世界冻结）
static float craftAnimT = 0;              // 合成动画计时 0..CRAFT_ANIM_LEN
static Texture2D craftAnimIcon = {};      // 合成动画最终产物图标
// 合成动画三阶段：材料逐一环绕摆放 -> 依次朝圆心相撞 -> 闪光融合为产物
constexpr float CRAFT_ANIM_LEN = 1.15f;
constexpr int   CRAFT_MAT_MAX  = 3;       // 环形摆放的材料图标数
static Texture2D craftMat[CRAFT_MAT_MAX] = {};   // 当前配方的材料图标
static int       craftMatN = CRAFT_MAT_MAX;
static bool hotbarOpen = false;           // 环形物品栏（滚轮唤出，停手自动收起）
static int hotSel = 0;                    // 环形栏选中槽位 0..9
static float hotbarHoldT = 0;             // 物品栏自动收起倒计时
static bool ghostBarOpen = false;         // 收鬼栏（F 开关；指挥已收服的鬼仆）
// ---- 傀儡操控（R 开关）：附体已收容的鬼仆，直接驾驶它行动/放技能；可切换、可多只同控 ----
static bool puppetOn = false;             // 傀儡操控中（玩家本体站桩， WASD 驾驶傀儡）
static int  puppetMain = -1;              // 主控鬼（ghosts 下标；←/→ 切换）
static std::vector<unsigned char> puppetGrp;  // 同控组标记（与 ghosts 等长；1=同控，空格加/减）
static float puppetMx = 0, puppetMy = 0;  // 本帧傀儡驾驶方向（WASD 重定向过来）
static int  gPuppetMax = 0;             // 曾同时操控的鬼仆最大数（成就：三线同牵）
static int  gBanished  = 0;             // 累计揪出并赶走的无面鬼数（成就：火眼金睛）
static int  gGraveDug  = 0;             // 累计掘坟数（残影 / 成就）
static bool gBowKill   = false;         // 用猎弓击杀过（成就：一箭封喉）
static bool gStalkSurvived = false;     // 被敲门鬼盯上后活到天亮（成就：浑身是胆）
static constexpr float HOTBAR_HOLD = 2.2f;   // 物品栏停留时长（秒）
static bool playerSailing = false;        // 泛舟中（拥有小舟 + 脚下是水）
static bool playerMoving = false;         // 本帧玩家是否在移动（鬼的规律判定用）
static bool playerUsedItem = false;       // 本帧是否使用了道具（缚灵术士窥人用物规律）
static bool playerMining = false;         // 本帧是否动土（砍/挖/采）——惊动擅入者死规律
static bool talBarOpen = false;           // 符咒环（X 开关，与物品栏互斥）
// ---- 五符：0 困 1 速 2 离 3 黄 4 仇 ----
enum Talisman : unsigned char { TAL_BIND = 0, TAL_BANISH, TAL_SEAL, TAL_BOLT, TAL_HIDE, TAL_N };
static const char* TAL_NAME[5] = { "困符", "速符", "离符", "黄符", "仇符" };
static const char* TAL_DESC[5] = { "困住鬼3秒", "疾行3秒", "30%抹去鬼的标记", "近身贴符鬼降1级", "50%挑得鬼鬼相斗" };
static const float TAL_CD[5]   = { 8.0f, 6.0f, 10.0f, 12.0f, 18.0f };   // 各自冷却（秒）
static const int   TAL_MAX     = 8;                                     // 每种上限

// ============================================================
//  命途效果翻译层：悟点 -> 机制改动
//  手册里写的每一句话，都必须能在这个块里对上号。
//  原则：不给「伤害 +10%」，只改「你如何看鬼、如何伏鬼、如何驭鬼、如何安身」。
// ============================================================
// ---- 器物：图鉴高阶配方的产出（与命途叠加，不互相替代）----
static int  gPuppetExtra = 0;         // 傀儡丝：同控上限 +
static bool gLantern = false;         // 引魂灯：自带常亮光环
static bool gWardNail = false;        // 镇魂钉：领地镇宅再强化
static int  gTalismanExtra = 0;       // 辟邪符匣：每种符上限 +
static bool gCapBell = false;         // 摄魂铃：收鬼冷却 -30%
static bool gRevive = false;          // 还魂香：替你死一次

static inline int PL(int k) { return Prog::PathLv(k); }
// 观：看破规律
static inline float SeeRuleRange() { return PL(PATH_SEE) >= 1 ? 70.0f : 0.0f; }   // 规律二字可见距离 +
static inline int   SeeMarkMax()   { return PL(PATH_SEE) >= 2 ? 2 : 1; }          // 同时标记数
static inline bool  SeeKeepFlaw()  { return PL(PATH_SEE) >= 3; }                  // 无面鬼始终留破绽
static inline int   SeeAskBonus()  { return PL(PATH_SEE) >= 4 ? 1 : 0; }          // 每日问询 +
static inline bool  SeeWarn()      { return PL(PATH_SEE) >= 5; }                  // 触发前预警
// 伏：符咒
static inline int   BindTalMax()    { return (PL(PATH_BIND) >= 1 ? 10 : 8) + gTalismanExtra; }  // 每种上限（含符匣）
static inline float BindCdMul()     { return PL(PATH_BIND) >= 2 ? 0.85f : 1.0f; } // 全符冷却
static inline int   BindUpNeed()    { return PL(PATH_BIND) >= 3 ? 2 : 3; }        // 升级所需次数
static inline float BindReach()     { return PL(PATH_BIND) >= 4 ? 24.0f : 0.0f; } // 贴符距离
static inline bool  BindDeepBanish() { return PL(PATH_BIND) >= 5; }               // 黄符额外降 1 级
// 驭：傀儡 / 鬼仆
static inline float RideLifeMul()  { return PL(PATH_RIDE) >= 1 ? 1.25f : 1.0f; }  // 阴气续航（消耗 -20%）
static inline int   RideGrpMax()   { return (PL(PATH_RIDE) >= 2 ? 3 : 2) + gPuppetExtra; }   // 同控上限（含傀儡丝）
static inline bool  RideBodyMove() { return PL(PATH_RIDE) >= 3; }                 // 傀儡时本体可动
static inline float RideDmgMul()   { return PL(PATH_RIDE) >= 4 ? 1.25f : 1.0f; }  // 鬼仆伤害
static inline int   RideCapBonus() { return PL(PATH_RIDE) >= 5 ? 10 : 0; }        // 收鬼成功率 +%
// 营：领地 / 生计
static inline int   HomeGather()    { return PL(PATH_HOME) >= 1 ? 1 : 0; }        // 采集产量 +
static inline float HomeHungerMul() { return PL(PATH_HOME) >= 2 ? 0.75f : 1.0f; } // 饥饿衰减
static inline int   HomeCapBonus()  { return PL(PATH_HOME) >= 3 ? 1 : 0; }        // 收容上限 +
static inline int   HomeSleepHeal() { return PL(PATH_HOME) >= 4 ? 70 : 40; }      // 睡眠回血
static inline float HomeAmbushP()   { return PL(PATH_HOME) >= 4 ? 0.20f : 0.35f; }// 睡袭概率
static inline bool  HomeWardPlus()  { return PL(PATH_HOME) >= 5 || gWardNail; }   // 镇宅光环增强（含镇魂钉）
// 符等级效果表 [符][等级1..3]
static const float TAL_BIND_T[3]  = { 3.0f, 4.5f, 6.0f };     // 困：禁锢时长
static const float TAL_SPD_T[3]   = { 3.0f, 5.0f, 7.0f };     // 速：疾行时长
static const int   TAL_AWAY_P[3]  = { 30, 50, 70 };           // 离：抹标记概率%
static const float TAL_LOW_T[3]   = { 10.0f, 20.0f, 30.0f };  // 黄：降级时长
static const int   TAL_RIVAL_P[3] = { 50, 65, 80 };           // 仇：挑拨成功率%
static Rectangle fsBtnRect = {};          // 暂停界面「全屏」按钮命中区（DrawUI 写入 / 主循环点击）

// 玩家是否泛舟中（渲染与移动共用：水面画在场景层之上，需单独补绘）
static bool CalcSailing() {
    if (!P.boat) return false;
    return W.TileAt((int)(P.x / TILE), (int)(P.y / TILE)) == Tile::Water;
}

// 鼠标屏幕坐标 -> 游戏内部分辨率坐标（整数倍放大 + 信箱居中，与窗口 blit 一致）
static Vector2 MouseGame() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int sc = (sw / VW < sh / VH) ? sw / VW : sh / VH;
    if (sc < 1) sc = 1;
    float dw = (float)(VW * sc), dh = (float)(VH * sc);
    Vector2 mp = GetMousePosition();
    return { (mp.x - (sw - dw) / 2.0f) / sc, (mp.y - (sh - dh) / 2.0f) / sc };
}
// 指针是否落在内部分辨率下的矩形内（UI 命中判定统一走这里）
static void CastGhostSkill(int gi);          // 前向声明（鬼仆技能，实现在下方）
static void GourdRelease(int i);             // 前向声明（葫芦放生）
static void GourdToBanner(int i);            // 前向声明（葫芦倒进镇鬼幡）
struct GhostAlly {
    CreatureKind kind = CreatureKind::GhostChild;
    float x = 0, y = 0;
    int hp = 1, maxHp = 1;        // 生命（被敌怪命中损耗）
    float dmg = 1.0f;             // 撕咬伤害（按摄魂幡复刻度缩放）
    float spd = 80.0f;            // 移动速度
    float radius = 6.0f;          // 碰撞半径
    bool flying = false;          // 飞行种（无视地形）
    float atkCd = 0, atkAnim = 0; // 攻击冷却 / 攻击前冲动画
    float animT = 0;
    float life = 120.0f;          // 阴气剩余秒数（耗尽退回幡中养伤，不是死亡）
    float maxLife = 120.0f;       // 阴气总时长（渲染比例用；按幡等级 120/150/180）
    unsigned char tier = 0;       // 原本的鬼等级 0..2（技能威力/献祭价值按它算）
    float fid = 1.0f;             // 复刻度 0.65~1.00（幡等级给底子，碎片精炼可提升）
    float hurtFlash = 0, iv = 0;  // 受击白闪 / 无敌帧
    int dir = 0;
    bool moving = false;
    // ---- 收鬼栏指挥（F 键面板：左键召唤出击 / 右键回收）----
    bool out = false;                // 是否奉命出击（false = 回阵位待命，不主动战斗）
    float cmdX = 0, cmdY = 0;        // 出击目标点（世界坐标）
    // ---- 升级系统（击杀积累：伤害/生命成长）----
    int level = 1;
    int exp = 0;
    // ---- 技能（镇鬼幡收服后可调用：每种鬼一门专属手段）----
    float skCd = 0;                 // 技能冷却剩余
    bool  temp = false;             // 临时召唤物（阴气耗尽即散）
    bool  invis = false;            // 伪装中（不参与战斗）
};
struct StoredGhost {                      // 葫芦里暂存的鬼（未改造，仍是"货物"）
    CreatureKind kind = CreatureKind::GhostChild;
    unsigned char tier = 0;
    float mul = 1.0f;
};
std::vector<GhostAlly> ghosts;               // 鬼仆（镇鬼幡收服；数量不限）
std::vector<StoredGhost> gourd;              // 葫芦内鬼（容量 = 葫芦等级 x 3）
int ghostBarPage = 0;                        // 鬼仆册当前页（0=鬼仆 1=葫芦）
// 鬼仆跟随阵位（相对玩家偏移，按槽位轮转）
static const float GHOST_SLOT[3][2] = { { -26.0f, 8.0f }, { 26.0f, 8.0f }, { 0.0f, 24.0f } };
static bool GB_Hit(int x, int y, int w, int h) {
    Vector2 m = MouseGame();
    return m.x >= x && m.x < x + w && m.y >= y && m.y < y + h;
}
// 鬼仆册行几何（绘制与点击共用，避免两处失配）
static inline int GhostRowY(int i) { return 38 + 50 + i * 26; }
// 鬼仆册点击：right=false 左键 / true 右键
static void GhostBarClick(bool right) {
    const int x0 = 170, w = 300, rwh = 26, rowsMax = 9;
    if (ghostBarPage == 0) {
        int n = (int)ghosts.size(); if (n > rowsMax) n = rowsMax;
        for (int i = 0; i < n; i++) {
            if (!GB_Hit(x0 + 6, GhostRowY(i), w - 12, rwh - 2)) continue;
            if (right) { ghosts[(size_t)i].out = !ghosts[(size_t)i].out; PlaySound(AU.pickup); }
            else       CastGhostSkill(i);
            return;
        }
    } else {
        int n = (int)gourd.size(); if (n > rowsMax) n = rowsMax;
        for (int i = 0; i < n; i++) {
            int ry = GhostRowY(i);
            if (!GB_Hit(x0 + 6, ry, w - 12, rwh - 2)) continue;
            if (GB_Hit(x0 + 168, ry + 3, 56, 18)) { GourdToBanner(i); return; }
            if (GB_Hit(x0 + 230, ry + 3, 56, 18) || right) { GourdRelease(i); return; }
            GourdToBanner(i);
            return;
        }
    }
}
#ifdef DEBUG_HEADLESS
// 无头验证：跳过开场卡与新手指导，保证自动截图画面干净
static bool introOn = false;              // 开场叙事卡（首次进入显示，任意键关闭）
static bool tutOn = false;                // 新手按键指导显示中（P 重唤）
#else
static bool introOn = true;               // 开场叙事卡（首次进入显示，任意键关闭）
static bool tutOn = true;                 // 新手按键指导显示中（P 重唤）
#endif
static bool tutMastered[18] = {};         // 教学键掌握状态（WASD/Shift/J/E/Tab/C/G/T/V/U/B/H/Esc/F/R）
static float tutAnimT = 0.0f;             // 教程浮入动画计时（P 重唤时归零，重新演示一遍）
static const char* TUT_MARK = "tut_seen.dat";   // 首玩标记：存在则不再自动弹教程（P 仍可手动重唤）
static unsigned gSeed = 20260818u;
static RenderTexture2D sceneRT = {};
static RenderTexture2D lightRT = {};
static RenderTexture2D reflRT = {};     // 场景快照（水面 Shader 倒影采样源，避免同纹理读写）

static int camX = 0, camY = 0;

// ---- 宠物系统（伙伴可升级：陪伴与助战积累经验）----
struct Pet {
    bool on = false;             // 是否已驯养
    int kind = 0;                // 0=兔子 1=小鹿
    float x = 0, y = 0;
    int dir = 0;
    float animT = 0;
    int hp = 60, maxHp = 60;
    float hurtFlash = 0;         // 受击白闪
    float iv = 0;                // 受击无敌帧
    float regenT = 0;
    // ---- 升级系统 ----
    int level = 1;               // 伙伴等级（maxHp/爪击成长）
    int xp = 0;                  // 距离下一级经验
    float atkCd = 0;             // 爪击冷却（伙伴参战）
};
static Pet PET;
static PetInfo PETINFO;          // creature.cpp 只读视图（每帧同步位置）
inline int PetNextXp(int lv) { return 30 + lv * 20; }   // 伙伴升级所需经验

// ---- 收鬼系统 ----
// 残魂：击败鬼怪后原地漂浮的魂火（12 秒后自行消散；V 键收服为鬼仆）
struct Soul {
    bool on = false;
    float x = 0, y = 0, age = 0;
    CreatureKind kind = CreatureKind::GhostChild;
    unsigned char tier = 0;       // 来源层级（决定可收性与复刻强度）
    float mul = 1.0f;             // 来源强度系数（鬼仆复刻基准）
};
static Soul souls[8];             // 固定池（零分配）

// ---- 收鬼特效（瞬时演出池，零分配）：牵引束 / 身份夺回环 / 护主盾环 ----
struct CapFx {
    bool on = false;
    int kind = 0;                 // 0=摄魂牵引束（玩家→目标） 1=身份夺回金环 2=护主盾环
    float x = 0, y = 0;           // 起点（玩家）
    float tx = 0, ty = 0;         // 终点（目标鬼；环类不用）
    float t = 0, dur = 0.45f;
};
static CapFx capFxs[6];
static void SpawnCapFx(int kind, float x, float y, float tx, float ty) {
    for (CapFx& f : capFxs)
        if (!f.on) {
            f.on = true; f.kind = kind;
            f.x = x; f.y = y; f.tx = tx; f.ty = ty;
            f.t = 0;
            f.dur = (kind == 1) ? 0.6f : 0.45f;
            return;
        }
}

// 鬼仆：收服后为我方作战的鬼（跟随玩家、自动撕咬敌怪；阴气限时存在）
// （GhostAlly / StoredGhost / ghosts / gourd / GHOST_SLOT 已在文件首部声明）
// ============================================================
// ---- 三阶驱鬼器物：桃木剑(击退) / 葫芦(暂储) / 镇鬼幡(收服为友方) ----
// 铁律：器物等级 N 只能压制等级 <= N 的鬼；鬼杀不死，只能被一层层限制
//   桃木剑 Lv N：击退 N 级及以下的鬼（剑越高，击退越远、硬直越久）
//   葫芦   Lv N：暂储 N 级及以下的鬼（容量 = 等级 x 3；用于搬运与交易）
//   镇鬼幡 Lv N：收服 N 级及以下的鬼（化为友方鬼仆，可调技能；数量不限）
// ============================================================
static inline int GourdCap() { return (int)P.gourdLv * 3; }

// ---- 鬼仆技能（镇鬼幡收服后可主动调用；每种鬼一门专属手段）----
enum class GSkill : unsigned char {
    Bite, Charge, Summon, Curse, Domain, Disguise,
    Tamper, Gamble, Contract, Knock, StealFace, Forge
};
struct SkillDef { const char* name; const char* desc; float cd; };
static const SkillDef SKILL_DEF[12] = {
    { "撕咬",   "扑咬最近的敌人",       6.0f },
    { "疾冲",   "朝指针突进撞伤沿途",   9.0f },
    { "召唤",   "召出一只鬼奴助战",    14.0f },
    { "落单咒", "范围内敌人迟滞",      12.0f },
    { "鬼域",   "范围内敌人持续失血",  16.0f },
    { "伪装",   "隐身脱战",            12.0f },
    { "篡改",   "范围内敌人力道大减",  12.0f },
    { "博弈",   "福祸各半",            10.0f },
    { "契约",   "耗木五，为你回血",    15.0f },
    { "敲门",   "震塌墙垣，重击敌人",  13.0f },
    { "夺面",   "范围内敌人溃逃",      14.0f },
    { "锻炉",   "为你披上护体",        18.0f },
};
static GSkill SkillOfKind(CreatureKind k) {
    switch (k) {
    case CreatureKind::GhostChild:  return GSkill::Bite;
    case CreatureKind::GhostTeen:   return GSkill::Charge;
    case CreatureKind::GhostAdult:  return GSkill::Summon;
    case CreatureKind::LoneGhost:   return GSkill::Curse;
    case CreatureKind::NineFace:    return GSkill::Domain;
    case CreatureKind::MimicAll:    return GSkill::Disguise;
    case CreatureKind::Faceless:    return GSkill::Tamper;
    case CreatureKind::GameGhost:   return GSkill::Gamble;
    case CreatureKind::DealGhost:   return GSkill::Contract;
    case CreatureKind::KnockGhost:  return GSkill::Knock;
    case CreatureKind::ManyFaces:   return GSkill::StealFace;
    default:                        return GSkill::Forge;     // 铁匠鬼 / 鬼域核心等
    }
}
static inline const char* SkillName(GSkill s) { return SKILL_DEF[(int)s].name; }
static inline float SkillCd(GSkill s)         { return SKILL_DEF[(int)s].cd;   }

// ============================================================
// ---- 摄魂幡三阶参数（等级差异深化：范围/冷却/磁吸/时限/复刻度全按等级区分）----
// Lv1 粗纸：范围64 冷却2.5s 磁吸90  时限120s 基础复刻度65%  残魂率65%  活鬼基率0.40
// Lv2 铜铃：范围80 冷却2.0s 磁吸120 时限150s 基础复刻度85%  残魂率85%  活鬼基率0.60
// Lv3 鎏金：范围96 冷却1.5s 磁吸150 时限180s 基础复刻度100% 残魂率100% 活鬼基率0.80
// ============================================================
static float CapRange(int lv)    { return lv >= 3 ? 96.0f  : (lv == 2 ? 80.0f  : 64.0f);  }  // 施法距离
static float CapCooldown(int lv) { return lv >= 3 ? 1.5f   : (lv == 2 ? 2.0f   : 2.5f);  }  // 收鬼冷却
static float CapMagnet(int lv)   { return lv >= 3 ? 150.0f : (lv == 2 ? 120.0f : 90.0f); }  // 残魂磁吸半径
static float CapGhostLife(int lv){ return lv >= 3 ? 180.0f : (lv == 2 ? 150.0f : 120.0f);}  // 鬼仆阴气时限
static float CapFidBase(int lv)  { return lv >= 3 ? 1.0f   : (lv == 2 ? 0.85f  : 0.65f); }  // 基础复刻度
static float CapSoulRate(int lv) { return lv >= 3 ? 1.0f   : (lv == 2 ? 0.85f  : 0.65f); }  // 残魂收服率（深层再减）
static float CapLiveBase(int lv) { return lv >= 3 ? 0.80f  : (lv == 2 ? 0.60f  : 0.40f); }  // 活鬼收服基率
static const char* CapName(int lv) { return lv >= 3 ? "鎏金摄魂幡" : (lv == 2 ? "铜铃摄魂幡" : "粗纸摄魂幡"); }

// ============================================================
// ---- NPC 系统（铁匠 + 幸存者：领地/工作台/猜拳/交易的人口）----
// ============================================================
struct Npc {
    bool on = true;
    int kind = 0;                 // 0=铁匠 1=幸存者
    int job = 0;                  // 幸存者职业：0医师 1猎人 2农夫 3工匠 4哨兵
    int state = 0;                // 幸存者：0游荡 1跟随 2入住（营地） 3营地门口候补（满了）
    int lv = 1;                   // 铁匠等级 1..3（决定可锻造的器物等级）
    bool hired = false;           // 铁匠：玩家献鬼永久雇来的（驻扎在雇佣地）
    float x = 0, y = 0;
    int dir = 0;
    float animT = 0;
    float wanderT = 0;            // 游荡换向计时
    float wx = 0, wy = 0;
};
static std::vector<Npc> npcs;     // 铁匠（1）+ 幸存者（野外游荡/跟随/入住）
static const char* JOB_NAME[5] = { "医师", "猎人", "农夫", "工匠", "哨兵" };
// 幸存者职业配色（渲染 NPC 时的衣服 tint）
static const Color JOB_TINT[5] = {
    { 190, 235, 200, 255 },   // 医师：素青
    { 210, 190, 150, 255 },   // 猎人：皮褐
    { 160, 200, 130, 255 },   // 农夫：草绿
    { 170, 180, 210, 255 },   // 工匠：铁灰
    { 220, 170, 170, 255 },   // 哨兵：赭红
};

// ---- 领地（玩家自建界碑 + 升级；同伴收容与无面鬼阈值的锚点）----
static bool campBuilt = false;         // 界碑已立（领地激活）
static int  campLv = 1;                // 领地等级 1..3（收容上限 / 每日问询次数 / 无面鬼阈值）
static Vector2 campCenter = {};        // 营地中心（= 石碑位置）
static int dayCampMark = -1;           // 上次结算产出的游戏日（猎人/农夫每日产出）
static inline int CampCap() { return 2 * campLv + HomeCapBonus(); }   // 收容上限随等级 2/4/6/8/10 人

// ---- 每日悬赏（留存锚点：每天都有三件小事值得做完，做完给实打实的补给）----
enum BountyType : int { BT_TREE = 0, BT_ROCK, BT_KILL, BT_GRAVE, BT_COOK, BT_CRAFT, BT_CAPTURE, BT_SURVIVE };
struct Bounty { int type = 0, need = 1, base = 0, done = 0; };
static Bounty gBounty[3];
static int  gBountyDay = -1;          // 已派发悬赏的游戏日（-1 = 尚未派发）
static int  gNCook = 0;               // 累计烤肉数（悬赏计数）
static int  gNCraft = 0;              // 累计炼器数
// ---- 世界随机事件 ----
static bool  gGlowRain = false;       // 荧雨夜：雨夜磷光养人（缓慢回血）
static bool  gTide = false;           // 鬼潮：今夜鬼物成群来袭
static float gTideT = 0;              // 鬼潮来袭节拍
static bool  gWasNight = false;       // 上一帧是否夜晚（侦测入夜时刻）
// ---- 选项（暂停菜单设置页；即时生效并随档保存）----
static bool  gOptDmg = true;          // 伤害飘字
static bool  gOptShake = true;        // 屏幕震动
static float gOptMaster = 1.0f;       // 总音量（音乐音量仍走 [ / ] 键）
static Rectangle optRows[6] = {};     // 暂停设置页六个可点行（DrawUI 写入，暂停分支读取）

// ============================================================
// 联机（局域网 UDP · 房主权威模拟；客人本地预测 + 快照插值）
// 房主：跑完整单机模拟 + 收客人输入 + 10Hz 广播快照
// 客人：本地移动预测（世界由种子重建 + 全量同步），生物/掉落/投射物走快照
// ============================================================
struct PeerView {
    bool on = false;
    unsigned addr = 0; unsigned short port = 0;
    int id = 0;
    char name[16] = "";
    float x = 0, y = 0, netTx = 0, netTy = 0;   // netT*：快照目标（平滑插值用）
    int dir = 0;
    bool moving = false, running = false, dead = false, atk = false;
    int hp = 100;
    float lastSeen = 0;                          // 房主：上次收包时刻（掉线剔除）
    unsigned char toolLv[6] = {};                // 客人装备（远程攻击判定用）
    int swordLv = 0, level = 1, hotSel = 0;
};
static bool  netOn = false;              // 联机会话激活
static bool  netHost = false;            // true=房主 / false=客人
static unsigned short netSock = (unsigned short)-1;
static unsigned netAddrHost = 0;         // 客人：房主地址
static unsigned short netPortHost = 0;
static int   netId = 0;                  // 自己 id（房主 0，客人 1..3）
static bool  netJoined = false;          // 客人：世界同步完成
static float netSendT = 0;               // 客人：状态上报节拍
static float netSnapT = 0;               // 房主：快照广播节拍
static float netLastRecv = 0;            // 客人：上次收包时刻
static unsigned netNextMobId = 1;        // 房主：生物快照 id 分配
static unsigned netDropSeq = 1;          // 房主：掉落 id 分配
static unsigned NetDropIdCb() { return netDropSeq++; }
static PeerView netPeers[4];             // [0]=房主；[1..3]=客人
static bool  titleJoin = false;          // 标题：输入房主 IP 模式
static char titleMsg[48] = "";          // 标题一次性提示
static float titleMsgT = 0;
static char  joinIp[16] = "";            // 标题：正在输入的 IP
// 前向声明（定义在文件后部，联机函数先行引用）
static bool titleHasSave = false;
static int  lastSaveDay = -1;
static bool SaveExists();

// 世界分块同步条目（客人收 'n' 包用）
struct NetObjEnt { unsigned char k; int tx, ty; unsigned char hp, hv; };
static std::vector<std::vector<NetObjEnt>> netChunks;
#ifdef DEBUG_AUTO_SHOT
static int fno = 0;   // 截图夹具帧号（文件级：Render 与主循环共用；标题分支带 continue，计数在循环开头推进）
#endif

// ---- 猜拳（游戏鬼）----
static int rpsState = 0;               // 0=无 1=等待出拳（按 1 石头 2 剪刀 3 布）
static int rpsGhostIdx = -1;           // 目标游戏鬼（mobs 下标）
static int rpsDraws = 0;               // 连续平局数（3 平局后它失去耐心离开）
static float rpsHintT = 0;             // 结果提示剩余时间
static const char* rpsHintTxt = "";

// ---- 交易（交易鬼）----
static int dealState = 0;              // 0=无 1=选择交易
static int dealGhostIdx = -1;          // 目标交易鬼（mobs 下标）

// ---- 铁匠（等级 / 锻造 / 献鬼雇佣）----
static bool smithMet = false;          // 已见过铁匠（对话只弹一次引导）
static bool smithOwned = false;        // 铁匠鬼已收服并交给铁匠（3 级工作台解锁）
static bool smithOpen = false;         // 铁匠铺面板（E 与铁匠交谈开启）
static int  smithIdx = -1;             // 正在交谈的铁匠（npcs 下标；-2 = 伴生铁匠鬼）
static int  gSmithGhostLv = 1;         // 伴生铁匠鬼自身等级（献鬼可升，等同铁匠等级）
static int  gSmithCompanionIdx = -1;   // 伴生铁匠鬼在 mobs 中的下标（-1 未上场；每帧刷新）
static const int SMITH_GHOST_IDX = -2; // smithIdx 哨兵值：正在和伴生铁匠鬼交谈
static int  forgeLv = 1;               // 锻造面板当前选择的器物等级（1..铁匠等级）
static float smithMsgT = 0;            // 面板内提示剩余时间
static char  smithMsg[64] = "";        // 面板内提示文字

// 当前交谈铁匠的等级（-2 = 伴生铁匠鬼）
static int SmithLevelOf() {
    if (smithIdx == SMITH_GHOST_IDX) return gSmithGhostLv;
    if (smithIdx >= 0 && smithIdx < (int)npcs.size()) return npcs[(size_t)smithIdx].lv;
    return 1;
}
// 当前交谈铁匠的落点（放工作台/特效用；伴生鬼跟随玩家）
static Vector2 SmithAnchorOf() {
    if (smithIdx == SMITH_GHOST_IDX) return { P.x + 26.0f, P.y - 34.0f };   // 玩家侧上方（右肩斜上）
    return W.smithPos;
}
// 设置等级
static void SmithSetLevel(int lv) {
    if (smithIdx == SMITH_GHOST_IDX) { gSmithGhostLv = lv; return; }
    if (smithIdx >= 0 && smithIdx < (int)npcs.size()) npcs[(size_t)smithIdx].lv = lv;
}
static bool SmithIsCompanion() { return smithIdx == SMITH_GHOST_IDX; }

// 锻造价目：三类器物 x 三个等级（木/石/铁/萤晶/碎片/血月之心）
struct ForgeCost { int wood, stone, iron, crystal, shard, heart; };
static const ForgeCost FORGE_COST[3][3] = {
    //  桃木剑              葫芦                镇鬼幡            （Lv1）
    { { 100,  0, 0, 0, 0, 0 }, { 60, 20, 0, 0, 0, 0 }, { 80, 40, 0, 0, 0, 0 } },
    //  Lv2
    { { 150,  0, 5, 0, 0, 0 }, { 100, 0, 3, 0, 0, 0 }, { 150, 0, 5, 2, 0, 0 } },
    //  Lv3
    { { 200,  0,10, 3, 3, 1 }, { 150, 0, 8, 5, 0, 0 }, { 200, 0,10, 5, 5, 0 } },
};

// ---- 鬼仆册（F）：0=镇鬼幡鬼仆  1=葫芦暂存 ----
// ghostBarPage 已前向声明在文件首部

// ---- 其他跨帧计时 ----
static float campHealT = 0;            // 营地医师回血节拍
static float captureCd = 0;            // 收鬼冷却（活体收鬼失败后可拉扯重试）
static bool bannerOpen = false;        // B 键幡面板：等级/成功率/复刻度/容量全参数
static bool playerAttackedFrame = false;   // 本帧是否出过手（鬼游戏「禁止出手」规则用）

// ============================================================
// ---- 大鬼域（鬼游戏·核心）：完全独立的封闭空间 ----
// 见到「鬼游戏」核心即被拽入；幽蓝结界圈出范围，未收服域主前一步也出不去。
// 域内持续放出「代理游戏鬼」——它们随时间漂移、越来越多，靠近便选中玩家开局。
// 被多只同时选中，就要同时应付多场小游戏；输任何一场，即死。
// ============================================================
static bool  domainPurged = false;     // 域主已被收服（鬼域永久消散，不再开局）
static bool  gdOn = false;             // 玩家身处鬼域（独立空间，锁死）
static float gdCX = 0, gdCY = 0;       // 域心（世界坐标）
static float gdR = 200.0f;             // 域半径（结界）
static float gdT = 0;                  // 已在域内的时间
static int   gdOwnerIdx = -1;          // 域主（鬼游戏核心）在 mobs 中的下标
static float gdSpawnT = 0.0f;          // 下一个代理游戏鬼的生成倒计时
static int   gdCap = 1;                // 代理上限（随时间增长）
static int   gdProxyN = 0;             // 累计生成代理数（统计）
static float gdInfoT = 0;              // 域内提示剩余时间
static char  gdInfo[80] = "";          // 域内提示文字
static float gdSealT = 0;              // 撞结界提示节拍

// 代理游戏鬼：域内漂移的小鬼，选中玩家后发起一场小游戏
enum GdGame : unsigned char { GD_RPS = 0, GD_REACT, GD_MEMORY, GD_GAME_N };
struct GdProxy {
    float x = 0, y = 0;
    float animT = 0;
    float driftA = 0;            // 漂移方向
    float driftT = 0;            // 换向倒计时
    float selCd = 1.2f;          // 距下次尝试选中的冷却
    bool  active = false;        // 已与玩家对局中（不再漂移）
    unsigned char game = GD_RPS; // 它擅长的游戏
};
static std::vector<GdProxy> gdProxies;

// 进行中的小游戏（可多个并行：三套键位互不冲突）
struct GdChallenge {
    bool  on = false;
    unsigned char game = GD_RPS;
    int   proxy = -1;            // 发起者（gdProxies 下标）
    float life = 6.0f;           // 剩余时间（归零判负）
    float maxLife = 6.0f;
    float t = 0;
    // 猜拳：它露的破绽（将要出的手势）+ 是否诈你
    int   tell = 0;
    bool  lie = false;
    // 反应：指针扫动 + 目标区
    float sweep = 0.0f;
    float zone = 0.5f;
    float zoneW = 0.17f;
    float sweepSpd = 1.0f;
    // 记方位：箭头序列（0上 1下 2左 3右）
    unsigned char seq[3] = { 0, 0, 0 };
    int   seqLen = 2;
    int   seqIn = 0;
    float showT = 1.7f;          // 展示阶段（展示中不可输入）
    float flashT = 0;            // 输错反馈
};
static GdChallenge gdChal[4];    // 最多同时 4 场
static int   gdWin = 0, gdFail = 0;   // 域内胜负统计

// 残魂生成（击败敌对鬼怪时调用；池满自动丢弃）
static void SpawnSoulAt(CreatureKind kind, float x, float y, unsigned char tier, float mul) {
    for (Soul& s : souls)
        if (!s.on) {
            s.on = true; s.x = x; s.y = y; s.age = 0;
            s.kind = kind; s.tier = tier; s.mul = mul;
            return;
        }
}

// ---- 活跃物体列表（性能优化：被击晃动/待再生浆果才逐帧更新，免全量遍历）----
static std::vector<int> objActive;
static void WakeObj(int idx) {
    for (int i : objActive) if (i == idx) return;
    objActive.push_back(idx);
}

// ---- Boss（血月尸王）追踪 ----
static bool bossSlain = false;   // 击杀成就标记
static float bossHowlT = 0;      // Boss 招式警示字幕计时
static char bossHowlTxt[32] = "";

// ---- 地牢进出过渡 ----

// 实际攻击伤害：基础 2 + 等级成长(每 2 级 +1) + 铁剑（Lv2 再 +2 / Lv3 血月大剑再 +3 = +8）
static inline int PlayerDmg() {
    int d = 2 + (P.level - 1) / 2;
    if (P.toolLv[0]) d += 3 + (P.toolLv[0] > 1 ? 2 : 0) + (P.toolLv[0] > 2 ? 3 : 0);
    return d;
}
// 护甲减伤系数：0=1.0 / 1=铁甲 0.62 / 2=萤晶甲 0.45
static inline float ArmorMul() {
    return P.armorLv == 1 ? 0.62f : P.armorLv == 2 ? 0.45f : 1.0f;
}
// 升级所需经验：20 + 级×15
static inline int XpNext(int lvl) { return 20 + lvl * 15; }

// ---------------- 怪物投射物（固定池，零分配）：酸弹 / 火球 ----------------
struct Spit { float x, y, vx, vy, life; bool on; bool fire; };
static Spit spits[32] = {};

void SpawnSpit(float x, float y, float vx, float vy) {
    for (Spit& s : spits)
        if (!s.on) {
            s.on = true; s.fire = false; s.x = x; s.y = y; s.vx = vx; s.vy = vy; s.life = 1.6f;
            return;
        }
}

void SpawnFireball(float x, float y, float vx, float vy) {
    for (Spit& s : spits)
        if (!s.on) {
            s.on = true; s.fire = true; s.x = x; s.y = y; s.vx = vx; s.vy = vy; s.life = 2.2f;
            return;
        }
}

// ---------------- 玩家箭矢（猎弓，固定池 24，零分配） ----------------
struct Arrow { float x, y, vx, vy, life; bool on; };
static Arrow arrows[24] = {};

static void FireArrow(float x, float y, float vx, float vy) {
    for (Arrow& a : arrows)
        if (!a.on) {
            a.on = true; a.x = x; a.y = y; a.vx = vx; a.vy = vy; a.life = 1.1f;
            return;
        }
}

// ---------------- 梗成就系统 ----------------
struct Achv { const char* name; const char* desc; bool got; };
static Achv achvs[11] = {
    /* 0 */ { "撸树一时爽", "一直撸树一直爽，万物始于树",     false },
    /* 1 */ { "干饭人",     "干饭不积极，思想有问题",         false },
    /* 2 */ { "火的力量",   "点亮第一堆篝火，文明 +1",        false },
    /* 3 */ { "命悬一线",   "从鬼门关爬回来一次，贴脸对线",   false },
    /* 4 */ { "万物皆可盘", "累计采集 50 次资源，盘学家认证", false },
    /* 5 */ { "怪物清道夫", "击败 10 只怪物，环保卫士",       false },
    /* 6 */ { "天选之子",   "开启遗迹宝箱，建议买张彩票",     false },
    /* 7 */ { "月下漫步",   "黑夜中裸奔 60 秒，最亮的仔",     false },
    /* 8 */ { "六边形战士", "等级达到 5 级，全属性进化",      false },
    /* 9 */ { "屠神者",     "净化鬼游戏核心，神挡杀神",       false },
    /* 10 */{ "敕令收鬼",   "收服第一只鬼仆",                 false },
};
static int achvShowIdx = -1;        // 当前弹窗成就
static float achvShowT = 0;         // 弹窗剩余时间

// 旧 11 项梗成就已并入 progress 模块的 30 项成就体系（凡/奇/秘/绝 + 残影）。
// 判定统一走 Prog::Tick；这个函数只留调用点，免得各处调用要改。
static void CheckAchvs() {}

// 元系统（命途 / 成就 / 图鉴 / 残影）每帧推进：把状态打包喂给 progress 模块
static void ProgTick(float dt);
static void BookKeys();          // 手册总册内的按键（实现见手册渲染处）

// ---------------- 天气：降雨 ----------------
struct RainDrop { float x, y, spd; };     // 屏幕空间雨滴（固定数组零分配）
static RainDrop rainDrops[150];
static bool rainSeeded = false;
static float rainAmt = 0.0f;              // 当前雨强 0..1（平滑渐变）
static float rainTarget = 0.0f;
static float weatherT = 35.0f;            // 距下次天气切换的秒数

// ---------------- 落叶（固定数组零分配；雨天水花/涟漪全部由 WaterRenderer 着色器程序化）----------------
struct Leaf { float x, y, vy, ph, ground, fade; bool on; };   // 飘落树叶
static Leaf leaves[48] = {};
static float leafT = 0.0f;                                    // 落叶生成计时

// 可见项（y 深度排序绘制）
struct VisItem { float y; int type; int idx; };   // 0=玩家 1=物体 2=生物 3=宠物 4=鬼仆
static std::vector<VisItem> vis;

// ---------------- 水面倒影源收集（u_objects[24]）----------------
// xy=世界位置 z=底部高度H w=宽度；仅近岸实体（landShore 标记）才产生倒影
// 修复倒影缺失：
//  1) 外扩 160px（倒影向下延伸最大约 2.8H，视口上缘外的物体倒影仍可见）；
//  2) 全量收集候选后按"离玩家距离"就近优先取前 24，
//     替代旧的瓦片扫描前 16 个——避免林岸倒影成片缺失。
struct ReflCand { float x, y, h, w, d2; };
static ReflCand reflCands[128];
static int reflCandN = 0;

static void CollectAdd(float x, float y, float h, float w) {
    if (reflCandN >= 128) return;
    float dx = x - P.x, dy = y - P.y;
    reflCands[reflCandN].x = x; reflCands[reflCandN].y = y;
    reflCands[reflCandN].h = h; reflCands[reflCandN].w = w;
    reflCands[reflCandN].d2 = dx * dx + dy * dy;
    reflCandN++;
}

static int CollectReflectObjects(float* a) {
    reflCandN = 0;
    CollectAdd(P.x, P.y - 4, 24.0f, 18.0f);    // 玩家（数组首位，不参与排序）
    const int m = 160;                                     // 视口外扩（覆盖倒影下延 2.8H）
    int tx0 = (camX - m) / TILE, ty0 = (camY - m) / TILE;
    int tx1 = (camX + VW + m) / TILE, ty1 = (camY + VH + m) / TILE;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    if (tx1 > MAP_W - 1) tx1 = MAP_W - 1;
    if (ty1 > MAP_H - 1) ty1 = MAP_H - 1;
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (!W.landShore[(size_t)ty * MAP_W + tx]) continue;
            int idx = W.ObjIndexAt(tx, ty);
            if (idx < 0) continue;
            const WorldObj& o = W.objs[(size_t)idx];
            float h = 0, wd = 0;
            switch (o.kind) {
            case ObjKind::Tree:     h = 42; wd = 30; break;
            case ObjKind::Rock:     h = 14; wd = 20; break;
            case ObjKind::Campfire: h = 14; wd = 16; break;
            case ObjKind::Stump:    h = 10; wd = 14; break;
            default: continue;
            }
            CollectAdd(tx * 16.0f + 8.0f, ty * 16.0f + 14.0f, h, wd);
        }
    for (const Creature& c : mobs) {
        int ctx = (int)(c.x / TILE), cty = (int)(c.y / TILE);
        if (ctx < 0 || cty < 0 || ctx >= MAP_W || cty >= MAP_H) continue;
        if (!W.landShore[(size_t)cty * MAP_W + ctx]) continue;
        CollectAdd(c.x, c.y - 4, 18.0f, 18.0f);
    }
    // 就近优先插入排序（跳过索引 0 的玩家；候选 ≤128，开销可忽略）
    for (int i = 2; i < reflCandN; i++) {
        ReflCand key = reflCands[i];
        int j = i - 1;
        while (j >= 1 && reflCands[j].d2 > key.d2) {
            reflCands[j + 1] = reflCands[j];
            j--;
        }
        reflCands[j + 1] = key;
    }
    int n = 0;
    for (int i = 0; i < reflCandN && n < WaterRenderer::MAX_OBJECTS; i++) {
        const ReflCand& c = reflCands[i];
        a[n*4] = c.x; a[n*4+1] = c.y; a[n*4+2] = c.h; a[n*4+3] = c.w;
        n++;
    }
    return n;
}

// ---------------- 前置声明 ----------------
void NewGame(unsigned seed);
void KillPlayer();
static void GdExit();                      // 鬼域崩塌（MobDied 收服域主时也要用）
void DoAttack();
void HitObject(int idx);
void UpdateGame(float dt);
static std::vector<WorldObj>& CurObjs();   // 上下文路由（地表/地牢当前层）

// ---------------- 工具：粒子 / 飘字 ----------------

static void SpawnParticles(float x, float y, int n, unsigned char r, unsigned char g, unsigned char b, float spd) {
    for (int i = 0; i < n; i++) {
        if (parts.size() >= 512) return;
        float a = ((float)rand() / RAND_MAX) * 6.283185f;
        float s = spd * (0.4f + 0.6f * ((float)rand() / RAND_MAX));
        Particle p;
        p.x = x; p.y = y;
        p.vx = cosf(a) * s; p.vy = sinf(a) * s - 40.0f;
        p.life = 0.3f + 0.4f * ((float)rand() / RAND_MAX);
        p.maxLife = p.life;
        p.r = r; p.g = g; p.b = b;
        parts.push_back(p);
    }
}

// creature.cpp 回调：受击碎屑
void SpawnHitParticles(float x, float y, unsigned char r, unsigned char g, unsigned char b, int n) {
    SpawnParticles(x, y, n, r, g, b, 95.0f);
}

// 硬边像素圆（酸弹等小物件；按行填充：每行一次 DrawRectangle，比逐像素快约一个数量级）
// 半径 -> 半宽查表（半径 0..31；按需扩展）。省掉每帧 60+ 次 sqrtf（DrawPixCircle 是热点）
static float gPixHalfW[32];
static bool  gPixHalfWInited = false;
static void InitPixHalfW() {
    if (gPixHalfWInited) return;
    gPixHalfWInited = true;
    for (int r = 0; r < 32; r++) gPixHalfW[r] = sqrtf((float)r);
}
static void DrawPixCircle(float cx, float cy, float r, Color c) {
    int R = (int)(r + 0.5f);
    if (R < 0) return;
    int icx = (int)cx, icy = (int)cy;
    float r2 = r * r;
    int cap = (R < 31) ? R : 31;
    for (int dy = -R; dy <= R; dy++) {
        int ady = dy < 0 ? -dy : dy;
        float d2 = (float)ady * (float)ady;
        if (d2 > r2) continue;
        int w;
        if (ady <= cap) w = (int)(gPixHalfW[ady] + 0.5f);   // 查表
        else            w = (int)sqrtf(r2 - d2);
        if (w < 0) w = 0;
        DrawRectangle(icx - w, icy + dy, w * 2 + 1, 1, c);
    }
}

static void FloatText(float x, float y, const char* txt, unsigned char r, unsigned char g, unsigned char b) {
    if (!gOptDmg && r == 255 && g == 255 && b == 255) return;   // 设置：关闭白色伤害飘字
    if (dmgs.size() >= 32) dmgs.erase(dmgs.begin());
    DmgText d;
    d.x = x; d.y = y; d.life = 0.8f;
    strncpy(d.txt, txt, 47); d.txt[47] = 0;    // 容量 48：装得下 15 个汉字（旧 11 字节会把中文拦腰截断）
    d.r = r; d.g = g; d.b = b;
    dmgs.push_back(d);
}

// ---------------- 等级经验 ----------------

// ---- 元系统 UI 状态（命途手册 / 剧情卷轴）----
static bool  bookOpen = false;          // 手册总册（L 开关）
static int   bookPage = 0;              // 0 命途  1 成就  2 图鉴  3 残卷
static int   bookSel  = 0;              // 页内光标
static bool  scrollOn = false;          // 剧情卷轴浮层（半透明 + 打字机）
static float scrollT = 0;               // 卷轴存在时长
static int   scrollKind = 0;            // 0=章节 1=结局
static int   scrollIdx = 0;             // 章节号 / 结局号
static int   scrollChars = 0;           // 打字机已显示字符数
static float scrollCharT = 0;           // 打字机节拍
static bool  scrollDone = false;        // 文本已打完（提示按键关闭）
static float insightFlashT = 0;         // 悟点获得时的高亮闪

// 打开剧情卷轴
static void OpenScroll(int kind, int idx) {
    scrollOn = true; scrollT = 0; scrollKind = kind; scrollIdx = idx;
    scrollChars = 0; scrollCharT = 0; scrollDone = false;
}

// 取当前卷轴全文
static const char* ScrollText() {
    return scrollKind == 0 ? Prog::ChapterText(scrollIdx) : Prog::EndingText(scrollIdx);
}
static const char* ScrollTitle() {
    return scrollKind == 0 ? Prog::ChapterTitle(scrollIdx) : Prog::EndingTitle(scrollIdx);
}

// 获得经验：升级回复全部生命、提升上限与手电半径（渲染层按等级取值）
// 每升一级 +1「悟」——悟点投进四条命途，这是升级唯一的意义
static void AddXp(int n) {
    if (P.dead) return;
    P.xp += n;
    while (P.xp >= XpNext(P.level)) {
        P.xp -= XpNext(P.level);
        P.level++;
        P.maxHp += 10;
        P.hp = P.maxHp;
        Prog::GainInsight(1);                       // 每级 +1 悟
        Prog::Push(PEV_LEVELUP, P.level, 1);
        PlaySound(AU.craft);
        SpawnParticles(P.x, P.y - 14, 16, 255, 225, 120, 120);
        FloatText(P.x, P.y - 40, "升级了!", 255, 235, 130);
    }
}

// ---------------- 生物事件回调（creature.cpp 调用） ----------------

static void GiveTalisman(int t, int n);      // 前向声明（MobDied/掘坟产出符咒用）

// 声音系鬼发声被听到：给出方位提示（屏幕边缘朱砂箭头）+ 提示音
static float wailHintT = 0;
static float wailHintX = 0, wailHintY = 0;
// 敲门鬼计时（声明提前：TrySleep 被袭时也要用到）
static float knockT = 25.0f;          // 距下一次敲门的倒计时
static float knockSummonT = 0.0f;     // 被盯上时，距下一次鬼来袭
void WailHeard(float x, float y) {
    wailHintX = x; wailHintY = y;
    wailHintT = 2.2f;
    PlaySound(AU.roar);
    SpawnParticles(x, y - 10, 10, 180, 60, 70, 90);
}

// 睡眠：靠近草席床按 E，快进到次日早晨并回血；**睡眠期间有被袭风险**
static void TrySleep() {
    if (P.dead) return;
    if (netOn && !netHost) { FloatText(P.x, P.y - 40, "联机模式只有房主能就寝", 255, 200, 120); return; }
    if (!IsNight(gameTime)) { FloatText(P.x, P.y - 34, "天还没黑", 255, 200, 120); return; }
    if (W.NearBed(P.x, P.y, 40.0f) < 0) {
        FloatText(P.x, P.y - 34, "需要草席床", 255, 160, 120);
        return;
    }
    // 快进到次日早晨（DayPhase 0.25）
    float ph = DayPhase(gameTime);
    gameTime += ((1.0f - ph) + 0.25f) * DAY_LEN;
    P.hp += HomeSleepHeal();
    if (P.hp > P.maxHp) P.hp = P.maxHp;
    P.hunger -= 20;
    if (P.hunger < 0) P.hunger = 0;
    PlaySound(AU.eat);
    SpawnParticles(P.x, P.y - 14, 10, 150, 255, 170, 120);
    FloatText(P.x, P.y - 34, "一觉到天明", 150, 235, 190);
    // 被袭风险 35%：醒来时它已经在屋里了
    if ((rand() % 100) < (int)(HomeAmbushP() * 100.0f)) {
        P.stalked = true;
        knockSummonT = 0.0f;
        PlaySound(AU.roar);
        FloatText(P.x, P.y - 52, "醒来时，它站在床边", 226, 40, 46);
    }
}

// ---------------- 铁匠交互（工作台授予 / 升级 / 铁匠鬼） ----------------
// 找铁匠屋内的工作台（等级存于 hp 字段；无 = -1）
[[maybe_unused]] static int SmithBenchLv() {
    int wi = W.NearWorkbench(W.smithPos.x, W.smithPos.y, 120.0f);
    if (wi < 0) return 0;
    return (int)W.objs[(size_t)wi].hp;
}
static void SmithPlaceOrUpgradeBench(int lv) {
    Vector2 anch = SmithAnchorOf();
    int wi = W.NearWorkbench(anch.x, anch.y, 120.0f);
    if (wi >= 0) {
        W.objs[(size_t)wi].hp = (unsigned char)lv;
        SpawnParticles(W.objs[(size_t)wi].tx * 16.0f + 8, W.objs[(size_t)wi].ty * 16.0f + 8,
                       14, 255, 224, 138, 110);
        return;
    }
    // 铁匠身旁找空位放置
    int bx = (int)(anch.x / 16.0f), by = (int)(anch.y / 16.0f);
    for (int r = 1; r <= 3; r++)
        for (int oy = -r; oy <= r; oy++)
            for (int ox = -r; ox <= r; ox++) {
                if (W.PlaceWorkbench(bx + ox, by + oy, lv)) {
                    SpawnParticles((bx + ox) * 16.0f + 8, (by + oy) * 16.0f + 8, 14, 255, 224, 138, 110);
                    return;
                }
            }
}

// ---- 面板内提示（2.6 秒：木料不足 / 鬼等级不够 等反馈）----
static void SmithMsg(const char* t) {
    strncpy(smithMsg, t, 63); smithMsg[63] = 0;
    smithMsgT = 2.6f;
}
// 手上有几只可作为代价的鬼（葫芦 + 鬼仆），按等级统计
static int GhostStock(int tier) {          // tier: 0..2（= 1/2/3 级鬼）
    int n = 0;
    for (const StoredGhost& g : gourd) if ((int)g.tier >= tier) n++;
    for (const GhostAlly& g : ghosts)  if (g.tier >= tier) n++;
    return n;
}
// 献出一只鬼（优先葫芦里等级最低但够格的，其次鬼仆）→ 返回其等级，无则 -1
static int ConsumeGhostForSmith(int tier) {
    int best = -1, bestTier = 99;
    for (size_t i = 0; i < gourd.size(); i++)
        if ((int)gourd[i].tier >= tier && (int)gourd[i].tier < bestTier) { best = (int)i; bestTier = gourd[i].tier; }
    if (best >= 0) { int t = gourd[(size_t)best].tier; gourd.erase(gourd.begin() + best); return t; }
    size_t gi = (size_t)-1; bestTier = 99;
    for (size_t i = 0; i < ghosts.size(); i++)
        if ((int)ghosts[i].tier >= tier && (int)ghosts[i].tier < bestTier) { gi = i; bestTier = ghosts[i].tier; }
    if (gi != (size_t)-1) { int t = ghosts[gi].tier; ghosts.erase(ghosts.begin() + (long)gi); return t; }
    return -1;
}

// 锻造：item 0=桃木剑 1=葫芦 2=镇鬼幡
static void SmithForge(int item, int lv) {
    if (smithIdx < 0 && smithIdx != SMITH_GHOST_IDX) return;
    int slv = SmithLevelOf();
    if (lv < 1 || lv > slv) { SmithMsg("这位铁匠打不出这个等级"); return; }
    const ForgeCost& c = FORGE_COST[lv - 1][item];
    if (P.wood < c.wood)     { SmithMsg(TextFormat(L10N("木料不足（需 %d）"), c.wood)); return; }
    if (P.stone < c.stone)   { SmithMsg(TextFormat(L10N("石料不足（需 %d）"), c.stone)); return; }
    if (P.iron < c.iron)     { SmithMsg(TextFormat(L10N("铁矿不足（需 %d）"), c.iron)); return; }
    if (P.crystal < c.crystal) { SmithMsg(TextFormat(L10N("萤晶不足（需 %d）"), c.crystal)); return; }
    if (P.gemShard < c.shard)  { SmithMsg(TextFormat(L10N("符文碎片不足（需 %d）"), c.shard)); return; }
    if (P.heart < c.heart)     { SmithMsg("缺少血月之心"); return; }
    P.wood -= c.wood; P.stone -= c.stone; P.iron -= c.iron;
    P.crystal -= c.crystal; P.gemShard -= c.shard; P.heart -= c.heart;

    unsigned char* dst = (item == 0) ? &P.swordLv : (item == 1 ? &P.gourdLv : &P.captureLv);
    if (*dst >= (unsigned char)lv) { SmithMsg("你手上这物件已不差于它"); return; }
    *dst = (unsigned char)lv;
    if (item == 0 && P.toolLv[0] < (unsigned char)lv) P.toolLv[0] = (unsigned char)lv;   // 剑即武器槽
    PlaySound(AU.craft);
    SpawnParticles(P.x, P.y - 16, 18, 255, 224, 138, 120);
    static const char* NMS[3] = { "辟邪桃木剑", "收鬼葫芦", "镇鬼幡" };
    FloatText(P.x, P.y - 44, TextFormat(L10N("%s Lv%d 到手!"), L10N(NMS[item]), lv), 255, 224, 138);
    SmithMsg(TextFormat(L10N("铁匠锤下生花：%s Lv%d"), L10N(NMS[item]), lv));
}

// 献鬼：mode 0=把这位铁匠升一级  1=永久雇一位同等级铁匠
static void SmithOfferGhost(int mode) {
    if (smithIdx < 0 && smithIdx != SMITH_GHOST_IDX) return;
    int slv = SmithLevelOf();
    int need = slv - 1;                        // 需献上的鬼等级（0=1级鬼 … 2=3级鬼）
    if (slv >= 3 && mode == 0) { SmithMsg("他的手艺已到顶了"); return; }
    if (GhostStock(need) <= 0) {
        SmithMsg(TextFormat(L10N("需献上一只 %d 级鬼"), need + 1));
        return;
    }
    ConsumeGhostForSmith(need);
    PlaySound(AU.capture);
    Vector2 anch = SmithAnchorOf();
    if (mode == 0) {
        SmithSetLevel(slv + 1);
        forgeLv = slv + 1;
        SpawnParticles(anch.x, anch.y - 20, 22, 255, 224, 138, 140);
        FloatText(anch.x, anch.y - 40, TextFormat(L10N("铁匠升至 Lv%d!"), slv + 1), 255, 224, 138);
        SmithMsg(TextFormat(L10N("炉火更旺了：铁匠 Lv%d（可造 %d 级器物）"), slv + 1, slv + 1));
    } else {
        // 永久雇佣：在玩家身侧落户，等级 = 当前铁匠等级
        Npc n;
        n.kind = 0; n.lv = slv; n.hired = true; n.state = 2;
        for (int r = 1; r <= 4; r++) {
            float ang = (float)(rand() % 360) * 0.0174533f;
            float nx = P.x + cosf(ang) * (18.0f + 10.0f * r);
            float ny = P.y + sinf(ang) * (18.0f + 10.0f * r) * 0.7f;
            if (W.CircleFree(nx, ny, 7.0f, false)) { n.x = nx; n.y = ny; break; }
            n.x = P.x + 20.0f; n.y = P.y;
        }
        npcs.push_back(n);
        SpawnParticles(n.x, n.y - 20, 20, 255, 224, 138, 130);
        FloatText(n.x, n.y - 40, TextFormat(L10N("雇下一位 Lv%d 铁匠"), n.lv), 255, 224, 138);
        SmithMsg(TextFormat(L10N("他在此地落了户：Lv%d 铁匠（永久）"), n.lv));
    }
}

static void SmithTalk(int idx) {
    smithMet = true;
    // ---- 收服了野外铁匠鬼：带回来合体 → 直接升 3 级（鬼界合成解锁）----
    // （铁匠本就是铁匠鬼：找回同族老伙计，炉火自通鬼界之火）
    if (!smithOwned) {
        for (size_t i = 0; i < ghosts.size(); i++) {
            if (ghosts[i].kind == CreatureKind::SmithGhost) {
                ghosts.erase(ghosts.begin() + (long)i);
                smithOwned = true;
                smithIdx = idx;                      // 先定位本次交谈的铁匠，再让工作台落在它身旁
                SmithPlaceOrUpgradeBench(3);
                SmithSetLevel(3);
                gSmithGhostLv = 3;
                PlaySound(AU.craft);
                TextCopy(bossHowlTxt, "铁匠鬼找回了他的老伙计");
                bossHowlT = 3.0f;
                FloatText(P.x, P.y - 44, "鬼界合成已解锁!", 255, 224, 138);
                break;
            }
        }
    }
    smithIdx = idx;
    int slv = SmithLevelOf();
    if (forgeLv > slv) forgeLv = slv;
    if (forgeLv < 1) forgeLv = 1;
    smithOpen = true;
    craftOpen = false; bannerOpen = false; ghostBarOpen = false; talBarOpen = false; bookOpen = false;   // 与手册总册互斥，任何时刻只开一个
    PlaySound(AU.pickup);
}

// 铁匠铺面板内的按键（1..5 选择 / Z X 调等级 / E 关闭）
static void SmithPanelKeys() {
    if (!smithOpen) return;
    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) { smithOpen = false; return; }
    if (smithIdx < 0 || smithIdx >= (int)npcs.size()) { smithOpen = false; return; }
    int slv = npcs[(size_t)smithIdx].lv;
    if (IsKeyPressed(KEY_Z) && forgeLv > 1) forgeLv--;
    if (IsKeyPressed(KEY_X) && forgeLv < slv) forgeLv++;
    if (IsKeyPressed(KEY_ONE))   SmithForge(0, forgeLv);
    if (IsKeyPressed(KEY_TWO))   SmithForge(1, forgeLv);
    if (IsKeyPressed(KEY_THREE)) SmithForge(2, forgeLv);
    if (IsKeyPressed(KEY_FOUR))  SmithOfferGhost(0);
    if (IsKeyPressed(KEY_FIVE))  SmithOfferGhost(1);
}

// ---------------- 猜拳结算（游戏鬼：赢=奖励 / 平=再猜 / 输=被袭） ----------------
static void RpsResolve(int player) {   // player 0石头 1剪刀 2布
    if (rpsGhostIdx < 0 || rpsGhostIdx >= (int)mobs.size()) { rpsState = 0; return; }
    Creature& c = mobs[(size_t)rpsGhostIdx];
    if (c.state == AState::Dead || c.kind != CreatureKind::GameGhost) { rpsState = 0; return; }
    int ghost = rand() % 3;
    static const char* RPS[3] = { "石头", "剪刀", "布" };
    int diff = (player - ghost + 3) % 3;   // 0平 1玩家赢 2玩家输
    PlaySound(AU.pickup);
    if (diff == 1) {                        // 玩家赢：它愿赌服输，留下谢礼离开
        rpsHintTxt = "你赢了! 它留下了谢礼";
        rpsHintT = 2.5f;
        SpawnSoulAt(CreatureKind::GameGhost, c.x, c.y - 4, c.tier, c.dmgMul);
        GiveTalisman(rand() % 5, 2);
        SpawnParticles(c.x, c.y - 10, 14, 200, 255, 200, 110);
        c.state = AState::Dead; c.deadFade = 0.4f;           // 消散（不走 MobDied：奖励已给）
        rpsState = 0;
    } else if (diff == 0) {                 // 平局：再猜（3 平局后它失去耐心）
        rpsDraws++;
        if (rpsDraws >= 3) {
            rpsHintTxt = "它觉得没意思，走了";
            rpsHintT = 2.5f;
            GiveTalisman(rand() % 5, 1);
            c.state = AState::Dead; c.deadFade = 0.4f;
            rpsState = 0;
        } else {
            rpsHintTxt = TextFormat(L10N("平局! 它出了%s，再来"), L10N(RPS[ghost]));
            rpsHintT = 2.0f;
        }
    } else {                                // 玩家输：触犯规则，它开始追杀
        rpsHintTxt = TextFormat(L10N("你输了! 它出了%s"), L10N(RPS[ghost]));
        rpsHintT = 2.5f;
        c.triggered = true;
        c.hurtFlash = 0.4f;
        PlaySound(AU.roar);
        SpawnParticles(c.x, c.y - 10, 12, 226, 40, 46, 110);
        rpsState = 0;
    }
}

// ---------------- 交易结算（交易鬼：等价交换） ----------------
static void DealResolve(int opt) {         // 1=寿命换痊愈 2=木换铁 3=鬼仆换碎片
    if (dealGhostIdx < 0 || dealGhostIdx >= (int)mobs.size()) { dealState = 0; return; }
    Creature& c = mobs[(size_t)dealGhostIdx];
    if (c.state == AState::Dead || c.kind != CreatureKind::DealGhost) { dealState = 0; return; }
    bool ok = false;
    if (opt == 1) {                         // 代价：10% 最大生命上限 -> 回满血
        int cost = P.maxHp / 10;
        if (cost < 5) cost = 5;
        P.maxHp -= cost;
        if (P.maxHp < 30) P.maxHp = 30;
        P.hp = P.maxHp;
        FloatText(P.x, P.y - 44, "它取走了你的一部分寿命", 226, 40, 46);
        ok = true;
    } else if (opt == 2) {                  // 代价：木 x20 -> 铁矿 x5
        if (P.wood >= 20) { P.wood -= 20; P.iron += 5; ok = true; }
        else FloatText(P.x, P.y - 44, "木料不足（需 20）", 255, 160, 120);
    } else if (opt == 3) {                  // 代价：鬼仆 x1 -> 符文碎片 x2
        if (!ghosts.empty()) {
            ghosts.erase(ghosts.begin());
            P.gemShard += 2;
            ok = true;
        } else FloatText(P.x, P.y - 44, "没有鬼仆可付", 255, 160, 120);
    }
    if (ok) {
        PlaySound(AU.capture);
        SpawnParticles(c.x, c.y - 10, 16, 220, 190, 120, 110);
        FloatText(P.x, P.y - 56, "契约达成", 255, 224, 138);
        c.state = AState::Dead; c.deadFade = 0.4f;           // 交易完成，它消散
    }
    dealState = 0;
}

// ============================================================
// ---- 领地 / 同伴 / 无面鬼同化 ----
//   领地：玩家自选地点立界碑（N 键），在界碑处升级（需**同伴数量**与材料同时满足）。
//   同伴：跟随/入住的幸存者 + 鬼仆 + 伙伴 + 混进来的无面鬼，统称"身边同伴"。
//   无面鬼：蛰伏时贴着你够久就混进队伍当同伴，并持续同化身边的真同伴。
//           身边同伴 > 5 且无面鬼占比达阈值 → 角色死亡（它已经占了你的队伍）。
//           它无破绽：不画选中范围、不显蛰伏态，只能靠每日有限的问询揪出来。
// ============================================================
enum CompType : int { CT_NPC = 0, CT_GHOST = 1, CT_FACELESS = 2, CT_PET = 3 };

static int   gCompTotal = 0;         // 身边同伴总数（每帧刷新：UI 与死亡判定共用）
static int   gCompFace = 0;          // 其中无面鬼的数量
static int   askLeft = 2;            // 今日剩余问询次数
static int   askDay = -1;            // 问询计数的日期标记
static int   askType = -1;           // 上次问询的目标类型（-1 无）
static int   askIdx = -1;            // 上次问询的目标下标
static float askWinT = 0;            // "再按一次驱逐"的窗口
static char  askLine[64] = "";       // 最近一次应答（HUD 短时显示）
static float askLineT = 0;

static const float COMP_R = 220.0f;                     // "身边"的判定半径
static inline int  TerrLv()      { return campBuilt ? campLv : 1; }
static inline int  FacelessPct() { return campLv >= 4 ? 25 : (campLv >= 2 ? 30 : 50); }   // Lv2 起 30%，Lv4 起 25%
static inline int  AskPerDay()   { return 2 + (campLv - 1) + SeeAskBonus(); }        // 领地每升一级 +1 次

// 身边同伴统计（无面鬼占比 = faceN / total）
static void CountCompanions(int& total, int& faceN) {
    total = 0; faceN = 0;
    for (size_t i = 0; i < npcs.size(); i++) {
        const Npc& n = npcs[i];
        if (!n.on || n.kind != 1 || n.state == 0) continue;      // 野外游荡的还不算你的同伴
        float dx = n.x - P.x, dy = n.y - P.y;
        if (dx * dx + dy * dy > COMP_R * COMP_R) continue;
        total++;
    }
    for (const GhostAlly& g : ghosts) {
        float dx = g.x - P.x, dy = g.y - P.y;
        if (dx * dx + dy * dy > COMP_R * COMP_R) continue;
        total++;
    }
    if (PET.on) {
        float dx = PET.x - P.x, dy = PET.y - P.y;
        if (dx * dx + dy * dy <= COMP_R * COMP_R) total++;
    }
    for (const Creature& c : mobs) {
        if (!c.infiltrated || c.state == AState::Dead) continue;
        float dx = c.x - P.x, dy = c.y - P.y;
        if (dx * dx + dy * dy > COMP_R * COMP_R) continue;
        total++; faceN++;
    }
}
// 名下同伴总数（领地升级的门槛：含不在身边的入住幸存者）
static int TotalCompanions() {
    int n = 0;
    for (const Npc& x : npcs) if (x.on && x.kind == 1 && x.state != 0) n++;
    n += (int)ghosts.size();
    if (PET.on) n++;
    for (const Creature& c : mobs) if (c.infiltrated && c.state != AState::Dead) n++;
    return n;
}

// 驱逐刚才问过的人：判断对了清掉一只无面鬼，判断错了永远失去一位真同伴
static void ExpelAsked() {
    int t = askType, i = askIdx;
    askWinT = 0; askType = -1; askIdx = -1;
    if (t == CT_FACELESS) {
        if (i >= 0 && i < (int)mobs.size() && mobs[(size_t)i].infiltrated) {
            float fx2 = mobs[(size_t)i].x, fy2 = mobs[(size_t)i].y;
            SpawnParticles(fx2, fy2 - 10, 18, 190, 185, 175, 120);
            mobs.erase(mobs.begin() + i);
            gBanished++;                            // 成就：火眼金睛
            PlaySound(AU.capturePull);
            TextCopy(askLine, "你把它赶出了队伍"); askLineT = 2.6f;
            FloatText(P.x, P.y - 48, "你把它赶出了队伍", 150, 255, 200);
        }
    } else if (t == CT_NPC) {
        if (i >= 0 && i < (int)npcs.size() && npcs[(size_t)i].on) {
            float nx = npcs[(size_t)i].x, ny = npcs[(size_t)i].y;
            npcs[(size_t)i].on = false;
            SpawnParticles(nx, ny - 10, 14, 190, 235, 200, 110);
            PlaySound(AU.hurt);
            TextCopy(askLine, "你错怪了好人，他走了"); askLineT = 2.6f;
            FloatText(P.x, P.y - 48, "你错怪了好人，他走了", 226, 40, 46);
        }
    }
}

// 问询（K）：向身边最近的一位"人"提问，依据回答判断它是不是无面鬼。
// -------- 再按一次 K = 当场驱逐刚才问过的那位（判断错了代价自负）--------
static void TryAskCompanion() {
    if (P.dead || gs != GS::Play) return;
    if (askWinT > 0.0f && askType >= 0) { ExpelAsked(); return; }

    int today = (int)(gameTime / DAY_LEN);
    if (askDay != today) { askDay = today; askLeft = AskPerDay(); }
    if (askLeft <= 0) {
        FloatText(P.x, P.y - 46, "今日问询已尽", 255, 160, 120);
        return;
    }
    // 最近的一位"人"（幸存者 / 混进来的无面鬼）—— 鬼仆一眼就是鬼，不必问
    int   bType = -1, bIdx = -1;
    float bd = 60.0f * 60.0f, bx = 0, by = 0;
    for (size_t i = 0; i < npcs.size(); i++) {
        const Npc& n = npcs[i];
        if (!n.on || n.kind != 1 || n.state == 0) continue;
        float dx = n.x - P.x, dy = n.y - P.y, q = dx * dx + dy * dy;
        if (q < bd) { bd = q; bType = CT_NPC; bIdx = (int)i; bx = n.x; by = n.y; }
    }
    for (size_t i = 0; i < mobs.size(); i++) {
        const Creature& c = mobs[i];
        if (!c.infiltrated || c.state == AState::Dead) continue;
        float dx = c.x - P.x, dy = c.y - P.y, q = dx * dx + dy * dy;
        if (q < bd) { bd = q; bType = CT_FACELESS; bIdx = (int)i; bx = c.x; by = c.y; }
    }
    if (bType < 0) { FloatText(P.x, P.y - 42, "身边没人可问", 255, 160, 120); return; }

    askLeft--;
    askType = bType; askIdx = bIdx; askWinT = 4.0f;
    int  day  = (int)(gameTime / DAY_LEN) + 1;            // 与顶栏"第 X 天"同一口径
    bool fake = (bType == CT_FACELESS);
    int  said = day;
    if (fake) {                                            // 它记错了日子 —— 破绽只在回答里
        int off = (rand() & 1) ? 2 : -2;
        said = day + off;
        if (said == day || said < 1) said = day + 3;
    }
    TextCopy(askLine, TextFormat(L10N("答：第 %d 天（再按 K 驱逐）"), said));
    askLineT = 4.0f;
    FloatText(bx, by - 34, TextFormat(L10N("第 %d 天"), said), fake ? 200 : 190,
              fake ? 185 : 235, fake ? 175 : 200);
    PlaySound(fake ? AU.pickup : AU.craft);
}

// 无面鬼同化：每一只混进队伍的，隔一阵子就把身边一个真同伴顶替掉
static void UpdateFacelessSpread(float dt) {
    for (size_t i = 0; i < mobs.size(); i++) {
        if (!mobs[i].infiltrated || mobs[i].state == AState::Dead) continue;
        if (mobs[i].convertT > 0.0f) { mobs[i].convertT -= dt; continue; }
        mobs[i].convertT = 24.0f;
        // 找它身边最近的真同伴（140px）
        int   vt = -1, vi = -1;
        float bd = 140.0f * 140.0f, vx = 0, vy = 0;
        for (size_t k = 0; k < npcs.size(); k++) {
            const Npc& n = npcs[k];
            if (!n.on || n.kind != 1 || n.state == 0) continue;
            float dx = n.x - mobs[i].x, dy = n.y - mobs[i].y, q = dx * dx + dy * dy;
            if (q < bd) { bd = q; vt = CT_NPC; vi = (int)k; vx = n.x; vy = n.y; }
        }
        for (size_t k = 0; k < ghosts.size(); k++) {
            const GhostAlly& g = ghosts[k];
            float dx = g.x - mobs[i].x, dy = g.y - mobs[i].y, q = dx * dx + dy * dy;
            if (q < bd) { bd = q; vt = CT_GHOST; vi = (int)k; vx = g.x; vy = g.y; }
        }
        if (vt < 0) continue;                              // 身边没真人可顶替
        if (vt == CT_NPC) npcs[(size_t)vi].on = false;
        else              ghosts.erase(ghosts.begin() + vi);
        Creature nc;
        nc.kind = CreatureKind::Faceless;
        nc.infiltrated = true;
        nc.x = vx; nc.y = vy;
        nc.hp = MobBaseHp(nc.kind);
        nc.tier = mobs[i].tier; nc.dmgMul = mobs[i].dmgMul;
        nc.tintSeed = (unsigned char)(rand() & 0xFF);
        nc.state = AState::Idle;
        nc.convertT = 24.0f;                               // 新来的先蛰伏一阵再动手
        mobs.push_back(nc);
        SpawnParticles(vx, vy - 10, 12, 190, 185, 175, 90);
        break;                                             // push_back 可能重排：本帧只处理一只
    }
}

// 领地房屋：与世界里的建筑一样，是座正经房子（外墙 + 屋顶 + 床 + 长明灯）。
// 随领地等级长大：半径 3/4/5 格；墙体加固：耐久 10/14/18（普通木墙只有 6）。
static int  CampRadius(int lv) { return 2 + lv; }
static void ClearTerritoryWalls(int bx, int by, int lv) {
    int r = CampRadius(lv);
    for (int ox = -r; ox <= r; ox++) for (int oy = -r; oy <= r; oy++) {
        if (ox != -r && ox != r && oy != -r && oy != r) continue;   // 只拆外墙圈
        int oi = W.ObjIndexAt(bx + ox, by + oy);
        if (oi >= 0 && W.objs[(size_t)oi].kind == ObjKind::Wall) W.RemoveObjAt(bx + ox, by + oy);
    }
}
static void BuildTerritoryHouse(int bx, int by, int lv) {
    int r = CampRadius(lv);
    unsigned char wallHp = (unsigned char)(6 + lv * 4);   // 防御力随等级提升
    for (int ox = -r; ox <= r; ox++) for (int oy = -r; oy <= r; oy++) {
        if (ox != -r && ox != r && oy != -r && oy != r) continue;   // 只砌外墙圈
        if (oy == r && (ox == 0 || ox == 1)) continue;              // 南门两格宽（房子变大仍保证进出）
        if (W.PlaceWall(bx + ox, by + oy)) {
            int oi = W.ObjIndexAt(bx + ox, by + oy);
            if (oi >= 0) W.objs[(size_t)oi].hp = wallHp;            // 领地墙比普通墙耐拆
        }
    }
    for (int ox = -r; ox <= r; ox++) W.MarkWallRoof(bx + ox, by - r);   // 北沿 = 屋顶（与其他房屋同款）
    // 屋内陈设：床 + 长明灯（已存在则跳过，升级重建不重复放）
    if (W.ObjIndexAt(bx - 1, by - 1) < 0) W.PlaceBed(bx - 1, by - 1);
    if (W.ObjIndexAt(bx + 1, by) < 0)     W.PlaceCampfire(bx + 1, by);
}

// 立界碑（N 键）：在脚下自建领地
static void TryBuildTerritory() {
    if (P.dead || gs != GS::Play) return;
    if (netOn && !netHost) { FloatText(P.x, P.y - 40, "只有房主能立界碑", 255, 200, 120); return; }
    if (campBuilt) { FloatText(P.x, P.y - 40, "已有领地，回界碑升级", 255, 200, 120); return; }
    if (P.wood < 30 || P.stone < 15) {
        FloatText(P.x, P.y - 40, "立界碑需 木30 石15", 255, 160, 120);
        return;
    }
    int tx = (int)(P.x / TILE), ty = (int)(P.y / TILE);
    if (tx < 4 || ty < 4 || tx > MAP_W - 5 || ty > MAP_H - 5) {
        FloatText(P.x, P.y - 40, "太靠边界，立不住", 255, 160, 120); return;
    }
    if (W.TileAt(tx, ty) == Tile::Water) {
        FloatText(P.x, P.y - 40, "水里立不住界碑", 255, 160, 120); return;
    }
    if (!W.CircleFree(P.x, P.y, 12.0f)) {
        FloatText(P.x, P.y - 40, "此处太窄，换个地方", 255, 160, 120); return;
    }
    if (!W.PlaceCampStone(tx, ty)) { FloatText(P.x, P.y - 40, "此处已有界碑", 255, 160, 120); return; }
    P.wood -= 30; P.stone -= 15;
    campBuilt = true; campLv = 1;
    campCenter = { tx * 16.0f + 8.0f, ty * 16.0f + 8.0f };
    BuildTerritoryHouse(tx, ty, 1);
    PlaySound(AU.craft); PlaySound(AU.place);
    SpawnParticles(campCenter.x, campCenter.y, 22, 200, 230, 180, 120);
    FloatText(P.x, P.y - 48, "领地已立! 收容幸存者吧", 150, 255, 200);
}

// ---------------- 每日悬赏 ----------------
static void GiveTalisman(int t, int n);      // 前向声明（实现在 2409 行附近）

static const char* BountyName(int t) {
    switch (t) {
    case BT_TREE:    return "砍树";
    case BT_ROCK:    return "采石";
    case BT_KILL:    return "诛邪";
    case BT_GRAVE:   return "掘坟";
    case BT_COOK:    return "烤肉";
    case BT_CRAFT:   return "炼器";
    case BT_CAPTURE: return "收鬼";
    default:         return "活到明天";
    }
}
static int BountyCounter(int t) {
    switch (t) {
    case BT_TREE:    return P.nTree;
    case BT_ROCK:    return P.nRock;
    case BT_KILL:    return P.kills;
    case BT_GRAVE:   return gGraveDug;
    case BT_COOK:    return gNCook;
    case BT_CRAFT:   return gNCraft;
    case BT_CAPTURE: return P.ghostCaught;
    case BT_SURVIVE: return (int)(gameTime / DAY_LEN);
    }
    return 0;
}
static void BountyComplete(int i) {
    Bounty& b = gBounty[i];
    if (b.done) return;
    b.done = 1;
    switch (b.type) {
    case BT_TREE:    P.wood += 12; break;
    case BT_ROCK:    P.stone += 10; break;
    case BT_KILL:    AddXp(30); break;
    case BT_GRAVE:   GiveTalisman(rand() % 5, 1); break;
    case BT_COOK:    P.cookedMeat += 3; break;
    case BT_CRAFT:   P.iron += 3; break;
    case BT_CAPTURE: AddXp(40); break;
    case BT_SURVIVE: P.berry += 8; break;
    }
    PlaySound(AU.pickup);
    FloatText(P.x, P.y - 56, TextFormat(L10N("悬赏达成:%s"), L10N(BountyName(b.type))), 255, 224, 138);
}
// 换日派发新悬赏（确定性按种子+天数，同一天进出存档结果一致）；顺手触发清晨事件
static void BountyUpdate() {
    int today = (int)(gameTime / DAY_LEN);
    if (gBountyDay == today) {
        for (int i = 0; i < 3; i++) {
            Bounty& b = gBounty[i];
            if (b.done || b.type == BT_SURVIVE) continue;
            if (BountyCounter(b.type) - b.base >= b.need) BountyComplete(i);
        }
        return;
    }
    // 先结算昨天的"活到明天"
    if (gBountyDay >= 0)
        for (int i = 0; i < 3; i++)
            if (gBounty[i].type == BT_SURVIVE && !gBounty[i].done) BountyComplete(i);
    // 派发今日三件（Fisher-Yates，LCG 定序列）
    unsigned rs = gSeed ^ (unsigned)(today * 2654435761u);
    auto rnd = [&]() { rs = rs * 1664525u + 1013904223u; return (int)(rs >> 16); };
    int pool[8] = { BT_TREE, BT_ROCK, BT_KILL, BT_GRAVE, BT_COOK, BT_CRAFT, BT_CAPTURE, BT_SURVIVE };
    for (int i = 7; i > 0; i--) { int j = rnd() % (i + 1); int t = pool[i]; pool[i] = pool[j]; pool[j] = t; }
    for (int i = 0; i < 3; i++) {
        gBounty[i].type = pool[i];
        gBounty[i].need = (pool[i] == BT_TREE || pool[i] == BT_ROCK) ? 3 :
                          (pool[i] == BT_KILL) ? 4 :
                          (pool[i] == BT_GRAVE) ? 2 : 1;
        gBounty[i].base = BountyCounter(pool[i]);
        gBounty[i].done = 0;
    }
    gBountyDay = today;
    FloatText(P.x, P.y - 50, "今日悬赏已更新(右上)", 190, 235, 200);
    // 清晨事件：35% 概率有交易鬼在附近出没（已有一只邻近则跳过）
    if ((rand() % 100) < 35) {
        bool nearDeal = false;
        for (const Creature& c : mobs)
            if (c.kind == CreatureKind::DealGhost && c.state != AState::Dead &&
                fabsf(c.x - P.x) + fabsf(c.y - P.y) < 420.0f) { nearDeal = true; break; }
        if (!nearDeal) {
            for (int tr = 0; tr < 20; tr++) {
                float a = (float)(rand() % 360) * 0.0174533f;
                float sx = P.x + cosf(a) * 170.0f, sy = P.y + sinf(a) * 150.0f;
                if (sx < 32 || sy < 32 || sx > (MAP_W - 2) * 16.0f || sy > (MAP_H - 2) * 16.0f) continue;
                if (!W.CircleFree(sx, sy, 8.0f)) continue;
                Creature c;
                c.kind = CreatureKind::DealGhost; c.x = sx; c.y = sy;
                c.hp = MobBaseHp(c.kind); c.state = AState::Idle;
                mobs.push_back(c);
                FloatText(P.x, P.y - 62, "远处传来交易鬼的吆喝声……", 150, 235, 190);
                break;
            }
        }
    }
}

// ---------------- E 键统一交互 ----------------
static void TryInteract() {
    if (P.dead || gs != GS::Play || craftOpen) return;
    if (netOn && !netHost) {                     // 客人：共享世界的交互归房主
        if (smithOpen) { smithOpen = false; return; }
        FloatText(P.x, P.y - 40, "联机模式：交互由房主进行", 255, 200, 120);
        return;
    }
    if (smithOpen) { smithOpen = false; return; }          // 面板开着：E 先关面板
    // 1) 铁匠（铁匠屋内的老铁匠 + 玩家雇来的铁匠）
    for (size_t i = 0; i < npcs.size(); i++) {
        if (!npcs[i].on || npcs[i].kind != 0) continue;
        float dx = npcs[i].x - P.x, dy = npcs[i].y - P.y;
        if (dx * dx + dy * dy < 60.0f * 60.0f) { SmithTalk((int)i); return; }
    }
    // 2) 可交互的鬼：交易鬼 / 游戏鬼（蛰伏态，50px）
    for (size_t i = 0; i < mobs.size(); i++) {
        Creature& c = mobs[i];
        if (c.companion) continue;                       // 伴生铁匠鬼走专用的末尾通道
        if (c.state == AState::Dead || c.triggered) continue;
        float dx = c.x - P.x, dy = c.y - P.y;
        if (dx * dx + dy * dy > 50.0f * 50.0f) continue;
        if (c.kind == CreatureKind::DealGhost) {
            dealState = 1; dealGhostIdx = (int)i;
            PlaySound(AU.pickup);
            return;
        }
        if (c.kind == CreatureKind::GameGhost) {
            rpsState = 1; rpsGhostIdx = (int)i; rpsDraws = 0;
            PlaySound(AU.pickup);
            return;
        }
        if (c.kind == CreatureKind::SmithGhost) {
            FloatText(P.x, P.y - 40, "它杀不死，只能用摄魂幡收", 150, 235, 190);
            return;
        }
    }
    // 3) 幸存者（游荡态，40px）：邀请跟随 -> 带回营地入住
    for (Npc& n : npcs) {
        if (!n.on || n.kind != 1 || n.state != 0) continue;
        float dx = n.x - P.x, dy = n.y - P.y;
        if (dx * dx + dy * dy > 44.0f * 44.0f) continue;
        n.state = 1;
        PlaySound(AU.pickup);
        SpawnParticles(n.x, n.y - 10, 8, 190, 235, 200, 90);
        FloatText(P.x, P.y - 44, "幸存者愿同行", 190, 235, 200);
        return;
    }
    // 4) 界碑：就地建领地 / 升级领地（需同伴数量与材料同时满足）
    {
        int ci = W.NearObj(ObjKind::CampStone, P.x, P.y, 44.0f);
        if (ci >= 0 && !campBuilt) {
            if (P.wood < 30 || P.stone < 15) {
                FloatText(P.x, P.y - 40, "建领地需 木30 石15", 255, 160, 120);
                return;
            }
            P.wood -= 30; P.stone -= 15;
            campBuilt = true;
            campLv = 1;
            const WorldObj& cs = W.objs[(size_t)ci];
            campCenter = { cs.tx * 16.0f + 8.0f, cs.ty * 16.0f + 8.0f };
            BuildTerritoryHouse(cs.tx, cs.ty, 1);
            PlaySound(AU.craft); PlaySound(AU.place);
            SpawnParticles(campCenter.x, campCenter.y, 22, 200, 230, 180, 120);
            FloatText(P.x, P.y - 48, "领地已立! 收容幸存者吧", 150, 255, 200);
            return;
        }
        if (ci >= 0 && campBuilt) {
            float ddx = campCenter.x - P.x, ddy = campCenter.y - P.y;
            if (ddx * ddx + ddy * ddy > 64.0f * 64.0f) {
                FloatText(P.x, P.y - 40, "这不是你的界碑", 255, 160, 120);
                return;
            }
            if (campLv >= 5) { FloatText(P.x, P.y - 40, "领地已是最高级", 255, 200, 120); return; }
            // 升级门槛：同伴数量 + 材料（1→2 / 2→3 / 3→4 / 4→5 逐档加码）
            int needC = 2 * campLv;
            static const int UP_W[5] = { 0, 40, 80, 120, 200 };
            static const int UP_S[5] = { 0, 25, 40, 60, 100 };
            static const int UP_I[5] = { 0,  0, 10, 20,  40 };
            static const int UP_C[5] = { 0,  0,  0,  5,  10 };
            int cw = UP_W[campLv], cs2 = UP_S[campLv], ci2 = UP_I[campLv], cc = UP_C[campLv];
            if (TotalCompanions() < needC) {
                FloatText(P.x, P.y - 40,
                          TextFormat(L10N("升级需同伴%d人（现有%d）"), needC, TotalCompanions()),
                          255, 160, 120);
                return;
            }
            if (P.wood < cw || P.stone < cs2 || P.iron < ci2 || P.crystal < cc) {
                FloatText(P.x, P.y - 40, TextFormat(L10N("升级需 木%d 石%d 铁%d 晶%d"), cw, cs2, ci2, cc),
                          255, 160, 120);
                return;
            }
            P.wood -= cw; P.stone -= cs2; P.iron -= ci2; P.crystal -= cc;
            // 房屋随等级重建：先拆旧墙圈，再按新等级砌更大更坚固的
            {
                int bx = (int)(campCenter.x / 16.0f), by = (int)(campCenter.y / 16.0f);
                ClearTerritoryWalls(bx, by, campLv);
                campLv++;
                BuildTerritoryHouse(bx, by, campLv);
            }
            PlaySound(AU.craft); PlaySound(AU.place);
            SpawnParticles(campCenter.x, campCenter.y, 26, 255, 224, 138, 120);
            FloatText(P.x, P.y - 48, TextFormat(L10N("领地升至Lv%d 房屋扩建 可收容%d人"), campLv, CampCap()), 255, 224, 138);
            if (campLv >= 2)
                FloatText(P.x, P.y - 62, "人多眼杂……无面鬼更难认了", 226, 40, 46);
            return;
        }
    }
    // 5) 床：睡觉
    if (W.NearBed(P.x, P.y, 40.0f) >= 0) { TrySleep(); return; }
    // 6) 伴生铁匠鬼（常在你身侧悬浮）：以上都没有可交互对象时，E 就是和他说话
    //    （放在最后 → 绝不抢占床/石碑/幸存者/交易鬼的交互优先级）
    if (gSmithCompanionIdx >= 0 && gSmithCompanionIdx < (int)mobs.size()) {
        const Creature& sc = mobs[(size_t)gSmithCompanionIdx];
        if (sc.kind == CreatureKind::SmithGhost && sc.companion) {
            float dx = sc.x - P.x, dy = sc.y - P.y;
            if (dx * dx + dy * dy < 52.0f * 52.0f) { SmithTalk(SMITH_GHOST_IDX); return; }
        }
    }
    FloatText(P.x, P.y - 34, "这里没什么可交互的", 150, 160, 150);
}

void MobHitPlayer(float dmg, float dirX, float dirY) {
    // 联机：鬼打的是房主为每只怪选定的目标——目标是客人就路由到客人状态（房主权威）
    if (netOn && netHost) {
        int ti = CreatureTargetIndex();
        if (ti > 0 && ti < 4 && netPeers[ti].on) {
            PeerView& pr = netPeers[ti];
            if (pr.dead) return;
            pr.hp -= (int)(dmg + 0.5f);
            if (pr.hp <= 0) { pr.hp = 0; pr.dead = true; }
            SpawnParticles(pr.x, pr.y - 10, 8, 200, 60, 60, 100);
            FloatText(pr.x, pr.y - 30, TextFormat("-%d", (int)(dmg + 0.5f)), 255, 90, 90);
            return;
        }
    }
    if (P.dead || P.invuln > 0) return;
    float dmg2 = dmg * ArmorMul();              // 护甲减伤
    if (P.shieldT > 0.0f) {                     // 鬼仆「锻炉」护体：再减半
        dmg2 *= 0.5f;
        SpawnParticles(P.x, P.y - 12, 4, 255, 224, 138, 70);
    }
    P.hp -= (int)(dmg2 + 0.5f);
    P.invuln = 0.8f;
    P.hurtFlash = 0.12f;
    float kb = 170.0f * (P.armorLv >= 2 ? 0.7f : 1.0f);   // 重甲击退抗性
    P.kx = dirX * kb; P.ky = dirY * kb;   // 击退玩家
    hurtVin = 0.5f;
    shakeT = 0.18f; shakeDur = 0.18f;
    PlaySound(AU.hurt);
    SetSoundPitch(AU.hurt, 0.9f + (rand() % 25) / 100.0f);
    SpawnParticles(P.x, P.y - 10, 8, 200, 60, 60, 100);
    FloatText(P.x, P.y - 30, TextFormat("-%d", (int)(dmg2 + 0.5f)), 255, 90, 90);
    // ---- 多面人命中：剥夺面容（叠加诅咒层：减速 -> 3 层重创）----
    for (const Creature& c : mobs) {
        if (c.kind != CreatureKind::ManyFaces || !c.triggered || c.state == AState::Dead) continue;
        float dx = c.x - P.x, dy = c.y - P.y;
        if (dx * dx + dy * dy < 44.0f * 44.0f) {
            P.faceStolen++;
            FloatText(P.x, P.y - 46, "面容被剥夺!", 200, 160, 255);
            SpawnParticles(P.x, P.y - 14, 8, 200, 180, 255, 90);
            break;
        }
    }
    if (P.hp <= 10 && P.hp > 0) P.nearDeath = true;      // 成就：命悬一线标记
    if (P.hp <= 0) KillPlayer();
}

// 怪物命中宠物（creature.cpp 回调）
void PetHit(float dmg, float dirX, float dirY) {
    if (!PET.on || PET.iv > 0) return;
    PET.hp -= (int)(dmg + 0.5f);
    PET.iv = 0.5f;
    PET.hurtFlash = 0.12f;
    PET.x += dirX * 8.0f; PET.y += dirY * 8.0f;          // 小击退
    PlaySound(AU.zombieHit);
    SpawnParticles(PET.x, PET.y - 6, 5, 240, 90, 90, 70);
    FloatText(PET.x, PET.y - 20, TextFormat("-%d", (int)(dmg + 0.5f)), 255, 130, 130);
    if (PET.hp <= 0) {                                   // 宠物不死亡：血量见底逃回森林
        PET.on = false;
        SpawnParticles(PET.x, PET.y - 8, 12, 220, 220, 220, 100);
        FloatText(PET.x, PET.y - 26, "伙伴逃走了", 220, 220, 230);
    }
}

// Boss 招式特效（creature.cpp 回调）：0=冲锋啸叫 1=召唤 2=震地
void BossFx(int kind) {
    if (kind == 2) {
        PlaySound(AU.shock);
        shakeT = 0.30f; shakeDur = 0.30f;
        TextCopy(bossHowlTxt, "震地!");
    } else if (kind == 0) {                       // 冲锋啸叫
        PlaySound(AU.roar);
        shakeT = 0.22f; shakeDur = 0.22f;
        TextCopy(bossHowlTxt, "野蛮冲锋!");
    } else {                                      // 召唤嘶吼
        PlaySound(AU.roar);
        shakeT = 0.22f; shakeDur = 0.22f;
        TextCopy(bossHowlTxt, "召唤尸群!");
    }
    bossHowlT = 1.6f;
}

// 穴蛛毒咬：施加中毒 DoT（每 0.8 秒扣血，共 4 秒）
void MobPoisonPlayer(float dps) {
    if (P.dead) return;
    P.poisonT = 4.0f;
    P.poisonDps = dps;
    FloatText(P.x, P.y - 40, "中毒!", 150, 255, 110);
}

// 无脸鬼命中：染血人脸覆盖（视野收拢 + 人脸在屏幕正中浮现，持续数秒）
void FaceCoverPlayer() {
    if (P.dead) return;
    P.faceCoverT = 4.0f;
    FloatText(P.x, P.y - 46, "它覆上了你的脸!", 176, 32, 46);
    PlaySound(AU.hurt);
    SetSoundPitch(AU.hurt, 0.7f);
    hurtVin = 0.45f;
    shakeT = 0.18f; shakeDur = 0.18f;
}

// 无脸鬼命中：记忆篡改（假血条）+ 窃取身份（偷经验）
//   - 假血条：HUD 显示的 hp 与实际不符（玩家以为自己还满血，实际在掉）
//   - 窃取身份：直接扣玩家等级经验（它把你的身份记忆偷走了）
//   - 护主被动：随身带有无脸鬼鬼仆时，篡改时长减半（以鬼制鬼——自家的无脸鬼替你护住记忆）
void MemoryHackPlayer() {
    if (P.dead) return;
    P.hacked = true;
    // 窃取经验：直接倒扣（它偷走了你的部分身份）；记账，收服它时全额追回
    int stolen = P.xp / 2 + 5;
    P.xp -= stolen; if (P.xp < 0) P.xp = 0;
    P.stolenXp += stolen;
    // 护主被动：携带无脸鬼鬼仆 → 篡改时长减半
    bool shield = false;
    for (const GhostAlly& g : ghosts)
        if (g.kind == CreatureKind::Faceless) { shield = true; break; }
    P.hackT = shield ? 6.0f : 12.0f;
    // 假血：显示成"满血"（玩家以为安全）
    P.fakeHp = P.maxHp;
    if (shield) {
        FloatText(P.x, P.y - 56, "无脸鬼护住了你的记忆!", 150, 255, 200);
        SpawnParticles(P.x, P.y - 16, 10, 150, 255, 200, 100);
        // 演出：护主盾环（青绿双环一闪）+ 低沉版记忆钟音（自家鬼低声护持）
        SpawnCapFx(2, P.x, P.y, 0, 0);
        SetSoundPitch(AU.memoryRestore, 0.8f);
        PlaySound(AU.memoryRestore);
    } else {
        FloatText(P.x, P.y - 56, "记忆被篡改! 经验被窃!", 176, 32, 46);
    }
    PlaySound(AU.roar);
}

// ============================================================
// 鬼杀不死：打散它只是「击溃」—— 延时在远处重新凝聚
// 真正一劳永逸的办法只有"收容"（葫芦暂储 / 镇鬼幡收服）
// ============================================================
struct Banished {
    bool on = false;
    CreatureKind kind = CreatureKind::GhostChild;
    unsigned char tier = 0;
    float mul = 1.0f;
    float t = 0;              // 重新凝聚倒计时
};
static std::vector<Banished> banished;      // 被击溃的鬼（等待重聚）
static int  ghostBanished = 0;              // 累计击溃数（不计入"击杀"）
// 凡兽可被杀死，鬼不可以
static inline bool IsGhostKind(CreatureKind k) {
    return k != CreatureKind::Rabbit && k != CreatureKind::Deer;
}
static void BanishGhost(CreatureKind kind, unsigned char tier, float mul) {
    for (Banished& b : banished) if (!b.on) {
        b.on = true; b.kind = kind; b.tier = tier; b.mul = mul; b.t = 150.0f; return;
    }
    banished.push_back({ true, kind, tier, mul, 150.0f });
}
// 该鬼已被收容（残魂或活体被收走）→ 撤掉它的重聚名额
static void CancelBanish(CreatureKind kind) {
    for (Banished& b : banished)
        if (b.on && b.kind == kind) { b.on = false; return; }
}

void MobDied(CreatureKind kind, float x, float y, unsigned char tier, float mul) {
    if (IsGhostKind(kind)) {
        // ---- 鬼不被杀死，只被击溃：留残魂、登记重聚、不计击杀 ----
        ghostBanished++;
        PlaySound(AU.zombieDie);
        SetSoundPitch(AU.zombieDie, 0.55f);               // 溃散音：低沉一档（不是死亡音）
        SpawnParticles(x, y - 10, 18, 150, 200, 235, 140);
        SpawnParticles(x, y - 10, 8, 220, 240, 255, 100);
        FloatText(x, y - 24, TextFormat(L10N("击溃 %s"), L10N(GhostName(kind))), 150, 220, 255);
        BanishGhost(kind, tier, mul);
        AddXp(6 + (int)tier * 4);
    } else {
        P.kills++;
        PlaySound(AU.zombieDie);
        SetSoundPitch(AU.zombieDie, 0.85f + (rand() % 35) / 100.0f);   // 击杀音随机变调
    }
    switch (kind) {
    case CreatureKind::Rabbit:
        SpawnParticles(x, y - 6, 10, 235, 235, 225, 110);
        W.SpawnDrop(ItemKind::RawMeat, x, y, -30, -70);
        AddXp(3);
        break;
    case CreatureKind::Deer:
        SpawnParticles(x, y - 8, 12, 200, 140, 90, 110);
        W.SpawnDrop(ItemKind::RawMeat, x, y, -40, -65);
        W.SpawnDrop(ItemKind::RawMeat, x, y, 45, -60);
        AddXp(5);
        break;

    // ---- 十大规则鬼 ----
    case CreatureKind::GhostChild:   // 鬼童：幼弱残魂 + 少量经验
        SpawnParticles(x, y - 8, 12, 160, 220, 190, 120);
        AddXp(6);
        break;
    case CreatureKind::GhostTeen:    // 鬼种·少年：铁矿概率
        SpawnParticles(x, y - 10, 14, 110, 160, 130, 130);
        if ((rand() % 100) < 40) W.SpawnDrop(ItemKind::IronOre, x, y, 20, -70);
        AddXp(14);
        break;
    case CreatureKind::GhostAdult: { // 鬼种·成年：S 级掉落（萤晶+符文碎片必掉）
        SpawnParticles(x, y - 14, 26, 90, 200, 160, 150);
        SpawnParticles(x, y - 14, 12, 150, 255, 220, 130);
        W.SpawnDrop(ItemKind::Crystal, x, y, -30, -74);
        W.SpawnDrop(ItemKind::Crystal, x, y, 32, -68);
        W.SpawnDrop(ItemKind::GemShard, x, y, 0, -84);
        TextCopy(bossHowlTxt, "鬼种·成年 已消散!");
        bossHowlT = 2.5f;
        AddXp(60);
        shakeT = 0.3f; shakeDur = 0.3f;
        break;
    }
    case CreatureKind::LoneGhost:    // 单鬼：S 级，铁矿+萤晶
        SpawnParticles(x, y - 12, 22, 200, 210, 235, 150);
        W.SpawnDrop(ItemKind::IronOre, x, y, -26, -72);
        W.SpawnDrop(ItemKind::Crystal, x, y, 28, -78);
        AddXp(50);
        break;
    case CreatureKind::NineFace:     // 9面鬼：符文碎片概率
        SpawnParticles(x, y - 10, 18, 200, 120, 255, 140);
        if ((rand() % 100) < 50) W.SpawnDrop(ItemKind::GemShard, x, y, 18, -74);
        AddXp(28);
        break;
    case CreatureKind::MimicAll:     // 所有人：腐肉（它伪装过人）
        SpawnParticles(x, y - 10, 14, 150, 150, 160, 130);
        W.SpawnDrop(ItemKind::RottenMeat, x, y, 20, -65);
        AddXp(12);
        break;
    case CreatureKind::Faceless:     // 无脸鬼：萤晶概率（吞过别的鬼）
        SpawnParticles(x, y - 10, 16, 170, 170, 180, 140);
        if ((rand() % 100) < 40) W.SpawnDrop(ItemKind::Crystal, x, y, 22, -72);
        AddXp(20);
        break;
    case CreatureKind::GameGhost:    // 游戏鬼：符咒奖励（懂得规矩的鬼）
        SpawnParticles(x, y - 8, 14, 200, 200, 160, 130);
        GiveTalisman(rand() % 5, 1);
        AddXp(10);
        break;
    case CreatureKind::DealGhost:    // 交易鬼：铁矿（它收过代价）
        SpawnParticles(x, y - 10, 14, 220, 190, 120, 140);
        W.SpawnDrop(ItemKind::IronOre, x, y, 0, -76);
        AddXp(12);
        break;
    case CreatureKind::KnockGhost:   // 敲门鬼：击退了夜的访客
        SpawnParticles(x, y - 8, 14, 130, 150, 220, 130);
        AddXp(10);
        break;
    case CreatureKind::ManyFaces:    // 多面人：萤晶概率
        SpawnParticles(x, y - 8, 12, 150, 190, 170, 130);
        if ((rand() % 100) < 30) W.SpawnDrop(ItemKind::Crystal, x, y, 20, -68);
        AddXp(14);
        break;
    case CreatureKind::GhostDomain: { // 鬼游戏·核心：净化鬼域，大量掉落
        SpawnParticles(x, y - 16, 34, 200, 60, 60, 170);
        SpawnParticles(x, y - 16, 22, 255, 230, 140, 150);
        W.SpawnDrop(ItemKind::Heart, x, y, 0, -88);            // 血月之心（必掉：核心之核）
        W.SpawnDrop(ItemKind::GemShard, x, y, -34, -76);
        W.SpawnDrop(ItemKind::GemShard, x, y, 36, -70);
        W.SpawnDrop(ItemKind::Crystal, x, y, -18, -84);
        W.SpawnDrop(ItemKind::Crystal, x, y, 22, -80);
        W.SpawnDrop(ItemKind::IronOre, x, y, -44, -62);
        TextCopy(bossHowlTxt, "鬼域已净化!");
        bossHowlT = 3.0f;
        bossSlain = true;                       // 成就：净化鬼域
        domainPurged = true;                    // 鬼域永久消散（游戏不再开局）
        GdExit();
        AddXp(120);
        shakeT = 0.4f; shakeDur = 0.4f;
        break;
    }
    case CreatureKind::SmithGhost:
        // 铁匠鬼是鬼，鬼不会死亡：没有任何掉落、没有"陨落"，
        // 只是被打得溃散，稍后会在自己的废弃铁匠铺里重新凝聚（见 Banished 重聚分支）
        SpawnParticles(x, y - 12, 16, 180, 220, 210, 120);
        TextCopy(bossHowlTxt, "铁匠鬼被打散了，他回铺子里去了……");
        bossHowlT = 2.5f;
        AddXp(20);
        break;
    }

    // ---- 收鬼系统：规则鬼死后原地留下残魂（S 级核心与限时鬼奴除外）----
    if (kind != CreatureKind::GhostAdult && kind != CreatureKind::GhostDomain &&
        kind != CreatureKind::KnockGhost)
        SpawnSoulAt(kind, x, y - 4, tier, mul);
}

// ---------------- 掉落拾取回调 ----------------

static void OnPickup(const Drop&, float, float);   // 定义在下，供包装回调前向引用
static void OnPickupDrop(ItemKind k) { OnPickup(Drop{ k, 0, 0, 0, 0, 0, 0 }, 0, 0); }

static void OnPickup(const Drop& dr, float, float) {
    ItemKind k = dr.kind;
    switch (k) {
    case ItemKind::Wood:    P.wood++;   break;
    case ItemKind::Stone:   P.stone++;  break;
    case ItemKind::Berry:   P.berry++;  break;
    case ItemKind::RawMeat:    P.rawMeat++;    break;
    case ItemKind::CookedMeat: P.cookedMeat++; break;
    case ItemKind::RottenMeat: P.rottenMeat++; break;
    case ItemKind::IronOre:    P.iron++;       break;
    case ItemKind::Crystal:    P.crystal++;    break;
    case ItemKind::GemShard:   P.gemShard++;   break;
    case ItemKind::Heart:      P.heart++;      break;
    }
    // 连拾手感：短时间连续拾取音调递升（收割的爽感），1.5 秒重置
    static int pickN = 0;
    static float pickT = 0.0f;
    float now2 = (float)GetTime();
    if (now2 - pickT > 1.5f) pickN = 0;
    pickT = now2;
    pickN++;
    PlaySound(AU.pickup);
    SetSoundPitch(AU.pickup, 0.95f + 0.05f * (pickN > 6 ? 6 : pickN));
    // 联机房主：自己捡到的掉落，向所有客人广播移除（先到先得）
    if (netOn && netHost && dr.netId) {
        Net::Buf b;
        b.d[0] = 'D'; b.n = 1;
        Net::W32(b, dr.netId);
        Net::W8(b, (unsigned)dr.kind);
        Net::W8(b, 0);                       // playerId 0 = 房主
        for (int i = 1; i < 4; i++)
            if (netPeers[i].on) Net::UdpSend(netSock, netPeers[i].addr, netPeers[i].port, b);
    }
}

// ---------------- 玩家死亡 ----------------

void KillPlayer() {
    if (P.dead) return;
    // 还魂香：替你死一次（只挡一次，挡完即燃尽）
    if (gRevive) {
        gRevive = false;
        P.hp = (int)(P.maxHp * 0.5f);
        if (P.hp < 20) P.hp = 20;
        P.invuln = 2.2f;
        P.stalked = false;
        PlaySound(AU.craft);
        SpawnParticles(P.x, P.y - 12, 20, 230, 130, 220, 150);
        FloatText(P.x, P.y - 52, "还魂香燃尽——替你挡了这一下", 230, 130, 220);
        return;
    }
    P.hp = 0;
    P.dead = true;
    P.deadT = 0;
    deathTime = gameTime;
    gs = GS::Dead;
    PlaySound(AU.playerDie);
    SpawnParticles(P.x, P.y - 12, 14, 200, 60, 60, 120);
}

// ---------------- 战斗：扇形攻击判定（工具接管伤害/攻速/击退） ----------------
// 环形栏选中工具生效：2 猎刀(+伤快速) 4 铁锤(重击高伤大击退) 1 猎弓(远程箭矢)

// 四种树统一判砍（阔叶/松/白桦/棕榈——此前只有普通树可砍是 bug）
static bool IsTreeKind(ObjKind k) {
    return k == ObjKind::Tree || k == ObjKind::TreePine ||
           k == ObjKind::TreeBirch || k == ObjKind::TreePalm;
}

// 出手者上下文：扇形判定的原点/朝向/装备（房主与远程客人共用一套横扫实现）
struct SweepActor {
    float ox, oy;                 // 判定原点（玩家脚下 -8y）
    int dir, hotSel;
    unsigned char toolLv[6];
    int swordLv, dmg;
    bool heavyHit;
};

// 扇形横扫：生物部分（120 度扇形，半径 46；圈内所有目标一并结算）
static bool SweepMobsAt(const SweepActor& A) {
    static const float FACE[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
    float fx = FACE[A.dir & 3][0], fy = FACE[A.dir & 3][1];
    bool hitAny = false;
    bool metUndying = false;
    for (auto& c : mobs) {
        if (c.state == AState::Dead) continue;
        // 不死单位（铁匠鬼）：打不动、杀不死，连激怒都不会——刀锋直接穿过去
        if (MobUndying(c.kind)) { metUndying = true; continue; }
        float dx = c.x - A.ox, dy = c.y - A.oy;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 46.0f || d < 0.001f) continue;
        float dot = (dx * fx + dy * fy) / d;
        if (dot < 0.45f) continue;

        bool isGhost = MobHostile(c.kind);                  // 神秘复苏：鬼为阴物，凡器难伤
        if (isGhost) c.triggered = true;                    // 打了它只会激怒它（等同触发规律）

        // ---- 桃木剑：唯一能"隔空压住"鬼的凡器（鬼仍杀不死，只能被击退）----
        float kb2 = A.heavyHit ? 300.0f : 240.0f, stun = 0.26f;
        if (isGhost) {
            bool wielding = (A.hotSel == 0 && A.swordLv > 0);
            int gTier = (int)c.tier + 1;                   // 1..3
            if (wielding && A.swordLv >= gTier) {
                int over = A.swordLv - gTier;              // 越级程度 0..2
                kb2  = 250.0f + 95.0f * over;
                stun = 0.28f + 0.34f * over;
                SpawnParticles(c.x, c.y - 10, 8, 255, 224, 138, 110);   // 桃木金光
            } else if (wielding) {
                kb2 = 70.0f; stun = 0.12f;                 // 压不住：只推开一点点
            } else {
                kb2 = 190.0f; stun = 0.18f;                // 徒手/他器：勉强挡开
            }
        }
        c.hp -= isGhost ? 0 : A.dmg;
        c.hurtFlash = 0.12f;
        c.kvx = dx / d * kb2; c.kvy = dy / d * kb2;                  // 击退（仍能驱退，拖延片刻）
        c.state = AState::Hurt;
        c.timer = stun;                                              // 硬直（桃木剑压得住才够久）

        if (isGhost) {
            PlaySound(AU.zombieHit);
            SetSoundPitch(AU.zombieHit, 0.85f + (rand() % 30) / 100.0f);   // 音调随机：打击不单调
            SpawnParticles(c.x, c.y - 10, 10, 90, 235, 190, 105);   // 青绿鬼火溅射
            SpawnParticles(c.x, c.y - 10, 4, 230, 255, 245, 120);   // 白火花
        } else {
            PlaySound(AU.hit);
            SetSoundPitch(AU.hit, 0.85f + (rand() % 30) / 100.0f);
            SpawnParticles(c.x, c.y - 10, 10, 220, 70, 70, 105);
            SpawnParticles(c.x, c.y - 10, 4, 255, 240, 200, 120);   // 白火花
        }
        // 连击音：连续命中时音调递升（连击手感），2 秒未命中重置
        {
            static int comboN = 0;
            static float comboT = 0.0f;
            float now2 = (float)GetTime();
            if (now2 - comboT > 2.0f) comboN = 0;
            comboT = now2;
            comboN++;
            float pitch = 1.0f + 0.05f * (comboN > 8 ? 8 : comboN);
            if (isGhost) SetSoundPitch(AU.zombieHit, pitch);
            else SetSoundPitch(AU.hit, pitch);
        }
        // 凡/兽：正常伤害数字；鬼：桃木剑驱退提示（等级压制直接写在飘字里）
        if (isGhost) {
            if (A.hotSel == 0 && A.swordLv > 0 && A.swordLv >= (int)c.tier + 1)
                FloatText(c.x, c.y - 22, "桃木剑·退!", 255, 224, 138);
            else if (A.hotSel == 0 && A.swordLv > 0)
                FloatText(c.x, c.y - 22, TextFormat(L10N("剑压不住%d级"), (int)c.tier + 1), 226, 40, 46);
            else
                FloatText(c.x, c.y - 22, "凡器难伤", 176, 32, 32);
        } else {
            FloatText(c.x, c.y - 22, TextFormat("%d", A.dmg), 255, 255, 255);
        }
        hitAny = true;
        if (c.hp <= 0) {
            // 单鬼「时间重启」：第一次被击杀原地满血复活
            if (MobPreDeath(c)) {
                TextCopy(bossHowlTxt, "时间……重启了!");
                bossHowlT = 2.2f;
                PlaySound(AU.roar);
                SpawnParticles(c.x, c.y - 10, 24, 200, 220, 255, 140);
                shakeT = 0.35f; shakeDur = 0.35f;
                hitStop = 0.22f;                                    // 慢动作：重启瞬间
                continue;
            }
            c.state = AState::Dead;
            c.deadFade = 0;
            MobDied(c.kind, c.x, c.y, c.tier, c.dmgMul);
            // 击杀反馈：慢动作 + 尸体爆裂粒子（打击感收尾）
            hitStop = A.heavyHit ? 0.20f : 0.15f;
            shakeT = 0.22f; shakeDur = 0.22f;
            SpawnParticles(c.x, c.y - 10, 16, 255, 240, 200, 130);
        }
    }
    if (metUndying)
        FloatText(A.ox, A.oy - 44, "它是杀不死的", 255, 224, 138);
    if (hitAny) {
        if (hitStop <= 0.0f) hitStop = A.heavyHit ? 0.13f : 0.10f;     // 全局打击停顿（重击更深）
        if (shakeT <= 0.0f) { shakeT = 0.16f; shakeDur = 0.16f; }      // 屏幕震动
    }
    return hitAny;
}

// 扇形横扫：资源部分（树/石/草/矿/果/箱一并收割）；返回命中瓦片坐标（联机客人上报房主用）
static int SweepObjectsAt(const SweepActor& A, int* outTx, int* outTy, int maxOut) {
    static const float FACE[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
    float fx = FACE[A.dir & 3][0], fy = FACE[A.dir & 3][1];
    constexpr int SWEEP_MAX = 6;                 // 单次横扫最多破坏的目标数
    int   hitIdx[SWEEP_MAX]; float hitD[SWEEP_MAX];
    int   hitN = 0;
    int tx0 = (int)((A.ox - 56) / TILE), tx1 = (int)((A.ox + 56) / TILE);
    int ty0 = (int)((A.oy - 48) / TILE), ty1 = (int)((A.oy + 56) / TILE);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            int idx = W.ObjIndexAt(tx, ty);
            if (idx < 0) continue;
            const WorldObj& o = CurObjs()[(size_t)idx];
            if (!IsTreeKind(o.kind) && o.kind != ObjKind::Rock &&
                o.kind != ObjKind::TallGrass && o.kind != ObjKind::OreRock &&
                !(o.kind == ObjKind::Berry && !o.harvested) &&
                !(o.kind == ObjKind::Chest && !o.harvested))
                continue;
            float cx = tx * 16.0f + 8.0f;
            float cy = ty * 16.0f + (IsTreeKind(o.kind) ? 13.0f : 8.0f);
            float dx = cx - A.ox, dy = cy - A.oy;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 44.0f || d < 0.001f) continue;
            float dot = (dx * fx + dy * fy) / d;
            if (dot < 0.3f) continue;
            // 按距离插进有序小表（近的先结算：树倒了会挡视线，先砍近的更符合手感）
            int pos = hitN < SWEEP_MAX ? hitN : SWEEP_MAX - 1;
            while (pos > 0 && hitD[pos - 1] > d) { hitD[pos] = hitD[pos - 1]; hitIdx[pos] = hitIdx[pos - 1]; pos--; }
            if (hitN < SWEEP_MAX) hitN++;
            hitD[pos] = d; hitIdx[pos] = idx;
        }
    for (int k = 0; k < hitN; k++) HitObject(hitIdx[k]);
    if (hitN > 0) {
        if (hitStop <= 0.0f) hitStop = A.heavyHit ? 0.08f : 0.05f;
        if (shakeT <= 0.0f) { shakeT = 0.10f; shakeDur = 0.10f; }
    }
    if (outTx && outTy) {
        if (hitN > maxOut) hitN = maxOut;
        for (int k = 0; k < hitN; k++) {
            outTx[k] = CurObjs()[(size_t)hitIdx[k]].tx;
            outTy[k] = CurObjs()[(size_t)hitIdx[k]].ty;
        }
    }
    return hitN;
}

void DoAttack() {
    static const float FACE[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
    int dir = P.dir;
    float fx = FACE[dir][0], fy = FACE[dir][1];
    playerAttackedFrame = true;              // 出手（鬼游戏「禁止出手」规则监察）

    SweepActor A;
    A.ox = P.x; A.oy = P.y - 8.0f; A.dir = dir; A.hotSel = hotSel;
    memcpy(A.toolLv, P.toolLv, 6);
    A.swordLv = (int)P.swordLv;
    A.dmg = PlayerDmg();
    if (hotSel == 2 && P.toolLv[2]) A.dmg += 1 + P.toolLv[2];        // 猎刀 +2/+3
    if (hotSel == 4 && P.toolLv[4]) A.dmg += 4 + 2 * P.toolLv[4];    // 铁锤 +6/+8
    A.heavyHit = (hotSel == 4 && P.toolLv[4]);                       // 重击（铁锤）

    // ---- 猎弓：发射箭矢，无近战判定 ----
    if (hotSel == 1 && P.toolLv[1]) {
        float sp = 300.0f + 40.0f * P.toolLv[1];
        FireArrow(P.x + fx * 10.0f, P.y - 8 + fy * 10.0f, fx * sp, fy * sp);
        return;
    }
    SweepMobsAt(A);
    SweepObjectsAt(A, nullptr, nullptr, 0);
}

// 联机客人出手：本地只演资源采集（砍自己的世界副本），打鬼走房主判定
static void GuestAttack() {
    static const float FACE[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
    int dir = P.dir;
    float fx = FACE[dir][0], fy = FACE[dir][1];
    playerAttackedFrame = true;

    Net::Buf b;
    b.d[0] = 'A'; b.n = 1;
    Net::W8(b, (unsigned)dir);

    if (hotSel == 1 && P.toolLv[1]) {          // 猎弓：本地演出 + 房主放箭
        float sp = 300.0f + 40.0f * P.toolLv[1];
        FireArrow(P.x + fx * 10.0f, P.y - 8 + fy * 10.0f, fx * sp, fy * sp);
        Net::UdpSend(netSock, netAddrHost, netPortHost, b);
        return;
    }
    SweepActor A;
    A.ox = P.x; A.oy = P.y - 8.0f; A.dir = dir; A.hotSel = hotSel;
    memcpy(A.toolLv, P.toolLv, 6);
    A.swordLv = (int)P.swordLv;
    A.dmg = PlayerDmg();
    if (hotSel == 2 && P.toolLv[2]) A.dmg += 1 + P.toolLv[2];
    if (hotSel == 4 && P.toolLv[4]) A.dmg += 4 + 2 * P.toolLv[4];
    A.heavyHit = (hotSel == 4 && P.toolLv[4]);
    Net::UdpSend(netSock, netAddrHost, netPortHost, b);   // 打鬼判定（房主）

    int txA[8], tyA[8];
    WSuppressDrops(true);
    int n = SweepObjectsAt(A, txA, tyA, 8);               // 本地采集演出
    WSuppressDrops(false);
    if (n > 0) {                                          // 命中瓦片上报房主结算（掉落房主生成）
        Net::Buf h;
        h.d[0] = 'H'; h.n = 1;
        Net::W8(h, (unsigned)n);
        for (int i = 0; i < n; i++) { Net::W16(h, (unsigned)txA[i]); Net::W16(h, (unsigned)tyA[i]); }
        Net::UdpSend(netSock, netAddrHost, netPortHost, h);
    }
}

// ---------------- 资源采集 ----------------

// 采集扣血（饱和减法）：hp 是 unsigned char，直接 -= 会下溢回绕成 255 导致资源永不破坏
static void GiveTalisman(int t, int n);      // 前向声明（HitObject 掘坟产出符咒用）

static inline void HarvestHp(unsigned char& hp, int pow) {
    playerMining = true;          // 动土（砍/挖/采）：会惊动《擅入者死》的鬼
    hp = (pow >= (int)hp) ? (unsigned char)0 : (unsigned char)((int)hp - pow);
}

void HitObject(int idx) {
    WorldObj& o = CurObjs()[(size_t)idx];
    WakeObj(idx);                              // 登记活跃物体（晃动/再生计时走小列表）
    float cx = o.tx * 16.0f + 8.0f, cy = o.ty * 16.0f + 8.0f;
    int pow = P.toolLv[3] ? (P.toolLv[3] > 1 ? 3 : 2) : 1;   // 铁斧：敲击计数（Lv2 三倍）
    // 清除物体占位
    auto ClearObjAt = [&o]() {
        W.objAt[(size_t)o.ty * MAP_W + o.tx] = -1;
    };
    switch (o.kind) {
    case ObjKind::Tree:
    case ObjKind::TreePine:      // 松/桦/棕榈与阔叶树同规则：可砍可掉木
    case ObjKind::TreeBirch:
    case ObjKind::TreePalm:
        HarvestHp(o.hp, pow);
        o.shake = 0.22f;
        PlaySound(AU.chop);
        SpawnParticles(cx, cy + 5, 6, 140, 96, 52, 90);       // 木屑
        if (o.hp <= 0) {
            o.kind = ObjKind::Stump;                          // 留树桩
            o.hp = 0;
            W.ClearTreeGround(o.tx, o.ty, A);                 // 该瓦片地面按地类还原，阴影残留清除
            PlaySound(AU.treeFall);
            SpawnParticles(cx, cy - 16, 16, 70, 140, 60, 130); // 树叶飞散
            SpawnParticles(cx, cy + 4, 10, 140, 96, 52, 110);
            W.SpawnDrop(ItemKind::Wood, cx, cy - 4, -42, -70);
            W.SpawnDrop(ItemKind::Wood, cx, cy - 4, 52, -60);
            if (HomeGather()) W.SpawnDrop(ItemKind::Wood, cx, cy - 4, 6, -80);   // 营1：多产一根
            shakeT = 0.12f; shakeDur = 0.12f;
            P.nTree++;
            AddXp(2);                                          // 撸树有经验
        }
        break;
    case ObjKind::Rock:
        HarvestHp(o.hp, pow);
        o.shake = 0.18f;
        PlaySound(AU.mine);
        SpawnParticles(cx, cy, 6, 150, 150, 155, 90);          // 石屑
        if (o.hp <= 0) {
            ClearObjAt();
            W.SpawnDrop(ItemKind::Stone, cx, cy, -48, -58);
            W.SpawnDrop(ItemKind::Stone, cx, cy, 58, -48);
            if (HomeGather()) W.SpawnDrop(ItemKind::Stone, cx, cy, 2, -72);      // 营1：多产一块
            SpawnParticles(cx, cy, 12, 150, 150, 155, 120);
            P.nRock++;
            AddXp(2);
        }
        break;
    case ObjKind::Berry:
        if (!o.harvested) {
            o.harvested = true;
            o.regrow = 60.0f;                                  // 60 秒再生
            o.shake = 0.15f;
            PlaySound(AU.pickup);
            SpawnParticles(cx, cy, 5, 220, 80, 90, 80);
            W.SpawnDrop(ItemKind::Berry, cx, cy - 2, 28, -75);
            if (HomeGather()) W.SpawnDrop(ItemKind::Berry, cx, cy - 2, -30, -72);  // 营1：多采一颗
            P.nBerry++;
            AddXp(1);
        }
        break;
    case ObjKind::TallGrass:
        ClearObjAt();
        PlaySound(AU.chop);
        SpawnParticles(cx, cy, 7, 90, 150, 70, 80);
        break;
    case ObjKind::GraveMound: {
        // 掘坟：产出萤晶/符文碎片，并有概率挖出符咒 —— 但动土会惊动附近的鬼
        o.shake = 0.25f;
        PlaySound(AU.mine);
        SpawnParticles(cx, cy, 8, 150, 130, 90, 90);
        HarvestHp(o.hp, 1);
        if (o.hp <= 0) {
            ClearObjAt();
            W.SpawnDrop(ItemKind::Crystal, cx, cy - 2, -40, -70);
            if ((rand() % 100) < 45) W.SpawnDrop(ItemKind::GemShard, cx, cy - 2, 40, -66);
            if ((rand() % 100) < 45) GiveTalisman(rand() % 5, 1);
            gGraveDug++;                            // 残影 / 成就：入土为安
            FloatText(P.x, P.y - 34, "掘开了坟", 226, 40, 46);
        }
        break;
    }
    case ObjKind::OreRock:
        // 矿脉专属获取：必须装备铁镐（环形栏槽 5 选中）
        if (!(hotSel == 5 && P.toolLv[5])) {
            o.shake = 0.15f;
            PlaySound(AU.mine);
            SpawnParticles(cx, cy, 3, 150, 150, 155, 60);
            FloatText(P.x, P.y - 34, "需要铁镐", 255, 160, 120);
            break;
        }
        HarvestHp(o.hp, 1 + P.toolLv[5] / 2);              // 镐 Lv2 采集更快
        o.shake = 0.2f;
        PlaySound(AU.mine);
        SpawnParticles(cx, cy, 7, 180, 200, 220, 90);      // 矿屑
        if (o.hp <= 0) {
            ClearObjAt();
            W.SpawnDrop(ItemKind::IronOre, cx, cy - 2, -40, -70);
            W.SpawnDrop(ItemKind::IronOre, cx, cy - 2, 44, -62);
            if ((rand() % 100) < 30)                       // 30% 萤晶
                W.SpawnDrop(ItemKind::Crystal, cx, cy - 2, 0, -80);
            if ((rand() % 100) < 12)                       // 12% 符文碎片（工具升级素材）
                W.SpawnDrop(ItemKind::GemShard, cx, cy - 2, -16, -84);
            SpawnParticles(cx, cy, 14, 190, 210, 235, 120);
            P.nRock++;
            AddXp(4);
        }
        break;
    case ObjKind::Chest:
        if (!o.harvested) {
            o.harvested = true;                                // chestOpen 贴图
            P.nChest++;
            AddXp(20);
            PlaySound(AU.craft);
            shakeT = 0.12f; shakeDur = 0.12f;
            SpawnParticles(cx, cy - 8, 18, 255, 220, 110, 130); // 金光喷发
            // ---- 遗迹战利品：稳定基础掉落 + 幸运加成 ----
            W.SpawnDrop(ItemKind::IronOre, cx, cy - 2, -34, -75);
            W.SpawnDrop(ItemKind::IronOre, cx, cy - 2, 36, -68);
            W.SpawnDrop(ItemKind::Crystal, cx, cy - 2, 0, -82);
            W.SpawnDrop(ItemKind::Wood, cx, cy - 2, -52, -55);
            W.SpawnDrop(ItemKind::Stone, cx, cy - 2, 54, -50);
            if ((rand() % 100) < 40)                           // 40% 欧皇加成：双萤晶
                W.SpawnDrop(ItemKind::Crystal, cx, cy - 2, -20, -86);
            FloatText(P.x, P.y - 36, "宝藏!", 255, 225, 120);
        }
        break;
    default:
        break;
    }
}

// ---------------- 生存动作：吃 / 合成 / 烤肉 ----------------
// （进食由环形物品栏 UseHotSlot 接管；这里仅保留合成与烤肉）

// 获得符咒（上限 8/种）
static void GiveTalisman(int t, int n) {
    if (t < 0 || t >= 5) return;
    P.talN[t] += n;
    if (P.talN[t] > BindTalMax()) P.talN[t] = BindTalMax();
}

// 使用符咒：找最近的敌对鬼为目标；封门符作用于最近的墙
static void UseTalisman(int t) {
    if (P.dead || t < 0 || t >= 5) return;
    if (P.talN[t] <= 0) {
        FloatText(P.x, P.y - 34, TextFormat(L10N("没有%s"), L10N(TAL_NAME[t])), 255, 120, 120);
        return;
    }
    if (P.talCd[t] > 0) {
        FloatText(P.x, P.y - 34, TextFormat(L10N("%s冷却%d秒"), L10N(TAL_NAME[t]), (int)(P.talCd[t] + 0.99f)),
                  255, 160, 120);
        return;
    }
    // 最近的敌对鬼（240px 内）
    int best = -1;
    float bd = 240.0f * 240.0f;
    for (size_t i = 0; i < mobs.size(); i++) {
        const Creature& c = mobs[i];
        if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
        float dx = c.x - P.x, dy = c.y - P.y;
        float q = dx * dx + dy * dy;
        if (q < bd) { bd = q; best = (int)i; }
    }
    bool used = false;
    int lv = P.talLv[t]; if (lv < 1) lv = 1; if (lv > 3) lv = 3;
    switch (t) {
    case TAL_BIND: {                                 // 困符：困住鬼 N 秒（符力越深困得越久）
        if (best >= 0) {
            Creature& c = mobs[(size_t)best];
            c.state = AState::Hurt; c.timer = TAL_BIND_T[lv - 1]; c.hurtFlash = 0.3f;
            c.kvx = 0; c.kvy = 0;
            SpawnParticles(c.x, c.y - 10, 10, 226, 40, 46, 90);
            FloatText(c.x, c.y - 26, TextFormat(L10N("困 %ds"), (int)TAL_BIND_T[lv - 1]), 226, 40, 46);
            used = true;
        } else FloatText(P.x, P.y - 34, "附近无鬼", 255, 160, 120);
        break;
    }
    case TAL_BANISH: {                               // 速符：符风灌体，疾行 N 秒
        P.talSpdT = TAL_SPD_T[lv - 1];
        SpawnParticles(P.x, P.y - 8, 16, 120, 235, 255, 150);
        FloatText(P.x, P.y - 34, TextFormat(L10N("疾行 %ds!"), (int)TAL_SPD_T[lv - 1]), 120, 235, 255);
        used = true;
        break;
    }
    case TAL_SEAL: {                                 // 离符：拨散符墨，N% 概率抹去鬼的 Q 标记
        if (best >= 0) {
            Creature& c = mobs[(size_t)best];
            if (!c.marked) {
                FloatText(P.x, P.y - 34, "它没有被标记", 255, 160, 120);
                break;                               // 不消耗不进冷却
            }
            used = true;
            // 离符亦能涤净阴物附身：顺手解除无脸鬼的记忆篡改（概念同源：拨散阴物留痕）
            if (P.hacked) {
                P.hacked = false; P.hackT = 0;
                FloatText(P.x, P.y - 40, "记忆恢复了", 150, 255, 200);
                SpawnCapFx(1, P.x, P.y, 0, 0);
                SetSoundPitch(AU.memoryRestore, 1.0f);
                PlaySound(AU.memoryRestore);
            }
            if ((rand() % 100) < TAL_AWAY_P[lv - 1]) {
                c.marked = false;
                SpawnParticles(c.x, c.y - 24, 12, 200, 180, 255, 120);
                FloatText(c.x, c.y - 26, "标记已抹去", 200, 180, 255);
            } else {
                FloatText(c.x, c.y - 26, "符力被弹开了", 160, 150, 180);
            }
        } else FloatText(P.x, P.y - 34, "附近无鬼", 255, 160, 120);
        break;
    }
    case TAL_BOLT: {                                 // 黄符：近身贴上鬼的脑门，让它暂时降 1 级
        if (best >= 0) {
            Creature& c = mobs[(size_t)best];
            float dx = c.x - P.x, dy = c.y - P.y;
            if (dx * dx + dy * dy > 96.0f * 96.0f) {  // 贴符要近身
                FloatText(P.x, P.y - 34, "太远了，贴不上（需近身）", 255, 160, 120);
                break;                               // 不消耗不进冷却
            }
            if (c.tier <= 0) {
                FloatText(P.x, P.y - 34, "它已经是最弱的鬼了", 255, 160, 120);
                break;
            }
            used = true;
            c.lowerT = TAL_LOW_T[lv - 1];
            c.triggered = true;                      // 贴脸行为当然会激怒它
            SpawnParticles(c.x, c.y - 20, 14, 255, 214, 90, 120);
            FloatText(c.x, c.y - 26, TextFormat(L10N("降1级 %ds"), (int)TAL_LOW_T[lv - 1]), 255, 214, 90);
            FloatText(P.x, P.y - 44, "现在能用低一级的器物收它了", 255, 224, 138);
        } else FloatText(P.x, P.y - 34, "附近无鬼", 255, 160, 120);
        break;
    }
    case TAL_HIDE: {                                 // 仇符：挑拨离间，N% 概率让鬼与最近同类相斗
        if (best >= 0) {
            // 找除目标外最近的另一只活鬼
            int rival = -1;
            float rd = 260.0f * 260.0f;
            for (size_t i = 0; i < mobs.size(); i++) {
                if ((int)i == best) continue;
                const Creature& o = mobs[i];
                if (o.state == AState::Dead || !MobHostile(o.kind)) continue;
                float dx = o.x - mobs[(size_t)best].x, dy = o.y - mobs[(size_t)best].y;
                float q = dx * dx + dy * dy;
                if (q < rd) { rd = q; rival = (int)i; }
            }
            if (rival < 0) { FloatText(P.x, P.y - 34, "附近只有一只鬼，挑拨不动", 255, 160, 120); break; }
            used = true;
            if ((rand() % 100) < TAL_RIVAL_P[lv - 1]) {
                Creature& a = mobs[(size_t)best];
                Creature& b = mobs[(size_t)rival];
                a.fightT = 12.0f; a.fightWith = rival; a.fightHitT = 0.4f;
                b.fightT = 12.0f; b.fightWith = best;  b.fightHitT = 0.8f;
                a.state = AState::Wander; a.timer = 0.2f;
                b.state = AState::Wander; b.timer = 0.2f;
                SpawnParticles((a.x + b.x) / 2, (a.y + b.y) / 2 - 12, 16, 255, 90, 70, 130);
                FloatText((a.x + b.x) / 2, (a.y + b.y) / 2 - 30, "鬼鬼相斗!", 255, 90, 70);
            } else {
                FloatText(mobs[(size_t)best].x, mobs[(size_t)best].y - 26, "它们没上当", 160, 150, 180);
            }
        } else FloatText(P.x, P.y - 34, "附近无鬼", 255, 160, 120);
        break;
    }
    default: break;
    }
    if (used) {
        P.talN[t]--;
        P.talCd[t] = TAL_CD[t] * BindCdMul();
        P.talUse[t]++;
        if (P.talUse[t] >= BindUpNeed() && P.talLv[t] < 3) {     // 用符 3 次升 1 级（符力随用随长）
            P.talUse[t] = 0;
            P.talLv[t]++;
            FloatText(P.x, P.y - 52, TextFormat(L10N("%s 升至 %d 级!"), L10N(TAL_NAME[t]), P.talLv[t]), 255, 224, 138);
            PlaySound(AU.craft);
        }
        PlaySound(AU.capture);
    }
}

// ---- E 键统一交互（铁匠 / 交易鬼 / 游戏鬼猜拳 / 幸存者 / 营地石碑 / 床）----
// 实现在下方 TryInteract：删除了旧建造系统（B/N 砌墙铺床）——屋子现在自然生成
static void TryInteract();

static void TryCraft() {
    if (P.dead) return;
    if (P.wood < 5) { FloatText(P.x, P.y - 34, "需要5木", 255, 120, 120); return; }
    static const float FACE[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
    float fx = FACE[P.dir][0], fy = FACE[P.dir][1];
    int tx = (int)((P.x + fx * 26) / TILE);
    int ty = (int)((P.y + fy * 26) / TILE);
    if (W.PlaceCampfire(tx, ty)) {
        P.wood -= 5;
        PlaySound(AU.craft);
        PlaySound(AU.place);
        SpawnParticles(tx * 16.0f + 8, ty * 16.0f + 8, 10, 255, 180, 80, 90);
        FloatText(P.x, P.y - 34, "长明灯已放置", 255, 200, 100);
        P.nFire++;                               // 成就：火的力量
    } else {
        FloatText(P.x, P.y - 34, "这里放不下", 255, 120, 120);
    }
}

// ---------------- 合成（TAB 面板；页 0 地表 / 页 1 废弃工作台限定） ----------------
// 工具槽位：0 铁剑 1 猎弓 2 猎刀 3 铁斧 4 铁锤 5 铁镐
// ---------------- 合成动画：材料图标表 ----------------
static Texture2D DropTex(ItemKind k);      // 前向声明（定义在本文件下方）

// 每个配方取 CRAFT_MAT_MAX 个材料图标用于环形摆放；
// 材料种类不足 3 种时按配比重复主材料，保证环绕视觉饱满。
// 顺序与 CraftMenu 中该配方的材料消耗一一对应。
static const ItemKind CRAFT_MATS[CRAFT_CNT][CRAFT_MAT_MAX] = {
    { ItemKind::IronOre,    ItemKind::Wood,     ItemKind::IronOre  },  // 0 铁剑 Lv1
    { ItemKind::IronOre,    ItemKind::Wood,     ItemKind::IronOre  },  // 1 铁斧 Lv1
    { ItemKind::Wood,       ItemKind::Wood,     ItemKind::IronOre  },  // 2 猎弓 Lv1
    { ItemKind::IronOre,    ItemKind::Wood,     ItemKind::IronOre  },  // 3 猎刀 Lv1
    { ItemKind::IronOre,    ItemKind::Stone,    ItemKind::IronOre  },  // 4 铁锤 Lv1
    { ItemKind::IronOre,    ItemKind::Wood,     ItemKind::IronOre  },  // 5 铁镐 Lv1
    { ItemKind::Wood,       ItemKind::Wood,     ItemKind::Stone    },  // 6 粗纸摄魂幡 Lv1
    { ItemKind::IronOre,    ItemKind::IronOre,  ItemKind::Wood     },  // 7 铁甲 Lv2
    { ItemKind::Crystal,    ItemKind::Berry,    ItemKind::Berry    },  // 8 萤火药剂 Lv2
    { ItemKind::Crystal,    ItemKind::Berry,    ItemKind::Berry    },  // 9 萤晶茶 Lv2
    { ItemKind::Berry,      ItemKind::Berry,    ItemKind::Berry    },  // 10 血莓酱 Lv2
    { ItemKind::CookedMeat, ItemKind::Berry,    ItemKind::Berry    },  // 11 炖肉 Lv2（需篝火）
    { ItemKind::IronOre,    ItemKind::GemShard, ItemKind::IronOre  },  // 12 工具升级 Lv2
    { ItemKind::IronOre,    ItemKind::Crystal,  ItemKind::GemShard },  // 13 铜铃摄魂幡 Lv2
    { ItemKind::Crystal,    ItemKind::GemShard, ItemKind::Heart    },  // 14 鎏金摄魂幡 Lv3
};

// 装载当前配方的材料图标（合成动画环形摆放用）
static void SetCraftMats(int idx) {
    if (idx < 0 || idx >= CRAFT_CNT) return;
    for (int i = 0; i < CRAFT_MAT_MAX; i++)
        craftMat[i] = DropTex(CRAFT_MATS[idx][i]);
    craftMatN = CRAFT_MAT_MAX;
}

// 各配方所需工作台等级（1/2/3；3 = 鬼界物品，需收服铁匠鬼）
static int RecipeBenchLv(int idx) {
    if (idx <= 6) return 1;
    if (idx <= 17) return 2;      // 15~17 傀儡丝 / 引魂灯 / 镇魂钉
    return 3;                     // 18~22 萤晶甲 / 符匣 / 摄魂铃 / 血月大剑 / 还魂香
}

// 附近可用的工作台等级（含营地工匠：庇护屋内工匠入住 = 2 级台效果）
static int BenchLvNear() {
    int lv = 0;
    int wi = W.NearWorkbench(P.x, P.y, 56.0f);
    if (wi >= 0) lv = (int)W.objs[(size_t)wi].hp;
    if (campBuilt && lv < 2)
        for (const Npc& n : npcs)
            if (n.on && n.kind == 1 && n.state == 2 && n.job == 3) {
                float dx = n.x - P.x, dy = n.y - P.y;
                if (dx * dx + dy * dy < 60.0f * 60.0f) { lv = 2; break; }
            }
    return lv;
}

static const char* ToolName(int slot) {
    static const char* N[6] = { "辟邪桃木剑", "猎弓", "猎刀", "铁斧", "铁锤", "铁镐" };
    return N[slot];
}

// 合成成功反馈：音效 + 粒子 + 飘字 + 播放合成动画（世界短暂冻结）
static void CraftFx(Texture2D icon, const char* label,
                    unsigned char r, unsigned char g, unsigned char b) {
    PlaySound(AU.craft);
    PlaySound(AU.place);
    craftAnimOn = true;
    craftAnimT = 0;
    craftAnimIcon = icon;
    gNCraft++;                               // 悬赏计数
    SpawnParticles(P.x, P.y - 14, 14, r, g, b, 120);
    FloatText(P.x, P.y - 38, label, r, g, b);
}

static void CraftMenu(int idx) {
    if (P.dead) return;
    if (idx < 0 || idx >= CRAFT_CNT) return;
    SetCraftMats(idx);      // 预载材料图标供合成动画环形摆放（合成失败亦无副作用）
    // ---- 工作台等级 gate：合成必须靠近工作台（等级由铁匠授予/升级）----
    int bench = BenchLvNear();
    int need = RecipeBenchLv(idx);
    if (bench <= 0) { FloatText(P.x, P.y - 34, "需靠近工作台", 255, 120, 120); return; }
    if (bench < need) {
        FloatText(P.x, P.y - 34, TextFormat(L10N("需%d级工作台"), need), 255, 120, 120);
        return;
    }
    // ---- 图鉴 gate：没悟得的配方做不出来（面板里显示为 ？？？，只给一句线索）----
    if (!Prog::RecipeKnown(idx)) {
        FloatText(P.x, P.y - 34, "尚不知此物做法", 255, 160, 120);
        return;
    }
    switch (idx) {
    case 0:   // 铁剑：铁矿×3 + 木×2 → 攻击 +3（Lv1）
        if (P.toolLv[0]) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.iron < 3 || P.wood < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 3; P.wood -= 2;
        P.toolLv[0] = 1;
        CraftFx(A.dropIronOre, "辟邪桃木剑! 攻击+3", 180, 220, 255);
        break;
    case 1:   // 铁斧：铁矿×2 + 木×2 → 采集翻倍（Lv1）
        if (P.toolLv[3]) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.iron < 2 || P.wood < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 2; P.wood -= 2;
        P.toolLv[3] = 1;
        CraftFx(A.dropIronOre, "铁斧! 采集翻倍", 240, 210, 160);
        break;
    case 2:   // 猎弓：木×3 + 铁矿×1 → 远程射击（Lv1）
        if (P.toolLv[1]) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.wood < 3 || P.iron < 1) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.wood -= 3; P.iron -= 1;
        P.toolLv[1] = 1;
        CraftFx(A.toolBow, "猎弓! 远程射击", 200, 235, 180);
        break;
    case 3:   // 猎刀：铁矿×2 + 木×1 → 攻速提升（Lv1）
        if (P.toolLv[2]) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.iron < 2 || P.wood < 1) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 2; P.wood -= 1;
        P.toolLv[2] = 1;
        CraftFx(A.toolKnife, "猎刀! 攻速提升", 225, 225, 238);
        break;
    case 4:   // 铁锤：铁矿×3 + 石×2 → 重击高伤（Lv1）
        if (P.toolLv[4]) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.iron < 3 || P.stone < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 3; P.stone -= 2;
        P.toolLv[4] = 1;
        CraftFx(A.toolHammer, "铁锤! 重击", 235, 210, 160);
        break;
    case 5:   // 铁镐：铁矿×2 + 木×2 → 开采矿脉（Lv1）
        if (P.toolLv[5]) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.iron < 2 || P.wood < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 2; P.wood -= 2;
        P.toolLv[5] = 1;
        CraftFx(A.toolPick, "铁镐! 可开采矿脉", 210, 225, 240);
        break;
    case 6:   // 粗纸摄魂幡：木×3 + 石×2 → 收鬼能力 Lv1（Lv1）
        if (P.captureLv >= 1) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.wood < 3 || P.stone < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.wood -= 3; P.stone -= 2;
        P.captureLv = 1;
        CraftFx(A.toolFan1, "摄魂幡! 按V收鬼", 150, 255, 200);
        break;
    case 7:   // 铁甲：铁矿×5 + 木×2 → 减伤 38%（Lv2）
        if (P.armorLv >= 1) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.iron < 5 || P.wood < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 5; P.wood -= 2;
        P.armorLv = 1;
        CraftFx(A.armorIron, "铁甲! 减伤38%", 200, 210, 230);
        break;
    case 8:   // 萤火药剂：萤晶×1 + 浆果×3 → 药剂入包（Lv2）
        if (P.crystal < 1 || P.berry < 3) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.crystal--; P.berry -= 3;
        P.potion++;
        CraftFx(A.dropCrystal, "萤火药剂!", 150, 255, 170);
        break;
    case 9:   // 萤晶茶：萤晶×1 + 浆果×2 → 立即饮用，夜视 90 秒（Lv2）
        if (P.crystal < 1 || P.berry < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.crystal--; P.berry -= 2;
        P.buffNightT = 90.0f;
        CraftFx(A.dropTea, "萤晶茶! 夜视90秒", 150, 255, 200);
        break;
    case 10:  // 血莓酱：浆果×3 → 立即食用，加速 30 秒（Lv2）
        if (P.berry < 3) { FloatText(P.x, P.y - 34, "血莓不足", 255, 120, 120); return; }
        P.berry -= 3;
        P.buffSpdT = 30.0f;
        P.hunger += 15; if (P.hunger > 100) P.hunger = 100;
        P.nEat++;
        CraftFx(A.dropJam, "血莓酱! 加速30秒", 230, 130, 220);
        break;
    case 11: { // 炖肉：熟肉×1 + 浆果×2（需长明灯）（Lv2）
        if (W.NearCampfire(P.x, P.y, 46) < 0) { FloatText(P.x, P.y - 34, "需要长明灯", 255, 160, 120); return; }
        if (P.cookedMeat < 1 || P.berry < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.cookedMeat--; P.berry -= 2;
        P.stew++;
        CraftFx(A.dropStew, "炖肉! 键0食用", 250, 190, 110);
        break; }
    case 12: { // 工具矿石升级：铁矿×2 + 符文碎片×1 → 环形栏选中工具 +1 级（Lv2）
        int s = hotSel;
        if (s > 5 || !P.toolLv[s]) { FloatText(P.x, P.y - 34, "先在环形栏选中工具", 255, 200, 120); return; }
        if (P.toolLv[s] >= 2) { FloatText(P.x, P.y - 34, "已满级", 255, 200, 120); return; }
        if (P.iron < 2 || P.gemShard < 1) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 2; P.gemShard--;
        P.toolLv[s]++;
        Texture2D ic = A.dropIronOre;
        switch (s) {
        case 1: ic = A.toolBowOre; break;
        case 2: ic = A.toolKnifeOre; break;
        case 3: ic = A.toolAxeOre; break;
        case 4: ic = A.toolHammerOre; break;
        case 5: ic = A.toolPickOre; break;
        default: break;
        }
        CraftFx(ic, TextFormat(L10N("%s 升级 Lv%d"), ToolName(s), P.toolLv[s]), 130, 220, 255);
        break; }
    case 13:  // 铜铃摄魂幡：铁矿×3 + 萤晶×1 + 符文碎片×1（需粗纸幡）（Lv2）
        if (P.captureLv >= 2) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.captureLv < 1) { FloatText(P.x, P.y - 34, "需粗纸幡", 255, 160, 120); return; }
        if (P.iron < 3 || P.crystal < 1 || P.gemShard < 1) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 3; P.crystal--; P.gemShard--;
        P.captureLv = 2;
        CraftFx(A.toolFan2, "摄魂幡升级! 鬼仆x2", 150, 255, 200);
        break;
    case 14:  // 鎏金摄魂幡：萤晶×3 + 符文碎片×3 + 血月之心×1（需铜铃幡）（Lv3 鬼界）
        if (P.captureLv >= 3) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.captureLv < 2) { FloatText(P.x, P.y - 34, "需铜铃幡", 255, 160, 120); return; }
        if (P.crystal < 3 || P.gemShard < 3 || P.heart < 1) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.crystal -= 3; P.gemShard -= 3; P.heart--;
        P.captureLv = 3;
        CraftFx(A.toolFan3, "鎏金摄魂幡! 鬼仆x3", 255, 224, 138);
        break;
    case 15:  // 傀儡丝：木20 + 铁3 -> 同控上限 +1（与驭命途叠加）
        if (P.wood < 20 || P.iron < 3) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.wood -= 20; P.iron -= 3;
        gPuppetExtra++;
        CraftFx(A.toolFan2, TextFormat(L10N("傀儡丝! 同控上限%d"), RideGrpMax()), 150, 220, 255);
        break;
    case 16:  // 引魂灯：木10 + 晶2 -> 自带常亮光环
        if (gLantern) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.wood < 10 || P.crystal < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.wood -= 10; P.crystal -= 2;
        gLantern = true;
        CraftFx(A.dropCrystal, "引魂灯! 夜里有光跟着你", 150, 255, 200);
        break;
    case 17:  // 镇魂钉：铁5 + 石10 -> 领地镇宅再强化
        if (gWardNail) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.iron < 5 || P.stone < 10) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 5; P.stone -= 10;
        gWardNail = true;
        CraftFx(A.dropIronOre, "镇魂钉! 邪物难近领地", 235, 210, 160);
        break;
    case 18:  // 萤晶甲：晶5 + 铁5 -> 减伤 55%
        if (P.armorLv >= 2) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.crystal < 5 || P.iron < 5) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.crystal -= 5; P.iron -= 5;
        P.armorLv = 2;
        CraftFx(A.armorIron, "萤晶甲! 减伤55%", 150, 255, 200);
        break;
    case 19:  // 辟邪符匣：木15 + 铁2 -> 每种符上限 +2
        if (P.wood < 15 || P.iron < 2) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.wood -= 15; P.iron -= 2;
        gTalismanExtra += 2;
        CraftFx(A.toolFan1, TextFormat(L10N("辟邪符匣! 符上限%d"), BindTalMax()), 216, 212, 200);
        break;
    case 20:  // 摄魂铃：铁8 + 晶3 -> 收鬼冷却 -30%
        if (gCapBell) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.iron < 8 || P.crystal < 3) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.iron -= 8; P.crystal -= 3;
        gCapBell = true;
        CraftFx(A.toolFan2, "摄魂铃! 收鬼冷却-三成", 150, 255, 200);
        break;
    case 21:  // 血月大剑：血月之心1 + 铁10 -> 终极武器
        if (P.toolLv[0] >= 3) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.heart < 1 || P.iron < 10) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.heart -= 1; P.iron -= 10;
        P.toolLv[0] = 3;
        CraftFx(A.dropHeart, "血月大剑! 攻击+8", 255, 120, 120);
        break;
    case 22:  // 还魂香：晶3 + 莓5 -> 替你死一次
        if (gRevive) { FloatText(P.x, P.y - 34, "已拥有", 255, 200, 120); return; }
        if (P.crystal < 3 || P.berry < 5) { FloatText(P.x, P.y - 34, "材料不足", 255, 120, 120); return; }
        P.crystal -= 3; P.berry -= 5;
        gRevive = true;
        CraftFx(A.dropJam, "还魂香! 这一命先记着", 230, 130, 220);
        break;

    default:
        break;
    }
}

static void TryCook() {
    if (P.dead) return;
    if (W.NearCampfire(P.x, P.y, 46) < 0) { FloatText(P.x, P.y - 34, "需要长明灯", 255, 160, 120); return; }
    if (P.rawMeat <= 0) { FloatText(P.x, P.y - 34, "没有生肉", 255, 160, 120); return; }
    P.rawMeat--;
    P.cookedMeat++;
    gNCook++;                                // 悬赏计数
    PlaySound(AU.craft);
    SpawnParticles(P.x, P.y - 14, 5, 255, 170, 70, 70);
    FloatText(P.x, P.y - 34, "烤好了", 255, 210, 110);
}

// 环形物品栏槽位使用：再次按下同槽号触发；工具槽(0-5)仅选中，6 药剂 7 浆果 8 熟肉
static void UseHotSlot(int n) {
    if (P.dead) return;
    playerUsedItem = true;              // 神秘复苏：用物会惊动窥人用物的缚灵术士
    switch (n) {
    case 6:      // 萤火药剂：回血 50
        if (P.potion <= 0) { FloatText(P.x, P.y - 34, "没有药剂", 255, 120, 120); return; }
        if (P.hp >= P.maxHp) { FloatText(P.x, P.y - 34, "血量已满", 255, 200, 120); return; }
        P.potion--;
        P.hp += 50;
        if (P.hp > P.maxHp) P.hp = P.maxHp;
        PlaySound(AU.eat);
        SpawnParticles(P.x, P.y - 14, 14, 140, 255, 170, 120);
        FloatText(P.x, P.y - 36, "回复生命", 150, 255, 170);
        break;
    case 7:      // 浆果：饱食 +15
        if (P.berry <= 0) { FloatText(P.x, P.y - 34, "没有血莓", 255, 120, 120); return; }
        P.berry--;
        P.hunger += 15;
        if (P.hunger > 100) P.hunger = 100;
        PlaySound(AU.eat);
        SpawnParticles(P.x, P.y - 14, 4, 240, 200, 100, 55);
        FloatText(P.x, P.y - 34, "+15", 140, 240, 120);
        P.nEat++;
        break;
    case 8:      // 熟肉：饱食 +40
        if (P.cookedMeat <= 0) { FloatText(P.x, P.y - 34, "没有熟肉", 255, 120, 120); return; }
        P.cookedMeat--;
        P.hunger += 40;
        if (P.hunger > 100) P.hunger = 100;
        PlaySound(AU.eat);
        SpawnParticles(P.x, P.y - 14, 4, 240, 200, 100, 55);
        FloatText(P.x, P.y - 34, "+40", 140, 240, 120);
        P.nEat++;
        break;
    case 9:      // 炖肉：饱食 +60 + 回血 buff 30 秒
        if (P.stew <= 0) { FloatText(P.x, P.y - 34, "没有炖肉", 255, 120, 120); return; }
        P.stew--;
        P.hunger += 60; if (P.hunger > 100) P.hunger = 100;
        P.buffRegenT = 30.0f; P.buffRegenTick = 0;
        PlaySound(AU.eat);
        SpawnParticles(P.x, P.y - 14, 6, 240, 200, 100, 55);
        FloatText(P.x, P.y - 36, "饱食+60 回血30秒", 140, 240, 120);
        P.nEat++;
        break;
    default:     // 工具槽：选中即装备（攻击行为由工具系统接管）
        break;
    }
}

// ---------------- 上下文路由（地表 / 地牢当前层） ----------------

static std::vector<WorldObj>& CurObjs() {
    return W.objs;
}
static std::vector<Drop>& CurDrops() {
    return W.drops;
}

// ---------------- 玩家更新 ----------------

static void TryTame();                       // 前向声明（T 键驯服，实现在下方）
static void TryCapture();                    // 前向声明（V 键收鬼，实现在下方）
static void GdExit();                        // 前向声明（收服域主后鬼域崩塌）
static void TryGourd();                      // 前向声明（Y 键收鬼入葫芦，实现在下方）
static void TryRefine();                     // 前向声明（H 键精炼鬼仆复刻度，实现在下方）
static void NetMaybeSendTalisman(int t);     // 前向声明（联机客人符咒上报，实现在联机服务块）

static void UpdatePlayer(float dt) {
    if (P.toolLv[0] > P.swordLv) P.swordLv = P.toolLv[0];   // 剑槽升级即桃木剑升级（合成/锻造两条路同步）
    if (P.invuln > 0) P.invuln -= dt;
    if (P.hurtFlash > 0) P.hurtFlash -= dt;
    if (P.atkCd > 0) P.atkCd -= dt;
    if (P.atkTime > 0) P.atkTime -= dt;
    // Buff 计时
    if (P.buffSpdT > 0) P.buffSpdT -= dt;
    if (P.talSpdT > 0) P.talSpdT -= dt;
    if (P.buffNightT > 0) P.buffNightT -= dt;
    // 死亡后 F/数字键的处理在本函数后半段，若此处直接 return 会导致开着环形栏死亡时无法关闭
    if (P.dead) { P.deadT += dt; hotbarOpen = false; return; }
    // 回血 buff：每 1 秒 +2
    if (P.buffRegenT > 0) {
        P.buffRegenT -= dt;
        P.buffRegenTick += dt;
        if (P.buffRegenTick >= 1.0f) {
            P.buffRegenTick -= 1.0f;
            if (P.hp < P.maxHp) {
                P.hp += 2;
                if (P.hp > P.maxHp) P.hp = P.maxHp;
                SpawnParticles(P.x, P.y - 16, 2, 150, 255, 170, 40);
            }
        }
    }

    // 八向移动（加减速平滑：速度向目标值插值，起步/停止有质感）
    float ix = 0, iy = 0;
    if (IsKeyDown(KEY_W)) iy -= 1;
    if (IsKeyDown(KEY_S)) iy += 1;
    if (IsKeyDown(KEY_A)) ix -= 1;
    if (IsKeyDown(KEY_D)) ix += 1;
    float len = sqrtf(ix * ix + iy * iy);
    bool moving = len > 0.01f;
    // 傀儡操控中：WASD 不再驾驶玩家本体，转向驾驶傀儡（本体站桩——附体的代价）
    if (puppetOn) {
        if (moving) { puppetMx = ix / len; puppetMy = iy / len; }
        else { puppetMx = 0.0f; puppetMy = 0.0f; }
        ix = 0.0f; iy = 0.0f; len = 0.0f; moving = false;
    } else { puppetMx = 0.0f; puppetMy = 0.0f; }
    playerMoving = moving;                                  // 供鬼的规律判定
    P.walking = moving || fabsf(P.vx) + fabsf(P.vy) > 12.0f;
    P.running = moving && IsKeyDown(KEY_LEFT_SHIFT);
    float spd = (P.running ? 170.0f : 120.0f) * (P.buffSpdT > 0 ? 1.22f : 1.0f) * (P.talSpdT > 0 ? 1.6f : 1.0f);
    // 泛舟：拥有小舟且脚下是水（地表限定）→ 水面可通行，航速略缓
    static bool prevSailing = false;
    playerSailing = CalcSailing();
    if (playerSailing) {
        spd *= 0.8f;
        if (!prevSailing) FloatText(P.x, P.y - 40, "泛舟渡海", 130, 220, 255);
    }
    prevSailing = playerSailing;
    // 小舟在手：地表水域不再视为墙（入水即上船，地牢水域仍是墙）
    const bool boatMode = P.boat;
    float tvx = 0, tvy = 0;
    if (moving) {
        ix /= len; iy /= len;
        if (fabsf(ix) > fabsf(iy)) P.dir = ix > 0 ? 2 : 3;
        else P.dir = iy > 0 ? 0 : 1;
        tvx = ix * spd; tvy = iy * spd;
    }
    float blend = (moving ? 14.0f : 18.0f) * dt;             // 起步快、停步更快
    if (blend > 1.0f) blend = 1.0f;
    P.vx += (tvx - P.vx) * blend;
    P.vy += (tvy - P.vy) * blend;
    if (fabsf(P.vx) > 0.5f || fabsf(P.vy) > 0.5f) {
        Vector2 np = W.MoveCircle(P.x, P.y, 5.0f, P.vx * dt, P.vy * dt, boatMode);
        P.x = np.x; P.y = np.y;
    }
    P.animT += moving ? dt : dt * 0.55f;

    // 受击击退滑移
    if (fabsf(P.kx) + fabsf(P.ky) > 0.5f) {
        Vector2 np = W.MoveCircle(P.x, P.y, 5.0f, P.kx * dt, P.ky * dt, boatMode);
        P.x = np.x; P.y = np.y;
        float dec = 1.0f - 8.0f * dt; if (dec < 0) dec = 0;
        P.kx *= dec; P.ky *= dec;
    }

    // 攻击（J 或 鼠标左键；冷却末段 0.16s 内输入自动缓冲，连击更顺滑）
    // 环形物品栏开启期间左键用于确认选槽，不触发攻击
    static float atkBuf = 0;
    bool atkPressed = ((IsKeyPressed(KEY_J) && !puppetOn) ||
                      (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hotbarOpen && !ghostBarOpen && !talBarOpen));
    if (atkPressed && P.atkCd > 0.0f && P.atkCd <= 0.16f) atkBuf = 0.16f;
    if (atkBuf > 0) atkBuf -= dt;
    if ((atkPressed && P.atkCd <= 0) || (atkBuf > 0 && P.atkCd <= 0)) {
        atkBuf = 0;
        if (hotSel == 2 && P.toolLv[2])            P.atkCd = 0.22f;   // 猎刀
        else if (hotSel == 4 && P.toolLv[4])       P.atkCd = 0.65f;   // 铁锤
        else if (hotSel == 1 && P.toolLv[1])       P.atkCd = 0.55f;   // 猎弓
        else                                       P.atkCd = 0.35f;
        P.atkTime = 0.18f;
        PlaySound(AU.swing);
        // 攻击前冲：向面向方向小幅位移，增强打击重量感
        static const float FACEL[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
        P.kx += FACEL[P.dir][0] * 110.0f;
        P.ky += FACEL[P.dir][1] * 110.0f;
        if (netOn && !netHost) GuestAttack();
        else DoAttack();
    }
    // ---- 环形物品栏：滚轮一动即唤出并切换，停手 2.2 秒自动收起（不再需要 F 键）----
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && !craftOpen) {
        hotbarOpen = true;
        ghostBarOpen = false;                              // 两个栏互斥
        hotSel = (hotSel + (wheel > 0 ? -1 : 1) + 10) % 10;
        hotbarHoldT = HOTBAR_HOLD;
        PlaySound(AU.pickup);
    }
    if (hotbarOpen) {
        hotbarHoldT -= dt;
        if (hotbarHoldT <= 0.0f) hotbarOpen = false;       // 停手后自动收起
    }
    // X：符咒环（与物品栏 / 收鬼栏互斥）。滚轮切换符咒，左键使用
    if (IsKeyPressed(KEY_X) && !craftOpen) {
        talBarOpen = !talBarOpen;
        if (talBarOpen) { hotbarOpen = false; ghostBarOpen = false; }
        PlaySound(AU.pickup);
    }
    if (talBarOpen) {
        float tw2 = GetMouseWheelMove();
        if (tw2 != 0.0f) P.talSel = (P.talSel + (tw2 > 0 ? 4 : 1)) % 5;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            NetMaybeSendTalisman(P.talSel);
            UseTalisman(P.talSel);
        }
    }
    // 符咒冷却 / 隐身计时
    for (int i = 0; i < 5; i++) if (P.talCd[i] > 0) P.talCd[i] -= dt;
    if (P.hideT > 0) P.hideT -= dt;

    // Q：标记 / 取消标记最近的鬼（标记后常亮显示它的规律，便于辨认与记忆）
    if (IsKeyPressed(KEY_Q) && !craftOpen && !P.dead) {
        int best = -1;
        float bd = 240.0f * 240.0f;
        for (size_t mi = 0; mi < mobs.size(); mi++) {
            const Creature& mc = mobs[mi];
            if (mc.state == AState::Dead || !MobHostile(mc.kind) || mc.infiltrated || mc.companion) continue;
            float mdx = mc.x - P.x, mdy = mc.y - P.y;
            float q = mdx * mdx + mdy * mdy;
            if (q < bd) { bd = q; best = (int)mi; }
        }
        if (best >= 0) {
            Creature& mc = mobs[(size_t)best];
            mc.marked = !mc.marked;
            PlaySound(AU.pickup);
            FloatText(mc.x, mc.y - 30, mc.marked ? "已标记" : "取消标记", 226, 40, 46);
        }
    }
    // F：切换收鬼栏（指挥已收服的鬼仆）——与环形物品栏/符咒环/幡面板/傀儡严格互斥，任何时刻只开一个
    if (IsKeyPressed(KEY_F) && !craftOpen) {
        ghostBarOpen = !ghostBarOpen;
        if (ghostBarOpen) { hotbarOpen = false; talBarOpen = false; bannerOpen = false; puppetOn = false; }
        PlaySound(AU.pickup);
    }
    // ---- 猜拳 / 交易面板：数字键 1/2/3 结算（优先于物品栏选槽）----
    if (rpsState == 1) {
        if (IsKeyPressed(KEY_ONE))   RpsResolve(0);
        if (IsKeyPressed(KEY_TWO))   RpsResolve(1);
        if (IsKeyPressed(KEY_THREE)) RpsResolve(2);
        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) rpsState = 0;   // 放弃
        return;   // 猜拳期间不做其他操作
    }
    if (dealState == 1) {
        if (IsKeyPressed(KEY_ONE))   DealResolve(1);
        if (IsKeyPressed(KEY_TWO))   DealResolve(2);
        if (IsKeyPressed(KEY_THREE)) DealResolve(3);
        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) dealState = 0; // 放弃
        return;
    }
    for (int n = 0; n < 10 && !gdOn && !ghostBarOpen; n++) {   // 收鬼栏开着：数字键不再唤出环形栏（防两栏重叠）
        KeyboardKey kk = (n == 9) ? KEY_ZERO : (KeyboardKey)(KEY_ONE + n);
        if (IsKeyPressed(kk)) {
            if (hotSel == n) UseHotSlot(n);
            else hotSel = n;
            hotbarOpen = true;                             // 选槽即唤出，便于确认
            hotbarHoldT = HOTBAR_HOLD;
        }
    }
    // ---- 收鬼栏指挥（独立于环形物品栏：不需要也不允许两栏同开）----
    // 面板内点行：左键放该鬼技能 / 右键切换出击；面板外：左键全体赴指针处 / 右键全体回幡
    if (ghostBarOpen) {
        bool inside = GB_Hit(168, 36, 304, 288);         // 面板内：逐行指令
        if (inside) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  GhostBarClick(false);
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) GhostBarClick(true);
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (ghosts.empty()) {
                FloatText(P.x, P.y - 38, "尚无鬼仆", 255, 160, 120);
            } else {
                Vector2 mg = MouseGame();
                for (size_t gi = 0; gi < ghosts.size(); gi++) {
                    float a = (float)gi * 1.9f;         // 扇形散开，避免挤成一团
                    ghosts[gi].cmdX = mg.x + camX + cosf(a) * 18.0f;
                    ghosts[gi].cmdY = mg.y + camY + sinf(a) * 18.0f;
                    ghosts[gi].out = true;
                }
                PlaySound(AU.place);
                FloatText(P.x, P.y - 38, "鬼仆听令", 120, 235, 190);
            }
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (!ghosts.empty()) {
                for (GhostAlly& g : ghosts) g.out = false;
                PlaySound(AU.pickup);
                FloatText(P.x, P.y - 38, "收鬼入幡", 120, 235, 190);
            }
        }
    } else if (hotbarOpen) {
        // 左键确认：消耗品槽使用，工具槽仅装备；确认后收起圆环
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int counts[10] = { P.toolLv[0], P.toolLv[1], P.toolLv[2],
                               P.toolLv[3], P.toolLv[4], P.toolLv[5],
                               P.potion, P.berry, P.cookedMeat, P.stew };
            if (counts[hotSel] > 0) {
                if (hotSel >= 6) UseHotSlot(hotSel);
                else PlaySound(AU.pickup);
                hotbarOpen = false;
            }
        }
    }
    // BGM 音量：[ 减 / ] 加（步进 0.1，范围 0..1）
    if (IsKeyPressed(KEY_LEFT_BRACKET) || IsKeyPressed(KEY_RIGHT_BRACKET)) {
        AU.bgmVol += IsKeyPressed(KEY_RIGHT_BRACKET) ? 0.1f : -0.1f;
        if (AU.bgmVol < 0.0f) AU.bgmVol = 0.0f;
        if (AU.bgmVol > 1.0f) AU.bgmVol = 1.0f;
        if (AU.bgm.ctxData != nullptr) SetMusicVolume(AU.bgm, AU.bgmVol);
        FloatText(P.x, P.y - 42, TextFormat(L10N("乐 %d%%"), (int)(AU.bgmVol * 100 + 0.5f)),
                  150, 235, 190);
    }
    if (IsKeyPressed(KEY_E)) TryInteract();                    // E：统一交互（铁匠/鬼/幸存者/营地/床）
    if (IsKeyPressed(KEY_C)) TryCraft();
    if (IsKeyPressed(KEY_G)) TryCook();
    if (IsKeyPressed(KEY_T)) TryTame();
    if (IsKeyPressed(KEY_V) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) TryCapture();
    if (IsKeyPressed(KEY_U)) TryGourd();                                // U：葫芦暂存（与 V 幡收服并列）
    if (IsKeyPressed(KEY_H) && !craftOpen && !P.dead) TryRefine();   // H：碎片精炼鬼仆复刻度
    if (IsKeyPressed(KEY_B) && !craftOpen) {                 // B：幡面板（收鬼全参数；与其他栏互斥）
        bannerOpen = !bannerOpen;
        if (bannerOpen) { ghostBarOpen = false; hotbarOpen = false; talBarOpen = false; }
    }
    if (IsKeyPressed(KEY_N) && !craftOpen && !P.dead) TryBuildTerritory();  // N：就地立界碑（建领地）
    if (IsKeyPressed(KEY_K) && !craftOpen && !P.dead) TryAskCompanion();    // K：问询同伴（再按一次驱逐）
    if (IsKeyPressed(KEY_L) && !craftOpen && !P.dead) {          // L：手册总册（命途/成就/图鉴/残卷）
        bookOpen = !bookOpen;
        if (bookOpen) { ghostBarOpen = false; hotbarOpen = false; talBarOpen = false; bannerOpen = false; smithOpen = false; bookSel = 0; }   // 与铁匠面板互斥，任何时刻只开一个
        PlaySound(AU.pickup);
    }
    if (bookOpen) { BookKeys(); return; }                        // 手册开启时独占按键

    // ---- 铁匠铺面板：开启时独占按键（1~5 选择 / Z X 调等级 / E 关闭）----
    if (smithOpen) { SmithPanelKeys(); return; }
    // ---- 鬼仆册（F）：左/右切页（镇鬼幡鬼仆 <-> 葫芦暂存）----
    if (ghostBarOpen) {
        if (IsKeyPressed(KEY_LEFT))  ghostBarPage = 0;
        if (IsKeyPressed(KEY_RIGHT)) ghostBarPage = 1;
        if (ghostBarPage == 1 && P.gourdLv == 0) ghostBarPage = 0;
    }

    // ---- 傀儡操控（R）：附体已收容的鬼仆——WASD 驾驶 / J 放它的技能 / ←→ 切换主控 / 空格 加入同控组 ----
    if (IsKeyPressed(KEY_R) && !craftOpen && !P.dead) {
        if (!puppetOn) {
            if (ghosts.empty()) {
                FloatText(P.x, P.y - 38, "幡中没有鬼仆可操控", 255, 160, 120);
            } else {
                puppetOn = true;
                ghostBarOpen = false; hotbarOpen = false; talBarOpen = false; bannerOpen = false;
                if (puppetGrp.size() != ghosts.size()) puppetGrp.assign(ghosts.size(), 0);
                puppetMain = 0;
                PlaySound(AU.capture);
                FloatText(P.x, P.y - 44, "傀儡操控：丝线已接上", 150, 220, 255);
            }
        } else {
            puppetOn = false;
            PlaySound(AU.pickup);
            FloatText(P.x, P.y - 44, "收回傀儡线", 150, 220, 255);
        }
    }
    if (puppetOn) {
        if (ghosts.empty()) { puppetOn = false; puppetMain = -1; puppetGrp.clear(); }
        else {
            if (puppetGrp.size() != ghosts.size()) puppetGrp.resize(ghosts.size(), 0);
            if (puppetMain < 0 || puppetMain >= (int)ghosts.size()) puppetMain = 0;
            if (IsKeyPressed(KEY_LEFT))  { puppetMain = (puppetMain + (int)ghosts.size() - 1) % (int)ghosts.size(); PlaySound(AU.pickup); }
            if (IsKeyPressed(KEY_RIGHT)) { puppetMain = (puppetMain + 1) % (int)ghosts.size(); PlaySound(AU.pickup); }
            if (IsKeyPressed(KEY_SPACE)) {          // 同控组加/减：一次控制多只鬼（上限随驭命途提升）
                if (!puppetGrp[(size_t)puppetMain]) {
                    int in = 1;                      // 主控自带一个名额
                    for (size_t i = 0; i < puppetGrp.size(); i++) if (puppetGrp[i]) in++;
                    if (in > gPuppetMax) gPuppetMax = in;               // 成就：三线同牵
                    if (in >= RideGrpMax()) {        // 丝线不够长
                        FloatText(P.x, P.y - 38, TextFormat(L10N("丝线只够牵住%d只"), RideGrpMax()), 255, 160, 120);
                        PlaySound(AU.hurt);
                    } else {
                        puppetGrp[(size_t)puppetMain] = 1;
                        GhostAlly& g = ghosts[(size_t)puppetMain];
                        FloatText(g.x, g.y - 26, "入傀儡阵", 150, 220, 255);
                        PlaySound(AU.pickup);
                    }
                } else {
                    puppetGrp[(size_t)puppetMain] = 0;
                    GhostAlly& g = ghosts[(size_t)puppetMain];
                    FloatText(g.x, g.y - 26, "出傀儡阵", 150, 220, 255);
                    PlaySound(AU.pickup);
                }
            }
            if (IsKeyPressed(KEY_J)) CastGhostSkill(puppetMain);   // 主控鬼放技能（目标 = 指针）
        }
    }

    // 脚下洞口 / 楼梯触发
}

// ---------------- 宠物系统：驯服 / 跟随 ----------------

// T 键驯服：消耗生肉×1，驯服最近 48px 内的兔子/小鹿
static void TryTame() {
    if (P.dead) return;
    if (netOn && !netHost) { FloatText(P.x, P.y - 40, "联机模式只有房主能驯服", 255, 200, 120); return; }
    if (PET.on) { FloatText(P.x, P.y - 34, "已有伙伴同行", 255, 200, 120); return; }
    if (P.rawMeat <= 0) { FloatText(P.x, P.y - 34, "需要生肉", 255, 120, 120); return; }
    float best = 48.0f * 48.0f;
    int bi = -1;
    for (size_t i = 0; i < mobs.size(); i++) {
        const Creature& c = mobs[i];
        if (c.kind != CreatureKind::Rabbit && c.kind != CreatureKind::Deer) continue;
        if (c.state == AState::Dead) continue;
        float dx = c.x - P.x, dy = c.y - P.y, q = dx * dx + dy * dy;
        if (q < best) { best = q; bi = (int)i; }
    }
    if (bi < 0) { FloatText(P.x, P.y - 34, "附近没有猎物", 255, 160, 120); return; }
    Creature& c = mobs[(size_t)bi];
    P.rawMeat--;
    PET.on = true;
    PET.kind = (c.kind == CreatureKind::Rabbit) ? 0 : 1;
    PET.hp = PET.maxHp;
    PET.x = c.x; PET.y = c.y;
    PET.iv = 0; PET.hurtFlash = 0; PET.regenT = 0;
    c.state = AState::Dead; c.deadFade = 0.4f;   // 原体淡出（不掉落不加经验）
    PlaySound(AU.pickup);
    SpawnParticles(c.x, c.y - 8, 12, 255, 175, 195, 110);
    FloatText(P.x, P.y - 40, PET.kind == 0 ? "驯服了素兔!" : "驯服了引魂鹿!", 255, 185, 205);
}

// 宠物每帧更新：跟随玩家（远距疾跑/超远传送），脱战缓慢回血
// 伙伴系统：陪伴积累经验升级（生命/爪击成长），主动爪击附近的敌怪
static void UpdatePet(float dt) {
    if (!PET.on) return;
    if (PET.iv > 0) PET.iv -= dt;
    if (PET.hurtFlash > 0) PET.hurtFlash -= dt;
    if (PET.atkCd > 0) PET.atkCd -= dt;
    if (PET.hp < PET.maxHp) {                    // 每 3 秒回 1 点
        PET.regenT += dt;
        if (PET.regenT >= 3.0f) { PET.regenT -= 3.0f; PET.hp++; }
    }
    // ---- 陪伴经验：每秒 +1 ----
    PET.xp += (int)dt;
    if (PET.xp >= PetNextXp(PET.level)) {        // 升级：生命上限 + 生命成长 + 爪击更强
        PET.xp -= PetNextXp(PET.level);
        PET.level++;
        PET.maxHp += 12;
        PET.hp = PET.maxHp;
        PlaySound(AU.craft);
        SpawnParticles(PET.x, PET.y - 10, 10, 255, 200, 130, 100);
        FloatText(PET.x, PET.y - 28, TextFormat(L10N("伙伴 Lv%d!"), PET.level), 255, 225, 130);
    }
    float dx = P.x - PET.x, dy = P.y - PET.y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d > 280.0f) {                            // 走丢保护：直接传送到身后
        PET.x = P.x - 14.0f; PET.y = P.y + 8.0f;
        return;
    }
    // ---- 参战：爪击 34px 内最近的已触发敌鬼（伙伴不再只是跟班）----
    if (PET.atkCd <= 0) {
        for (Creature& c : mobs) {
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            if (MobUndying(c.kind)) continue;                // 不死单位：伙伴也啃不动
            float mdx = c.x - PET.x, mdy = c.y - PET.y;
            float md = sqrtf(mdx * mdx + mdy * mdy);
            if (md > 34.0f) continue;
            int pdmg = 4 + PET.level * 2;
            c.hp -= pdmg;
            c.hurtFlash = 0.1f;
            c.kvx += mdx / (md < 0.01f ? 1.0f : md) * 60.0f;
            c.kvy += mdy / (md < 0.01f ? 1.0f : md) * 60.0f;
            PET.atkCd = 1.2f;
            PlaySound(AU.hit);
            SetSoundPitch(AU.hit, 0.9f + (rand() % 20) / 100.0f);
            SpawnParticles(c.x, c.y - 10, 5, 255, 200, 130, 80);
            FloatText(c.x, c.y - 22, TextFormat("%d", pdmg), 255, 225, 130);
            if (c.hp <= 0 && !MobPreDeath(c)) {
                c.state = AState::Dead;
                c.deadFade = 0;
                MobDied(c.kind, c.x, c.y, c.tier, c.dmgMul);
            }
            break;
        }
    }
    if (d > 24.0f) {                             // 24px 内视为贴身，停止移动
        float spd = (d > 90.0f ? 195.0f : 125.0f) + PET.level * 3.0f;
        float nx = dx / d, ny = dy / d;
        PET.x += nx * spd * dt;
        PET.y += ny * spd * dt;
        PET.dir = fabsf(nx) > fabsf(ny) ? (nx > 0 ? 2 : 3) : (ny > 0 ? 0 : 1);
        PET.animT += dt;
    } else {
        PET.animT += dt * 0.4f;
    }
}

// ---------------- 收鬼系统 ----------------

// 怪物命中鬼仆（creature.cpp 盟友仇恨路由回调）
void AllyGhostHit(int idx, float dmg, float dirX, float dirY) {
    if (idx < 0 || idx >= (int)ghosts.size()) return;
    GhostAlly& g = ghosts[(size_t)idx];
    if (g.iv > 0) return;
    g.hp -= (int)(dmg + 0.5f);
    g.iv = 0.5f;
    g.hurtFlash = 0.12f;
    PlaySound(AU.zombieHit);
    SpawnParticles(g.x, g.y - 8, 5, 200, 120, 255, 80);
    FloatText(g.x, g.y - 20, TextFormat("-%d", (int)(dmg + 0.5f)), 200, 130, 255);
    if (g.hp <= 0) {                              // 鬼仆被击溃：魂火溃散
        SpawnParticles(g.x, g.y - 8, 12, 150, 220, 255, 100);
        FloatText(g.x, g.y - 26, "鬼仆消散", 150, 220, 255);
        ghosts.erase(ghosts.begin() + idx);
    }
}

// 鬼仆撕咬敌怪：直接扣血 + 击退 + 受击硬直（击杀走 MobDied 正常掉落）
// 鬼仆系统：击杀积累经验升级（伤害/生命成长）
// 无脸鬼鬼仆特技「记忆错乱」：撕咬 15% 概率让敌鬼忘了要杀谁，停止敌对 2.5 秒（以鬼制鬼）
static void HitMobByGhost(Creature& c, CreatureKind attacker, float dmg, float dx, float dy) {
    if (MobUndying(c.kind)) return;                  // 不死单位（铁匠鬼）：鬼仆也咬不动
    if (c.infiltrated) return;                       // 混在队伍里的无面鬼：自家鬼认不出它（无破绽）
    c.hp -= (int)(dmg + 0.5f);
    c.hurtFlash = 0.1f;
    float l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
    c.kvx += dx / l * 140.0f;
    c.kvy += dy / l * 140.0f;
    c.state = AState::Hurt;
    c.timer = 0.18f;
    PlaySound(AU.hit);
    SetSoundPitch(AU.hit, 0.85f + (rand() % 30) / 100.0f);
    SpawnParticles(c.x, c.y - 10, 5, 130, 255, 200, 85);
    FloatText(c.x, c.y - 22, TextFormat("%d", (int)(dmg + 0.5f)), 150, 255, 210);
    // ---- 无脸鬼鬼仆：篡改敌鬼的记忆（错乱 = 强控，忘仇 2.5 秒）----
    if (attacker == CreatureKind::Faceless && c.state != AState::Dead &&
        ((float)rand() / RAND_MAX) < 0.15f) {
        c.confuseT = 2.5f;
        c.state = AState::Wander;
        c.timer = 0.8f;
        FloatText(c.x, c.y - 34, "记忆错乱!", 190, 130, 255);
        SpawnParticles(c.x, c.y - 14, 8, 190, 150, 255, 90);
        // 演出：紫色错乱星环在敌鬼头顶炸开 + 失谐咕哝音（鬼在它耳边絮语）
        for (int k = 0; k < 6; k++) {
            float a = (float)k * 1.047f;
            SpawnParticles(c.x + cosf(a) * 8.0f, c.y - 14 + sinf(a) * 5.0f, 2, 220, 160, 255, 45.0f);
        }
        PlaySound(AU.confuseWarble);
    }
    if (c.hp <= 0 && !MobPreDeath(c)) {
        c.state = AState::Dead;
        c.deadFade = 0;
        MobDied(c.kind, c.x, c.y, c.tier, c.dmgMul);
        // 击杀经验：全部鬼仆 + 伙伴（成长系统）
        for (GhostAlly& g : ghosts) {
            g.exp += 3;
            if (g.exp >= g.level * 8) {           // 升级：伤害 +15% / 上限 +20%
                g.exp -= g.level * 8;
                g.level++;
                g.dmg *= 1.15f;
                g.maxHp = (int)(g.maxHp * 1.2f);
                g.hp = g.maxHp;
                SpawnParticles(g.x, g.y - 10, 8, 150, 255, 210, 100);
                FloatText(g.x, g.y - 26, TextFormat(L10N("鬼仆 Lv%d"), g.level), 140, 255, 210);
            }
        }
        if (PET.on) PET.xp += 5;
    }
}

// V 键收鬼：对 64px 内最近的残魂**或活鬼**施法收服
// 残魂：按幡等级固定成功率；活鬼：越残血越好收（凭操作把追杀你的鬼收进来）
// 活鬼收服失败：它被震退定住 1.6 秒 —— 拉扯的窗口，冷却好了可以再试
static void TryCapture() {
    if (P.dead) return;
    if (netOn && !netHost) { FloatText(P.x, P.y - 40, "客人的摄魂幡在此界无效", 255, 200, 120); return; }
    if (captureCd > 0.0f) {
        FloatText(P.x, P.y - 34, TextFormat(L10N("摄魂幡冷却%d秒"), (int)(captureCd + 0.99f)), 255, 160, 120);
        return;
    }
    if (P.captureLv == 0) { FloatText(P.x, P.y - 34, "需要摄魂幡", 255, 120, 120); return; }
    // ---- 目标选择：幡等级决定施法距离（Lv1 64 / Lv2 80 / Lv3 96）内最近的残魂 / 活鬼 ----
    float capR = CapRange(P.captureLv);
    float best = capR * capR;
    int soulIdx = -1, mobIdx = -1;
    for (int i = 0; i < 8; i++) {
        const Soul& s = souls[i];
        if (!s.on) continue;
        float dx = s.x - P.x, dy = s.y - P.y, q = dx * dx + dy * dy;
        if (q < best) { best = q; soulIdx = i; mobIdx = -1; }
    }
    for (size_t i = 0; i < mobs.size(); i++) {
        const Creature& c = mobs[i];
        if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
        if (c.companion) continue;                       // 伴生铁匠鬼：不可被收服
        float dx = c.x - P.x, dy = c.y - P.y, q = dx * dx + dy * dy;
        if (q < best) { best = q; mobIdx = (int)i; soulIdx = -1; }
    }
    if (soulIdx < 0 && mobIdx < 0) { FloatText(P.x, P.y - 34, "附近没有鬼魂", 255, 160, 120); return; }
    // 镇鬼幡：容量不限（等级只决定"能收几级鬼"，不限制数量）
    // ---- 公共参数 ----
    CreatureKind kind;
    unsigned char tier;
    float mul;
    float chance;
    float fx2 = 0, fy2 = 0, fxx = 0, fyy = 0;    // 活鬼收服失败时的震退方向
    if (soulIdx >= 0) {
        const Soul& s = souls[soulIdx];
        kind = s.kind; tier = s.tier; mul = s.mul;
        chance = P.captureLv >= 3 ? 1.0f                            // 鎏金幡：残魂必成
               : CapSoulRate(P.captureLv) - s.tier * (P.captureLv == 2 ? 0.07f : 0.10f) + RideCapBonus() / 100.0f;
    } else {
        const Creature& c = mobs[(size_t)mobIdx];
        kind = c.kind; mul = c.dmgMul;
        // 黄符降级：被贴符的鬼暂时按低一级收容判定（这正是黄符的战略价值）
        tier = (c.lowerT > 0.0f && c.tier > 0) ? (unsigned char)(c.tier - 1) : c.tier;
        // 活鬼收服：幡等级给底子（Lv1 0.40 / Lv2 0.60 / Lv3 0.80），残血度给加成（打残了再收，是"凭操作收容"的核心）
        float hpF = MobBaseHp(kind) > 0 ? c.hp / (float)MobBaseHp(kind) : 1.0f;
        if (hpF > 1.0f) hpF = 1.0f;
        chance = CapLiveBase(P.captureLv) * (0.45f + 0.55f * (1.0f - hpF)) + RideCapBonus() / 100.0f;
        // 铁匠鬼打不动（不死单位）→ 不能按残血度打折，直接吃幡的满额基率
        if (kind == CreatureKind::SmithGhost) chance = CapLiveBase(P.captureLv);
        // 鬼域域主：凡器伤不了它，只能在域内赢下代理游戏后收服（赢一局 +10%，基础 +30%）
        if (kind == CreatureKind::GhostDomain) {
            chance = 0.30f + 0.10f * (float)gdWin;
            if (chance > 0.95f) chance = 0.95f;
        }
        fxx = c.x - P.x; fyy = c.y - P.y;
        float l = sqrtf(fxx * fxx + fyy * fyy); if (l < 0.001f) l = 1;
        fx2 = fxx / l; fy2 = fyy / l;
    }
    if ((int)tier > P.captureLv - 1) { FloatText(P.x, P.y - 34, "幡等级不足", 255, 120, 120); return; }

    // ---- 施法演出：摄魂牵引束（玩家→目标的青绿珠链汇聚）+ 牵引啸音 ----
    {
        float cbx = (soulIdx >= 0) ? souls[soulIdx].x : mobs[(size_t)mobIdx].x;
        float cby = (soulIdx >= 0) ? souls[soulIdx].y : mobs[(size_t)mobIdx].y;
        SpawnCapFx(0, P.x, P.y, cbx, cby);
        PlaySound(AU.capturePull);
    }

    if ((float)rand() / RAND_MAX < chance) {
        // ---- 收服成功：复刻度 = 幡等级给底子（Lv1 65% / Lv2 85% / Lv3 100%），影响生命/伤害/速度 ----
        float fid = CapFidBase(P.captureLv);
        float gx, gy;
        if (soulIdx >= 0) { gx = souls[soulIdx].x; gy = souls[soulIdx].y; souls[soulIdx].on = false; }
        else {
            Creature& c = mobs[(size_t)mobIdx];
            gx = c.x; gy = c.y;
            c.state = AState::Dead; c.deadFade = 0.4f;   // 原体消散（不走 MobDied：不掉落不加击杀）
        }
        GhostAlly g;
        g.kind = kind; g.x = gx; g.y = gy;
        g.fid = fid;
        g.maxHp = (int)(MobBaseHp(kind) * mul * fid + 0.5f);
        if (g.maxHp < 1) g.maxHp = 1;
        g.hp = g.maxHp;
        g.dmg = MobMeleeDmg(kind) * mul * fid * 0.75f * RideDmgMul();
        g.spd = MobChaseSpd(kind) * (0.70f + 0.20f * fid);   // 复刻度越高走得越快（0.83~0.90）
        g.radius = MobRadius(kind);
        g.flying = MobFlying(kind);
        g.maxLife = (kind == CreatureKind::SmithGhost) ? 300.0f : CapGhostLife(P.captureLv);
        g.life = g.maxLife;
        // 收容增幅：每收容 1 只鬼，场上全部鬼仆的身体数值翻倍（HP/伤害 ×2）。
        // 速度不跟着涨（走得快就没平衡了）；破坏力另由"1 下溃散"保证。
        if (!ghosts.empty()) {
            for (GhostAlly& eg : ghosts) {
                eg.maxHp *= 2; eg.hp *= 2; eg.dmg *= 2.0f;
                SpawnParticles(eg.x, eg.y - 12, 6, 150, 255, 210, 70);
            }
            FloatText(P.x, P.y - 58, "鬼仆们的身子涨了一倍!", 150, 255, 210);
        }
        ghosts.push_back(g);
        P.ghostCaught++;
        CancelBanish(kind);                       // 已被收容：不再重聚
        // ---- 收服域主：鬼域即刻崩塌（唯一出路）----
        if (kind == CreatureKind::GhostDomain) {
            domainPurged = true;
            GdExit();
            AddXp(80);
            TextCopy(bossHowlTxt, "域主被收服——鬼域散了");
            bossHowlT = 3.0f;
            for (int k = 0; k < 40; k++)
                SpawnParticles(gx + (float)(rand() % 160 - 80), gy + (float)(rand() % 120 - 60),
                               1, 140, 200, 255, 160);
            FloatText(P.x, P.y - 52, "鬼域崩塌！", 140, 210, 255);
        }
        captureCd = CapCooldown(P.captureLv) * (gCapBell ? 0.7f : 1.0f);   // 摄魂铃：收鬼冷却 -30%
        PlaySound(AU.capture);
        SpawnParticles(gx, gy - 8, 16, 120, 255, 200, 110);
        SpawnParticles(gx, gy - 8, 6, 200, 255, 235, 90);
        FloatText(P.x, P.y - 40, TextFormat(L10N("收服了鬼仆! 复刻度%d%%"), (int)(fid * 100 + 0.5f)), 140, 255, 210);
        // ---- 无脸鬼专属：收服即夺回它偷走的身份 ----
        // 正被篡改 → 立刻恢复记忆（假血条失效）；被偷的经验全额追回（从它身上搜出来）
        if (kind == CreatureKind::Faceless) {
            if (P.hacked) {
                P.hacked = false;
                P.hackT = 0;
                FloatText(P.x, P.y - 54, "覆在脸上的手被扯开了!", 150, 255, 200);
            }
            if (P.stolenXp > 0) {
                P.xp += P.stolenXp;
                FloatText(P.x, P.y - 66, TextFormat(L10N("追回经验 +%d"), P.stolenXp), 255, 235, 130);
                SpawnParticles(gx, gy - 10, 10, 255, 235, 130, 100);
                P.stolenXp = 0;
            }
            // 演出：身份归位金环（双环扩散 + 符文光点上升）+ 记忆恢复钟音
            SpawnCapFx(1, P.x, P.y, 0, 0);
            SetSoundPitch(AU.memoryRestore, 1.0f);
            PlaySound(AU.memoryRestore);
        }
        shakeT = 0.12f; shakeDur = 0.12f;
    } else {
        // ---- 收服失败 ----
        if (soulIdx >= 0) {
            souls[soulIdx].on = false;               // 残魂受惊溃散
            PlaySound(AU.hurt);
            SpawnParticles(souls[soulIdx].x, souls[soulIdx].y - 8, 12, 200, 200, 210, 100);
            FloatText(P.x, P.y - 40, "魂飞魄散!", 255, 160, 160);
            captureCd = CapCooldown(P.captureLv) * (gCapBell ? 0.7f : 1.0f);   // 摄魂铃：收鬼冷却 -30%
        } else {
            // 活鬼收服失败：被幡力震退定住 —— 逃跑或再试的窗口
            Creature& c = mobs[(size_t)mobIdx];
            c.triggered = true;                      // 它被激怒了
            c.state = AState::Hurt;
            c.timer = 1.6f;                          // 定住 1.6 秒
            c.kvx = fx2 * 340.0f; c.kvy = fy2 * 340.0f;
            captureCd = 2.5f * (gCapBell ? 0.7f : 1.0f);   // 收鬼失败重试冷却：摄魂铃 -30%（与其他路径一致）
            PlaySound(AU.hurt);
            PlaySound(AU.roar);
            SpawnParticles(c.x, c.y - 10, 14, 200, 200, 210, 110);
            FloatText(P.x, P.y - 40, "收服失败!它定住了", 255, 160, 160);
            shakeT = 0.15f; shakeDur = 0.15f;
        }
    }
}

// ============================================================
// 葫芦（Y 键）：把鬼「暂储」进来 —— 不改造成友方，只是封进容器
// 用途：搬运转手（献给铁匠换等级 / 雇佣）、或倒进镇鬼幡变成自家鬼仆
// 等级铁律：葫芦 Lv N 只能收 N 级及以下的鬼；容量 = 等级 x 3
// ============================================================
static float gourdCd = 0.0f;
static void TryGourd() {
    if (P.dead) return;
    if (P.gourdLv == 0) { FloatText(P.x, P.y - 34, "需要收鬼葫芦", 255, 160, 120); return; }
    if (gourdCd > 0.0f) { FloatText(P.x, P.y - 34, TextFormat(L10N("葫芦封口%d秒"), (int)(gourdCd + 0.99f)), 255, 160, 120); return; }
    if ((int)gourd.size() >= GourdCap()) {
        FloatText(P.x, P.y - 34, TextFormat(L10N("葫芦满了(%d/%d)"), (int)gourd.size(), GourdCap()), 255, 160, 120);
        return;
    }
    float R = 52.0f + 10.0f * P.gourdLv;
    // 优先残魂，其次活鬼（取范围内最近）
    int soulIdx = -1, mobIdx = -1; float bs = R * R, bm = R * R;
    for (size_t i = 0; i < 8; i++) {
        if (!souls[i].on) continue;
        float dx = souls[i].x - P.x, dy = souls[i].y - P.y, q = dx * dx + dy * dy;
        if (q < bs) { bs = q; soulIdx = (int)i; }
    }
    for (size_t i = 0; i < mobs.size(); i++) {
        const Creature& c = mobs[i];
        if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
        float dx = c.x - P.x, dy = c.y - P.y, q = dx * dx + dy * dy;
        if (q < bm) { bm = q; mobIdx = (int)i; }
    }
    if (soulIdx < 0 && mobIdx < 0) { FloatText(P.x, P.y - 34, "近处无鬼可收", 255, 160, 120); return; }
    CreatureKind kind; unsigned char tier; float mul; float gx, gy;
    float chance;
    if (soulIdx >= 0) { const Soul& s = souls[soulIdx]; kind = s.kind; tier = s.tier; mul = s.mul; gx = s.x; gy = s.y; chance = 0.95f; }
    else {
        const Creature& c = mobs[(size_t)mobIdx];
        kind = c.kind; mul = c.dmgMul; gx = c.x; gy = c.y;
        // 黄符降级：贴了黄符的鬼按低一级判定
        tier = (c.lowerT > 0.0f && c.tier > 0) ? (unsigned char)(c.tier - 1) : c.tier;
        float hpF = MobBaseHp(kind) > 0 ? c.hp / (float)MobBaseHp(kind) : 1.0f;
        if (hpF > 1.0f) hpF = 1.0f;
        chance = (0.30f + 0.18f * P.gourdLv) * (0.5f + 0.5f * (1.0f - hpF));  // 葫芦不改造：比幡好收
    }
    if ((int)tier + 1 > P.gourdLv) {                       // 等级压制
        FloatText(P.x, P.y - 34, TextFormat(L10N("葫芦压不住 %d 级鬼"), (int)tier + 1), 255, 120, 120);
        gourdCd = 1.0f;
        return;
    }
    SpawnCapFx(0, P.x, P.y, gx, gy);
    PlaySound(AU.capture);
    if (((float)rand() / RAND_MAX) < chance) {
        StoredGhost s; s.kind = kind; s.tier = tier; s.mul = mul;
        gourd.push_back(s);
        CancelBanish(kind);                        // 封进葫芦：不再重聚
        if (soulIdx >= 0) souls[soulIdx].on = false;
        else { Creature& c = mobs[(size_t)mobIdx]; c.state = AState::Dead; c.deadFade = 0.4f; }
        SpawnParticles(gx, gy - 8, 14, 255, 224, 138, 110);
        FloatText(P.x, P.y - 40, TextFormat(L10N("封进葫芦(%d/%d)"), (int)gourd.size(), GourdCap()), 255, 224, 138);
        gourdCd = 1.6f;
    } else {
        gourdCd = 2.2f;
        if (mobIdx >= 0) {
            Creature& c = mobs[(size_t)mobIdx];
            c.triggered = true; c.state = AState::Hurt; c.timer = 1.2f;
            float dx = c.x - P.x, dy = c.y - P.y, l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
            c.kvx = dx / l * 280.0f; c.kvy = dy / l * 280.0f;
        }
        PlaySound(AU.hurt);
        FloatText(P.x, P.y - 40, "它挣脱了封口!", 255, 160, 160);
    }
}
// 放生：把葫芦里的鬼放回世界（重新成为敌对鬼，等于没解决掉）
static void GourdRelease(int i) {
    if (i < 0 || i >= (int)gourd.size()) return;
    StoredGhost s = gourd[(size_t)i];
    gourd.erase(gourd.begin() + i);
    Creature c;
    c.kind = s.kind; c.x = P.x + (float)(rand() % 40 - 20); c.y = P.y + 12.0f;
    c.tier = s.tier; c.dmgMul = s.mul;
    c.hp = MobBaseHp(s.kind);
    c.triggered = true; c.state = AState::Chase;
    mobs.push_back(c);
    SpawnParticles(c.x, c.y - 8, 14, 200, 120, 255, 110);
    FloatText(P.x, P.y - 44, "放它出来了", 200, 160, 255);
    PlaySound(AU.roar);
}
// 倒进镇鬼幡：葫芦里的鬼改造成自家鬼仆（幡等级够则必成）
static void GourdToBanner(int i) {
    if (i < 0 || i >= (int)gourd.size()) return;
    StoredGhost s = gourd[(size_t)i];
    if (P.captureLv == 0) { FloatText(P.x, P.y - 34, "需要镇鬼幡", 255, 160, 120); return; }
    if ((int)s.tier + 1 > P.captureLv) {
        FloatText(P.x, P.y - 34, TextFormat(L10N("幡压不住 %d 级鬼"), (int)s.tier + 1), 255, 120, 120);
        return;
    }
    gourd.erase(gourd.begin() + i);
    GhostAlly g;
    g.kind = s.kind; g.x = P.x; g.y = P.y + 8.0f;
    g.fid = CapFidBase(P.captureLv);
    g.maxHp = (int)(MobBaseHp(s.kind) * s.mul * g.fid + 0.5f); if (g.maxHp < 1) g.maxHp = 1;
    g.hp = g.maxHp;
    g.dmg = MobMeleeDmg(s.kind) * s.mul * g.fid * 0.75f * RideDmgMul();
    g.spd = MobChaseSpd(s.kind) * (0.70f + 0.20f * g.fid);
    g.radius = MobRadius(s.kind);
    g.flying = MobFlying(s.kind);
    g.tier = s.tier;
    g.maxLife = CapGhostLife(P.captureLv);
    g.life = g.maxLife;
    // 收容增幅（同 V 键收服）：已有鬼仆数值翻倍
    if (!ghosts.empty()) {
        for (GhostAlly& eg : ghosts) {
            eg.maxHp *= 2; eg.hp *= 2; eg.dmg *= 2.0f;
            SpawnParticles(eg.x, eg.y - 12, 6, 150, 255, 210, 70);
        }
        FloatText(P.x, P.y - 58, "鬼仆们的身子涨了一倍!", 150, 255, 210);
    }
    ghosts.push_back(g);
    P.ghostCaught++;
    PlaySound(AU.capture);
    SpawnParticles(P.x, P.y - 8, 16, 120, 255, 200, 110);
    FloatText(P.x, P.y - 44, TextFormat(L10N("化为鬼仆：%s"), L10N(GhostName(s.kind))), 140, 255, 210);
}

// ============================================================
// 鬼仆技能（镇鬼幡收服的鬼，可由玩家点名放手段；每种鬼一门）
// ============================================================
static void CastGhostSkill(int gi) {
    if (gi < 0 || gi >= (int)ghosts.size()) return;
    GhostAlly& g = ghosts[(size_t)gi];
    if (g.skCd > 0.0f) { FloatText(g.x, g.y - 26, TextFormat(L10N("还差%d秒"), (int)(g.skCd + 0.99f)), 255, 160, 120); return; }
    GSkill s = SkillOfKind(g.kind);
    // 指针处 = 世界坐标（技能多以指针为中心）
    Vector2 mg = MouseGame();
    float mx = mg.x + camX, my = mg.y + camY;
    bool done = true;
    switch (s) {
    case GSkill::Bite: {                       // 撕咬：扑向最近敌人，重咬一口
        int bi = -1; float bd = 200.0f * 200.0f;
        for (size_t m = 0; m < mobs.size(); m++) {
            const Creature& c = mobs[m];
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            float dx = c.x - g.x, dy = c.y - g.y, q = dx * dx + dy * dy;
            if (q < bd) { bd = q; bi = (int)m; }
        }
        if (bi < 0) { FloatText(g.x, g.y - 26, "近处无敌人", 255, 160, 120); return; }
        Creature& c = mobs[(size_t)bi];
        float dx = c.x - g.x, dy = c.y - g.y, l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
        g.x = c.x - dx / l * 18.0f; g.y = c.y - dy / l * 18.0f;
        HitMobByGhost(c, g.kind, g.dmg * 3.0f, dx / l, dy / l);
        c.kvx = dx / l * 320.0f; c.kvy = dy / l * 320.0f;
        SpawnParticles(c.x, c.y - 8, 14, 255, 160, 160, 120);
        break;
    }
    case GSkill::Charge: {                     // 疾冲：突进到指针处，撞飞沿途
        float dx = mx - g.x, dy = my - g.y, l = sqrtf(dx * dx + dy * dy); if (l < 0.001f) l = 1;
        float step = (l > 190.0f) ? 190.0f : l;
        float nx = g.x + dx / l * step, ny = g.y + dy / l * step;
        if (!g.flying) { Vector2 np = W.MoveCircle(g.x, g.y, g.radius, dx / l * step, dy / l * step); nx = np.x; ny = np.y; }
        SpawnParticles(g.x, g.y - 8, 10, 160, 220, 255, 90);
        g.x = nx; g.y = ny;
        for (size_t m = 0; m < mobs.size(); m++) {
            Creature& c = mobs[m];
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            float ex = c.x - g.x, ey = c.y - g.y, d2 = ex * ex + ey * ey;
            if (d2 > 78.0f * 78.0f) continue;
            float d = sqrtf(d2); if (d < 0.001f) d = 1;
            HitMobByGhost(c, g.kind, g.dmg * 2.0f, ex / d, ey / d);
            c.kvx = ex / d * 380.0f; c.kvy = ey / d * 380.0f;
        }
        SpawnParticles(g.x, g.y - 8, 16, 180, 230, 255, 130);
        break;
    }
    case GSkill::Summon: {                     // 召唤：召一只临时鬼奴（阴气耗尽即散）
        if (ghosts.size() > 14) { FloatText(P.x, P.y - 34, "鬼仆太多", 255, 160, 120); return; }
        GhostAlly n;
        n.kind = CreatureKind::GhostChild; n.x = g.x; n.y = g.y + 10.0f;
        n.fid = 0.7f; n.maxHp = 18; n.hp = 18; n.dmg = g.dmg * 0.6f;
        n.spd = MobChaseSpd(CreatureKind::GhostChild) * 0.8f;
        n.radius = MobRadius(CreatureKind::GhostChild);
        n.maxLife = 40.0f; n.life = 40.0f; n.temp = true; n.out = true;
        n.cmdX = mx; n.cmdY = my;
        ghosts.push_back(n);
        SpawnParticles(g.x, g.y - 8, 16, 200, 160, 255, 120);
        FloatText(g.x, g.y - 30, "鬼奴现身", 200, 170, 255);
        break;
    }
    case GSkill::Curse: {                      // 落单咒：范围内敌人踉跄 + 失血
        int hit = 0;
        for (size_t m = 0; m < mobs.size(); m++) {
            Creature& c = mobs[m];
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            float ex = c.x - mx, ey = c.y - my, d2 = ex * ex + ey * ey;
            if (d2 > 110.0f * 110.0f) continue;
            float d = sqrtf(d2); if (d < 0.001f) d = 1;
            c.kvx = ex / d * 260.0f; c.kvy = ey / d * 260.0f;
            c.dotT = 5.0f; c.dotDps = 2.0f;
            hit++;
        }
        SpawnParticles(mx, my, 18, 150, 120, 220, 140);
        FloatText(mx, my - 20, hit ? TextFormat(L10N("咒住 %d 只"), hit) : "无人中咒", 180, 150, 255);
        break;
    }
    case GSkill::Domain: {                     // 鬼域：范围内敌人持续失血
        for (size_t m = 0; m < mobs.size(); m++) {
            Creature& c = mobs[m];
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            float ex = c.x - mx, ey = c.y - my;
            if (ex * ex + ey * ey > 130.0f * 130.0f) continue;
            c.dotT = 8.0f; c.dotDps = 3.5f;
        }
        SpawnParticles(mx, my, 22, 210, 60, 60, 150);
        FloatText(mx, my - 20, "鬼域张开", 226, 60, 60);
        break;
    }
    case GSkill::Disguise: {                   // 伪装：替你披上隐身（6 秒不触规律）
        P.hideT = (P.hideT > 6.0f) ? P.hideT : 6.0f;
        SpawnParticles(P.x, P.y - 12, 14, 170, 200, 220, 100);
        FloatText(P.x, P.y - 44, "它替你遮了形", 170, 220, 240);
        break;
    }
    case GSkill::Tamper: {                     // 篡改：范围内敌人忘了要杀你
        int hit = 0;
        for (size_t m = 0; m < mobs.size(); m++) {
            Creature& c = mobs[m];
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            float ex = c.x - mx, ey = c.y - my;
            if (ex * ex + ey * ey > 120.0f * 120.0f) continue;
            c.confuseT = 6.0f; c.triggered = false; hit++;
        }
        SpawnParticles(mx, my, 16, 170, 170, 190, 120);
        FloatText(mx, my - 20, hit ? TextFormat(L10N("乱了 %d 只"), hit) : "无人中招", 190, 190, 200);
        break;
    }
    case GSkill::Gamble: {                     // 博弈：福祸各半
        if ((rand() & 1) == 0) {
            P.hp += 40; if (P.hp > P.maxHp) P.hp = P.maxHp;
            FloatText(P.x, P.y - 44, "它赌赢了：回血 40", 120, 255, 180);
            SpawnParticles(P.x, P.y - 12, 14, 120, 255, 180, 110);
        } else {
            P.hp -= 15; if (P.hp < 1) P.hp = 1;
            FloatText(P.x, P.y - 44, "它赌输了：-15", 226, 90, 90);
            SpawnParticles(P.x, P.y - 12, 12, 226, 90, 90, 110);
            hurtVin = 0.4f;
        }
        break;
    }
    case GSkill::Contract: {                   // 契约：耗木五，为你回血
        if (P.wood < 5) { FloatText(P.x, P.y - 34, "木料不足（需 5）", 255, 160, 120); return; }
        P.wood -= 5;
        P.hp += 35; if (P.hp > P.maxHp) P.hp = P.maxHp;
        FloatText(P.x, P.y - 44, "契约成立：回血 35", 255, 224, 138);
        SpawnParticles(P.x, P.y - 12, 12, 255, 224, 138, 110);
        break;
    }
    case GSkill::Knock: {                      // 敲门：震塌墙垣，重击敌人
        int wx = (int)(mx / TILE), wy = (int)(my / TILE);
        int oi = W.ObjIndexAt(wx, wy);
        if (oi >= 0 && W.objs[(size_t)oi].kind == ObjKind::Wall && W.objs[(size_t)oi].hp > 0) {
            W.objs[(size_t)oi].hp = 0;
            SpawnParticles(mx, my, 18, 150, 130, 100, 140);
            FloatText(mx, my - 20, "墙塌了", 200, 180, 140);
        }
        for (size_t m = 0; m < mobs.size(); m++) {
            Creature& c = mobs[m];
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            float ex = c.x - mx, ey = c.y - my, d2 = ex * ex + ey * ey;
            if (d2 > 96.0f * 96.0f) continue;
            float d = sqrtf(d2); if (d < 0.001f) d = 1;
            HitMobByGhost(c, g.kind, g.dmg * 2.5f, ex / d, ey / d);
            c.kvx = ex / d * 460.0f; c.kvy = ey / d * 460.0f;
        }
        shakeT = 0.25f; shakeDur = 0.25f;
        PlaySound(AU.roar);
        break;
    }
    case GSkill::StealFace: {                  // 夺面：范围内敌人僵在原地
        int hit = 0;
        for (size_t m = 0; m < mobs.size(); m++) {
            Creature& c = mobs[m];
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            float ex = c.x - mx, ey = c.y - my;
            if (ex * ex + ey * ey > 120.0f * 120.0f) continue;
            c.state = AState::Hurt; c.timer = 3.0f; c.hurtFlash = 0.3f; hit++;
        }
        SpawnParticles(mx, my, 18, 200, 180, 255, 140);
        FloatText(mx, my - 20, hit ? TextFormat(L10N("夺面 %d 张"), hit) : "无人被夺", 200, 180, 255);
        break;
    }
    case GSkill::Forge: {                      // 锻炉：为你披上护体（8 秒减伤）
        P.shieldT = 8.0f;
        SpawnParticles(P.x, P.y - 12, 16, 255, 224, 138, 120);
        FloatText(P.x, P.y - 44, "护体 8 秒", 255, 224, 138);
        break;
    }
    default: done = false; break;
    }
    if (!done) return;
    g.skCd = SkillCd(s);
    PlaySound(AU.capture);
    SetSoundPitch(AU.capture, 1.25f);
    SetSoundPitch(AU.capture, 1.0f);
}

// H 键精炼：消耗 1 枚鬼界碎片，提升身边最近鬼仆的复刻度（+5%，封顶 100%）
// 复刻度提升 → 生命/伤害/速度按新复刻度重算（保留当前生命比例与等级成长）
// —— 粗纸幡收的残次品（65%），也能靠碎片一点点养成完全体
static void TryRefine() {
    if (P.dead) return;
    if (P.captureLv == 0) { FloatText(P.x, P.y - 34, "需要摄魂幡", 255, 160, 120); return; }
    // 最近未满复刻度的鬼仆（已满 100% 的跳过——精炼默认打在还差火候的那只上）
    float best = 96.0f * 96.0f;
    int idx = -1;
    for (size_t i = 0; i < ghosts.size(); i++) {
        if (ghosts[i].fid >= 1.0f) continue;           // 已完全体：不浪费碎片
        float dx = ghosts[i].x - P.x, dy = ghosts[i].y - P.y, q = dx * dx + dy * dy;
        if (q < best) { best = q; idx = (int)i; }
    }
    if (idx < 0) {
        if (ghosts.empty()) FloatText(P.x, P.y - 34, "没有鬼仆", 255, 160, 120);
        else FloatText(P.x, P.y - 34, "复刻度已满", 255, 224, 138);
        return;
    }
    if (P.gemShard < 1) { FloatText(P.x, P.y - 34, "需要鬼界碎片", 255, 160, 120); return; }
    GhostAlly& g = ghosts[(size_t)idx];
    P.gemShard--;
    float oldFid = g.fid;
    g.fid += 0.05f;
    if (g.fid > 1.0f) g.fid = 1.0f;
    // 属性按复刻度比率整体缩放（等级成长是乘法叠加，按比率缩放不破坏成长结果）
    float r = g.fid / oldFid;
    float hpRatio = g.maxHp > 0 ? (float)g.hp / g.maxHp : 1.0f;
    g.maxHp = (int)(g.maxHp * r + 0.5f);
    if (g.maxHp < 1) g.maxHp = 1;
    g.hp = (int)(g.maxHp * hpRatio + 0.5f);
    if (g.hp < 1) g.hp = 1;
    g.dmg *= r;
    g.spd = MobChaseSpd(g.kind) * (0.70f + 0.20f * g.fid);
    // 演出：复刻金环 + 记忆恢复钟音 + 青绿粒子
    SpawnCapFx(1, g.x, g.y, 0, 0);
    SetSoundPitch(AU.memoryRestore, 1.1f);
    PlaySound(AU.memoryRestore);
    SpawnParticles(g.x, g.y - 8, 10, 255, 224, 138, 90);
    FloatText(g.x, g.y - 34, TextFormat(L10N("精炼! 复刻度%d%%"), (int)(g.fid * 100 + 0.5f)), 255, 224, 138);
}

// 鬼仆每帧更新：自动追咬附近敌怪 / 跟随玩家环形阵位；阴气随时间消散
static void UpdateGhosts(float dt) {
    // 傀儡组与 ghosts 等长维护：增多补 0，减少（有鬼消散）则傀儡线全断，防止下标错绑
    if (puppetGrp.size() < ghosts.size()) puppetGrp.resize(ghosts.size(), 0);
    else if (puppetGrp.size() > ghosts.size()) puppetGrp.assign(ghosts.size(), 0);
    if (puppetMain >= (int)ghosts.size()) puppetMain = ghosts.empty() ? -1 : 0;
    if (ghosts.empty() && puppetOn) puppetOn = false;
    for (size_t i = 0; i < ghosts.size(); ) {
        GhostAlly& g = ghosts[i];
        if (g.iv > 0) g.iv -= dt;
        if (g.hurtFlash > 0) g.hurtFlash -= dt;
        if (g.atkCd > 0) g.atkCd -= dt;
        if (g.atkAnim > 0) g.atkAnim -= dt;
        if (g.skCd > 0) g.skCd -= dt;
        g.animT += dt;
        g.moving = false;
        if (g.temp) {                               // 临时召唤的鬼奴：阴气耗尽即散
            g.life -= dt / RideLifeMul();
            if (g.life <= 0.0f) {
                SpawnParticles(g.x, g.y - 8, 10, 180, 160, 255, 80);
                ghosts.erase(ghosts.begin() + (long)i);
                continue;
            }
        }
        // 阴气：不再自动消散（镇鬼幡的容量不限，鬼仆是长期战力）
        // 只在"奉命出击/受伤"时消耗，回幡待命时缓慢回补；归零则退回幡中养伤
        g.life -= g.out ? dt * 0.35f / RideLifeMul() : 0.0f;
        if (!g.out) g.life += dt * 0.8f;
        if (g.life > g.maxLife) g.life = g.maxLife;
        if (g.life <= 0.0f) {                       // 阴气耗尽：退回幡中养伤（不是死亡）
            g.life = g.maxLife * 0.5f;
            g.out = false;
            g.hp = g.maxHp;
            SpawnParticles(g.x, g.y - 8, 10, 120, 220, 255, 80);
            FloatText(g.x, g.y - 22, "退回幡中养伤", 150, 220, 255);
        }
        // ---- 傀儡操控：主控鬼 + 同控组成员由玩家直接驾驶（跳过自主 AI，贴身仍会自动撕咬）----
        {
            bool puppeted = puppetOn && ((int)i == puppetMain ||
                            ((int)i < (int)puppetGrp.size() && puppetGrp[i] != 0));
            if (puppeted) {
                if (puppetMx != 0.0f || puppetMy != 0.0f) {
                    float spd2 = g.spd * 1.25f;                       // 附体驾驶比自主跟随略快
                    if (g.flying) {
                        g.x += puppetMx * spd2 * dt; g.y += puppetMy * spd2 * dt;
                    } else {
                        Vector2 np = W.MoveCircle(g.x, g.y, g.radius, puppetMx * spd2 * dt, puppetMy * spd2 * dt);
                        g.x = np.x; g.y = np.y;
                    }
                    g.dir = fabsf(puppetMx) > fabsf(puppetMy) ? (puppetMx > 0 ? 2 : 3) : (puppetMy > 0 ? 0 : 1);
                    g.moving = true;
                    g.animT += dt * 0.7f;
                }
                if (g.atkCd <= 0) {                                  // 贴身自动撕咬（傀儡的凶性还在）
                    for (size_t m = 0; m < mobs.size(); m++) {
                        Creature& c = mobs[m];
                        if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
                        float dx = c.x - g.x, dy = c.y - g.y;
                        if (dx * dx + dy * dy > 26.0f * 26.0f) continue;
                        float dmg = g.dmg;
                        if (dmg < (float)c.hp) dmg = (float)c.hp;     // 1 咬即溃散（与自主撕咬一致）
                        HitMobByGhost(c, g.kind, dmg, dx, dy);
                        g.atkCd = 1.1f; g.atkAnim = 0.16f;
                        break;
                    }
                }
                i++;
                continue;
            }
        }
        // 搜索攻击目标：玩家 320px 内最近的敌对活怪（系绳范围，不会追出太远）
        int best = -1; float bd = 260.0f * 260.0f;
        for (size_t m = 0; m < mobs.size(); m++) {
            const Creature& c = mobs[m];
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            float pdx = c.x - P.x, pdy = c.y - P.y;
            if (pdx * pdx + pdy * pdy > 320.0f * 320.0f) continue;
            float dx = c.x - g.x, dy = c.y - g.y;
            float q = dx * dx + dy * dy;
            if (q < bd) { bd = q; best = (int)m; }
        }
        if (best >= 0) {                            // 战斗：追击 + 撕咬
            Creature& tgt = mobs[(size_t)best];
            float dx = tgt.x - g.x, dy = tgt.y - g.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 24.0f) {
                if (g.flying) {
                    g.x += dx / d * g.spd * dt; g.y += dy / d * g.spd * dt;
                } else {
                    Vector2 np = W.MoveCircle(g.x, g.y, g.radius, dx / d * g.spd * dt, dy / d * g.spd * dt);
                    g.x = np.x; g.y = np.y;
                }
                g.dir = fabsf(dx) > fabsf(dy) ? (dx > 0 ? 2 : 3) : (dy > 0 ? 0 : 1);
                g.moving = true;
            } else if (g.atkCd <= 0) {
                // 破坏速度：鬼仆的爪子撕得开任何鬼——对敌鬼 1 咬即溃散（伤害拉到其现有 HP）
                float dmg = g.dmg;
                if (MobHostile(tgt.kind) && dmg < (float)tgt.hp) dmg = (float)tgt.hp;
                HitMobByGhost(tgt, g.kind, dmg, dx, dy);
                g.atkCd = 1.1f;
                g.atkAnim = 0.16f;
            }
        } else {                                    // 无目标：奉命出击则赴指针处，否则回阵位待命
            int sl = (int)i % 3;
            float fx = g.out ? g.cmdX : P.x + GHOST_SLOT[sl][0];
            float fy = g.out ? g.cmdY : P.y + GHOST_SLOT[sl][1];
            float dx = fx - g.x, dy = fy - g.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (g.out && d <= 12.0f) g.out = false; // 抵达指针处后转待命（不再顶着指针）
            if (d > 300.0f && !g.out) {             // 走丢保护（仅跟随态）
                g.x = fx; g.y = fy;
            } else if (d > (g.out ? 10.0f : 14.0f)) {
                float spd = (d > 110.0f ? g.spd * 1.7f : g.spd * 0.85f);
                if (g.flying) {
                    g.x += dx / d * spd * dt; g.y += dy / d * spd * dt;
                } else {
                    Vector2 np = W.MoveCircle(g.x, g.y, g.radius, dx / d * spd * dt, dy / d * spd * dt);
                    g.x = np.x; g.y = np.y;
                }
                g.dir = fabsf(dx) > fabsf(dy) ? (dx > 0 ? 2 : 3) : (dy > 0 ? 0 : 1);
                g.moving = true;
            }
        }
        if (g.moving) g.animT += dt * 0.7f;         // 移动时加快动画节奏
        i++;
    }
}

// 9面鬼 空间位移特效（creature.cpp 回调：瞬移残影粒子）
void GhostTeleportFx(float x, float y) {
    SpawnParticles(x, y - 10, 12, 190, 150, 255, 110);
    SpawnParticles(x, y - 10, 5, 240, 220, 255, 90);
}

// ---------------- NPC 更新（幸存者：游荡 / 跟随 / 入住营地 / 候补） ----------------
static void UpdateNpcs(float dt) {
    for (Npc& n : npcs) {
        if (!n.on) continue;
        n.animT += dt;
        if (n.kind != 1) continue;                 // 铁匠：站桩不动
        if (n.state == 0) {                        // 游荡：原地小范围踱步
            n.wanderT -= dt;
            if (n.wanderT <= 0.0f) {
                n.wanderT = 2.0f + (rand() % 30) / 10.0f;
                n.wx = ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
                n.wy = ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
            }
            if (n.wx != 0.0f || n.wy != 0.0f) {
                Vector2 np = W.MoveCircle(n.x, n.y, 5.0f, n.wx * 14.0f * dt, n.wy * 14.0f * dt);
                n.x = np.x; n.y = np.y;
                n.dir = fabsf(n.wx) > fabsf(n.wy) ? (n.wx > 0 ? 2 : 3) : (n.wy > 0 ? 0 : 1);
            }
        } else if (n.state == 1 || n.state == 3) { // 跟随玩家 / 营地门口候补
            float tx = (n.state == 1) ? P.x : campCenter.x;
            float ty = (n.state == 1) ? P.y : campCenter.y;
            float dx = tx - n.x, dy = ty - n.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 300.0f) { n.x = P.x - 20.0f; n.y = P.y + 10.0f; }   // 走丢保护
            else if (d > (n.state == 1 ? 26.0f : 12.0f)) {
                float spd = (d > 90.0f ? 175.0f : 110.0f);
                Vector2 np = W.MoveCircle(n.x, n.y, 5.0f, dx / d * spd * dt, dy / d * spd * dt);
                n.x = np.x; n.y = np.y;
                n.dir = fabsf(dx) > fabsf(dy) ? (dx > 0 ? 2 : 3) : (dy > 0 ? 0 : 1);
            }
            // 抵达营地：入住（受收容上限约束）
            if (campBuilt && n.state == 1) {
                float cdx = campCenter.x - n.x, cdy = campCenter.y - n.y;
                if (cdx * cdx + cdy * cdy < 60.0f * 60.0f) {
                    int settled = 0;
                    for (const Npc& m : npcs) if (m.on && m.kind == 1 && m.state == 2) settled++;
                    if (settled < CampCap()) {
                        n.state = 2;
                        PlaySound(AU.pickup);
                        SpawnParticles(n.x, n.y - 10, 8, 190, 235, 200, 90);
                        FloatText(n.x, n.y - 30, TextFormat(L10N("%s入住营地"), L10N(JOB_NAME[n.job])), 190, 235, 200);
                    } else {
                        n.state = 3;               // 住满：在营地门口候补（升级后自动入住）
                        FloatText(n.x, n.y - 30, "营地住满，暂在门外等候", 255, 200, 120);
                    }
                }
            }
            // 候补转正：营地升级腾出名额
            if (campBuilt && n.state == 3) {
                int settled = 0;
                for (const Npc& m : npcs) if (m.on && m.kind == 1 && m.state == 2) settled++;
                if (settled < CampCap()) {
                    n.state = 2;
                    PlaySound(AU.pickup);
                    SpawnParticles(n.x, n.y - 10, 8, 190, 235, 200, 90);
                }
            }
        }
        // state==2 入住：在营地内小范围踱步（生活感）
        if (n.state == 2 && campBuilt) {
            n.wanderT -= dt;
            if (n.wanderT <= 0.0f) {
                n.wanderT = 3.0f + (rand() % 40) / 10.0f;
                float a = (rand() % 360) * 0.0174533f;
                n.wx = cosf(a); n.wy = sinf(a);
            }
            float cdx = n.x + n.wx * 10.0f * dt - campCenter.x;
            float cdy = n.y + n.wy * 10.0f * dt - campCenter.y;
            if (cdx * cdx + cdy * cdy < 42.0f * 42.0f && (n.wx != 0.0f || n.wy != 0.0f)) {
                Vector2 np = W.MoveCircle(n.x, n.y, 5.0f, n.wx * 10.0f * dt, n.wy * 10.0f * dt);
                n.x = np.x; n.y = np.y;
                n.dir = fabsf(n.wx) > fabsf(n.wy) ? (n.wx > 0 ? 2 : 3) : (n.wy > 0 ? 0 : 1);
            }
        }
    }
    // ---- 幸存者补充：野外维持 4 名游荡幸存者（收容后会有新人流落出来）----
    static float sSurvT = 0.0f;
    sSurvT += dt;
    if (sSurvT >= 25.0f) {
        sSurvT = 0.0f;
        int wander = 0, total = 0;
        for (const Npc& n : npcs)
            if (n.on && n.kind == 1) { total++; if (n.state == 0) wander++; }
        if (wander < 4 && total < 10) {
            for (int tryN = 0; tryN < 40; tryN++) {
                int tx = 6 + rand() % (MAP_W - 12);
                int ty = 6 + rand() % (MAP_H - 12);
                if (W.TileAt(tx, ty) == Tile::Water) continue;
                float x = tx * 16.0f + 8.0f, y = ty * 16.0f + 8.0f;
                float dxs = x - P.x, dys = y - P.y;
                if (dxs * dxs + dys * dys < 260.0f * 260.0f) continue;   // 别刷在玩家眼前
                if (!W.CircleFree(x, y, 6.0f)) continue;
                Npc n;
                n.kind = 1; n.job = rand() % 5; n.state = 0;
                n.x = x; n.y = y;
                npcs.push_back(n);
                break;
            }
        }
    }
}


// ============================================================
// 联机服务：房主收包 / 快照广播 / 客人收包与本地模拟
// 协议（单字节 opcode）：J 加入 W 欢迎 n 世界块 Q 重求世界 K 快照
//   S 状态 E 装备 A 攻击 H 采集 P 拾取 T 符咒 D 掉落归属 R 还魂 F 满员 L 离开
// ============================================================
static void NetStop() {
    if (netSock != (unsigned short)-1) Net::UdpClose(netSock);
    netSock = (unsigned short)-1;
    netOn = false; netHost = false; netJoined = false;
    WDropIdCb = nullptr;
    for (auto& p : netPeers) p = PeerView{};
}

static bool NetHostStart() {
    Net::Init();
    unsigned short s = Net::UdpOpen(Net::PORT);
    if (s == (unsigned short)-1) return false;
    NetStop();                    // 清旧会话
    netSock = s; netOn = true; netHost = true; netId = 0; netJoined = true;
    netNextMobId = 1; netDropSeq = 1;
    WDropIdCb = NetDropIdCb;      // 掉落分配网络 id（快照下发客人）
    for (auto& p : netPeers) p = PeerView{};
    netPeers[0].on = true;
    strncpy(netPeers[0].name, "房主", 15);
    unsigned ns = (unsigned)(GetTime() * 1000.0) ^ (unsigned)(rand() * 2654435761u);
    NewGame(ns);                  // 联机从新世界开局（双方种子一致，世界天然同步）
    gs = GS::Play;
    introOn = false; tutOn = false; craftOpen = false;
    lastSaveDay = (int)(gameTime / DAY_LEN);
    return true;
}

static bool NetJoin(const char* ip) {
    Net::Init();
    unsigned a = Net::ResolveIp(ip);
    if (!a) return false;
    unsigned short s = Net::UdpOpen(0);
    if (s == (unsigned short)-1) return false;
    NetStop();
    netSock = s; netOn = true; netHost = false;
    netAddrHost = a; netPortHost = Net::PORT;
    netJoined = false; netLastRecv = 0;
    netChunks.clear();
    for (auto& p : netPeers) p = PeerView{};
    Net::Buf b;
    b.d[0] = 'J'; b.n = 1;
    Net::WStr(b, "旅人", 16);
    Net::UdpSend(netSock, netAddrHost, netPortHost, b);
    return true;
}

// 世界分块下发（join 与 'Q' 重求共用）
static void NetSendWorld(unsigned a, unsigned short pt) {
    unsigned total = (unsigned)((W.objs.size() + 95) / 96);
    if (total == 0) total = 1;
    for (unsigned seq = 0; seq < total; seq++) {
        for (int rp = 0; rp < 2; rp++) {          // 每块发两遍：LAN 不丢即可靠
            Net::Buf b;
            b.d[0] = 'n'; b.n = 1;
            Net::W32(b, seq); Net::W32(b, total);
            size_t i0 = (size_t)seq * 96, i1 = i0 + 96;
            if (i1 > W.objs.size()) i1 = W.objs.size();
            Net::W16(b, (unsigned)(i1 - i0));
            for (size_t i = i0; i < i1; i++) {
                const WorldObj& o = W.objs[i];
                Net::W8(b, (unsigned)o.kind);
                Net::W8(b, o.tx); Net::W8(b, o.ty);
                Net::W8(b, o.hp);
                Net::W8(b, o.harvested ? 1 : 0);
            }
            Net::UdpSend(netSock, a, pt, b);
        }
    }
}

// 客人应用世界物体（kind=0xFF 表示该瓦片已清空）
static void NetApplyWorldObj(unsigned char kind, int tx, int ty, unsigned char hp, unsigned char hv) {
    int idx = W.ObjIndexAt(tx, ty);
    if (kind == 0xFF) {
        if (idx >= 0) W.RemoveObjAt(tx, ty);
        return;
    }
    if (idx >= 0) {
        WorldObj& o = W.objs[(size_t)idx];
        o.kind = (ObjKind)kind; o.hp = hp; o.harvested = hv != 0;
        return;
    }
    switch ((ObjKind)kind) {     // 新增物体（墙/床/灯/碑/台）：种子世界本来没有
    case ObjKind::Wall:      W.PlaceWall(tx, ty); break;
    case ObjKind::Bed:       W.PlaceBed(tx, ty); break;
    case ObjKind::Campfire:  W.PlaceCampfire(tx, ty); break;
    case ObjKind::CampStone: W.PlaceCampStone(tx, ty); break;
    case ObjKind::Workbench: W.PlaceWorkbench(tx, ty, hp ? hp : 1); break;
    default: return;             // 树/石等以种子世界为准，差异忽略
    }
    int ni = W.ObjIndexAt(tx, ty);
    if (ni >= 0) { W.objs[(size_t)ni].hp = hp; W.objs[(size_t)ni].harvested = hv != 0; }
}

// 房主的快照广播（10Hz）：生物/NPC/掉落/投射物/同伴/事件
static void NetBroadcast() {
    for (int pi = 1; pi < 4; pi++) {
        if (!netPeers[pi].on) continue;
        Net::Buf b;
        b.d[0] = 'K'; b.n = 1;
        Net::WFl(b, gameTime);
        Net::WFl(b, rainAmt);
        Net::W8(b, (gTide ? 1u : 0u) | (gGlowRain ? 2u : 0u));
        Net::W8(b, campBuilt ? 1u : 0u);
        Net::W8(b, (unsigned)campLv);
        Net::WFl(b, campCenter.x); Net::WFl(b, campCenter.y);
        // 房主本人
        Net::WFl(b, P.x); Net::WFl(b, P.y);
        Net::W8(b, (unsigned)((P.dir & 3) | (P.dead ? 4 : 0) | (playerMoving ? 8 : 0) |
                              (P.atkTime > 0 ? 16 : 0)));
        Net::W16(b, (unsigned)(P.hp < 0 ? 0 : P.hp));
        // 同伴（客人端会忽略自己的那条）
        int cnt = 0;
        for (int i = 1; i < 4; i++) if (netPeers[i].on) cnt++;
        Net::W8(b, (unsigned)cnt);
        for (int i = 1; i < 4; i++) if (netPeers[i].on) {
            PeerView& pr = netPeers[i];
            Net::W8(b, (unsigned)i);
            Net::WFl(b, pr.x); Net::WFl(b, pr.y);
            Net::W8(b, (unsigned)((pr.dir & 3) | (pr.dead ? 4 : 0) | (pr.moving ? 8 : 0) |
                                  (pr.running ? 16 : 0) | (pr.atk ? 32 : 0)));
            Net::W16(b, (unsigned)(pr.hp < 0 ? 0 : pr.hp));
            Net::WStr(b, pr.name, 16);
        }
        // 生物：任一存活玩家 480px 内，上限 64
        unsigned char nm = 0;
        int mobStart = b.n;
        Net::W8(b, 0);
        for (size_t i = 0; i < mobs.size() && nm < 64; i++) {
            Creature& c = mobs[i];
            float bd = 1e18f;
            {
                float dx = c.x - P.x, dy = c.y - P.y;
                float d2 = dx * dx + dy * dy;
                if (d2 < bd) bd = d2;
            }
            for (int t = 1; t < 4; t++) if (netPeers[t].on) {
                float dx = c.x - netPeers[t].x, dy = c.y - netPeers[t].y;
                float d2 = dx * dx + dy * dy;
                if (d2 < bd) bd = d2;
            }
            if (bd > 480.0f * 480.0f) continue;
            if (c.netId < 0) c.netId = (int)netNextMobId++;
            Net::W32(b, (unsigned)c.netId);
            Net::W8(b, (unsigned)c.kind);
            Net::WFl(b, c.x); Net::WFl(b, c.y);
            Net::W8(b, (unsigned)c.state);
            Net::W8(b, (unsigned)c.dir);
            Net::W16(b, (unsigned)(c.hp < 0 ? 0 : c.hp));
            Net::W8(b, c.tier);
            Net::W8(b, c.tintSeed);
            Net::W16(b, c.ruleMask);
            Net::W8(b, (unsigned)((c.triggered ? 1 : 0) | (c.marked ? 2 : 0) |
                                  (c.companion ? 4 : 0) | (c.infiltrated ? 8 : 0) |
                                  (c.wailer ? 16 : 0)));
            nm++;
        }
        b.d[mobStart] = nm;
        // NPC（上限 16）
        unsigned char nn = 0;
        int npcStart = b.n;
        Net::W8(b, 0);
        for (size_t i = 0; i < npcs.size() && nn < 16; i++) {
            const Npc& n = npcs[i];
            if (!n.on) continue;
            float dx = n.x - P.x, dy = n.y - P.y;
            if (dx * dx + dy * dy > 480.0f * 480.0f) continue;
            Net::W8(b, (unsigned)i);
            Net::W8(b, (unsigned)n.kind);
            Net::W8(b, (unsigned)n.job);
            Net::W8(b, (unsigned)n.state);
            Net::WFl(b, n.x); Net::WFl(b, n.y);
            Net::W8(b, (unsigned)n.dir);
            nn++;
        }
        b.d[npcStart] = nn;
        // 掉落（上限 40）
        unsigned char ndr = 0;
        int dropStart = b.n;
        Net::W8(b, 0);
        for (size_t i = 0; i < W.drops.size() && ndr < 40; i++) {
            const Drop& d = W.drops[i];
            Net::W32(b, d.netId);
            Net::W8(b, (unsigned)d.kind);
            Net::WFl(b, d.x); Net::WFl(b, d.y);
            ndr++;
        }
        b.d[dropStart] = ndr;
        // 投射物（酸弹/火球/箭矢，上限 16）
        unsigned char npr = 0;
        int prStart = b.n;
        Net::W8(b, 0);
        for (const Spit& s : spits) if (s.on && npr < 16) {
            Net::W8(b, s.fire ? 1 : 0);
            Net::WFl(b, s.x); Net::WFl(b, s.y);
            Net::WFl(b, s.vx); Net::WFl(b, s.vy);
            npr++;
        }
        for (const Arrow& a : arrows) if (a.on && npr < 16) {
            Net::W8(b, 2);
            Net::WFl(b, a.x); Net::WFl(b, a.y);
            Net::WFl(b, a.vx); Net::WFl(b, a.vy);
            npr++;
        }
        b.d[prStart] = npr;
        Net::UdpSend(netSock, netPeers[pi].addr, netPeers[pi].port, b);
    }
}

// 房主：客人请求拾取 → 权威移除并广播归属
static void NetHostPickup(int slot, unsigned did) {
    for (size_t i = 0; i < W.drops.size(); i++) {
        if (W.drops[i].netId != did) continue;
        ItemKind k = W.drops[i].kind;
        W.drops[i] = W.drops.back();
        W.drops.pop_back();
        Net::Buf b;
        b.d[0] = 'D'; b.n = 1;
        Net::W32(b, did);
        Net::W8(b, (unsigned)k);
        Net::W8(b, (unsigned)slot);
        for (int pi = 1; pi < 4; pi++) if (netPeers[pi].on)
            Net::UdpSend(netSock, netPeers[pi].addr, netPeers[pi].port, b);
        return;
    }
}

// 房主：客人远程攻击 → 用客人装备打鬼（P 备份/还原，不污染房主状态）
static void RemoteSweepMobs(PeerView& pr, int dir) {
    if (pr.dead) return;
    if (pr.hotSel == 1 && pr.toolLv[1]) {          // 猎弓：房主放箭
        static const float FACE[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
        float fx = FACE[dir & 3][0], fy = FACE[dir & 3][1];
        float sp = 300.0f + 40.0f * pr.toolLv[1];
        FireArrow(pr.x + fx * 10.0f, pr.y - 8 + fy * 10.0f, fx * sp, fy * sp);
        return;
    }
    Player PB = P; int hotB = hotSel;              // 备份房主状态
    P.x = pr.x; P.y = pr.y; P.dir = dir; hotSel = pr.hotSel;
    memcpy(P.toolLv, pr.toolLv, 6);
    P.swordLv = (unsigned char)pr.swordLv;
    P.level = pr.level;
    SweepActor A;
    A.ox = pr.x; A.oy = pr.y - 8.0f; A.dir = dir; A.hotSel = pr.hotSel;
    memcpy(A.toolLv, pr.toolLv, 6);
    A.swordLv = pr.swordLv;
    A.dmg = 2 + (pr.level - 1) / 2;
    if (pr.toolLv[0]) A.dmg += 3 + (pr.toolLv[0] > 1 ? 2 : 0) + (pr.toolLv[0] > 2 ? 3 : 0);
    if (pr.hotSel == 2 && pr.toolLv[2]) A.dmg += 1 + pr.toolLv[2];
    if (pr.hotSel == 4 && pr.toolLv[4]) A.dmg += 4 + 2 * pr.toolLv[4];
    A.heavyHit = (pr.hotSel == 4 && pr.toolLv[4]);
    SweepMobsAt(A);
    P = PB; hotSel = hotB;
}

// 房主：客人符咒 → 作用鬼的部分在房主世界结算（自体 buff 客人本地已演）
static void RemoteTalisman(PeerView& pr, int t) {
    if (pr.dead || t < 0 || t >= 5) return;
    if (t == TAL_BANISH) return;                   // 速符：自体 buff，客人本地
    Player PB = P;
    P.x = pr.x; P.y = pr.y;
    P.talN[t] = 1; P.talCd[t] = 0.0f;              // 客人已扣过自己的符
    UseTalisman(t);
    int talB = P.talN[t];                          // UseTalisman 可能扣到备份头上？
    P = PB;                                        // 整体还原，房主计数不受影响
    (void)talB;
}

// 房主：每帧收包 + 掉线剔除 + 10Hz 广播
static void NetHostService(float dt) {
    Net::Buf b;
    unsigned a; unsigned short pt;
    while (Net::UdpRecv(netSock, a, pt, b)) {
        Net::Rd r = Net::BeginRead(b.d, b.n);
        unsigned char op = (unsigned char)Net::R8(r);
        if (!r.ok) continue;
        int slot = -1;
        for (int i = 1; i < 4; i++)
            if (netPeers[i].on && netPeers[i].addr == a && netPeers[i].port == pt) slot = i;
        if (op == 'J') {
            if (slot < 0) {
                for (int i = 1; i < 4; i++) if (!netPeers[i].on) { slot = i; break; }
                if (slot < 0) {                          // 满员
                    Net::Buf f;
                    f.d[0] = 'F'; f.n = 1;
                    Net::UdpSend(netSock, a, pt, f);
                    continue;
                }
                PeerView& pr = netPeers[slot];
                pr = PeerView{};
                pr.on = true; pr.addr = a; pr.port = pt; pr.id = slot;
                Net::RStr(r, pr.name, 16);
                if (!pr.name[0]) strncpy(pr.name, "旅人", 15);
                pr.x = pr.netTx = W.spawn.x;
                pr.y = pr.netTy = W.spawn.y;
                pr.hp = 100;
                pr.lastSeen = (float)GetTime();
                Net::Buf w;
                w.d[0] = 'W'; w.n = 1;
                Net::W32(w, gSeed);
                Net::W8(w, (unsigned)slot);
                Net::WFl(w, gameTime);
                Net::W8(w, campBuilt ? 1u : 0u);
                Net::W8(w, (unsigned)campLv);
                Net::WFl(w, campCenter.x); Net::WFl(w, campCenter.y);
                Net::WFl(w, rainAmt);
                Net::W8(w, rainTarget > 0.5f ? 1u : 0u);
                Net::UdpSend(netSock, a, pt, w);
                NetSendWorld(a, pt);
                FloatText(P.x, P.y - 60, TextFormat(L10N("%s 加入了荒野"), pr.name), 190, 235, 200);
            }
            continue;
        }
        if (op == 'Q') { NetSendWorld(a, pt); continue; }   // 客人重求世界
        if (slot < 0) continue;                             // 未加入者的其它包一律忽略
        PeerView& pr = netPeers[slot];
        pr.lastSeen = (float)GetTime();
        switch (op) {
        case 'S':
            pr.netTx = Net::RFl(r); pr.netTy = Net::RFl(r);
            if (!pr.moving && !pr.running) { pr.x = pr.netTx; pr.y = pr.netTy; }  // 静止直接落位
            {
                unsigned f = Net::R8(r);
                pr.dir = (int)(f & 3);
                pr.moving = (f & 4) != 0; pr.running = (f & 8) != 0;
                pr.dead = (f & 16) != 0; pr.atk = (f & 32) != 0;
            }
            pr.hp = (int)Net::R16(r);
            break;
        case 'E':
            for (int i = 0; i < 6; i++) pr.toolLv[i] = (unsigned char)Net::R8(r);
            pr.swordLv = (int)Net::R8(r);
            pr.level = (int)Net::R8(r);
            pr.hotSel = (int)Net::R8(r);
            break;
        case 'A':
            RemoteSweepMobs(pr, (int)Net::R8(r));
            break;
        case 'H': {
            int n = (int)Net::R8(r);
            if (n > 8) n = 8;
            for (int i = 0; i < n && r.ok; i++) {
                int tx = (int)Net::R16(r), ty = (int)Net::R16(r);
                int idx = W.ObjIndexAt(tx, ty);
                if (idx < 0) continue;
                unsigned char tlB[6];
                memcpy(tlB, P.toolLv, 6);
                int hotB = hotSel;
                memcpy(P.toolLv, pr.toolLv, 6);
                if (pr.hotSel == 5) hotSel = 5;      // 客人持镐：矿脉照挖
                HitObject(idx);
                memcpy(P.toolLv, tlB, 6);
                hotSel = hotB;
            }
            break;
        }
        case 'P':
            NetHostPickup(slot, Net::R32(r));
            break;
        case 'T':
            RemoteTalisman(pr, (int)Net::R8(r));
            break;
        case 'R':
            pr.dead = false; pr.hp = 100;
            break;
        case 'L':
            pr.on = false;
            FloatText(P.x, P.y - 60, TextFormat(L10N("%s 离开了"), pr.name), 255, 160, 120);
            break;
        default: break;
        }
    }
    float now = (float)GetTime();
    for (int i = 1; i < 4; i++)
        if (netPeers[i].on && now - netPeers[i].lastSeen > 10.0f) {
            FloatText(P.x, P.y - 60, TextFormat(L10N("%s 掉线了"), netPeers[i].name), 255, 160, 120);
            netPeers[i].on = false;
        }
    netSnapT -= dt;
    if (netSnapT <= 0.0f) {
        netSnapT = 0.1f;
        NetBroadcast();
    }
}

// 客人应用快照（生物插值目标 / 掉落重建 / 投射物落位）
static void NetApplySnapshot(Net::Rd& r) {
    gameTime = Net::RFl(r);
    rainAmt = Net::RFl(r);
    {
        unsigned ev = Net::R8(r);
        bool tideB = gTide, glowB = gGlowRain;
        gTide = (ev & 1) != 0; gGlowRain = (ev & 2) != 0;
        if (gTide && !tideB) FloatText(P.x, P.y - 46, "今夜……有东西在聚集", 226, 40, 46);
        if (gGlowRain && !glowB) FloatText(P.x, P.y - 46, "荧雨夜——磷光养人", 140, 235, 200);
    }
    campBuilt = Net::R8(r) != 0;
    campLv = (int)Net::R8(r);
    campCenter.x = Net::RFl(r); campCenter.y = Net::RFl(r);
    // 房主
    {
        float hx = Net::RFl(r), hy = Net::RFl(r);
        unsigned f = Net::R8(r);
        int hhp = (int)Net::R16(r);
        PeerView& pr = netPeers[0];
        bool fresh = !pr.on;
        pr.on = true; pr.id = 0;
        strncpy(pr.name, "房主", 15);
        if (fresh) { pr.x = pr.netTx = hx; pr.y = pr.netTy = hy; }
        else { pr.netTx = hx; pr.netTy = hy; }
        pr.dir = (int)(f & 3);
        pr.dead = (f & 4) != 0; pr.moving = (f & 8) != 0; pr.atk = (f & 16) != 0;
        pr.hp = hhp;
    }
    // 同伴 + 自己的权威血量
    unsigned np = Net::R8(r);
    bool seen[4] = {};
    for (unsigned i = 0; i < np && r.ok; i++) {
        unsigned id = Net::R8(r);
        float x = Net::RFl(r), y = Net::RFl(r);
        unsigned f = Net::R8(r);
        int hp = (int)Net::R16(r);
        char nm[16] = "";
        Net::RStr(r, nm, 16);
        if (id >= 4) continue;
        seen[id] = true;
        if ((int)id == netId) {                  // 自己：血量以房主为准
            int prev = P.hp;
            P.hp = hp;
            if (P.hp < prev) { P.hurtFlash = 0.12f; hurtVin = 0.5f; }
            if (P.dead != ((f & 4) != 0)) { P.dead = (f & 4) != 0; if (P.dead) deathTime = gameTime; }
            continue;
        }
        PeerView& pr = netPeers[id];
        bool fresh = !pr.on;
        pr.on = true; pr.id = (int)id;
        if (fresh) { pr.x = pr.netTx = x; pr.y = pr.netTy = y; }
        else { pr.netTx = x; pr.netTy = y; }
        strncpy(pr.name, nm, 15);
        pr.dir = (int)(f & 3);
        pr.dead = (f & 4) != 0; pr.moving = (f & 8) != 0;
        pr.running = (f & 16) != 0; pr.atk = (f & 32) != 0;
        pr.hp = hp;
    }
    for (int i = 0; i < 4; i++)
        if (i != netId && !seen[i]) netPeers[i].on = false;
    // 生物
    unsigned nm2 = Net::R8(r);
    std::vector<char> claim(mobs.size(), 0);
    for (unsigned i = 0; i < nm2 && r.ok; i++) {
        int id = (int)Net::R32(r);
        unsigned char kind = (unsigned char)Net::R8(r);
        float x = Net::RFl(r), y = Net::RFl(r);
        unsigned char st = (unsigned char)Net::R8(r);
        unsigned char dir2 = (unsigned char)Net::R8(r);
        int hp = (int)Net::R16(r);
        unsigned char tier = (unsigned char)Net::R8(r);
        unsigned char tint = (unsigned char)Net::R8(r);
        unsigned short ruleMask = (unsigned short)Net::R16(r);
        unsigned char fl = (unsigned char)Net::R8(r);
        int idx = -1;
        for (size_t j = 0; j < mobs.size(); j++)
            if (mobs[j].netId == id) { idx = (int)j; break; }
        if (idx < 0) {
            Creature c;
            c.netId = id;
            c.kind = (CreatureKind)kind;
            mobs.push_back(c);
            idx = (int)mobs.size() - 1;
        }
        Creature& c = mobs[(size_t)idx];
        if (c.kind != (CreatureKind)kind) { CreatureKind k2 = c.kind; c = Creature(); c.netId = id; c.kind = k2 == (CreatureKind)kind ? k2 : (CreatureKind)kind; }
        claim[(size_t)idx] = 1;
        int hpPrev = c.hp;
        c.netTx = x; c.netTy = y;
        if (fabsf(c.x) < 0.01f && fabsf(c.y) < 0.01f) { c.x = x; c.y = y; }
        c.state = (AState)st;
        c.dir = (int)dir2;
        c.hp = hp;
        c.tier = tier;
        c.tintSeed = tint;
        c.ruleMask = ruleMask;
        c.triggered = (fl & 1) != 0;
        c.marked = (fl & 2) != 0;
        c.companion = (fl & 4) != 0;
        c.infiltrated = (fl & 8) != 0;
        c.wailer = (fl & 16) != 0;
        if (hp < hpPrev) c.hurtFlash = 0.12f;
        if (c.state != AState::Dead) c.deadFade = 0;
    }
    for (size_t j = mobs.size(); j-- > 0; )
        if (mobs[j].netId >= 0 && !claim[j]) { mobs[j] = mobs.back(); mobs.pop_back(); }
    // NPC
    unsigned nn = Net::R8(r);
    for (unsigned i = 0; i < nn && r.ok; i++) {
        unsigned char id = (unsigned char)Net::R8(r);
        unsigned char kind = (unsigned char)Net::R8(r);
        unsigned char job = (unsigned char)Net::R8(r);
        unsigned char st = (unsigned char)Net::R8(r);
        float x = Net::RFl(r), y = Net::RFl(r);
        unsigned char dir2 = (unsigned char)Net::R8(r);
        if (id >= 32) continue;
        if ((size_t)id >= npcs.size()) npcs.resize(id + 1);
        Npc& n = npcs[id];
        n.on = true;
        n.kind = (int)kind; n.job = (int)job; n.state = (int)st;
        n.x = x; n.y = y; n.dir = (int)dir2;
    }
    // 掉落
    unsigned ndr = Net::R8(r);
    std::vector<char> dclaim(W.drops.size(), 0);
    for (unsigned i = 0; i < ndr && r.ok; i++) {
        unsigned did = Net::R32(r);
        unsigned char kind = (unsigned char)Net::R8(r);
        float x = Net::RFl(r), y = Net::RFl(r);
        int idx = -1;
        for (size_t j = 0; j < W.drops.size(); j++)
            if (W.drops[j].netId == did) { idx = (int)j; break; }
        if (idx < 0) {
            Drop d;
            d.kind = (ItemKind)kind;
            d.x = x; d.y = y;
            d.netId = did;
            W.drops.push_back(d);
            idx = (int)W.drops.size() - 1;
        }
        W.drops[(size_t)idx].x = x;
        W.drops[(size_t)idx].y = y;
        dclaim[(size_t)idx] = 1;
    }
    for (size_t j = W.drops.size(); j-- > 0; )
        if (!dclaim[j]) { W.drops[j] = W.drops.back(); W.drops.pop_back(); }
    // 投射物
    for (Spit& s : spits) s.on = false;
    for (Arrow& a2 : arrows) a2.on = false;
    unsigned npr = Net::R8(r);
    for (unsigned i = 0; i < npr && r.ok; i++) {
        unsigned char k = (unsigned char)Net::R8(r);
        float x = Net::RFl(r), y = Net::RFl(r);
        float vx = Net::RFl(r), vy = Net::RFl(r);
        if (k == 2) {
            for (Arrow& a2 : arrows) if (!a2.on) {
                a2.on = true; a2.x = x; a2.y = y; a2.vx = vx; a2.vy = vy; a2.life = 1.5f;
                break;
            }
        } else {
            for (Spit& s : spits) if (!s.on) {
                s.on = true; s.fire = (k == 1);
                s.x = x; s.y = y; s.vx = vx; s.vy = vy;
                s.life = s.fire ? 2.2f : 1.6f;
                break;
            }
        }
    }
}

// 客人收包
static void NetGuestService() {
    Net::Buf b;
    unsigned a; unsigned short pt;
    while (Net::UdpRecv(netSock, a, pt, b)) {
        netLastRecv = (float)GetTime();
        Net::Rd r = Net::BeginRead(b.d, b.n);
        unsigned char op = (unsigned char)Net::R8(r);
        if (!r.ok) continue;
        switch (op) {
        case 'F':     // 满员
            FloatText(P.x, P.y - 50, "房间已满", 226, 40, 46);
            NetStop();
            gs = GS::Title;
            titleHasSave = SaveExists();
            break;
        case 'W': {
            unsigned seed = Net::R32(r);
            netId = (int)Net::R8(r);
            float gt = Net::RFl(r);
            int cb = (int)Net::R8(r);
            int cl = (int)Net::R8(r);
            float cx = Net::RFl(r), cy = Net::RFl(r);
            float ra = Net::RFl(r);
            int rt = (int)Net::R8(r);
            NewGame(seed);                            // 同一种子：世界地形天然一致
            gameTime = gt;
            campBuilt = cb != 0; campLv = cl;
            campCenter = { cx, cy };
            rainAmt = ra; rainTarget = rt ? 1.0f : 0.0f;
            mobs.clear();                             // 动态实体等快照重建
            npcs.clear();
            W.drops.clear();
            gs = GS::Play;
            introOn = false; tutOn = false; craftOpen = false;
            break;
        }
        case 'n': {
            unsigned seq = Net::R32(r), total = Net::R32(r);
            unsigned cnt = Net::R16(r);
            if (total == 0 || total > 64) break;
            if (netChunks.size() != total) netChunks.clear(), netChunks.resize(total);
            auto& ch = netChunks[seq];
            ch.clear();
            for (unsigned i = 0; i < cnt && r.ok; i++) {
                NetObjEnt e;
                e.k = (unsigned char)Net::R8(r);
                e.tx = (int)Net::R8(r);
                e.ty = (int)Net::R8(r);
                e.hp = (unsigned char)Net::R8(r);
                e.hv = (unsigned char)Net::R8(r);
                ch.push_back(e);
            }
            bool all = true;
            for (const auto& c : netChunks) if (c.empty()) { all = false; break; }
            if (all) {
                for (const auto& c : netChunks)
                    for (const auto& e : c)
                        NetApplyWorldObj(e.k, e.tx, e.ty, e.hp, e.hv);
                netChunks.clear();
                netJoined = true;
                FloatText(P.x, P.y - 50, "已进入他人的荒野", 190, 235, 200);
            }
            break;
        }
        case 'K':
            NetApplySnapshot(r);
            break;
        case 'D': {
            unsigned did = Net::R32(r);
            unsigned char kind = (unsigned char)Net::R8(r);
            unsigned char pid = (unsigned char)Net::R8(r);
            for (size_t i = 0; i < W.drops.size(); i++)
                if (W.drops[i].netId == did) {
                    W.drops[i] = W.drops.back();
                    W.drops.pop_back();
                    break;
                }
            if ((int)pid == netId) OnPickupDrop((ItemKind)kind);   // 自己捡到的：入账
            break;
        }
        default: break;
        }
    }
    // 未加入期间重发 Join（1 秒一拍）
    if (!netJoined) {
        static float jt = 0;
        jt += GetFrameTime();
        if (jt > 1.0f) {
            jt = 0;
            Net::Buf b2;
            b2.d[0] = 'J'; b2.n = 1;
            Net::WStr(b2, "旅人", 16);
            Net::UdpSend(netSock, netAddrHost, netPortHost, b2);
        }
    }
}

// 联机客人：本地预测移动 + 快照插值（世界/生物房主权威，只演自己的手感）
static void UpdateGuest(float dt) {
    NetGuestService();
    // 同步完成前：世界站桩等待
    if (!netJoined) {
        for (size_t i = 0; i < parts.size(); ) {
            Particle& p = parts[i];
            p.life -= dt;
            if (p.life <= 0) { p = parts.back(); parts.pop_back(); continue; }
            p.vy += 270.0f * dt;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            i++;
        }
        return;
    }
    UpdatePlayer(dt);
    // 生物朝快照目标平滑插值
    float k = fminf(1.0f, dt * 12.0f);
    for (Creature& c : mobs) {
        if (c.netId >= 0) {
            c.x += (c.netTx - c.x) * k;
            c.y += (c.netTy - c.y) * k;
        }
        c.animT += dt;
        if (c.hurtFlash > 0) c.hurtFlash -= dt;
        if (c.state == AState::Dead) c.deadFade += dt;
    }
    for (int i = 0; i < 4; i++) {
        PeerView& pr = netPeers[i];
        if (!pr.on || i == netId) continue;
        pr.x += (pr.netTx - pr.x) * k;
        pr.y += (pr.netTy - pr.y) * k;
    }
    // 投射物本地积分（纯视觉；命中结算在房主）
    for (Spit& s : spits) if (s.on) {
        s.life -= dt; s.x += s.vx * dt; s.y += s.vy * dt;
        if (s.life <= 0.0f) s.on = false;
    }
    for (Arrow& a2 : arrows) if (a2.on) {
        a2.life -= dt; a2.x += a2.vx * dt; a2.y += a2.vy * dt;
        if (a2.life <= 0.0f) a2.on = false;
    }
    // 粒子池
    for (size_t i = 0; i < parts.size(); ) {
        Particle& p = parts[i];
        p.life -= dt;
        if (p.life <= 0) { p = parts.back(); parts.pop_back(); continue; }
        p.vy += 270.0f * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        i++;
    }
    // 飘字池
    for (size_t i = 0; i < dmgs.size(); ) {
        DmgText& d = dmgs[i];
        d.life -= dt;
        d.y -= 34.0f * dt;
        if (d.life <= 0) { d = dmgs.back(); dmgs.pop_back(); continue; }
        i++;
    }
    // 饥饿本地演（快照血量为权威：饿死不伤血，但吃得少会断回血节奏）
    if (!P.dead) {
        hungerT += dt;
        if (hungerT >= 3.0f) { hungerT -= 3.0f; if (P.hunger > 0) P.hunger--; }
    }
    // 拾取请求：贴近的掉落向房主申请（先到先得，房主权威）
    static float pickReqT = 0;
    pickReqT -= dt;
    if (pickReqT <= 0.0f && !P.dead) {
        for (const Drop& d : W.drops) {
            float dx = d.x - P.x, dy = d.y - P.y;
            if (dx * dx + dy * dy < 20.0f * 20.0f && d.netId) {
                Net::Buf b;
                b.d[0] = 'P'; b.n = 1;
                Net::W32(b, d.netId);
                Net::UdpSend(netSock, netAddrHost, netPortHost, b);
                pickReqT = 0.2f;
                break;
            }
        }
    }
    // 状态 + 装备上报（20Hz / 变化即报）
    netSendT -= dt;
    if (netSendT <= 0.0f) {
        netSendT = 0.05f;
        Net::Buf b;
        b.d[0] = 'S'; b.n = 1;
        Net::WFl(b, P.x); Net::WFl(b, P.y);
        Net::W8(b, (unsigned)((P.dir & 3) | (playerMoving ? 4 : 0) | (P.running ? 8 : 0) |
                              (P.dead ? 16 : 0) | (P.atkTime > 0 ? 32 : 0)));
        Net::W16(b, (unsigned)(P.hp < 0 ? 0 : P.hp));
        Net::UdpSend(netSock, netAddrHost, netPortHost, b);
    }
    {
        static unsigned char lastTool[6] = { 255, 255, 255, 255, 255, 255 };
        static int lastSword = -1, lastLv = -1, lastHot = -1;
        bool chg = memcmp(lastTool, P.toolLv, 6) != 0 || lastSword != (int)P.swordLv ||
                   lastLv != P.level || lastHot != hotSel;
        if (chg) {
            memcpy(lastTool, P.toolLv, 6);
            lastSword = (int)P.swordLv; lastLv = P.level; lastHot = hotSel;
            Net::Buf b;
            b.d[0] = 'E'; b.n = 1;
            for (int i = 0; i < 6; i++) Net::W8(b, P.toolLv[i]);
            Net::W8(b, (unsigned)P.swordLv);
            Net::W8(b, (unsigned)P.level);
            Net::W8(b, (unsigned)hotSel);
            Net::UdpSend(netSock, netAddrHost, netPortHost, b);
        }
    }
    // 符咒：本地全演（自体 buff 即刻生效），作用鬼的部分同时上报房主
    // （在 UpdatePlayer 的 UseTalisman 调用点包裹，见 NetMaybeSendTalisman）
    // 掉线判定
    if (netLastRecv > 0.0f && (float)GetTime() - netLastRecv > 5.0f) {
        FloatText(P.x, P.y - 50, "与房主失去联系", 226, 40, 46);
        NetStop();
        gs = GS::Title;
        titleHasSave = SaveExists();
    }
}

// 客人用符咒时：自体效果本地 UseTalisman 已演；额外把"作用鬼"的符上报房主结算
static void NetMaybeSendTalisman(int t) {
    if (!netOn || netHost) return;
    if (t < 0 || t >= 5 || t == TAL_BANISH) return;    // 速符=自体 buff，不上报
    Net::Buf b;
    b.d[0] = 'T'; b.n = 1;
    Net::W8(b, (unsigned)t);
    Net::UdpSend(netSock, netAddrHost, netPortHost, b);
}

// 绘制其他联机玩家（场景层，紧随本地玩家之后）
static void DrawRemotePlayers() {
    if (!netOn) return;
    float tnow = (float)GetTime();
    for (int i = 0; i < 4; i++) {
        if (i == netId || !netPeers[i].on) continue;
        const PeerView& pr = netPeers[i];
        int d = (pr.dir == 3) ? 2 : pr.dir;
        Texture2D tex;
        if (pr.dead) tex = A.player[d][0];
        else if (pr.atk) tex = A.player[d][6];
        else if (pr.moving) tex = A.player[d][2 + (int)(tnow * 9) % 4];
        else tex = A.player[d][(int)(tnow * 2) % 2];
        Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
        if (pr.dir == 3) src.width = -src.width;
        if (!pr.dead) DrawEllipse(pr.x, pr.y - 1, 8, 3, (Color) { 0, 0, 0, 70 });
        float rot = pr.dead ? 90.0f : 0.0f;
        Vector2 org = pr.dead ? (Vector2) { 12, 27 } : (Vector2) { 0, 0 };
        DrawTexturePro(tex, src, { pr.x - 12, pr.y - 28, 24, 28 }, org, rot, WHITE);
        // 头顶名字 + 血条
        const char* nm = pr.name;
        ZhText(nm, (int)pr.x - ZhWidth(nm, 10) / 2, (int)pr.y - 44, 10, (Color) { 150, 235, 200, 220 });
        if (!pr.dead && pr.hp < 100) {
            DrawRectangle((int)pr.x - 12, (int)pr.y - 34, 24, 3, (Color) { 20, 10, 14, 180 });
            int w = (int)(24.0f * (pr.hp < 0 ? 0 : pr.hp) / 100.0f);
            DrawRectangle((int)pr.x - 12, (int)pr.y - 34, w, 3,
                          pr.hp > 40 ? (Color) { 120, 220, 170, 255 } : (Color) { 226, 80, 70, 255 });
        }
    }
}

// ---------------- 主更新 ----------------

// 敲门鬼：入夜且身处屋内（邻域 >=5 面墙）会被找上门。
// 被盯上后无法躲避，只能贴符 + 用鬼仆/雷符缠斗，**撑到天亮**才解除。
static void UpdateKnocker(float dt, bool night) {
    if (P.dead) { P.stalked = false; return; }
    bool indoors = W.CountWallAround(P.x, P.y, 2) >= 5;
    if (P.stalked) {
        if (!night) {                                  // 天亮：它走了
            P.stalked = false;
            gStalkSurvived = true;                  // 成就：浑身是胆（被盯上一整夜，活到天亮）
            FloatText(P.x, P.y - 40, "天亮了，它走了", 150, 235, 190);
            return;
        }
        knockSummonT -= dt;
        if (knockSummonT <= 0.0f) {                    // 缠斗：敲门鬼实体化来袭（限时 15 秒）
            knockSummonT = 9.0f;
            for (int tryN = 0; tryN < 24; tryN++) {
                float a = (float)(rand() % 360) * 0.0174533f;
                float sx = P.x + cosf(a) * 115.0f, sy = P.y + sinf(a) * 115.0f;
                if (!W.CircleFree(sx, sy, 8.0f)) continue;
                Creature c;
                c.kind = CreatureKind::KnockGhost;
                c.x = sx; c.y = sy;
                c.hp = MobBaseHp(CreatureKind::KnockGhost);
                c.state = AState::Chase;               // 直扑玩家
                c.triggered = true;                    // 已被惊动，不守规矩
                c.lifeT = 15.0f;                       // 实体化 15 秒后自行消散
                mobs.push_back(c);
                break;
            }
            PlaySound(AU.roar);
            FloatText(P.x, P.y - 40, "咚! 它进来了", 226, 40, 46);
        }
    } else {
        knockSummonT = 4.0f;
        if (indoors && night) {
            knockT -= dt;
            if (knockT <= 0.0f) {
                knockT = 25.0f;
                P.stalked = true;
                PlaySound(AU.roar);
                FloatText(P.x, P.y - 40, "咚、咚、咚……", 226, 40, 46);
            }
        } else {
            knockT = 25.0f;                            // 不在屋内/白天：重置
        }
    }
}

// ============================================================
// 大鬼域：独立空间状态机 + 代理游戏鬼 + 并行小游戏
// ============================================================
static const char* GD_GAME_NAME[GD_GAME_N] = { "猜拳", "反应", "记方位" };
static const char* GD_GAME_KEY[GD_GAME_N]  = { "1/2/3", "空格", "方向键" };
static const char* GD_HAND[3] = { "石头", "剪刀", "布" };      // 猜拳手势名

static void GdInfo(const char* t, float sec = 2.6f) {
    strncpy(gdInfo, t, 79); gdInfo[79] = 0; gdInfoT = sec;
}
static int GdFreeSlot() {
    for (int i = 0; i < 4; i++) if (!gdChal[i].on) return i;
    return -1;
}
// 代理鬼发起一场小游戏
static void GdOpenChallenge(int pi) {
    if (pi < 0 || pi >= (int)gdProxies.size()) return;
    int ci = GdFreeSlot();
    if (ci < 0) return;
    GdProxy& pr = gdProxies[(size_t)pi];
    pr.active = true;
    GdChallenge& c = gdChal[ci];
    c = GdChallenge();
    c.on = true;
    c.proxy = pi;
    c.game = pr.game;
    if (c.game == GD_RPS) {
        c.maxLife = c.life = 6.0f;
        c.tell = rand() % 3;                        // 它将出的手势
        c.lie  = (rand() % 100) < 18;               // 18% 概率诈你
    } else if (c.game == GD_REACT) {
        c.maxLife = c.life = 6.0f;
        c.zone = 0.24f + (float)(rand() % 52) / 100.0f;
        c.zoneW = 0.15f;
        c.sweep = 0.0f;
        c.sweepSpd = 0.70f + (float)(rand() % 45) / 100.0f;
    } else {
        c.maxLife = c.life = 8.0f;
        c.seqLen = 2 + (rand() % 2);                // 2~3 步
        for (int k = 0; k < 3; k++) c.seq[k] = (unsigned char)(rand() % 4);
        c.showT = 1.7f;
        c.seqIn = 0;
    }
    PlaySound(AU.shock);
    GdInfo(TextFormat(L10N("被选中：%s（按 %s）"), L10N(GD_GAME_NAME[c.game]), L10N(GD_GAME_KEY[c.game])));
}
// 结算一场小游戏（赢 = 代理消散；输 = 索命）
static void GdResolve(int ci, bool win) {
    if (ci < 0 || ci >= 4) return;
    GdChallenge& c = gdChal[ci];
    if (!c.on) return;
    int pi = c.proxy;
    c.on = false;
    if (pi >= 0 && pi < (int)gdProxies.size()) {
        GdProxy& pr = gdProxies[(size_t)pi];
        if (win) {
            SpawnParticles(pr.x, pr.y - 8, 14, 180, 240, 255, 120);
            gdProxies[pi] = gdProxies.back();
            gdProxies.pop_back();
        } else {
            pr.active = false;
            pr.selCd = 1.8f;
        }
    }
    if (win) {
        gdWin++;
        P.hp += 6; if (P.hp > P.maxHp) P.hp = P.maxHp;
        PlaySound(AU.craft);
        GdInfo(TextFormat(L10N("赢了一局！它散了（剩 %d 只代理）"), (int)gdProxies.size() + 1));
    } else {
        gdFail++;
        FloatText(P.x, P.y - 40, "你输了", 226, 40, 46);
        PlaySound(AU.roar);
        hurtVin = 1.0f;
        shakeT = 0.4f; shakeDur = 0.4f;
        P.hp = 0;
        KillPlayer();                                // 输了就死
    }
}
// 进入鬼域（见到鬼游戏核心即被拽入）
static void GdEnter(int ownerIdx) {
    gdOn = true;
    gdOwnerIdx = ownerIdx;
    gdCX = W.domainPos.x; gdCY = W.domainPos.y;
    gdR = 200.0f;
    gdT = 0; gdSpawnT = 1.2f; gdCap = 1; gdProxyN = 0; gdWin = 0; gdFail = 0;
    gdProxies.clear();
    for (GdChallenge& c : gdChal) c.on = false;
    PlaySound(AU.roar);
    TextCopy(bossHowlTxt, "你看见了它——鬼域张开了");
    bossHowlT = 3.2f;
    shakeT = 0.5f; shakeDur = 0.5f;
    hurtVin = 0.8f;
    GdInfo("幽蓝结界锁死：收服域主才能出去", 4.2f);
}
// 鬼域崩塌
static void GdExit() {
    gdOn = false;
    gdProxies.clear();
    for (GdChallenge& c : gdChal) c.on = false;
    gdOwnerIdx = -1;
}
// 每帧推进（世界内 & 独立空间）
static void GdUpdate(float dt) {
    if (gdInfoT > 0.0f) gdInfoT -= dt;
    if (gdSealT > 0.0f) gdSealT -= dt;
    if (P.dead) { if (gdOn) GdExit(); return; }

    // ---- 触发：看见鬼游戏核心（未被收服）即入域 ----
    if (!gdOn) {
        if (W.domainPos.x > 0.0f && !domainPurged) {
            int oi = -1;
            for (size_t i = 0; i < mobs.size(); i++)
                if (mobs[i].kind == CreatureKind::GhostDomain && mobs[i].state != AState::Dead) { oi = (int)i; break; }
            if (oi >= 0) {
                float dx = mobs[(size_t)oi].x - P.x, dy = mobs[(size_t)oi].y - P.y;
                if (dx * dx + dy * dy < 200.0f * 200.0f) GdEnter(oi);
            }
        }
        return;
    }

    // ---- 域内：独立空间 ----
    gdT += dt;
    if (gdOwnerIdx < 0 || gdOwnerIdx >= (int)mobs.size() ||
        mobs[(size_t)gdOwnerIdx].kind != CreatureKind::GhostDomain ||
        mobs[(size_t)gdOwnerIdx].state == AState::Dead) {
        gdOwnerIdx = -1;
        for (size_t i = 0; i < mobs.size(); i++)
            if (mobs[i].kind == CreatureKind::GhostDomain && mobs[i].state != AState::Dead) { gdOwnerIdx = (int)i; break; }
        if (gdOwnerIdx < 0) { GdExit(); return; }
    }
    gdCX = mobs[(size_t)gdOwnerIdx].x; gdCY = mobs[(size_t)gdOwnerIdx].y;

    // ---- 结界：把玩家锁在圈里（撞界即回推 + 掉血提示）----
    {
        float dx = P.x - gdCX, dy = P.y - gdCY;
        float d = sqrtf(dx * dx + dy * dy);
        float lim = gdR - 14.0f;
        if (d > lim && d > 0.001f) {
            P.x = gdCX + dx / d * lim;
            P.y = gdCY + dy / d * lim;
            if (gdSealT <= 0.0f) {
                gdSealT = 1.5f;
                P.hp -= 3;
                hurtVin = fmaxf(hurtVin, 0.35f);
                PlaySound(AU.shock);
                GdInfo("结界：收服域主之前，一步也出不去");
                if (P.hp <= 0) { KillPlayer(); return; }
            }
        }
    }

    // ---- 代理上限随时间增长；定期放出新的代理游戏鬼 ----
    gdCap = 1 + (int)(gdT / 14.0f);
    if (gdCap > 4) gdCap = 4;
    gdSpawnT -= dt;
    if (gdSpawnT <= 0.0f && (int)gdProxies.size() < gdCap) {
        gdSpawnT = 9.0f - gdT * 0.05f;
        if (gdSpawnT < 4.0f) gdSpawnT = 4.0f;
        float a = (float)(rand() % 360) * 0.0174533f;
        float rr = 70.0f + (float)(rand() % 90);
        GdProxy pr;
        pr.x = gdCX + cosf(a) * rr;
        pr.y = gdCY + sinf(a) * rr * 0.8f;
        pr.game = (unsigned char)(rand() % GD_GAME_N);
        pr.selCd = 1.4f;
        pr.driftA = a;
        pr.driftT = 1.0f;
        gdProxies.push_back(pr);
        gdProxyN++;
        SpawnParticles(pr.x, pr.y - 8, 12, 120, 180, 255, 90);
        GdInfo(TextFormat(L10N("又一只代理游戏鬼飘了进来（%s）"), L10N(GD_GAME_NAME[pr.game])));
    }

    // ---- 代理漂移 + 选中玩家（靠近即开局；最多同时 4 场）----
    int activeN = 0;
    for (GdChallenge& c : gdChal) if (c.on) activeN++;
    for (size_t i = 0; i < gdProxies.size(); i++) {
        GdProxy& pr = gdProxies[i];
        pr.animT += dt;
        if (pr.active) continue;
        pr.driftT -= dt;
        if (pr.driftT <= 0.0f) {
            pr.driftT = 1.2f + (float)(rand() % 100) / 60.0f;
            pr.driftA = (float)(rand() % 360) * 0.0174533f;
        }
        pr.x += cosf(pr.driftA) * 26.0f * dt;
        pr.y += sinf(pr.driftA) * 20.0f * dt;
        float dx = pr.x - gdCX, dy = pr.y - gdCY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > gdR - 26.0f && d > 0.001f) {
            pr.x = gdCX + dx / d * (gdR - 26.0f);
            pr.y = gdCY + dy / d * (gdR - 26.0f);
            pr.driftA += 3.1415927f;                 // 撞结界就折返
        }
        pr.selCd -= dt;
        if (pr.selCd <= 0.0f) {
            float px2 = P.x - pr.x, py2 = P.y - pr.y;
            if (px2 * px2 + py2 * py2 < 64.0f * 64.0f && activeN < 4) {
                GdOpenChallenge((int)i);
                activeN++;
            } else {
                pr.selCd = 0.6f;
            }
        }
    }

    // ---- 小游戏推进 + 输入（三套键位互不冲突 → 可同时应付多场）----
    for (int i = 0; i < 4; i++) {
        GdChallenge& c = gdChal[i];
        if (!c.on) continue;
        c.t += dt;
        c.life -= dt;
        if (c.flashT > 0.0f) c.flashT -= dt;
        if (c.life <= 0.0f) { GdResolve(i, false); continue; }
        if (c.game == GD_RPS) {
            // 石头(1) 克 剪刀(1)；剪刀(2) 克 布(2)；布(3) 克 石头(0)
            if (IsKeyPressed(KEY_ONE))        GdResolve(i, c.tell == 1);
            else if (IsKeyPressed(KEY_TWO))   GdResolve(i, c.tell == 2);
            else if (IsKeyPressed(KEY_THREE)) GdResolve(i, c.tell == 0);
        } else if (c.game == GD_REACT) {
            c.sweep += c.sweepSpd * dt;
            if (c.sweep > 1.0f) c.sweep -= 1.0f;
            if (IsKeyPressed(KEY_SPACE)) {
                float dd = fabsf(c.sweep - c.zone);
                GdResolve(i, dd <= c.zoneW);
            }
        } else {
            if (c.showT > 0.0f) { c.showT -= dt; continue; }   // 展示阶段不可输入
            int in = -1;
            if (IsKeyPressed(KEY_UP))         in = 0;
            else if (IsKeyPressed(KEY_DOWN))  in = 1;
            else if (IsKeyPressed(KEY_LEFT))  in = 2;
            else if (IsKeyPressed(KEY_RIGHT)) in = 3;
            if (in >= 0) {
                if (in == (int)c.seq[c.seqIn]) {
                    c.seqIn++;
                    if (c.seqIn >= c.seqLen) GdResolve(i, true);
                } else {
                    GdResolve(i, false);
                }
            }
        }
    }

    // ---- 域主提示：没有对局时靠近它，需要镇鬼幡才能收服 ----
    if (activeN == 0 && gdOwnerIdx >= 0 && gdOwnerIdx < (int)mobs.size() && gdInfoT <= 0.0f) {
        float dx = mobs[(size_t)gdOwnerIdx].x - P.x, dy = mobs[(size_t)gdOwnerIdx].y - P.y;
        float r = CapRange(P.captureLv > 0 ? P.captureLv : 1);
        if (dx * dx + dy * dy < r * r) {
            if (P.captureLv == 0) GdInfo("域主就在眼前：你需要镇鬼幡（V）才能收服它", 2.6f);
            else GdInfo("按 V 收服域主，鬼域即可崩塌", 2.6f);
        }
    }
}

// 鬼域 HUD：顶部横幅 + 并行小游戏卡片（屏幕空间）
static void GdDrawHud() {
    if (!gdOn || gs != GS::Play) return;
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 2.0f);

    // ---- 顶部横幅 ----
    {
        const int bw = 300, bx = (VW - bw) / 2, by = 6, bh = 22;
        DrawRectangle(bx - 2, by - 2, bw + 4, bh + 4, (Color) { 4, 8, 18, 235 });
        DrawRectangle(bx, by, bw, bh, (Color) { 12, 24, 48, 240 });
        DrawRectangle(bx, by, bw, 1, (Color) { 120, 190, 255, (unsigned char)(180 + 60 * pulse) });
        DrawRectangle(bx, by + bh - 1, bw, 1, (Color) { 40, 90, 170, 200 });
        const char* ttl = "鬼 域 · 游 戏 鬼 的 场";
        ZhTextS(TXT_SYS, ttl, bx + (bw - ZhWidth(ttl, 12)) / 2, by + 2, 12);
        const char* sub = TextFormat(L10N("代理 %d/%d   已赢 %d   收服域主(V) 方能脱出"),
                                     (int)gdProxies.size(), gdCap, gdWin);
        ZhTextS(TXT_MUT, sub, bx + (bw - ZhWidth(sub, 10)) / 2, by + bh - 12, 10);
    }

    // ---- 并行小游戏卡片 ----
    int n = 0;
    for (int i = 0; i < 4; i++) if (gdChal[i].on) n++;
    if (n > 0) {
        const int cw = 146, ch = 84, gap = 8;
        int total = n * cw + (n - 1) * gap;
        int x0 = (VW - total) / 2, y0 = 34;
        int drawn = 0;
        for (int i = 0; i < 4; i++) {
            const GdChallenge& c = gdChal[i];
            if (!c.on) continue;
            int cx = x0 + drawn * (cw + gap);
            drawn++;
            DrawRectangle(cx - 2, y0 - 2, cw + 4, ch + 4, (Color) { 4, 8, 18, 240 });
            DrawRectangle(cx, y0, cw, ch, (Color) { 10, 20, 40, 244 });
            DrawRectangle(cx, y0, cw, 1, (Color) { 140, 200, 255, 210 });
            // 标题：游戏名 + 键位
            ZhTextS(TXT_SYS, GD_GAME_NAME[c.game], cx + 6, y0 + 3, 12);
            const char* kh = GD_GAME_KEY[c.game];
            ZhTextS(TXT_MUT, kh, cx + cw - ZhWidth(kh, 10) - 6, y0 + 5, 10);
            // 时间条
            float k = c.life / c.maxLife;
            DrawRectangle(cx + 6, y0 + 18, cw - 12, 3, (Color) { 20, 30, 50, 255 });
            Color tc = k > 0.5f ? (Color) { 120, 230, 180, 255 }
                     : (k > 0.25f ? (Color) { 240, 210, 120, 255 } : (Color) { 240, 90, 90, 255 });
            DrawRectangle(cx + 6, y0 + 18, (int)((cw - 12) * k), 3, tc);

            int my = y0 + 26;
            if (c.game == GD_RPS) {
                int shown = c.lie ? (c.tell + 1) % 3 : c.tell;
                const char* hm = TextFormat(L10N("它要出：%s"), L10N(GD_HAND[shown]));
                ZhTextS(TXT_GOLD, hm, cx + 6, my, 12);
                const char* hi = "1石头 2剪刀 3布";
                ZhTextS(TXT_BODY, hi, cx + 6, my + 18, 10);
                // 三只手势提示位
                for (int h = 0; h < 3; h++) {
                    int hx = cx + 8 + h * 44;
                    bool beat = (h == 0 && c.tell == 1) || (h == 1 && c.tell == 2) || (h == 2 && c.tell == 0);
                    DrawRectangle(hx, my + 34, 40, 16, beat ? (Color) { 30, 70, 50, 240 } : (Color) { 20, 28, 44, 220 });
                    DrawRectangle(hx, my + 34, 1, 16, (Color) { 120, 180, 240, 180 });
                    ZhText(GD_HAND[h], hx + 4, my + 37, 10,
                           beat ? (Color) { 150, 255, 200, 255 } : (Color) { 180, 195, 215, 255 });
                }
            } else if (c.game == GD_REACT) {
                ZhTextS(TXT_BODY, "指针进亮区按 空格", cx + 6, my, 10);
                int bx2 = cx + 8, bw2 = cw - 16, by2 = my + 22;
                DrawRectangle(bx2, by2, bw2, 14, (Color) { 18, 28, 46, 245 });
                int zx = bx2 + (int)((c.zone - c.zoneW) * bw2);
                int zw = (int)(c.zoneW * 2 * bw2); if (zw < 3) zw = 3;
                DrawRectangle(zx, by2, zw, 14, (Color) { 40, 110, 170, 245 });
                DrawRectangle(zx, by2, zw, 1, (Color) { 150, 230, 255, 220 });
                int mx = bx2 + (int)(c.sweep * bw2);
                DrawRectangle(mx - 1, by2 - 3, 3, 20, (Color) { 255, 240, 190, 245 });
            } else {
                if (c.showT > 0.0f) {
                    ZhTextS(TXT_WARN, "记住顺序！", cx + 6, my, 12);
                } else {
                    const char* hh = TextFormat(L10N("按顺序输入 %d/%d"), c.seqIn, c.seqLen);
                    ZhTextS(TXT_BODY, hh, cx + 6, my, 10);
                }
                static const char* AR[4] = { "上", "下", "左", "右" };
                for (int s = 0; s < 3; s++) {
                    int sx = cx + 8 + s * 44;
                    bool used = s >= c.seqLen;
                    bool done = s < c.seqIn;
                    DrawRectangle(sx, my + 24, 40, 26, used ? (Color) { 14, 20, 32, 160 } : (Color) { 20, 34, 56, 235 });
                    DrawRectangle(sx, my + 24, 1, 26, (Color) { 110, 175, 240, 170 });
                    if (!used) {
                        int shown2 = (int)c.seq[s];
                        if (c.showT > 0.0f || done)
                            ZhText(AR[shown2], sx + 12, my + 30, 12,
                                   done ? (Color) { 150, 255, 200, 255 } : (Color) { 235, 245, 255, 255 });
                        else
                            ZhText("?", sx + 14, my + 30, 12, (Color) { 150, 175, 205, 255 });
                    }
                }
            }
        }
    }

    // ---- 域内提示 ----
    if (gdInfoT > 0.0f && gdInfo[0]) {
        int iw = ZhWidth(gdInfo, 12);
        unsigned char a = (unsigned char)(gdInfoT > 0.6f ? 255 : (int)(gdInfoT / 0.6f * 255));
        DrawRectangle((VW - iw) / 2 - 6, VH - 150, iw + 12, 18, (Color) { 6, 12, 26, (unsigned char)(a * 0.8f) });
        ZhText(gdInfo, (VW - iw) / 2, VH - 147, 12, (Color) { 170, 215, 255, a });
    }
}

void UpdateGame(float dt) {
    if (scrollOn) return;              // 剧情卷轴：世界与输入全冻结，专心读字
    if (netOn && !netHost) { UpdateGuest(dt); return; }   // 联机客人：轻量本地模拟
    // 伴生铁匠鬼下标（mobs 会因移除而换位 → 每帧重扫，保证 E 交互永远指向它）
    gSmithCompanionIdx = -1;
    for (size_t i = 0; i < mobs.size(); i++)
        if (mobs[i].companion && mobs[i].kind == CreatureKind::SmithGhost) { gSmithCompanionIdx = (int)i; break; }
    // 铁匠鬼消失兜底：旧存档没有它 / 异常路径被移除时，立刻在玩家侧上方重新凝聚
    if (gSmithCompanionIdx < 0 && gs == GS::Play) {
        Creature c;
        c.kind = CreatureKind::SmithGhost;
        c.companion = true;
        c.x = P.x + 26.0f; c.y = P.y - 34.0f;
        c.hp = MobBaseHp(c.kind);
        c.state = AState::Idle;
        mobs.push_back(c);
        gSmithCompanionIdx = (int)mobs.size() - 1;
    }

    gameTime += dt;
    if (shakeT > 0) shakeT -= dt;
    if (hurtVin > 0) hurtVin -= dt * 1.7f;
    if (bossHowlT > 0) bossHowlT -= dt;

    UpdateKnocker(dt, IsNight(gameTime));
    if (wailHintT > 0.0f) wailHintT -= dt;
    if (rpsHintT > 0.0f) rpsHintT -= dt;
    if (smithMsgT > 0.0f) smithMsgT -= dt;
    if (gourdCd > 0.0f) gourdCd -= dt;
    if (P.shieldT > 0.0f) P.shieldT -= dt;
    // ---- 鬼仆「鬼域」持续失血结算（鬼杀不死，但能一点点磨到它退走）----
    for (size_t i = 0; i < mobs.size(); i++) {
        Creature& c = mobs[i];
        if (c.dotT <= 0.0f || c.state == AState::Dead) continue;
        c.dotT -= dt;
        HitMobByGhost(c, CreatureKind::GhostChild, c.dotDps * dt, 0.0f, 1.0f);
    }
    // ---- 被击溃的鬼：倒计时结束在远离玩家处重新凝聚（除非它已被收容）----
    for (Banished& b : banished) {
        if (!b.on) continue;
        b.t -= dt;
        if (b.t > 0.0f) continue;
        b.on = false;
        bool placed = false;
        for (int k = 0; k < 30 && !placed; k++) {
            float sx, sy;
            if (b.kind == CreatureKind::SmithGhost) {
                // 铁匠鬼：固定在自己的废弃铁匠铺里重新凝聚（铺子就是他的家，他不会死也不会离家）
                sx = W.smithGhostPos.x; sy = W.smithGhostPos.y;
                if (W.smithGhostPos.x <= 0.0f) {          // 无铺子（异常兜底）：远处随机
                    float a = (float)(rand() % 360) * 0.0174533f;
                    sx = P.x + cosf(a) * 560.0f; sy = P.y + sinf(a) * 450.0f;
                }
                placed = true;
            } else {
                float a = (float)(rand() % 360) * 0.0174533f;
                float r = 520.0f + (float)(rand() % 160);
                sx = P.x + cosf(a) * r; sy = P.y + sinf(a) * r * 0.8f;
                if (sx < 32.0f || sy < 32.0f || sx > (MAP_W - 2) * 16.0f || sy > (MAP_H - 2) * 16.0f) continue;
                if (!W.CircleFree(sx, sy, 8.0f)) continue;
            }
            Creature c;
            c.kind = b.kind; c.x = sx; c.y = sy;
            c.tier = b.tier; c.dmgMul = b.mul;
            c.hp = MobBaseHp(b.kind);
            if (c.hp <= 0) c.hp = 1;
            RollGhostRule(c);
            mobs.push_back(c);
            placed = true;
        }
    }

    // ---- 天气：晴/雨随机轮换，雨强平滑过渡（地牢恒晴）----
    if (!rainSeeded) {
        for (auto& d : rainDrops) {
            d.x = (float)(rand() % (VW + 80)) - 60.0f;
            d.y = (float)(rand() % (VH + 120)) - 80.0f;
            d.spd = 400.0f + (float)(rand() % 170);
        }
        rainSeeded = true;
    }
    weatherT -= dt;
    if (weatherT <= 0) {
        bool toRain = rainTarget < 0.5f;
        rainTarget = toRain ? 1.0f : 0.0f;
        weatherT = toRain ? (28.0f + (float)(rand() % 16))
                          : (55.0f + (float)(rand() % 70));
        // 荧雨夜：入夜后转雨才有，30% 概率——磷光养人，伤势缓愈
        gGlowRain = false;
        if (toRain && IsNight(gameTime) && (rand() % 100) < 30) {
            gGlowRain = true;
            FloatText(P.x, P.y - 46, "荧雨夜——磷光养人，伤势缓愈", 140, 235, 200);
        }
    }
    rainAmt += (rainTarget - rainAmt) * (1.0f - expf(-1.6f * dt));
    if (rainAmt > 0.02f) {
        for (auto& d : rainDrops) {
            d.y += d.spd * dt;
            d.x += 66.0f * dt;                       // 风斜
            if (d.y > VH + 4 || d.x > VW + 60) {     // 落地：落在水面溅起水花（扩散圈由海面着色器绘制）
                float wx = camX + d.x, wy = camY + d.y;
                int wtx = (int)(wx / TILE), wty = (int)(wy / TILE);
                if ((unsigned)wtx < (unsigned)MAP_W && (unsigned)wty < (unsigned)MAP_H &&
                    W.TileAt(wtx, wty) == Tile::Water) {
                    if (parts.size() < 480) SpawnParticles(wx, wy, 1, 140, 160, 185, 40.0f);
                }
                d.x = (float)(rand() % (VW + 80)) - 60.0f;
                d.y = -8.0f - (float)(rand() % 70);
                d.spd = 400.0f + (float)(rand() % 170);
            }
        }
    }

    // ---- 落叶：从视口内树冠随机飘落（雨天风大更密更快；地牢无树冠落叶）----
leafT -= dt;
if (leafT <= 0.0f) {
    leafT = 0.08f + 0.10f * (1.0f - rainAmt);
    for (int tr = 0; tr < 6; tr++) {
        int ltx = camX / TILE + rand() % (VW / TILE + 1);
        int lty = camY / TILE + rand() % (VH / TILE + 1);
        if ((unsigned)ltx >= (unsigned)MAP_W || (unsigned)lty >= (unsigned)MAP_H) continue;
        int oi = W.ObjIndexAt(ltx, lty);
        if (oi < 0 || W.objs[(size_t)oi].kind != ObjKind::Tree) continue;
        for (auto& L : leaves)
            if (!L.on) {
                L.on = true;
                L.x = ltx * 16.0f + 2.0f + (float)(rand() % 12);
                L.y = lty * 16.0f - 26.0f + (float)(rand() % 12);
                L.vy = 15.0f + (float)(rand() % 14) + rainAmt * 14.0f;
                L.ph = (float)(rand() % 628) / 100.0f;
                L.ground = lty * 16.0f + 12.0f + (float)(rand() % 5);
                L.fade = 1.0f;
                break;
            }
        break;
    }
}
    for (auto& L : leaves)
        if (L.on) {
            if (L.y < L.ground) {                       // 空中：正弦飘摆 + 风推
                L.ph += dt * (2.6f + 2.2f * rainAmt);
                L.x += sinf(L.ph) * 16.0f * dt + 9.0f * dt * (1.0f + 2.0f * rainAmt);
                L.y += L.vy * dt;
                if (L.y > L.ground) L.y = L.ground;
            } else {                                    // 落地：停留渐隐
                L.fade -= dt * 0.8f;
                if (L.fade <= 0.0f) L.on = false;
            }
        }

    UpdatePlayer(dt);
    // 同步盟友只读视图（creature.cpp 用于仇恨吸引目标解算：宠物 + 鬼仆）
    PETINFO.on = PET.on; PETINFO.x = PET.x; PETINFO.y = PET.y;
    PETINFO.ghostN = (int)ghosts.size();
    for (size_t i = 0; i < ghosts.size() && i < 3; i++) {
        PETINFO.ghostX[i] = ghosts[i].x;
        PETINFO.ghostY[i] = ghosts[i].y;
    }
    // ---- 神秘复苏：玩家状态上下文（每种鬼按自己的规律判定是否被触发）----
    PlayerCtx pc;
    pc.hpPct = P.maxHp > 0 ? P.hp / (float)P.maxHp : 1.0f;
    pc.running = P.running;
    pc.moving = playerMoving;
    pc.dir = P.dir;
    pc.usedItem = playerUsedItem;
    pc.mining = playerMining;
    pc.hiding = (P.hideT > 0.0f) || gdOn;      // 鬼域内：其他鬼一律不触发（专心应付游戏）
    // ---- 落单判定（单鬼规则）：120px 内有宠物/鬼仆/跟随幸存者即不算落单 ----
    pc.alone = true;
    if (PET.on && fabsf(PET.x - P.x) + fabsf(PET.y - P.y) < 160.0f) pc.alone = false;
    for (const GhostAlly& g : ghosts)
        if (fabsf(g.x - P.x) + fabsf(g.y - P.y) < 160.0f) { pc.alone = false; break; }
    if (pc.alone)
        for (const Npc& n : npcs)
            if (n.on && n.kind == 1 && n.state == 1 &&
                fabsf(n.x - P.x) + fabsf(n.y - P.y) < 160.0f) { pc.alone = false; break; }
    // 哨兵（入住营地的哨兵 200px 内视为不落单：它在替你望风）
    if (pc.alone && campBuilt)
        for (const Npc& n : npcs)
            if (n.on && n.kind == 1 && n.state == 2 && n.job == 4 &&
                fabsf(n.x - P.x) + fabsf(n.y - P.y) < 200.0f) { pc.alone = false; break; }
    // 面容剥夺（多面人）：3 层减速 + 每 8 秒消退一层
    pc.slowK = 1.0f - 0.12f * P.faceStolen;
    if (P.faceStolen > 0) {
        P.faceStolenT += dt;
        if (P.faceStolenT >= 8.0f) {
            P.faceStolenT = 0;
            P.faceStolen--;
        }
    }
    {   // 是否贴水（水鬼的规律）
        int ptx = (int)(P.x / TILE), pty = (int)(P.y / TILE);
        for (int oy = -1; oy <= 1 && !pc.nearWater; oy++)
            for (int ox = -1; ox <= 1 && !pc.nearWater; ox++)
                if (W.TileAt(ptx + ox, pty + oy) == Tile::Water) pc.nearWater = true;
    }
    {   // 身边是否已有同伴（无面鬼据此决定是否值得混进来）
        int t0 = 0, f0 = 0;
        CountCompanions(t0, f0);
        pc.hasAlly = (t0 > 0);
    }
    {
        // 联机：鬼挑"最近的存活玩家"下手（单机 = 只有自己）
        PlayerTarget tgts[4];
        int nT = 1;
        tgts[0].x = P.x; tgts[0].y = P.y; tgts[0].dead = P.dead; tgts[0].pc = pc;
        if (netOn && netHost) {
            for (int i = 1; i < 4; i++) if (netPeers[i].on) {
                PlayerTarget& t = tgts[nT];
                t.x = netPeers[i].x; t.y = netPeers[i].y; t.dead = netPeers[i].dead;
                t.pc = pc;
                t.pc.moving = netPeers[i].moving;
                t.pc.running = netPeers[i].running;
                t.pc.dir = netPeers[i].dir;
                t.pc.alone = true; t.pc.hasAlly = false;
                t.pc.hpPct = netPeers[i].hp / 100.0f;
                // 客人贴近水域判定（9 格邻域，与房主同一口径）
                int ptx = (int)(t.x / TILE), pty = (int)(t.y / TILE);
                t.pc.nearWater = false;
                for (int oy = -1; oy <= 1 && !t.pc.nearWater; oy++)
                    for (int ox = -1; ox <= 1 && !t.pc.nearWater; ox++)
                        if (W.TileAt(ptx + ox, pty + oy) == Tile::Water) t.pc.nearWater = true;
                nT++;
            }
        }
        UpdateCreaturesT(mobs, W, tgts, nT, gameTime, dt, PETINFO);
    }
    // 联机：房主收包（加入/攻击/采集/拾取/符咒）+ 10Hz 快照广播
    if (netOn && netHost) NetHostService(dt);
    // ---- 无面鬼同化：混入队伍的会隔一阵子顶替掉一个真同伴 ----
    UpdateFacelessSpread(dt);
    // ---- 身边同伴统计（领地 UI + 无面鬼占比判定共用）----
    CountCompanions(gCompTotal, gCompFace);
    // ---- 无面鬼·清算：身边同伴 > 5 且无面鬼占比达阈值 → 你的队伍已经不是你的了 ----
    if (!P.dead && gs == GS::Play && gCompTotal > 5 &&
        gCompFace * 100 >= gCompTotal * FacelessPct()) {
        FloatText(P.x, P.y - 50, "身边的人……都不对劲", 226, 40, 46);
        SpawnParticles(P.x, P.y - 12, 26, 190, 185, 175, 140);
        KillPlayer();
    }
    // ---- 问询窗口 / 应答显示计时 ----
    if (askWinT > 0.0f) { askWinT -= dt; if (askWinT <= 0.0f) { askType = -1; askIdx = -1; } }
    if (askLineT > 0.0f) askLineT -= dt;
    playerUsedItem = false;                                 // 单帧事件，用后立即清
    playerMining = false;
    UpdatePet(dt);
    UpdateGhosts(dt);
    UpdateNpcs(dt);

    // ---- 领地镇宅：房屋周围形成庇护圈，邪物靠近即被推离；等级越高圈越大、推力越强 ----
    // Lv1 仅能迟缓，Lv2 起还能把追猎中的鬼扰成游荡（防御能力随等级实实在在变强）
    if (campBuilt && !P.dead) {
        float rr = (float)CampRadius(campLv) * 16.0f + 26.0f;
        for (Creature& c : mobs) {
            if (c.state == AState::Dead || c.companion || c.infiltrated || !MobHostile(c.kind)) continue;
            float dx = c.x - campCenter.x, dy = c.y - campCenter.y;
            float d2 = dx * dx + dy * dy;
            if (d2 > rr * rr || d2 < 1.0f) continue;
            float d = sqrtf(d2);
            float push = (30.0f + 34.0f * campLv) * dt * (HomeWardPlus() ? 1.5f : 1.0f);
            c.x += dx / d * push; c.y += dy / d * push;
            if (campLv >= 4) c.atkCd += dt * 1.2f;   // 领地 Lv4：圈内邪物出手更慢
            if ((campLv >= 2 || HomeWardPlus()) && c.state == AState::Chase) { c.state = AState::Wander; c.timer = 0.6f; }
        }
    }

    // ---- 鬼种吞魂进化：鬼童/少年靠近残魂即吞噬（3 魂进阶形态）----
    for (Creature& c : mobs) {
        if (c.state == AState::Dead) continue;
        if (c.kind != CreatureKind::GhostChild && c.kind != CreatureKind::GhostTeen) continue;
        for (Soul& s : souls) {
            if (!s.on) continue;
            float dx = s.x - c.x, dy = s.y - c.y;
            if (dx * dx + dy * dy > 26.0f * 26.0f) continue;
            s.on = false;                                   // 吞噬
            c.phase++;
            SpawnParticles(c.x, c.y - 10, 10, 120, 255, 200, 110);
            if (c.phase >= 3) {                             // 进化！
                CreatureKind nk = (c.kind == CreatureKind::GhostChild) ?
                                   CreatureKind::GhostTeen : CreatureKind::GhostAdult;
                c.kind = nk;
                c.hp = MobBaseHp(nk);
                c.phase = 0;
                c.triggered = true;                         // 进化后暴起
                PlaySound(AU.roar);
                TextCopy(bossHowlTxt, nk == CreatureKind::GhostAdult ? "它吞够了魂……进化了!"
                                                                     : "鬼童吞噬魂火，长大了!");
                bossHowlT = 2.5f;
                shakeT = 0.25f; shakeDur = 0.25f;
                SpawnParticles(c.x, c.y - 12, 22, 90, 235, 190, 130);
            } else {
                FloatText(c.x, c.y - 26, "它在吞魂……", 176, 32, 32);
            }
            break;
        }
    }

    // ---- 大鬼域（鬼游戏·核心）：独立封闭空间 / 代理游戏鬼 / 并行小游戏 ----
    GdUpdate(dt);
    playerAttackedFrame = false;                     // 单帧事件，用后即清
    if (captureCd > 0.0f) captureCd -= dt;
    if (P.faceCoverT > 0.0f) P.faceCoverT -= dt;
    // ---- 收鬼特效池计时 ----
    for (CapFx& f : capFxs) {
        if (!f.on) continue;
        f.t += dt;
        if (f.t >= f.dur) f.on = false;
    }
    // ---- 记忆篡改计时：结束后恢复（无脸鬼的覆盖褪去）----
    if (P.hacked) {
        P.hackT -= dt;
        if (P.hackT <= 0.0f) {
            P.hacked = false;
            FloatText(P.x, P.y - 44, "记忆恢复了", 150, 255, 200);
            PlaySound(AU.pickup);
            // 演出：小型身份归位环（记忆自己挣回来了，弱化版金环）
            SpawnCapFx(1, P.x, P.y, 0, 0);
        }
    }

    // ---- 面容剥夺第 3 层：额外重创（多面人剥夺了你的脸）----
    if (P.faceStolen >= 3 && !P.dead) {
        P.hp -= 1;
        if (P.hp <= 0) KillPlayer();
    }

    // ---- 每日悬赏 + 世界事件（入夜侦测 / 鬼潮来袭 / 荧雨回血）----
    BountyUpdate();
    {
        bool nowNight = IsNight(gameTime);
        if (!gWasNight && nowNight && gameTime > DAY_LEN * 0.9f && !gTide && (rand() % 100) < 35) {
            gTide = true; gTideT = 6.0f;
            FloatText(P.x, P.y - 46, "今夜……有东西在聚集", 226, 40, 46);
            PlaySound(AU.roar);
        }
        gWasNight = nowNight;
        if (gTide && !nowNight) {
            gTide = false;
            FloatText(P.x, P.y - 46, "天亮了，它们退散了", 150, 235, 190);
        }
        if (gTide && nowNight) {
            gTideT -= dt;
            if (gTideT <= 0.0f) {
                gTideT = 9.0f;
                for (int k = 0; k < 2; k++) {
                    for (int tr = 0; tr < 20; tr++) {
                        float a = (float)(rand() % 360) * 0.0174533f;
                        float sx = P.x + cosf(a) * 300.0f, sy = P.y + sinf(a) * 260.0f;
                        if (sx < 32 || sy < 32 || sx > (MAP_W - 2) * 16.0f || sy > (MAP_H - 2) * 16.0f) continue;
                        if (!W.CircleFree(sx, sy, 8.0f)) continue;
                        Creature c;
                        c.kind = (rand() % 2) ? CreatureKind::GhostTeen : CreatureKind::GhostChild;
                        c.x = sx; c.y = sy;
                        c.hp = MobBaseHp(c.kind); if (c.hp <= 0) c.hp = 1;
                        c.state = AState::Chase;
                        c.triggered = true;
                        c.tier = (unsigned char)(rand() % 3);
                        c.dmgMul = MobScaleMul(gameTime, c.tier);
                        mobs.push_back(c);
                        break;
                    }
                }
            }
        }
        // 荧雨夜生效中：每秒回 1 血 + 周身磷光粒子
        static float glowTick = 0;
        if (gGlowRain && rainAmt > 0.3f && !P.dead) {
            glowTick += dt;
            if (glowTick >= 1.0f) {
                glowTick -= 1.0f;
                if (P.hp < P.maxHp) { P.hp++; SpawnParticles(P.x, P.y - 16, 3, 120, 235, 200, 45); }
            }
            if ((rand() & 63) < 2)
                SpawnParticles(P.x + (rand() % 120 - 60), P.y + (rand() % 90 - 45), 1, 120, 235, 200, 35.0f);
        }
    }

    // ---- 领地：入住幸存者的每日产出（猎人给肉 / 农夫给血莓 / 工匠给石）----
    if (campBuilt) {
        int today = (int)(gameTime / DAY_LEN);
        if (dayCampMark != today) {
            dayCampMark = today;
            int nhunt = 0, nfarm = 0, nart = 0;
            for (const Npc& n : npcs)
                if (n.on && n.kind == 1 && n.state == 2) {
                    if (n.job == 1) nhunt++;
                    else if (n.job == 2) nfarm++;
                    else if (n.job == 3) nart++;
                }
            int mul = campLv >= 5 ? 2 : 1;      // 领地 Lv5：营地产出翻倍
            if (nhunt + nfarm + nart > 0) {
                P.cookedMeat += 2 * nhunt * mul;
                P.berry += 4 * nfarm * mul;
                P.stone += 2 * nart * mul;
                FloatText(P.x, P.y - 44,
                          TextFormat(L10N("营地送来 熟肉x%d 血莓x%d 石x%d"),
                                     2 * nhunt * mul, 4 * nfarm * mul, 2 * nart * mul),
                          190, 235, 200);
                PlaySound(AU.pickup);
            }
        }
        // 医师：营地 120px 内每秒回血 1
        bool healer = false;
        for (const Npc& n : npcs)
            if (n.on && n.kind == 1 && n.state == 2 && n.job == 0 &&
                fabsf(n.x - P.x) + fabsf(n.y - P.y) < 120.0f) { healer = true; break; }
        if (healer && !P.dead && P.hp < P.maxHp) {
            campHealT += dt;
            if (campHealT >= 1.0f) { campHealT = 0; P.hp++; }
        }
    }

    // ---- 残魂更新（12 秒后消散；持幡时被幡力牵引飘近）----
    for (Soul& s : souls) {
        if (!s.on) continue;
        s.age += dt;
        if (s.age > 12.0f) {
            s.on = false;
            SpawnParticles(s.x, s.y - 10, 6, 130, 200, 180, 50);
            continue;
        }
        if (P.captureLv > 0 && !P.dead) {
            float dx = P.x - s.x, dy = (P.y - 8) - s.y;
            float d = sqrtf(dx * dx + dy * dy);
            float mag = CapMagnet(P.captureLv);          // 磁吸半径按幡等级（Lv1 90 / Lv2 120 / Lv3 150）
            if (d > 10.0f && d < mag) {
                s.x += dx / d * 30.0f * dt;
                s.y += dy / d * 30.0f * dt;
            }
        }
    }

    // 物体计时（晃动 / 浆果再生；仅活跃物体，免全量遍历）
    for (size_t i = 0; i < objActive.size(); ) {
        WorldObj& o = CurObjs()[(size_t)objActive[i]];
        bool keep = false;
        if (o.shake > 0) { o.shake -= dt; if (o.shake > 0) keep = true; }
        if (o.kind == ObjKind::Berry && o.harvested) {
            o.regrow -= dt;
            if (o.regrow <= 0) o.harvested = false;
            else keep = true;
        }
        if (keep) i++;
        else { objActive[i] = objActive.back(); objActive.pop_back(); }
    }

    // 篝火火星粒子（地表专属）
    for (int fi : W.campfires) {
        if ((rand() & 63) < 3) {
            const WorldObj& o = W.objs[(size_t)fi];
            SpawnParticles(o.tx * 16.0f + 8 + (rand() % 9 - 4), o.ty * 16.0f + 5,
                           1, 255, 170, 60, 30.0f);
        }
    }

    // 掉落物（磁吸 + 拾取）
    W.UpdateDrops(P.x, P.y, dt, OnPickup);

    // 粒子池
    for (size_t i = 0; i < parts.size(); ) {
        Particle& p = parts[i];
        p.life -= dt;
        if (p.life <= 0) { p = parts.back(); parts.pop_back(); continue; }
        p.vy += 270.0f * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        i++;
    }

    // 飘字池
    for (size_t i = 0; i < dmgs.size(); ) {
        DmgText& d = dmgs[i];
        d.life -= dt;
        d.y -= 34.0f * dt;
        if (d.life <= 0) { d = dmgs.back(); dmgs.pop_back(); continue; }
        i++;
    }

    // ---- 生存数值 ----
    if (!P.dead) {
        hungerT += dt;
        if (hungerT >= 3.0f / HomeHungerMul()) {                       // 饥饿每 3 秒 -1
            hungerT -= 3.0f;
            if (P.hunger > 0) P.hunger--;
        }
        if (P.hunger <= 0) {                          // 饿到 0：生命每 2 秒 -5
            starveT += dt;
            if (starveT >= 2.0f) {
                starveT -= 2.0f;
                P.hp -= 5;
                hurtVin = 0.3f;
                PlaySound(AU.hurt);
                if (P.hp <= 0) KillPlayer();
            }
        } else if (P.hunger > 80 && P.hp < P.maxHp) {     // 饱食缓慢回血
            regenT += dt;
            if (regenT >= 1.5f) { regenT = 0; if (P.hp < P.maxHp) P.hp++; }
        }
        if (W.NearCampfire(P.x, P.y, 56) >= 0 && P.hp < P.maxHp) {   // 篝火旁回血
            healT += dt;
            if (healT >= 1.0f) {
                healT = 0;
                P.hp += 2;
                if (P.hp > P.maxHp) P.hp = P.maxHp;
            }
        }
    }

    // ---- 怪物投射物（酸弹/火球）：直线飞行，命中玩家或超时消失 ----
    for (Spit& s : spits) {
        if (!s.on) continue;
        s.life -= dt;
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        // 尾迹粒子（酸=绿 / 火=橙红）
        if ((rand() % 3) == 0) {
            if (s.fire) SpawnParticles(s.x, s.y, 1, 255, 140, 50, 14.0f);
            else        SpawnParticles(s.x, s.y, 1, 150, 220, 80, 14.0f);
        }
        float dxp = s.x - P.x, dyp = s.y - (P.y - 8);
        bool hitPlayer = !P.dead && P.invuln <= 0 &&
                         dxp * dxp + dyp * dyp < 49.0f;
        if (hitPlayer) {
            float l = sqrtf(dxp * dxp + dyp * dyp);
            if (l < 0.001f) l = 1;
            MobHitPlayer(s.fire ? 12.0f : 7.0f, -dxp / l, -dyp / l);   // 击退方向朝远离施放者
        }
        if (s.life <= 0 || hitPlayer) {
            if (s.fire) SpawnParticles(s.x, s.y, 8, 255, 130, 40, 80);
            else        SpawnParticles(s.x, s.y, 6, 150, 220, 80, 70);
            s.on = false;
        }
    }

    // ---- 玩家箭矢（猎弓）：直线飞行，命中生物/撞墙/超时消失 ----
    for (Arrow& a : arrows) {
        if (!a.on) continue;
        a.life -= dt;
        a.x += a.vx * dt;
        a.y += a.vy * dt;
        bool gone = a.life <= 0;
        // 撞墙（水=墙的地牢语义同样适用）
        int atx = (int)(a.x / TILE), aty = (int)(a.y / TILE);
        if (!gone && W.TileAt(atx, aty) == Tile::Water) {
            gone = true;
            SpawnParticles(a.x, a.y, 3, 150, 130, 100, 50);
        }
        // 命中生物
        if (!gone && !P.dead) {
            for (auto& c : mobs) {
                if (c.state == AState::Dead) continue;
                if (MobUndying(c.kind)) continue;              // 不死单位：箭矢穿身而过
                float dx = c.x - a.x, dy = (c.y - 8) - a.y;
                if (dx * dx + dy * dy > 100.0f) continue;      // 半径 10
                int dmg = PlayerDmg() + 1;                     // 箭矢伤害 +1
                bool isGhost = MobHostile(c.kind);
                if (isGhost) c.triggered = true;                // 惊动它
                c.hp -= isGhost ? 0 : dmg;
                c.hurtFlash = 0.1f;
                float l = sqrtf(a.vx * a.vx + a.vy * a.vy);
                if (l > 0.001f) {                              // 沿箭矢方向击退
                    c.kvx += a.vx / l * 170.0f;
                    c.kvy += a.vy / l * 170.0f;
                }
                c.state = AState::Hurt;
                c.timer = 0.24f;
                PlaySound(AU.hit);
                SetSoundPitch(AU.hit, 0.85f + (rand() % 30) / 100.0f);
                SpawnParticles(c.x, c.y - 10, 8, isGhost ? 90 : 220,
                               isGhost ? 235 : 70, isGhost ? 190 : 70, 95);
                FloatText(c.x, c.y - 22, isGhost ? "凡器难伤" : TextFormat("%d", dmg),
                          isGhost ? 176 : 255, isGhost ? 32 : 255, isGhost ? 32 : 255);
                if (c.hp <= 0 && !MobPreDeath(c)) {
                    if (!isGhost) gBowKill = true;          // 成就：一箭封喉（猎弓放倒活物）
                    c.state = AState::Dead;
                    c.deadFade = 0;
                    MobDied(c.kind, c.x, c.y, c.tier, c.dmgMul);
                    hitStop = 0.15f;
                    shakeT = 0.2f; shakeDur = 0.2f;
                    SpawnParticles(c.x, c.y - 10, 14, 255, 240, 200, 130);
                }
                gone = true;
                break;
            }
        }
        if (gone) a.on = false;
    }

    // ---- 中毒 DoT（穴蛛毒咬）：每 0.8 秒扣血 ----
    if (P.poisonT > 0 && !P.dead) {
        P.poisonT -= dt;
        P.poisonTick += dt;
        if (P.poisonTick >= 0.8f) {
            P.poisonTick -= 0.8f;
            P.hp -= (int)(P.poisonDps + 0.5f);
            SpawnParticles(P.x, P.y - 12, 3, 130, 220, 90, 40);
            if (P.hp <= 0) KillPlayer();
        }
        if (P.poisonT <= 0) P.poisonTick = 0;
    }

    // ---- 濒死心跳：血量 < 25% 时低频心跳（氛围 + 危机预警），节奏随血量加快 ----
    if (!P.dead && P.hp < P.maxHp / 4) {
        static float beatT = 0.0f;
        float interval = 0.6f + 0.5f * P.hp / (P.maxHp / 4.0f);   // 越残跳越快
        beatT += dt;
        if (beatT >= interval) {
            beatT = 0.0f;
            PlaySound(AU.hurt);
            SetSoundPitch(AU.hurt, 0.55f);              // 低沉的心跳闷响
            hurtVin = fmaxf(hurtVin, 0.16f);            // 每一跳伴随轻微红边搏动
        }
    }

    // ---- 遗迹探索发现（靠近 120px 触发）----
    for (size_t i = 0; i < W.ruins.size() && i < 4; i++) {
        if (ruinFound[i]) continue;
        float dx = W.ruins[i].x - P.x, dy = W.ruins[i].y - P.y;
        if (dx * dx + dy * dy < 120.0f * 120.0f) {
            ruinFound[i] = true;
            AddXp(15);
            PlaySound(AU.place);
            FloatText(P.x, P.y - 44, "发现村庄遗迹!", 255, 235, 140);
            SpawnParticles(P.x, P.y - 20, 14, 255, 230, 130, 110);
        }
    }

    // 指引箭头摆动
    arrowPh += dt;

    // ---- 夜间无照明游荡计时（成就：月下漫步）----
    if (IsNight(gameTime) && W.NearCampfire(P.x, P.y, 110.0f) < 0)
        P.nightOut += dt;

    // 成就条件轮询（旧表已并入新系统）
    CheckAchvs();
    // ---- 元系统推进：命途 / 成就 / 图鉴 / 残影剧情 ----
    ProgTick(dt);
}

// ---------------- 元系统：每帧推进 + 事件表现 ----------------

// 把 main 的散落状态打包成一张快照喂给 progress 模块（模块本身不认识任何游戏类型）
static void ProgTick(float dt) {
    ProgStat st;
    st.level = P.level;
    st.kills = P.kills;
    st.ghostCaught = P.ghostCaught;
    st.day = (int)(gameTime / DAY_LEN) + 1;
    st.nightOut = P.nightOut;
    st.nTree = P.nTree; st.nRock = P.nRock; st.nBerry = P.nBerry;
    st.nEat = P.nEat;   st.nFire = P.nFire; st.nChest = P.nChest;
    st.nGrave = gGraveDug;
    st.wood = P.wood;   st.stone = P.stone; st.iron = P.iron;
    st.crystal = P.crystal; st.shard = P.gemShard; st.heart = P.heart;
    st.campLv = campBuilt ? campLv : 0;
    int settled = 0;
    for (const Npc& n : npcs) if (n.on && n.kind == 1 && (n.state == 1 || n.state == 2)) settled++;
    st.allyN = settled + (int)ghosts.size() + (PET.on ? 1 : 0);
    st.facelessN = gCompFace;
    st.banished = gBanished;
    int tn = 0;
    for (int i = 0; i < 6; i++) if (P.toolLv[i] > 0) tn++;
    st.toolN = tn;
    st.armorLv = P.armorLv; st.captureLv = P.captureLv;
    st.swordLv = P.swordLv; st.gourdLv = P.gourdLv;
    st.smithLv = gSmithGhostLv;
    int rf = 0;
    for (int i = 0; i < 4; i++) if (ruinFound[i]) rf++;
    st.ruinFound = rf;
    st.puppetMax = gPuppetMax;
    st.bossSlain = bossSlain;
    st.domainPurged = domainPurged;
    st.smithOwned = smithOwned;
    st.boat = P.boat;
    st.nearDeath = P.nearDeath;
    st.stalkSurvived = gStalkSurvived;
    st.bowKill = gBowKill;
    st.hp = P.hp; st.maxHp = P.maxHp;
    st.dead = P.dead;
    Prog::Tick(st, dt);
}

// 把 progress 攒下的事件翻译成音效 / 飘字 / 卷轴
static void ConsumeProgEvents() {
    ProgEvent ev;
    while (Prog::Pop(&ev)) {
        switch (ev.t) {
        case PEV_ACHV:                                   // 复用已有的白幡横幅
            achvShowIdx = ev.a; achvShowT = 3.2f;
            PlaySound(AU.craft);
            break;
        case PEV_RECIPE:
            PlaySound(AU.pickup);
            FloatText(P.x, P.y - 60, TextFormat(L10N("悟得 %s"), Prog::RecipeName(ev.a)), 150, 235, 190);
            break;
        case PEV_SHARD:
            PlaySound(AU.place);
            FloatText(P.x, P.y - 70, TextFormat(L10N("拾得残影 %d/12"), Prog::ShardCount()), 190, 170, 255);
            SpawnParticles(P.x, P.y - 18, 14, 190, 170, 255, 140);
            break;
        case PEV_CHAPTER:                                // 章节：弹卷轴（世界冻结）
            OpenScroll(0, ev.a);
            PlaySound(AU.roar);
            break;
        case PEV_LEVELUP:
            insightFlashT = 1.6f;
            FloatText(P.x, P.y - 82, "悟 +1（按 L 翻手册）", 255, 235, 130);
            break;
        case PEV_INSIGHT:
            insightFlashT = 1.6f;
            FloatText(P.x, P.y - 66, TextFormat(L10N("悟 +%d"), ev.a), 255, 224, 138);
            break;
        case PEV_ENDING:                                 // 结局：卷轴 + 回标题
            OpenScroll(1, ev.a);
            PlaySound(AU.roar);
            break;
        default: break;
        }
    }
}

// ---------------- 绘制：单个实体 ----------------

static void DrawPlayer() {
    int d = (P.dir == 3) ? 2 : P.dir;
    Texture2D tex;
    if (P.dead) tex = A.player[d][0];
    else if (P.hurtFlash > 0) tex = A.playerWhite[d];
    else if (P.atkTime > 0) {
        int ph = P.atkTime > 0.13f ? 0 : (P.atkTime > 0.06f ? 1 : 2);
        tex = A.player[d][6 + ph];
    } else if (P.walking) tex = A.player[d][2 + (int)(P.animT * 9) % 4];
    else tex = A.player[d][(int)(P.animT * 2) % 2];

    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    if (P.dir == 3) src.width = -src.width;   // 左方向镜像

    unsigned char alpha = 255;
    if (P.invuln > 0 && !P.dead) alpha = ((int)(P.invuln * 18) % 2) ? 70 : 255;   // 无敌闪烁

    // 泛舟：脚下画小舟（随波轻晃；面向左时船头镜像朝左）
    if (playerSailing) {
        float bob = sinf((float)GetTime() * 2.2f) * 1.2f;
        Rectangle bsrc = { 0, 0, (float)A.boat.width, (float)A.boat.height };
        if (P.dir == 3) bsrc.width = -bsrc.width;
        DrawTexturePro(A.boat, bsrc, { P.x - 15, P.y - 12 + bob, 30, 14 }, { 0, 0 }, 0, WHITE);
    } else if (!P.dead) {
        DrawEllipse(P.x, P.y - 1, 8, 3, (Color) { 0, 0, 0, 70 });   // 陆地投影
    }
    float rot = P.dead ? 90.0f : 0.0f;
    Vector2 org = P.dead ? (Vector2) { 12, 27 } : (Vector2) { 0, 0 };
    DrawTexturePro(tex, src, { P.x - 12, P.y - 28, 24, 28 }, org, rot, { 255, 255, 255, alpha });
}

// 按种类取怪物帧纹理组（DrawMob / DrawGhostAlly / HUD 共用）
// 十大规则鬼复用既有纹理 + 各自专属 tint（DrawMob 内）
static const Texture2D* MobFrames(CreatureKind k) {
    switch (k) {
    case CreatureKind::Rabbit:      return A.rabbit;
    case CreatureKind::Deer:        return A.deer;
    case CreatureKind::GhostChild:  return A.spitter;    // 鬼童：小体
    case CreatureKind::GhostTeen:   return A.runner;     // 少年：中体
    case CreatureKind::GhostAdult:  return A.brute;      // 成年：大坦克
    case CreatureKind::LoneGhost:   return A.wraith;     // 单鬼：幽白穿墙
    case CreatureKind::NineFace:    return A.warlock;    // 9面鬼：法系
    case CreatureKind::MimicAll:    return A.zombie;     // 所有人：人形
    case CreatureKind::Faceless:    return A.gargoyle;   // 无脸鬼：潜伏石像
    case CreatureKind::GameGhost:   return A.skeleton;   // 游戏鬼：人形
    case CreatureKind::DealGhost:   return A.golem;      // 交易鬼：宝相庄严
    case CreatureKind::KnockGhost:  return A.bat;        // 敲门鬼：飞行
    case CreatureKind::ManyFaces:   return A.spider;     // 多面人：多足多面
    case CreatureKind::GhostDomain: return A.boss;       // 鬼域核心：巨型
    case CreatureKind::SmithGhost:  return A.golem;      // 铁匠鬼：铁塔之躯
    default:                        return A.zombie;
    }
}
static Texture2D MobWhiteTex(CreatureKind k) {
    switch (k) {
    case CreatureKind::Rabbit:      return A.rabbitWhite;
    case CreatureKind::Deer:        return A.deerWhite;
    case CreatureKind::GhostChild:  return A.spitterWhite;
    case CreatureKind::GhostTeen:   return A.runnerWhite;
    case CreatureKind::GhostAdult:  return A.bruteWhite;
    case CreatureKind::LoneGhost:   return A.wraithWhite;
    case CreatureKind::NineFace:    return A.warlockWhite;
    case CreatureKind::Faceless:    return A.gargoyleWhite;
    case CreatureKind::GameGhost:   return A.skeletonWhite;
    case CreatureKind::DealGhost:   return A.golemWhite;
    case CreatureKind::KnockGhost:  return A.batWhite;
    case CreatureKind::ManyFaces:   return A.spiderWhite;
    case CreatureKind::GhostDomain: return A.bossWhite;
    case CreatureKind::SmithGhost:  return A.golemWhite;
    default:                        return A.zombieWhite;
    }
}

static void DrawMob(const Creature& c) {
    const Texture2D* frames = MobFrames(c.kind);
    Texture2D white = MobWhiteTex(c.kind);
    bool moving = (c.state == AState::Wander || c.state == AState::Flee ||
                   c.state == AState::Chase || c.state == AState::Attack);
    int fi = moving ? 2 + (int)(c.animT * 8) % 2 : (int)(c.animT * 2) % 2;
    Texture2D tex = (c.hurtFlash > 0) ? white : frames[fi];

    float w = (float)tex.width, h = (float)tex.height;
    Rectangle src = { 0, 0, w, h };
    if (c.dir == 3) src.width = -w;

    float yo = 0;
    if (c.kind == CreatureKind::Rabbit && moving)
        yo = -fabsf(sinf(c.animT * 17.0f)) * (c.state == AState::Flee ? 5.0f : 3.0f);   // 蹦跳
    if (c.kind == CreatureKind::KnockGhost)
        yo = -sinf(c.animT * 11.0f) * 2.5f;                      // 振翅浮动
    if (c.kind == CreatureKind::LoneGhost)
        yo = -sinf(c.animT * 3.2f) * 2.0f - 3.0f;                // 幽体悬浮

    // ---- 各鬼专属色调 ----
    Color tint = WHITE;
    switch (c.kind) {
    case CreatureKind::GhostChild:  tint = { 190, 235, 205, 255 }; break;   // 鬼童：稚青
    case CreatureKind::GhostTeen:   tint = { 170, 220, 190, 255 }; break;   // 少年：青灰
    case CreatureKind::GhostAdult:  tint = { 150, 210, 180, 255 }; break;   // 成年：深青
    case CreatureKind::LoneGhost:   tint = { 225, 230, 245, 255 }; break;   // 单鬼：幽白
    case CreatureKind::NineFace:
        // 9面鬼：层 0..3 渐深（红光覆盖 -> 空间 -> 现实 -> 回溯）
        switch (c.phase) {
        case 1:  tint = { 200, 170, 255, 255 }; break;
        case 2:  tint = { 220, 150, 235, 255 }; break;
        case 3:  tint = { 245, 120, 220, 255 }; break;
        default: tint = { 180, 190, 255, 255 }; break;
        }
        break;
    case CreatureKind::MimicAll:    tint = { 235, 220, 200, 255 }; break;   // 所有人：人肤色（伪装）
    case CreatureKind::Faceless:    tint = { 190, 185, 175, 255 }; break;   // 捂脸鬼：灰白人形
    case CreatureKind::GameGhost:   tint = { 220, 220, 200, 255 }; break;   // 游戏鬼：骨白
    case CreatureKind::DealGhost:   tint = { 220, 200, 150, 255 }; break;   // 交易鬼：鎏金
    case CreatureKind::KnockGhost:  tint = { 170, 160, 210, 255 }; break;   // 敲门鬼：暗紫
    case CreatureKind::ManyFaces:   tint = { 200, 180, 220, 255 }; break;   // 多面人：面紫
    case CreatureKind::GhostDomain: tint = { 235, 120, 110, 255 }; break;   // 鬼域核心：域红
    case CreatureKind::SmithGhost:  tint = { 235, 210, 160, 255 }; break;   // 铁匠鬼：铁金
    default: break;
    }
    // 前摇红闪警示（所有近战鬼通用）
    if (c.state == AState::Windup)
        tint = { 255, 150, 150, 255 };
    // 远程蓄力：鬼童青绿 / 9面鬼紫红
    if ((c.kind == CreatureKind::GhostChild) && c.state == AState::Windup)
        tint = { 190, 255, 150, 255 };
    if (c.kind == CreatureKind::NineFace && c.state == AState::Windup)
        tint = { 230, 160, 255, 255 };

    unsigned char alpha = 255;
    if (c.kind == CreatureKind::LoneGhost && c.state != AState::Dead)
        alpha = 160;                                              // 幽体半透明

    // ---- 无破绽伪装：「所有人」与「无脸鬼」蛰伏时与幸存者完全同款（人形 + 职业色）----
    const bool disguised = ((c.kind == CreatureKind::MimicAll || c.kind == CreatureKind::Faceless) &&
                            !c.triggered && c.state != AState::Dead);
    if (disguised) {
        int d2 = (c.dir == 3) ? 2 : c.dir;
        tex = A.player[d2][(int)(c.animT * 2) % 2];
        w = (float)tex.width; h = (float)tex.height;
        src = { 0, 0, w, h };
        if (c.dir == 3) src.width = -w;
        tint = JOB_TINT[c.tintSeed % 5];
    }
    // 无脸鬼（真身）：人形 + 一片空白无特征的脸（无眼无鼻无嘴）
    // 混入队伍的：领地 Lv1 还留着"脸是空白的"这一丝破绽；领地 Lv2 起彻底无破绽（与幸存者同款）
    const bool faceCover = (c.kind == CreatureKind::Faceless && c.state != AState::Dead &&
                            !(c.infiltrated && campLv >= 2));
    if (faceCover) {
        int d2 = (c.dir == 3) ? 2 : c.dir;
        if (c.hurtFlash <= 0) tex = A.player[d2][(int)(c.animT * 2) % 2];
        w = (float)tex.width; h = (float)tex.height;
        src = { 0, 0, w, h };
        if (c.dir == 3) src.width = -w;
    }

    // ---- 蛰伏态（规则未触发）灰暗半透明（「所有人」除外：它不能露破绽）----
    const bool dormant = MobHostile(c.kind) && !c.triggered && c.state != AState::Dead && !disguised;
    if (dormant) {
        tint = { 112, 112, 126, 255 };
        alpha = 140;
    }
    float rot = 0;
    Vector2 org = { 0, 0 };
    float mscale = (c.kind == CreatureKind::GhostDomain) ? 1.7f : 1.0f;   // 鬼域核心渲染放大
    if (c.state == AState::Dead) {       // 尸体倒下 + 3 秒淡出
        rot = 90.0f;
        org = { w * 0.5f, h };
        alpha = (unsigned char)(255.0f * (1.0f - c.deadFade / 3.0f));
        if (alpha < 10) return;
    } else if (!disguised) {
        DrawEllipse(c.x, c.y - 1, w * mscale * 0.38f, 2.6f, (Color) { 0, 0, 0, 60 });
    } else {
        DrawEllipse(c.x, c.y - 1, 8, 3, (Color) { 0, 0, 0, 60 });   // 与幸存者同款投影
    }
    DrawTexturePro(tex, src, { c.x - w * mscale / 2, c.y - h * mscale + yo,
                               w * mscale, h * mscale }, org, rot,
                   { tint.r, tint.g, tint.b, alpha });

    // 无脸鬼：脸是一片空白（无眼无鼻无嘴的肉色平面）；前摇时血脸浮现（它要盖到你脸上）
    if (faceCover) {
        float bx0 = c.x - w / 2.0f, by0 = c.y - h + yo;
        // 空白脸：一片平涂的肉色，无任何五官
        DrawRectangle((int)(bx0 + w * 0.18f), (int)(by0 + h * 0.20f), (int)(w * 0.64f), (int)(h * 0.30f),
                      (Color) { 210, 188, 175, 245 });
        DrawRectangle((int)(bx0 + w * 0.18f), (int)(by0 + h * 0.20f), (int)(w * 0.64f), 1,
                      (Color) { 90, 80, 70, 200 });
        // 前摇中：染血人脸浮现（它把那张脸慢慢举起来准备覆盖）
        if (c.state == AState::Windup) {
            float k = 1.0f - c.timer / 0.35f;     // 血脸渐显
            if (k > 1.0f) k = 1.0f;
            if (k < 0) k = 0;
            // 两滴血泪眼 + 嘴角血线
            DrawRectangle((int)(bx0 + w * 0.26f), (int)(by0 + h * 0.26f), 2, 2 + (int)(3 * k),
                          (Color) { 130, 20, 20, (unsigned char)(200 * k + 55) });
            DrawRectangle((int)(bx0 + w * 0.58f), (int)(by0 + h * 0.26f), 2, 2 + (int)(3 * k),
                          (Color) { 130, 20, 20, (unsigned char)(200 * k + 55) });
            DrawRectangle((int)(bx0 + w * 0.30f), (int)(by0 + h * 0.44f), (int)(w * 0.40f), 1,
                          (Color) { 130, 20, 20, (unsigned char)(200 * k + 55) });
            // 脸缘血渍（边缘滴落的血）
            if (k > 0.5f) {
                DrawPixel((int)(bx0 + w * 0.22f), (int)(by0 + h * 0.34f),
                          (Color) { 120, 15, 15, (unsigned char)(180 * k) });
                DrawPixel((int)(bx0 + w * 0.78f), (int)(by0 + h * 0.38f),
                          (Color) { 120, 15, 15, (unsigned char)(180 * k) });
            }
        }
    }

    // 随机外观：按 tintSeed 在鬼身上叠 2~3 个随机色块（同类鬼也各不相同）
    if (c.state != AState::Dead && !disguised && !faceCover) {
        float bw = w * mscale, bh = h * mscale;
        float bx0 = c.x - bw / 2.0f, by0 = c.y - bh + yo;
        for (int k = 0; k < 7; k++) {
            unsigned char sd = (unsigned char)(c.tintSeed + k * 97);
            if ((sd & 7) == 0) continue;
            int bwid = (int)bw > 1 ? (int)bw : 1;
            int bhei = (int)(bh * 0.7f) > 1 ? (int)(bh * 0.7f) : 1;
            int px = (int)(bx0 + (float)((sd >> 2) % bwid));
            int py = (int)(by0 + (float)((sd >> 3) % bhei));
            int bs = 1 + ((sd >> 5) & 1);
            DrawRectangle(px, py, bs, bs,
                          { (unsigned char)(80 + (sd & 0x3F)),
                            (unsigned char)(55 + ((sd >> 1) & 0x3F)),
                            (unsigned char)(75 + ((sd >> 2) & 0x3F)), 165 });
        }
    }

    // 被玩家标记：仅朱砂三角符（跟踪用；规律绝不显示 —— 要自己探索）
    if (c.marked && c.state != AState::Dead) {
        float nt = (float)GetTime();
        float pulse = 0.5f + 0.5f * sinf(nt * 3.2f);
        int my = (int)(c.y - h * mscale - 3);
        for (int k = -1; k <= 1; k++)
            DrawPixel((int)c.x + k, my, { 226, 40, 46, (unsigned char)(200 + 55 * pulse) });
        DrawPixel((int)c.x, my - 1, { 226, 40, 46, (unsigned char)(200 + 55 * pulse) });
    }

    // 记忆错乱中：头顶紫色问号打转（忘了要杀谁）—— 无脸鬼鬼仆的篡改特技可视化
    if (c.confuseT > 0.0f && c.state != AState::Dead) {
        float nt = (float)GetTime();
        float bob = sinf(nt * 6.0f) * 2.0f;
        float spin = sinf(nt * 9.0f) * 2.0f;
        int qy = (int)(c.y - h * mscale - 12 + bob);
        unsigned char qa = (unsigned char)(170 + 80 * (0.5f + 0.5f * sinf(nt * 7.0f)));
        ZhText("?", (int)(c.x - 2 + spin), qy, 10, (Color) { 190, 130, 255, qa });
        // 环绕的紫色迷尘（记忆被搅乱的粒子表现）
        for (int k = 0; k < 3; k++) {
            float a = nt * 4.0f + (float)k * 2.094f;
            DrawPixel((int)(c.x + cosf(a) * 9.0f), (int)(c.y - h * mscale * 0.6f + sinf(a) * 5.0f),
                      (Color) { 190, 130, 255, (unsigned char)(120 + 100 * sinf(nt * 5.0f + k)) });
        }
    }

    // 黄符降级中：头顶黄签"↓1"（贴在脑门上的符纸）
    if (c.lowerT > 0.0f && c.state != AState::Dead) {
        float nt = (float)GetTime();
        float sway = sinf(nt * 5.0f) * 1.0f;
        int ty = (int)(c.y - h * mscale - 14);
        ZhText("↓1", (int)(c.x - 5 + sway), ty, 10, (Color) { 255, 214, 90, 230 });
    }

    // 仇符互斗中：头顶赤色"怒"字
    if (c.fightT > 0.0f && c.state != AState::Dead) {
        float nt = (float)GetTime();
        float bob = sinf(nt * 8.0f) * 1.5f;
        unsigned char ra = (unsigned char)(190 + 65 * (0.5f + 0.5f * sinf(nt * 9.0f)));
        ZhText("怒", (int)(c.x - 5), (int)(c.y - h * mscale - 22 + bob), 10, (Color) { 255, 90, 70, ra });
    }
}

// 鬼仆渲染：复用怪物贴图 + 青绿鬼火色调半透明 + 底部魂火 + 头顶阴气条
static void DrawGhostAlly(const GhostAlly& g) {
    const Texture2D* frames = MobFrames(g.kind);
    int fi = g.moving ? 2 + (int)(g.animT * 8) % 2 : (int)(g.animT * 2) % 2;
    Texture2D tex = frames[fi];
    float w = (float)tex.width, h = (float)tex.height;
    Rectangle src = { 0, 0, w, h };
    if (g.dir == 3) src.width = -w;
    // 悬浮起伏 + 攻击前冲位移
    static const float FACE[4][2] = { {0,1},{0,-1},{1,0},{-1,0} };
    float yo = -sinf(g.animT * 3.0f) * 1.6f - 1.0f;
    float lx = 0, ly = 0;
    if (g.atkAnim > 0) {
        lx = FACE[g.dir][0] * g.atkAnim * 20.0f;
        ly = FACE[g.dir][1] * g.atkAnim * 14.0f;
    }
    // 底部魂火光点（青绿脉冲）
    float pulse = 0.5f + 0.5f * sinf(g.animT * 6.0f);
    DrawPixCircle(g.x, g.y - 2, 2.2f + pulse * 1.2f, (Color) { 77, 255, 184, 110 });
    // 本体：青绿鬼火色调（受击白闪）
    Color tint = g.hurtFlash > 0 ? (Color) { 255, 255, 255, 255 }
                                 : (Color) { 150, 255, 215, 205 };
    DrawTexturePro(tex, src, { g.x - w / 2 + lx, g.y - h + yo + ly, w, h },
                   { 0, 0 }, 0, tint);
    // 头顶阴气条（剩余存在时间；将尽转暖色警示）
    float lf = g.life / g.maxLife;
    if (lf < 0) lf = 0;
    DrawRectangle((int)(g.x - 7), (int)(g.y - h - 6), 14, 2, (Color) { 8, 10, 16, 180 });
    DrawRectangle((int)(g.x - 7), (int)(g.y - h - 6), (int)(14 * lf + 0.5f), 2,
                  g.life < 20.0f ? (Color) { 255, 150, 120, 230 }
                                 : (Color) { 120, 255, 200, 230 });
}

// NPC 渲染：人形 + 职业着色（与「所有人」的伪装完全同款 —— 真假只能玩家自己分辨）
static void DrawNpc(const Npc& n) {
    int d = (n.dir == 3) ? 2 : n.dir;
    Texture2D tex = A.player[d][(int)(n.animT * 2) % 2];
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    if (n.dir == 3) src.width = -src.width;
    // 铁匠就是铁匠鬼：鬼形态呈现（半透明青白 + 悬浮 + 底部鬼火），锤子标记保留供识别
    bool isSmith = (n.kind == 0);
    float bob = isSmith ? sinf((float)GetTime() * 2.2f + n.x) * 1.5f : 0.0f;
    Color tint = isSmith ? (Color) { 185, 235, 222, 178 }          // 鬼体：青白半透明
                         : JOB_TINT[n.job];                        // 幸存者：职业色
    DrawEllipse(n.x, n.y - 1, 8, 3, isSmith ? (Color) { 40, 90, 80, 70 }
                                            : (Color) { 0, 0, 0, 60 });
    DrawTexturePro(tex, src, { n.x - 12, n.y - 28 + bob, 24, 28 }, { 0, 0 }, 0, tint);
    if (isSmith) {
        // 底部鬼火（两粒青焰，随悬浮呼吸）
        float gl = 0.5f + 0.5f * sinf((float)GetTime() * 3.1f + n.y);
        DrawPixCircle(n.x - 4, n.y - 2 + bob, 1.4f + gl, (Color) { 120, 240, 210, (unsigned char)(120 + 90 * gl) });
        DrawPixCircle(n.x + 5, n.y - 3 + bob, 1.2f + gl * 0.7f, (Color) { 90, 220, 200, (unsigned char)(100 + 80 * gl) });
        // 头顶锤子标记（金点 + 白点）
        DrawPixel((int)n.x - 1, (int)(n.y - 33 + bob), (Color) { 255, 224, 138, 255 });
        DrawPixel((int)n.x, (int)(n.y - 33 + bob), (Color) { 255, 224, 138, 255 });
        DrawPixel((int)n.x - 1, (int)(n.y - 32 + bob), (Color) { 240, 236, 224, 255 });
        DrawPixel((int)n.x, (int)(n.y - 32 + bob), (Color) { 240, 236, 224, 255 });
    }
    // 幸存者不显示任何头顶文字（名字/职业一律不给 —— 真假自己分辨）
}

// ---- 屋顶层（画在全部实体之上）----
// 从屋外看：整片立体坡屋顶盖住屋内（里面有什么完全看不见）；走进屋内/门口：该屋顶渐隐露出内部。
// 每座屋顶带独立渐隐动画（House.roofA），出屋即恢复。
static void DrawHouseRoofs() {
    const float dt = GetFrameTime();
    for (auto& h : W.houses) {
        int hw = h.w, hh = h.h;
        // 屋体像素矩形（瓦片范围）
        int px0 = (h.tx - hw) * 16, py0 = (h.ty - hh) * 16;
        int pw = (hw * 2 + 1) * 16, ph = (hh * 2 + 1) * 16;
        // 视口剔除（含出檐余量）
        if (px0 + pw < camX - 8 || px0 > camX + VW + 8 || py0 + ph < camY - 24 || py0 > camY + VH + 8) continue;
        // 玩家是否在屋内 / 门口缓冲区（进屋 → 屋顶目标 alpha 0）
        float dxp = P.x - (px0 + pw * 0.5f), dyp = P.y - (py0 + ph);
        bool inside = (P.x > px0 - 6 && P.x < px0 + pw + 6 && P.y > py0 - 6 && P.y < py0 + ph + 6)
                      || (dxp * dxp + dyp * dyp < 30.0f * 30.0f);
        // 靠近屋内的人（铁匠/幸存者 90px 内）→ 屋顶也掀开：不然根本看不见要跟谁交谈
        bool npcNear = false;
        for (const Npc& n : npcs) {
            if (!n.on) continue;
            if (n.x < px0 - 4 || n.x > px0 + pw + 4 || n.y < py0 - 4 || n.y > py0 + ph + 4) continue;
            float ddx = n.x - P.x, ddy = n.y - P.y;
            if (ddx * ddx + ddy * ddy < 90.0f * 90.0f) { npcNear = true; break; }
        }
        bool hasSmith = npcNear;   // 屋顶锤标：这屋里住着铁匠
        float target = (inside || npcNear) ? 0.0f : 255.0f;
        h.roofA += (target - h.roofA) * (inside ? 10.0f : 5.0f) * dt;    // 进屋快 / 出屋稍缓
        if (h.roofA < 1.0f) { h.roofA = 0.0f; continue; }               // 全透：跳过绘制
        if (h.roofA > 254.0f) h.roofA = 255.0f;
        unsigned char A = (unsigned char)(h.roofA + 0.5f);

        // 屋顶几何：北沿抬高 8px 表现坡度，四周出檐 2px。
        // （原来抬高 12 + 南向 15 太夸张，整座房子几乎只剩屋顶 → 这里压扁，
        //   并且只盖住墙体高度的 3/4，把真实南墙（含门）露出来）
        int rx = px0 - 2, ry = py0 - 8, rw = pw + 4, rh = ph * 3 / 4 + 9;
        Color base    = { 44, 52, 72, A };     // 主坡面
        Color ridge   = { 86, 96, 126, A };    // 脊线
        Color ridgeHi = { 110, 120, 150, A };
        Color seam    = { 26, 30, 44, A };     // 瓦楞
        Color eave    = { 30, 35, 50, A };     // 檐口
        Color eaveHi  = { 62, 72, 96, A };     // 檐口受光
        Color gable   = { 34, 40, 56, A };     // 山墙侧沿
        // 南向落地投影（画在屋顶矩形外扩一圈，制造"屋顶悬在屋体上"的厚度）
        DrawRectangle(rx + 4, ry + 8, rw, rh, (Color) { 0, 0, 0, (unsigned char)(46 * h.roofA / 255) });
        // 主坡面
        DrawRectangle(rx, ry, rw, rh, base);
        // 瓦楞横线（每 7px）
        for (int yy = ry + 6; yy < ry + rh - 2; yy += 7)
            DrawRectangle(rx + 2, yy, rw - 4, 1, seam);
        // 脊线（横向高光带，上部 1px 更亮）
        int ryy = ry + (int)(rh * 0.42f);
        DrawRectangle(rx, ryy, rw, 4, ridge);
        DrawRectangle(rx, ryy, rw, 1, ridgeHi);
        // 北檐口（厚 4px）+ 受光
        DrawRectangle(rx - 1, ry - 1, rw + 2, 4, eave);
        DrawRectangle(rx - 1, ry - 1, rw + 2, 1, eaveHi);
        // 南檐口：出挑 2px + 檐下阴影（投在南墙上）
        DrawRectangle(rx - 1, ry + rh - 3, rw + 2, 3, eave);
        DrawRectangle(rx - 1, ry + rh, rw + 2, 2, (Color) { 12, 14, 22, (unsigned char)(120 * h.roofA / 255) });
        // 南墙墙裙：屋顶下露出一道木墙（"屋 = 墙 + 顶"，不是一块大屋顶）
        {
            int wy = ry + rh + 2;
            DrawRectangle(rx + 1, wy, rw - 2, 7, (Color) { 104, 76, 44, A });
            DrawRectangle(rx + 1, wy, rw - 2, 1, (Color) { 146, 110, 66, A });
            DrawRectangle(rx + 1, wy + 6, rw - 2, 1, (Color) { 46, 32, 20, (unsigned char)(A * 3 / 4) });
            for (int xx = rx + 4; xx < rx + rw - 3; xx += 7)
                DrawRectangle(xx, wy + 1, 1, 5, (Color) { 74, 52, 30, (unsigned char)(A * 3 / 4) });
        }
        // 左右山墙侧沿（屋顶厚度）
        DrawRectangle(rx - 2, ry, 2, rh, gable);
        DrawRectangle(rx + rw, ry, 2, rh, gable);
        // 烟囱（铁匠屋必有；普通屋右上角）：砖身 + 烟孔 + 炊烟在上方另画
        if (h.kind == 1 || ((h.tx * 7 + h.ty * 13) % 3 == 0)) {
            int cx2 = rx + rw - 14, cy2 = ry + 8;
            DrawRectangle(cx2, cy2, 8, 12, (Color) { 96, 62, 50, A });
            DrawRectangle(cx2, cy2, 8, 2, (Color) { 128, 86, 66, A });
            DrawRectangle(cx2 + 1, cy2 + 12, 6, 2, (Color) { 52, 34, 30, A });
        }
        // 炊烟（白天缓缓升起的小烟柱，屋更有生活气）
        if (h.roofA > 200.0f && !IsNight(gameTime)) {
            float nowT = (float)GetTime();
            for (int k = 0; k < 3; k++) {
                float ph2 = fmodf(nowT * 0.35f + (float)k * 0.33f, 1.0f);
                int sx2 = rx + rw - 11 + (int)(sinf(nowT * 1.7f + (float)k) * 3.0f);
                int sy2 = ry + 4 - (int)(ph2 * 22.0f);
                unsigned char sa = (unsigned char)((1.0f - ph2) * 70.0f);
                if (sa > 4) DrawPixCircle((float)sx2, (float)sy2, 1.5f + ph2 * 2.0f,
                                          (Color) { 200, 205, 210, sa });
            }
        }
        // 屋顶铁匠锤标（屋顶盖着时也知道这屋能打铁）：金锤头 + 白柄
        if (hasSmith && h.roofA > 60.0f) {
            float nt = (float)GetTime();
            float bob = sinf(nt * 2.6f) * 1.5f;
            int mx = rx + rw / 2, my = ry + rh / 2 - 6 + (int)bob;
            unsigned char ma = (unsigned char)(200 + 55 * (0.5f + 0.5f * sinf(nt * 3.0f)));
            DrawRectangle(mx - 3, my - 2, 7, 5, (Color) { 255, 224, 138, ma });
            DrawRectangle(mx - 1, my + 2, 2, 6, (Color) { 240, 236, 224, ma });
        }
    }
}

// 宠物渲染：复用兔子/小鹿贴图，蹦跳相位 + 受击白闪 + 头顶小心心
static void DrawPet() {
    if (!PET.on) return;
    const Texture2D* frames = PET.kind == 0 ? A.rabbit : A.deer;
    Texture2D tex = (PET.hurtFlash > 0) ? (PET.kind == 0 ? A.rabbitWhite : A.deerWhite)
                                        : frames[(int)(PET.animT * 6) % 2];
    float w = (float)tex.width, h = (float)tex.height;
    Rectangle src = { 0, 0, w, h };
    if (PET.dir == 3) src.width = -w;
    float yo = -fabsf(sinf(PET.animT * 9.0f)) * 2.5f;    // 轻快蹦跳
    DrawEllipse(PET.x, PET.y - 1, w * 0.38f, 2.2f, (Color) { 0, 0, 0, 50 });
    DrawTexturePro(tex, src, { PET.x - w / 2, PET.y - h + yo, w, h },
                   { 0, 0 }, 0, WHITE);
    // 头顶心形标记（宠物身份）
    DrawPixel(PET.x - 2, PET.y - h - 3 + yo, (Color) { 255, 120, 140, 230 });
    DrawPixel(PET.x + 1, PET.y - h - 3 + yo, (Color) { 255, 120, 140, 230 });
    DrawPixel(PET.x - 1, PET.y - h - 2 + yo, (Color) { 255, 120, 140, 230 });
    DrawPixel(PET.x, PET.y - h - 2 + yo, (Color) { 255, 160, 175, 230 });
    if (PET.iv > 0) {                                    // 低血量警示圈
        DrawCircleLines((int)PET.x, (int)PET.y - 4, 9, (Color) { 255, 90, 90, 140 });
    }
}

static void DrawObj(const WorldObj& o, float nowT) {
    float bx = o.tx * 16.0f, by = o.ty * 16.0f;
    float sh = (o.shake > 0) ? sinf(o.shake * 55.0f) * 2.2f : 0;   // 被击晃动
    switch (o.kind) {
    case ObjKind::Tree: {
        // 阴影随砍伐进度淡化（hp 3→2→1），砍倒变树桩后完全不画阴影
        float shA = (o.hp >= 3) ? 1.0f : (o.hp == 2) ? 0.55f : 0.25f;
        float shR = 10.0f * (0.6f + 0.4f * shA);   // 阴影也同步缩小
        DrawEllipse(bx + 8, by + 13, shR, 3, (Color) { 0, 0, 0, (unsigned char)(70 * shA) });
        float sway = sinf(nowT * 1.5f + o.tx * 1.7f) * (1.1f + 1.4f * rainAmt);   // 树冠随风摆动
        DrawTexturePro(A.tree, { 0, 0, 32, 44 }, { bx + 8 - 16 + sh + sway, by + 16 - 44, 32, 44 }, { 0, 0 }, 0, WHITE);
        break;
    }
    case ObjKind::Rock:
        DrawEllipse(bx + 8, by + 14, 9, 2.6f, (Color) { 0, 0, 0, 70 });
        DrawTexturePro(A.rock, { 0, 0, 20, 14 }, { bx + 8 - 10 + sh, by + 16 - 14, 20, 14 }, { 0, 0 }, 0, WHITE);
        break;
    case ObjKind::Berry:
        DrawTexturePro(o.harvested ? A.bushEmpty : A.bushFull,
                       { 0, 0, 16, 12 }, { bx + sh, by + 4, 16, 12 }, { 0, 0 }, 0, WHITE);
        break;
    case ObjKind::Stump:
        DrawTexturePro(A.stump, { 0, 0, 16, 12 }, { bx, by + 4, 16, 12 }, { 0, 0 }, 0, WHITE);
        break;
    case ObjKind::TallGrass:
        DrawTexturePro(A.tallGrass, { 0, 0, 12, 10 }, { bx + 2 + sh * 0.5f, by + 6, 12, 10 }, { 0, 0 }, 0, WHITE);
        break;
    case ObjKind::Flower:
        DrawTexturePro(A.flower[o.tx & 1], { 0, 0, 6, 6 }, { bx + 5, by + 6, 6, 6 }, { 0, 0 }, 0, WHITE);
        break;
    case ObjKind::Campfire:
        DrawTexturePro(A.campfire[(int)(nowT * 8) % 2], { 0, 0, 16, 16 }, { bx, by, 16, 16 }, { 0, 0 }, 0, WHITE);
        break;
    case ObjKind::RuinWall:
        DrawTexturePro(A.ruinWall[o.tx & 1], { 0, 0, 16, 16 }, { bx + sh, by, 16, 16 }, { 0, 0 }, 0, WHITE);
        break;
    case ObjKind::TreePine: {
        float shA = (o.hp >= 3) ? 1.0f : (o.hp == 2) ? 0.55f : 0.25f;
        DrawEllipse(bx + 8, by + 13, 10 * shA + 4, 3, (Color) { 0, 0, 0, (unsigned char)(70 * shA) });
        float sway = sinf(nowT * 1.2f + o.tx * 1.3f) * (0.8f + 1.0f * rainAmt);
        DrawTexturePro(A.treePine, { 0, 0, 24, 54 }, { bx + 8 - 12 + sh + sway, by + 16 - 54, 24, 54 }, { 0, 0 }, 0, WHITE);
        break;
    }
    case ObjKind::TreeBirch: {
        float shA = (o.hp >= 3) ? 1.0f : (o.hp == 2) ? 0.55f : 0.25f;
        DrawEllipse(bx + 8, by + 13, 9 * shA + 4, 2.8f, (Color) { 0, 0, 0, (unsigned char)(65 * shA) });
        float sway = sinf(nowT * 1.7f + o.ty * 1.1f) * (1.0f + 1.2f * rainAmt);
        DrawTexturePro(A.treeBirch, { 0, 0, 26, 40 }, { bx + 8 - 13 + sh + sway, by + 16 - 40, 26, 40 }, { 0, 0 }, 0, WHITE);
        break;
    }
    case ObjKind::TreePalm: {
        float shA = (o.hp >= 3) ? 1.0f : (o.hp == 2) ? 0.55f : 0.25f;
        DrawEllipse(bx + 8, by + 13, 10 * shA + 4, 2.8f, (Color) { 0, 0, 0, (unsigned char)(60 * shA) });
        float sway = sinf(nowT * 2.2f + o.tx * 2.1f) * (1.4f + 1.8f * rainAmt);   // 海风摇摆
        DrawTexturePro(A.treePalm, { 0, 0, 32, 38 }, { bx + 8 - 16 + sh + sway, by + 16 - 38, 32, 38 }, { 0, 0 }, 0, WHITE);
        break;
    }
    case ObjKind::OreRock:
        DrawEllipse(bx + 8, by + 14, 9.5f, 2.6f, (Color) { 0, 0, 0, 70 });
        DrawTexturePro(A.oreRock, { 0, 0, 18, 14 }, { bx + 8 - 9 + sh, by + 16 - 14, 18, 14 }, { 0, 0 }, 0, WHITE);
        {   // 金属闪光
            float gl = 0.5f + 0.5f * sinf(nowT * 3.5f + o.tx * 2.7f);
            if (gl > 0.72f)
                DrawPixCircle(bx + 5 + (o.tx % 5), by + 5, 1.2f,
                              (Color) { 210, 240, 255, (unsigned char)(200 * (gl - 0.72f) / 0.28f) });
        }
        break;
    case ObjKind::Wall: {
        if (o.hp <= 0) break;                          // 已塌（占位已清，惰性标记）
        // 立体墙：北沿墙 = 高出的青瓦屋顶（出檐 + 瓦楞 + 檐下阴影）；其余 = 加高木板前墙
        if (o.regrow > 0.5f) {                         // 屋顶沿（regrow 复用作标记）
            // 瓦面：从瓦片底部拔起 34px 高的青瓦斜面
            DrawRectangle((int)bx, (int)by - 18, 16, 34, (Color) { 38, 44, 62, 255 });
            // 出檐：左右各探出 2px 的檐口（连排时自然接成整条屋檐）
            DrawRectangle((int)bx - 2, (int)by - 18, 20, 4, (Color) { 30, 35, 50, 255 });
            // 屋脊高光 + 瓦楞横线（瓦片质感）
            DrawRectangle((int)bx - 2, (int)by - 18, 20, 2, (Color) { 82, 92, 120, 255 });
            DrawRectangle((int)bx, (int)by - 6, 16, 1, (Color) { 26, 30, 44, 255 });
            DrawRectangle((int)bx, (int)by + 1, 16, 1, (Color) { 26, 30, 44, 255 });
            DrawRectangle((int)bx, (int)by + 8, 16, 1, (Color) { 26, 30, 44, 255 });
            // 檐下阴影投在屋身上（立体感的关键：上暗下亮的层次）
            DrawRectangle((int)bx, (int)by + 11, 16, 5, (Color) { 12, 14, 22, 130 });
        } else {
            // 前墙/侧墙：24px 高木板墙 + 顶梁带 + 侧厚 + 接地暗边
            // 1) 侧厚（右探出 2px 暗木色，让墙有"厚度"而非纸片）
            DrawRectangle((int)(bx + sh) + 14, (int)by - 6, 2, 24, (Color) { 56, 38, 22, 255 });
            DrawRectangle((int)(bx + sh) + 14, (int)by - 6, 1, 24, (Color) { 92, 64, 38, 255 });
            // 2) 墙身本体
            DrawTexturePro(A.wallWood, { 0, 0, 16, 24 },
                           { bx + sh, by - 8.0f, 16, 24 }, { 0, 0 }, 0, WHITE);
            // 3) 顶梁（3px 厚）
            DrawRectangle((int)(bx + sh), (int)by - 8, 16, 3, (Color) { 96, 70, 40, 255 });
            // 4) 顶梁高光
            DrawRectangle((int)(bx + sh), (int)by - 8, 16, 1, (Color) { 140, 105, 65, 200 });
            // 5) 接地暗边（双重：深色厚边 + 半透渐隐）
            DrawRectangle((int)(bx + sh), (int)by + 5, 16, 3, (Color) { 10, 8, 8, 120 });
            DrawRectangle((int)(bx + sh), (int)by + 14, 16, 2, (Color) { 0, 0, 0, 160 });
            // 6) 木纹竖线（3 条分隔，让墙看起来是拼板而非纯色）
            DrawRectangle((int)(bx + sh) + 4, (int)by - 5, 1, 13, (Color) { 36, 24, 14, 80 });
            DrawRectangle((int)(bx + sh) + 8, (int)by - 5, 1, 13, (Color) { 36, 24, 14, 80 });
            DrawRectangle((int)(bx + sh) + 12, (int)by - 5, 1, 13, (Color) { 36, 24, 14, 80 });
            if (o.hp <= 3) {                       // 耐久告急：朱砂符纹闪烁 + 裂纹
                float gl = 0.5f + 0.5f * sinf(nowT * 7.0f);
                DrawRectangle((int)bx + 8, (int)by - 1, 1, 14,
                              (Color) { 226, 40, 46, (unsigned char)(90 + 120 * gl) });
                if (o.hp <= 1)
                    DrawRectangle((int)bx + 2, (int)by + 3, 12, 1, (Color) { 20, 12, 12, 200 });
            }
        }
        break;
    }
    case ObjKind::Bed:
        DrawEllipse(bx + 8, by + 13, 8.0f, 2.2f, (Color) { 0, 0, 0, 60 });
        DrawTexturePro(A.bedStraw, { 0, 0, 16, 14 },
                       { bx + sh, by + 2, 16, 14 }, { 0, 0 }, 0, WHITE);
        break;
    case ObjKind::CampStone: {
        // 营地石碑：素白碑身 + 朱砂"营"字 + 呼吸光圈（领地系统的锚点标志）
        // 接地深椭圆阴影
        DrawEllipse(bx + 8, by + 14, 10.0f, 2.8f, (Color) { 0, 0, 0, 90 });
        // 碑座（深色方基）
        DrawRectangle((int)bx + 2, (int)by + 12, 12, 3, (Color) { 80, 72, 64, 255 });
        DrawRectangle((int)bx + 2, (int)by + 12, 12, 1, (Color) { 110, 100, 90, 255 });
        // 碑身侧厚（让石碑有体积）
        DrawRectangle((int)bx + 13, (int)by - 4, 1, 18, (Color) { 138, 130, 118, 255 });
        // 碑身
        DrawRectangle((int)bx + 3, (int)by - 4, 10, 18, (Color) { 188, 180, 168, 255 });
        // 碑顶圆角高光
        DrawRectangle((int)bx + 3, (int)by - 4, 10, 1, (Color) { 240, 236, 224, 255 });
        // 底座接地阴影
        DrawRectangle((int)bx + 4, (int)by + 8, 8, 2, (Color) { 90, 84, 74, 255 });
        ZhText("营", (int)bx + 4, (int)by + 1, 10, (Color) { 176, 32, 32, 255 });
        if (!campBuilt) {                          // 未建营地：呼吸光圈提示
            float gl = 0.5f + 0.5f * sinf(nowT * 2.0f);
            DrawPixCircle(bx + 8, by + 5, 13.0f + gl * 3.0f,
                          (Color) { 120, 255, 200, (unsigned char)(30 + 30 * gl) });
        }
        break;
    }
    case ObjKind::GraveMound:
        DrawEllipse(bx + 8, by + 14, 9.0f, 2.6f, (Color) { 0, 0, 0, 75 });
        DrawTexturePro(A.graveMound, { 0, 0, 18, 16 },
                       { bx + 8 - 9 + sh, by + 16 - 16, 18, 16 }, { 0, 0 }, 0, WHITE);
        {   // 坟头鬼火：青绿磷光（暗处最先看到的就是它，也是乱葬岗的远距离标志）
            float gl = 0.5f + 0.5f * sinf(nowT * 1.7f + o.tx * 3.1f);
            DrawPixCircle(bx + 8 + sinf(nowT * 0.8f + o.ty) * 3.0f, by + 2 - gl * 3.0f,
                          1.4f, (Color) { 90, 235, 190, (unsigned char)(110 + 110 * gl) });
        }
        break;
    case ObjKind::Workbench: {
        DrawEllipse(bx + 8, by + 14, 10, 2.8f, (Color) { 0, 0, 0, 70 });
        DrawTexturePro(A.workbench, { 0, 0, 20, 16 }, { bx + 8 - 10 + sh, by + 16 - 16, 20, 16 }, { 0, 0 }, 0, WHITE);
        if (o.harvested) break;   // harvested=false 常亮：可合成提示光点
        float gl = 0.5f + 0.5f * sinf(nowT * 2.8f + o.ty);
        DrawPixCircle(bx + 10, by - 2 + gl * 2, 1.6f, (Color) { 130, 220, 255, (unsigned char)(150 + 90 * gl) });
        break;
    }
    case ObjKind::Chest: {
        // ---- 宝箱：木箱 + 铁扣 + 2.5D 侧厚 + 接地阴影 + 锁孔 ----
        // 接地椭圆阴影
        DrawEllipse(bx + 8, by + 14, 8.5f, 2.6f, (Color) { 0, 0, 0, 80 });
        // 箱体侧厚（右探出 2px）
        DrawRectangle((int)(bx + sh) + 14, (int)by + 4, 2, 10, (Color) { 58, 38, 22, 255 });
        DrawRectangle((int)(bx + sh) + 14, (int)by + 4, 1, 10, (Color) { 96, 64, 36, 255 });
        // 箱体（深木底色，衬出贴图）
        DrawRectangle((int)(bx + sh), (int)by + 3, 16, 11, (Color) { 80, 52, 30, 255 });
        // 原有宝箱精灵（开/关两态）
        DrawTexturePro(o.harvested ? A.chestOpen : A.chestClosed,
                       { 0, 0, 16, 14 }, { bx + sh, by + 1, 16, 14 }, { 0, 0 }, 0, WHITE);
        // 顶部高光（箱盖亮边）
        DrawRectangle((int)(bx + sh), (int)by + 1, 16, 1, (Color) { 180, 130, 80, 180 });
        // 铁扣（已开启则不画）
        if (!o.harvested) {
            DrawRectangle((int)(bx + sh) + 7, (int)by + 5, 2, 3, (Color) { 70, 60, 50, 255 });
            DrawRectangle((int)(bx + sh) + 7, (int)by + 5, 2, 1, (Color) { 110, 90, 70, 255 });
        }
        // 接地暗边
        DrawRectangle((int)(bx + sh), (int)by + 13, 16, 2, (Color) { 12, 8, 4, 150 });
        if (!o.harvested) {                     // 未开启宝箱微光提示
            float gl = 0.5f + 0.5f * sinf(nowT * 3.0f + o.ty);
            DrawRectangle((int)(bx + 6), (int)(by - 8 + gl), 4, 4,
                          (Color) { 255, 230, 120, (unsigned char)(150 + 80 * gl) });
        }
        break;
    }
    }
}

static Texture2D DropTex(ItemKind k) {
    switch (k) {
    case ItemKind::Wood:    return A.dropWood;
    case ItemKind::Stone:   return A.dropStone;
    case ItemKind::Berry:   return A.dropBerry;
    case ItemKind::RawMeat:    return A.dropRawMeat;
    case ItemKind::CookedMeat: return A.dropCooked;
    case ItemKind::IronOre:    return A.dropIronOre;
    case ItemKind::Crystal:    return A.dropCrystal;
    case ItemKind::GemShard:   return A.dropCrystal;   // 暂用萤晶贴图（渲染时紫色调区分）
    case ItemKind::Heart:      return A.dropHeart;     // 血月之心（猩红心脏）
    default:                 return A.dropRotten;
    }
}

// ---------------- UI（中式微恐 · 幽灯冥火风） ----------------
// 设计语言：墨黑底 + 鎏金铜扣描边 + 纸白字色；铜钱 / 白灯笼 / 鬼眼饰件；
// 界面如「悬于幽冥中的符箓面板」，冷色为主，鎏金只留给边框（克制使用）。

namespace UIC {
    const Color Ink    {   8, 10, 16, 238 };   // 墨黑外层（框体遮罩 #080a10）
    const Color Panel  {  16, 19, 24, 246 };   // 墨黑面板底（#101318，深于场景底色）
    const Color PanelHi{  78, 94, 98, 255 };   // 面板顶边极淡青灰高光
    const Color Gold   { 139, 105, 20, 255 };  // 鎏金铜扣（#8b6914，克制）
    const Color GoldHi {  77, 255, 184, 255 }; // 魂灯青（选中/充足/正面 #4dffb8）
    const Color GoldDim{  90,  64, 16, 255 };  // 暗鎏金铜（#5a4010）
    const Color Cream  { 216, 212, 200, 255 }; // 纸白正文（丧纸色 #d8d4c8，非纯白）
    const Color Mute   { 107, 101, 96, 255 };  // 纸灰次级说明（#6b6560）
    const Color Ruby   { 204,  48, 48, 255 };  // 朱砂红（危险/警示 #cc3030）
    const Color HpC    { 204,  48, 48, 255 };  // 生命（朱砂红）
    const Color FoodC  { 212, 160, 23, 255 };  // 饥饿（鎏金 #d4a017）
    const Color XpC    { 102, 204, 255, 255 }; // 经验（幽蓝 #66ccff）
    const Color OkC    {  77, 255, 184, 255 }; // 材料充足（魂灯青）
    const Color LackC  { 204,  48, 48, 255 };  // 材料不足（朱砂红）
    // ---- 冥婚血煞组（环形物品栏 / 强调：冥婚仪俗式中式恐怖，暗血红为骨、朱砂为纹）----
    const Color Blood  { 122,  14,  22, 255 }; // 血煞暗红（槽位主色 #7a0e16）
    const Color BloodHi{ 226,  40,  46, 255 }; // 猩红高亮（选中 #e2282e）
    const Color BloodDk{  46,   8,  14, 255 }; // 凝血底（符纸暗红底）
    const Color Cinna  { 176,  32,  32, 255 }; // 朱砂（描边 / 八卦点 / 符纹）
    const Color Paper  { 188, 174, 150, 255 }; // 符纸黄（空白槽 #bcae96）
}

// 3x3 鎏金菱钻（角饰 / 分隔符 / 选中标记）
static void UIDiamond(int cx, int cy, Color c) {
    DrawPixel(cx, cy - 1, c);
    DrawPixel(cx - 1, cy, c); DrawPixel(cx, cy, c); DrawPixel(cx + 1, cy, c);
    DrawPixel(cx, cy + 1, c);
}

// 硬边像素圆环（环形物品栏槽位描边）
// 性能：每行直接解出左右端点（O(r) 行扫描），替代逐像素判断（O(r^2)）——热栏常开时省可观开销
static void UIPixRing(int cx, int cy, int r, Color c) {
    int r2 = r * r, ri2 = (r - 1) * (r - 1);
    for (int dy = -r; dy <= r; dy++) {
        int dy2 = dy * dy;
        int rem = r2 - dy2;
        if (rem < 0) continue;
        int outer = (int)sqrtf((float)rem);              // 外圈半径在该行的半宽
        // 内圈：找最大的 |dx| 使 dx^2+dy2 <= ri2（即 dx^2 <= ri2-dy2）
        int inner2 = ri2 - dy2;
        int inner = inner2 >= 0 ? (int)sqrtf((float)inner2) : -1;
        // 该行环带：|dx| 在 (inner, outer] 之间（1px 厚环）
        if (inner < outer) {
            if (inner >= 0) {
                DrawRectangle(cx - outer, cy + dy, outer - inner, 1, c);
                DrawRectangle(cx + inner + 1, cy + dy, outer - inner, 1, c);
            } else {
                DrawRectangle(cx - outer, cy + dy, outer * 2 + 1, 1, c);
            }
        }
    }
}

// 云纹角饰（中式回云纹小像素：角点 + 两道短卷云尾）
static void UICloud(int cx, int cy, int dx, int dy, Color c) {
    DrawPixel(cx, cy, c);
    DrawPixel(cx + dx, cy, c); DrawPixel(cx, cy + dy, c);
    DrawPixel(cx + dx * 2, cy + dy, c); DrawPixel(cx + dx, cy + dy * 2, c);
}

// 符箓面板：切角墨底（八边形轮廓）+ 1px 鎏金铜边 + 顶部青灰高光 + 四角云纹
static void UIPanel(int x, int y, int w, int h) {
    DrawRectangle(x - 1, y - 2, w + 2, h + 4, UIC::Ink);       // 切角外沿
    DrawRectangle(x - 2, y - 1, w + 4, h + 2, UIC::Ink);
    DrawRectangle(x, y - 1, w, 1, UIC::Cinna);                 // 四边朱砂框（不到角）
    DrawRectangle(x, y + h, w, 1, UIC::Cinna);
    DrawRectangle(x - 1, y, 1, h, UIC::Cinna);
    DrawRectangle(x + w, y, 1, h, UIC::Cinna);
    DrawRectangle(x, y, w, h, UIC::Panel);                     // 墨黑面板底
    DrawRectangle(x, y, w, 1, UIC::PanelHi);                   // 顶边极淡青灰高光
    if (w >= 8 && h >= 12) {                                   // 内圈血线（冥婚喜帖的双层框）
        DrawRectangle(x + 1, y + 1, w - 2, 1, (Color) { 92, 14, 20, 130 });
        DrawRectangle(x + 1, y + h - 2, w - 2, 1, (Color) { 92, 14, 20, 130 });
    }
    UICloud(x - 1, y - 1, 1, 1, UIC::Cinna); UICloud(x + w, y - 1, -1, 1, UIC::Cinna);
    UICloud(x - 1, y + h, 1, -1, UIC::Cinna); UICloud(x + w, y + h, -1, -1, UIC::Cinna);
}

// 刻度条：墨黑槽 + 顶部冷光高光 + 冷色梢头 + 四等分刻度
static void PixelBar(int x, int y, int w, int h, float pct, Color fill) {
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;
    DrawRectangle(x - 1, y - 1, w + 2, h + 2, (Color) { 26, 6, 10, 255 });   // 暗血红外槽
    DrawRectangle(x, y, w, h, (Color) { 20, 24, 30, 255 });                  // 近黑内槽（保数值可读）
    int fw = (int)(w * pct + 0.5f);
    if (fw > 0) {
        DrawRectangle(x, y, fw, h, fill);
        DrawRectangle(x, y, fw, 1, (Color) { 200, 235, 225, 60 });    // 顶部冷光高光
        DrawPixel(x + fw - 1, y, UIC::BloodHi);                       // 猩红梢头
        DrawPixel(x + fw - 1, y + h - 1, UIC::BloodHi);
    }
    for (int i = 1; i < 4; i++) {                                     // 四等分刻度
        int tx = x + w * i / 4;
        DrawPixel(tx, y, (Color) { 8, 10, 16, 170 });
        DrawPixel(tx, y + h - 1, (Color) { 8, 10, 16, 170 });
    }
}

// 铜钱（方孔圆钱：外圆鎏金铜 + 中方孔墨黑）
static void UICoin(int cx, int cy) {
    DrawPixCircle((float)cx, (float)cy, 3, UIC::BloodDk);
    DrawPixCircle((float)cx, (float)cy, 2, UIC::Blood);
    DrawPixel(cx - 1, cy - 1, UIC::Cinna);
    DrawRectangle(cx - 1, cy - 1, 2, 2, UIC::Ink);                    // 方孔
}

// 白灯笼徽记 9x6（椭圆白纸灯笼 + 上下鎏金箍 + 一点青绿焰）
static void UICrown(int x, int y) {
    DrawRectangle(x + 3, y, 3, 1, UIC::Blood);                        // 上朱砂箍
    DrawPixCircle(x + 4.0f, y + 3.0f, 3.2f, UIC::Cream);              // 白纸灯笼身
    DrawPixel(x + 2, y + 2, (Color) { 168, 160, 144, 255 });          // 纸面折线
    DrawPixel(x + 6, y + 2, (Color) { 168, 160, 144, 255 });
    DrawRectangle(x + 3, y + 5, 3, 1, UIC::Blood);                    // 下朱砂箍
    DrawPixel(x + 4, y + 3, UIC::GoldHi);                             // 青绿焰
}

// 赤红鬼眼徽记（两点猩红，静态）
static void UIGhostEye(int x, int y) {
    DrawPixel(x + 1, y + 1, UIC::Ruby); DrawPixel(x + 2, y + 1, UIC::Ruby);
    DrawPixel(x + 6, y + 1, UIC::Ruby); DrawPixel(x + 7, y + 1, UIC::Ruby);
    DrawPixel(x + 1, y, (Color) { 120, 20, 20, 255 }); DrawPixel(x + 7, y, (Color) { 120, 20, 20, 255 });
}

// 铜扣分隔线（中央菱钻）
static void UIRule(int cx, int y, int w) {
    DrawRectangle(cx - w / 2, y, w, 1, UIC::Blood);
    UIDiamond(cx, y, UIC::Cinna);
}

static void DrawUI() {
    // ---- 左上纹章状态面板：生命 / 饥饿 / 等级经验 ----
    UIPanel(4, 4, 120, 44);
    DrawTexture(A.heart, 10, 8, WHITE);
    // ---- 记忆篡改（无脸鬼）：血条显示假值（玩家以为满血，实际在掉）----
    // 骗术：显示 P.fakeHp（篡改时被"修正"的高值）；唯一破绽是条尾 1px 暗紫微闪（极难察觉）
    {
        int showHp = P.hacked ? P.fakeHp : P.hp;
        float hpPct = showHp / (float)P.maxHp;
        if (hpPct > 1.0f) hpPct = 1.0f;
        PixelBar(22, 9, 88, 6, hpPct, UIC::HpC);
        if (P.hacked) {
            // 篡改破绽：条尾暗紫微闪（每 2 秒闪 0.2 秒，留给细心玩家的唯一线索）
            float ph = fmodf((float)GetTime(), 2.0f);
            if (ph < 0.2f) {
                int tip = 22 + (int)(88 * hpPct) - 2;
                if (tip < 22) tip = 22;
                DrawRectangle(tip, 9, 2, 6, (Color) { 130, 60, 180, 200 });
            }
        }
    }
    DrawTexture(A.drumstick, 10, 20, WHITE);
    PixelBar(22, 21, 88, 6, P.hunger / 100.0f, UIC::FoodC);
    DrawTexture(A.starIcon, 10, 32, WHITE);
    ZhText(TextFormat("Lv%d", P.level), 22, 33, 10, UIC::Cream);
    PixelBar(46, 34, 64, 5, P.xp / (float)XpNext(P.level), UIC::XpC);

    // ---- 右上：三阶驱鬼器物徽记（剑/葫芦/幡；动态宽度，零持有时整行隐藏）----
    int tw = 12, tn = 0;
    auto Slot = [&](int lv, const char* nm, int id, Color hi, Color lo) {
        if (lv <= 0) return;
        tw += 14 + ZhWidth(nm, 10) + 10 + 6;
        tn++;
    };
    Slot(P.swordLv, "剑", 0, UIC::Gold, (Color) { 0, 0, 0, 0 });
    Slot(P.gourdLv, "葫", 1, UIC::Cinna, (Color) { 0, 0, 0, 0 });
    Slot(P.captureLv, "幡", 2, UIC::BloodHi, (Color) { 0, 0, 0, 0 });
    if (tn > 0) {
        int tx = VW - tw - 4, ty = 4;
        UIPanel(tx, ty, tw, 18);
        int cx = tx + 6;
        // 桃木剑
        if (P.swordLv > 0) {
            DrawRectangle(cx, ty + 4, 8, 10, (Color) { 140, 90, 50, 255 });
            DrawRectangle(cx + 1, ty + 4, 2, 10, (Color) { 200, 150, 90, 255 });
            const char* t = TextFormat(L10N("剑%d"), (int)P.swordLv);
            ZhText(t, cx + 12, ty + 4, 10, UIC::Gold);
            cx += 14 + ZhWidth(t, 10) + 10;
        }
        // 葫芦
        if (P.gourdLv > 0) {
            DrawPixCircle(cx + 4, ty + 9, 6.0f, (Color) { 180, 130, 60, 255 });
            DrawPixCircle(cx + 4, ty + 9, 4.0f, (Color) { 220, 175, 100, 255 });
            const char* t = TextFormat(L10N("葫%d"), (int)P.gourdLv);
            ZhText(t, cx + 12, ty + 4, 10, UIC::Cinna);
            cx += 14 + ZhWidth(t, 10) + 10;
        }
        // 镇鬼幡
        if (P.captureLv > 0) {
            DrawRectangle(cx + 4, ty + 2, 1, 14, UIC::Blood);
            DrawRectangle(cx + 1, ty + 4, 7, 4, (Color) { 220, 30, 40, 255 });
            const char* t = TextFormat(L10N("幡%d"), (int)P.captureLv);
            ZhText(t, cx + 12, ty + 4, 10, UIC::BloodHi);
            cx += 14 + ZhWidth(t, 10) + 10;
        }
    }

    int rw = 15;
    auto MRes = [&](int n) { if (n > 0) rw += 10 + ZhWidth(TextFormat("x%d", n), 10) + 7; };
    MRes(P.wood); MRes(P.stone); MRes(P.berry); MRes(P.rawMeat);
    MRes(P.cookedMeat); MRes(P.rottenMeat); MRes(P.iron); MRes(P.crystal);
    if (rw > 15) {
        UIPanel(4, 52, rw, 15);
        UICoin(12, 59);
        int rx = 20;
        auto Res = [&](Texture2D ic, int n) {
            if (n <= 0) return;
            DrawTexture(ic, rx, 55, WHITE);
            const char* s = TextFormat("x%d", n);
            ZhText(s, rx + 10, 56, 10, UIC::Cream);
            rx += 10 + ZhWidth(s, 10) + 7;
        };
        Res(A.dropWood, P.wood);
        Res(A.dropStone, P.stone);
        Res(A.dropBerry, P.berry);
        Res(A.dropRawMeat, P.rawMeat);
        Res(A.dropCooked, P.cookedMeat);
        Res(A.dropRotten, P.rottenMeat);
        Res(A.dropIronOre, P.iron);
        Res(A.dropCrystal, P.crystal);
    }

    // 顶部中央：纪日绶带（昼夜图标 + 第 X 天，冷色配色）
    bool night = IsNight(gameTime);
    int day = (int)(gameTime / DAY_LEN) + 1;
    const char* ds = TextFormat(L10N("第 %d 天"), day);
    int dw = ZhWidth(ds, 12);
    int dbw = dw + 34, dbx = VW / 2 - dbw / 2;
    UIPanel(dbx, 4, dbw, 16);
    DrawTexture(night ? A.moonIcon : A.sunIcon, dbx + 6, 6, WHITE);
    ZhText(ds, dbx + 21, 6, 12, UIC::Cream);
    // 左右白幡飘带小像素（素白丧仪之色）
    DrawRectangle(dbx - 5, 11, 2, 3, UIC::Cream); DrawPixel(dbx - 6, 12, (Color) { 168, 160, 144, 255 });
    DrawRectangle(dbx + dbw + 3, 11, 2, 3, UIC::Cream); DrawPixel(dbx + dbw + 5, 12, (Color) { 168, 160, 144, 255 });

    // ---- 每日悬赏小账册（右上角：今天值得做完的三件小事）----
    if ((gs == GS::Play || gs == GS::Pause) && !bookOpen) {
        int bw = 86;                                     // 面板宽度随文案自适应（避免英文溢出）
        for (int i = 0; i < 3; i++) {
            const Bounty& b0 = gBounty[i];
            int c0 = b0.type == BT_SURVIVE ? 0 : BountyCounter(b0.type) - b0.base;
            if (c0 > b0.need) c0 = b0.need;
            if (c0 < 0) c0 = 0;
            int w0 = ZhWidth(TextFormat(L10N("%s %d/%d"), L10N(BountyName(b0.type)),
                                        b0.done ? b0.need : c0, b0.need), 10) + 12;
            if (w0 > bw) bw = w0;
        }
        int tw0 = ZhWidth(L10N("今日悬赏"), 10) + 12;
        if (tw0 > bw) bw = tw0;
        int bx = VW - bw - 4, by = 4;
        UIPanel(bx, by, bw, 58);
        const char* btitle = L10N("今日悬赏");
        ZhText(btitle, bx + bw / 2 - ZhWidth(btitle, 10) / 2, by + 4, 10, UIC::GoldHi);
        for (int i = 0; i < 3; i++) {
            const Bounty& b = gBounty[i];
            int cur = b.type == BT_SURVIVE ? 0 : BountyCounter(b.type) - b.base;
            if (cur > b.need) cur = b.need;
            if (cur < 0) cur = 0;
            const char* ln = TextFormat(L10N("%s %d/%d"), L10N(BountyName(b.type)), b.done ? b.need : cur, b.need);
            int ly = by + 17 + i * 12;
            ZhText(ln, bx + 6, ly, 10, b.done ? UIC::Mute : UIC::Cream);
            if (b.done) DrawRectangle(bx + 5, ly + 5, bw - 11, 1, UIC::Mute);   // 完成划线
        }
    }

    // ---- 无面鬼渗透预警：只给"不对劲"的氛围，绝不报具体数字（无破绽原则）----
    if (gs == GS::Play && !P.dead && gCompTotal > 3 && gCompFace > 0 &&
        gCompFace * 100 >= gCompTotal * (FacelessPct() - 15)) {
        float pl = 0.5f + 0.5f * sinf((float)GetTime() * 3.2f);
        const char* ws = "队伍里……好像多了一个人";
        int ww = ZhWidth(ws, 12);
        ZhText(ws, VW / 2 - ww / 2, 52, 12,
               (Color) { 226, 40, 46, (unsigned char)(150 + 90 * pl) });
    }
    // ---- 问询应答（短暂显示：玩家拿它跟顶栏的"第 X 天"对照）----
    if (askLineT > 0.0f && askLine[0]) {
        unsigned char aa = (unsigned char)(askLineT > 0.6f ? 255 : (int)(askLineT / 0.6f * 255));
        int aw = ZhWidth(askLine, 12);
        DrawRectangle(VW / 2 - aw / 2 - 6, VH - 66, aw + 12, 18, (Color) { 6, 12, 26, (unsigned char)(aa * 0.8f) });
        ZhText(askLine, VW / 2 - aw / 2, VH - 63, 12, (Color) { 200, 215, 235, aa });
    }

    // 装备与 Buff 状态面板（资源袋下方一行；动态宽度）
    int ew = 6;
    auto MTag = [&](const char* s) { ew += 11 + ZhWidth(s, 10) + 7; };
    if (P.armorLv == 1) MTag("铁甲");
    if (P.armorLv == 2) MTag("萤晶甲");
    if (P.buffSpdT > 0) MTag(TextFormat(L10N("加速%ds"), (int)P.buffSpdT));
    if (P.buffRegenT > 0) MTag(TextFormat(L10N("回血%ds"), (int)P.buffRegenT));
    if (P.buffNightT > 0) MTag(TextFormat(L10N("夜视%ds"), (int)P.buffNightT));
    if (P.captureLv > 0) MTag(TextFormat(L10N("摄魂幡Lv%d"), P.captureLv));
    if (campBuilt) ew += 11 + ZhWidth(TextFormat(L10N("领地Lv%d"), campLv), 10) + 7;
    ew += 11 + ZhWidth(TextFormat(L10N("同伴%d"), gCompTotal), 10) + 7;
    ew += 11 + ZhWidth(TextFormat(L10N("问%d"), askLeft), 10) + 7;
    if (PET.on) ew += 13 + ZhWidth(TextFormat("Lv%d %d", PET.level, PET.hp), 10) + 7;
    for (const GhostAlly& g0 : ghosts)           // 鬼仆状态位（按实测宽推进，避免压字）
        ew += 13 + ZhWidth(TextFormat("%d", g0.hp), 10) + 7;
    if (ew > 6) {
        UIPanel(4, 72, ew, 15);
        int ex = 10;
        auto EquipTag = [&](Texture2D ic, const char* label, Color col) {
            DrawTexture(ic, ex, 75, WHITE);
            ZhText(label, ex + 11, 76, 10, col);
            ex += 11 + ZhWidth(label, 10) + 7;
        };
        if (P.armorLv == 1) EquipTag(A.armorIron, "铁甲", (Color) { 184, 196, 214, 255 });
        if (P.armorLv == 2) EquipTag(A.armorCrystal, "萤晶甲", (Color) { 102, 204, 255, 255 });
        if (P.buffSpdT > 0)
            EquipTag(A.dropJam, TextFormat(L10N("加速%ds"), (int)P.buffSpdT), (Color) { 160, 255, 220, 255 });
        if (P.buffRegenT > 0)
            EquipTag(A.dropStew, TextFormat(L10N("回血%ds"), (int)P.buffRegenT), (Color) { 120, 230, 170, 255 });
        if (P.buffNightT > 0)
            EquipTag(A.dropTea, TextFormat(L10N("夜视%ds"), (int)P.buffNightT), (Color) { 150, 220, 255, 255 });
        if (P.captureLv > 0)
            EquipTag(P.captureLv >= 3 ? A.toolFan3 : (P.captureLv == 2 ? A.toolFan2 : A.toolFan1),
                     TextFormat(L10N("摄魂幡Lv%d"), P.captureLv), (Color) { 140, 255, 210, 255 });
        if (PET.on) {
            Texture2D petIc = PET.kind == 0 ? A.rabbit[0] : A.deer[0];
            DrawTexturePro(petIc, { 0, 0, (float)petIc.width, (float)petIc.height },
                           { (float)ex, 74, 11, 11 }, { 0, 0 }, 0, WHITE);
            const char* ps = TextFormat("Lv%d %d", PET.level, PET.hp);
            ZhText(ps, ex + 13, 76, 10, (Color) { 255, 225, 130, 255 });
            ex += 13 + ZhWidth(ps, 10) + 7;
        }
        // ---- 领地 / 同伴 / 今日问询（无面鬼渗透的公开信息只有"总数"，不报无面鬼数）----
        if (campBuilt) {
            const char* ts = TextFormat(L10N("领地Lv%d"), campLv);
            ZhText(ts, ex, 76, 10, (Color) { 255, 224, 138, 255 });
            ex += 11 + ZhWidth(ts, 10) + 7;
        }
        {
            const char* cs2 = TextFormat(L10N("同伴%d"), gCompTotal);
            ZhText(cs2, ex, 76, 10, (Color) { 190, 235, 200, 255 });
            ex += 11 + ZhWidth(cs2, 10) + 7;
            const char* as = TextFormat(L10N("问%d"), askLeft);
            ZhText(as, ex, 76, 10, askLeft > 0 ? (Color) { 150, 220, 255, 255 }
                                               : (Color) { 130, 120, 120, 255 });
            ex += 11 + ZhWidth(as, 10) + 7;
        }
        // 鬼仆状态：怪物小图 + 剩余生命
        for (const GhostAlly& g : ghosts) {
            Texture2D gi = MobFrames(g.kind)[0];
            DrawTexturePro(gi, { 0, 0, (float)gi.width, (float)gi.height },
                           { (float)ex, 74, 11, 11 }, { 0, 0 }, 0, (Color) { 150, 255, 215, 255 });
            const char* gs2 = TextFormat("%d", g.hp);
            ZhText(gs2, ex + 13, 76, 10, (Color) { 140, 255, 210, 255 });
            ex += 13 + ZhWidth(gs2, 10) + 7;
        }
    }

    // ---- 鬼条（仅"已触发/正在追杀你"的鬼：鬼名 + 威胁级 + 血条；蛰伏的鬼不显示——无破绽）----
    // 不在视野时：屏幕边缘朱砂箭头指向它（跑得再远也甩不掉的那种）
    {
        const Creature* tgt = nullptr;
        float bd = 340.0f * 340.0f;
        for (const Creature& c : mobs) {
            if (c.state == AState::Dead || !MobHostile(c.kind) || c.infiltrated) continue;
            if (!c.triggered) continue;              // 蛰伏的鬼绝不露出任何信息
            float dx = c.x - P.x, dy = c.y - P.y;
            float q = dx * dx + dy * dy;
            if (q < bd) { bd = q; tgt = &c; }
        }
        if (tgt && !P.dead && gs == GS::Play) {
            const char* gname = GhostName(tgt->kind);
            const char* grank = GhostRank(tgt->kind);
            int bw = 170, bx = VW / 2 - bw / 2;
            // 鬼名 + 等级徽记（S 级猩红，A 级朱砂，B 级纸灰）
            Color rankC = (grank[0] == 'S') ? (Color) { 255, 96, 80, 255 }
                         : (grank[0] == 'A') ? (Color) { 226, 40, 46, 255 }
                                             : (Color) { 168, 160, 144, 255 };
            int nw = ZhWidth(gname, 12);
            ZhText(gname, VW / 2 - nw / 2 - 10, 26, 12, rankC);
            ZhText(grank, VW / 2 + nw / 2 + 2, 26, 12, rankC);
            PixelBar(bx, 40, bw, 6, tgt->hp / (float)MobBaseHp(tgt->kind), UIC::Ruby);
            // ---- 方向指针：鬼不在视野内时，屏幕边缘朱砂箭头指向它 ----
            float ex = tgt->x - camX, ey = tgt->y - camY;
            bool inView = (ex >= 14.0f && ex <= VW - 14.0f && ey >= 14.0f && ey <= VH - 14.0f);
            if (!inView) {
                float cx = VW * 0.5f, cy = VH * 0.5f;
                float dx = ex - cx, dy = ey - cy;
                float len = sqrtf(dx * dx + dy * dy);
                if (len > 0.001f) {
                    dx /= len; dy /= len;
                    float mx = VW * 0.5f - 36.0f, my = VH * 0.5f - 36.0f;
                    float tt = 1.0e9f;
                    if (fabsf(dx) > 1.0e-4f) tt = fminf(tt, mx / fabsf(dx));
                    if (fabsf(dy) > 1.0e-4f) tt = fminf(tt, my / fabsf(dy));
                    float ax = cx + dx * tt, ay = cy + dy * tt;
                    float nt = (float)GetTime();
                    float pulse = 0.5f + 0.5f * sinf(nt * 6.0f);
                    // 像素三角箭头（先描黑边再填色）
                    float nx = -dy, ny = dx;
                    Color ac = { (unsigned char)(160 + 66 * pulse), 30, 36, 240 };
                    Vector2 tp = { ax + dx * 7.0f, ay + dy * 7.0f };
                    Vector2 b1 = { ax - dx * 4.0f + nx * 5.5f, ay - dy * 4.0f + ny * 5.5f };
                    Vector2 b2 = { ax - dx * 4.0f - nx * 5.5f, ay - dy * 4.0f - ny * 5.5f };
                    DrawTriangle({ tp.x + 1, tp.y + 1 }, { b1.x + 1, b1.y + 1 },
                                 { b2.x + 1, b2.y + 1 }, (Color) { 6, 9, 13, 210 });
                    DrawTriangle(tp, b1, b2, ac);
                    // 距离徽章
                    const char* dtxt = TextFormat("%dm", (int)(len / 16.0f));
                    int dw2 = ZhWidth(dtxt, 10);
                    int tx2 = (int)(ax + dx * 16.0f) - dw2 / 2;
                    int ty2 = (int)(ay + dy * 16.0f) - 5;
                    if (tx2 < 2) tx2 = 2;
                    if (tx2 > VW - dw2 - 2) tx2 = VW - dw2 - 2;
                    if (ty2 < 2) ty2 = 2;
                    if (ty2 > VH - 14) ty2 = VH - 14;
                    UIPanel(tx2 - 4, ty2 - 2, dw2 + 8, 14);
                    ZhText(dtxt, tx2, ty2, 10, (Color) { 226, 40, 46, 255 });
                }
            }
        }
    }

    // ---- 猜拳结果提示（淡出）----
    if (rpsHintT > 0.0f && rpsHintTxt[0]) {
        float fa = rpsHintT < 0.4f ? rpsHintT / 0.4f : 1.0f;
        int sw = ZhWidth(rpsHintTxt, 12);
        ZhText(rpsHintTxt, VW / 2 - sw / 2, VH - 86, 12,
               (Color) { 216, 212, 200, (unsigned char)(fa * 255) });
    }

    // ---- 猜拳面板（游戏鬼：按 1/2/3 出拳，平局或输掉才杀人）----
    if (rpsState == 1 && gs == GS::Play) {
        int pw = 190, ph = 74, px = VW / 2 - pw / 2, py = VH / 2 - ph / 2 - 20;
        UIPanel(px, py, pw, ph);
        UICrown(px + 12, py + 7);
        ZhText("游戏鬼：猜拳", px + 26, py + 6, 12, UIC::BloodHi);
        ZhText("赢了它就走，输了……", px + 16, py + 24, 10, UIC::Mute);
        ZhText("[1] 石头   [2] 剪刀   [3] 布", px + 16, py + 44, 12, UIC::Cream);
        ZhText("E 放弃（它不喜欢逃跑）", px + 16, py + 60, 10, UIC::Mute);
    }

    // ---- 交易面板（交易鬼：等价交换，所有馈赠早已标好价格）----
    if (dealState == 1 && gs == GS::Play) {
        int pw = 230, ph = 92, px = VW / 2 - pw / 2, py = VH / 2 - ph / 2 - 20;
        UIPanel(px, py, pw, ph);
        UICrown(px + 12, py + 7);
        ZhText("交易鬼：等价交换", px + 26, py + 6, 12, UIC::Gold);
        ZhText("[1] 以寿命换痊愈（上限-10%，回满）", px + 14, py + 26, 10, UIC::Cream);
        ZhText("[2] 以木料换铁矿（木20 -> 铁5）", px + 14, py + 42, 10, UIC::Cream);
        ZhText("[3] 以鬼仆换碎片（鬼仆1 -> 碎片2）", px + 14, py + 58, 10, UIC::Cream);
        ZhText("E 放弃", px + 14, py + 76, 10, UIC::Mute);
    }

    // ---- 交互提示（靠近可交互目标时：底部中间提示按 E）----
    if (!P.dead && !craftOpen && gs == GS::Play && rpsState == 0 && dealState == 0) {
        const char* hint = nullptr;
        if (W.smithPos.x > 0.0f) {
            float dx = W.smithPos.x - P.x, dy = W.smithPos.y - P.y;
            if (dx * dx + dy * dy < 60.0f * 60.0f) hint = "按 E 与铁匠交谈";
        }
        if (!hint) {
            for (const Creature& c : mobs) {
                if (c.state == AState::Dead || c.triggered) continue;
                float dx = c.x - P.x, dy = c.y - P.y;
                if (dx * dx + dy * dy > 50.0f * 50.0f) continue;
                if (c.kind == CreatureKind::DealGhost) { hint = "按 E 与交易鬼交易"; break; }
                if (c.kind == CreatureKind::GameGhost) { hint = "按 E 与游戏鬼猜拳"; break; }
            }
        }
        if (!hint)
            for (const Npc& n : npcs) {
                if (!n.on || n.kind != 1 || n.state != 0) continue;
                float dx = n.x - P.x, dy = n.y - P.y;
                if (dx * dx + dy * dy < 44.0f * 44.0f) {
                    hint = "按 E 邀请同行";
                    break;
                }
            }
        if (!hint && W.NearObj(ObjKind::CampStone, P.x, P.y, 44.0f) >= 0 && campBuilt && campLv < 3) {
            int cw = (campLv == 1) ? 40 : 80;
            int cs2 = (campLv == 1) ? 20 : 40;
            hint = TextFormat(L10N("按 E 升级营地（木%d石%d）"), cw, cs2);
        }
        if (!hint && W.NearBed(P.x, P.y, 40.0f) >= 0)
            hint = "按 E 睡觉";
        if (hint) {
            int cw = ZhWidth(hint, 12);
            UIPanel(VW / 2 - (cw + 18) / 2, VH - 40, cw + 18, 16);
            ZhText(hint, VW / 2 - cw / 2, VH - 36, 12, UIC::BloodHi);
        }
    }

    // ---- 营地面板（右上角：等级 + 入住/上限 + 职业图标行）----
    if (campBuilt) {
        int settled = 0, jobs = 0;
        for (const Npc& n : npcs)
            if (n.on && n.kind == 1 && n.state == 2) { settled++; jobs |= (1 << n.job); }
        int pw2 = 60 + settled * 34;
        UIPanel(VW - pw2 - 4, 4, pw2, 16);
        ZhText(TextFormat(L10N("营地Lv%d %d/%d"), campLv, settled, CampCap()),
               VW - pw2 + 2, 7, 10, UIC::GoldHi);
        int jx = VW - pw2 + 76;
        for (int j = 0; j < 5; j++) {
            if (!(jobs & (1 << j))) continue;
            // 职业徽记小点（职业色）
            DrawRectangle(jx, 8, 8, 8, JOB_TINT[j]);
            ZhText(JOB_NAME[j], jx + 10, 7, 10, UIC::Cream);
            jx += 10 + ZhWidth(JOB_NAME[j], 10) + 4;
        }
    }

    // ---- 染血人脸覆盖：视野收拢 + 血脸在眼前浮现（无脸鬼命中演出）----
    if (P.faceCoverT > 0.0f) {
        float k = P.faceCoverT / 4.0f;
        if (k > 1.0f) k = 1.0f;
        float nt = (float)GetTime();
        float r = 34.0f + 52.0f * (1.0f - k) + sinf(nt * 9.0f) * 3.0f;   // 越挣视野越窄
        float px2 = P.x - camX, py2 = P.y - 10 - camY;
        // 视野收拢（黑圈）
        BeginBlendMode(BLEND_MULTIPLIED);
        DrawTexturePro(A.lightGrad, { 0, 0, 128, 128 },
                       { px2 - r, py2 - r, r * 2, r * 2 }, { 0, 0 }, 0,
                       Fade(WHITE, 0.55f + 0.4f * k));
        EndBlendMode();
        // 血脸压感（暗紫手掌纹路从边缘压进来）
        BeginBlendMode(BLEND_MULTIPLIED);
        DrawTexturePro(A.lightGrad, { 0, 0, 128, 128 },
                       { px2 - r * 1.7f, py2 - r * 1.7f, r * 3.4f, r * 3.4f }, { 0, 0 }, 0,
                       (Color) { 130, 90, 160, (unsigned char)(140 * k) });
        EndBlendMode();
        // 染血人脸：屏幕正中渐渐压下来的一张脸（血泪眼 + 嘴）
        {
            float appear = 1.0f - k;                    // 覆盖进度：0→1
            unsigned char fa = (unsigned char)(150 * appear);
            int fx2 = VW / 2, fy2 = VH / 2 - (int)(30 * (1.0f - appear));
            // 脸底（苍白皮肤）
            DrawRectangle(fx2 - 26, fy2 - 30, 52, 60, (Color) { 205, 185, 172, fa });
            DrawRectangle(fx2 - 26, fy2 - 30, 52, 2, (Color) { 150, 130, 118, fa });
            // 血泪双目（随覆盖加深）
            int tear = 4 + (int)(10 * appear);
            DrawRectangle(fx2 - 16, fy2 - 14, 5, tear, (Color) { 130, 20, 20, fa });
            DrawRectangle(fx2 + 11, fy2 - 14, 5, tear, (Color) { 130, 20, 20, fa });
            // 嘴（咧开的血线，微微颤动）
            int mw = 20 + (int)(6 * sinf(nt * 6.0f));
            DrawRectangle(fx2 - mw / 2, fy2 + 14, mw, 3, (Color) { 120, 15, 15, fa });
            // 边缘滴血
            DrawRectangle(fx2 - 24, fy2 - 6, 2, 8 + (int)(6 * appear), (Color) { 110, 12, 12, fa });
            DrawRectangle(fx2 + 22, fy2 + 2, 2, 6 + (int)(8 * appear), (Color) { 110, 12, 12, fa });
        }
    }

    // ---- 面容剥夺警示（右上角营地面板下方：层数点）----
    if (P.faceStolen > 0) {
        int fw = 86;
        int fy = campBuilt ? 24 : 4;
        UIPanel(VW - fw - 4, fy, fw, 14);
        ZhText("面容", VW - fw + 2, fy + 3, 10, (Color) { 200, 160, 255, 255 });
        for (int i = 0; i < 3; i++)
            DrawRectangle(VW - fw + 32 + i * 12, fy + 4, 9, 6,
                          i < P.faceStolen ? (Color) { 200, 160, 255, 240 }
                                           : (Color) { 60, 50, 80, 200 });
    }

    // Boss 招式警示 / 击杀字幕（顶部下方居中，朱砂菱钻夹注，淡出）
    if (bossHowlT > 0 && bossHowlTxt[0]) {
        float fa = bossHowlT < 0.4f ? bossHowlT / 0.4f : 1.0f;
        int sw = ZhWidth(bossHowlTxt, 16);
        ZhText(bossHowlTxt, VW / 2 - sw / 2, 52, 16,
               (Color) { 255, 96, 80, (unsigned char)(fa * 255) });
        unsigned char da = (unsigned char)(fa * 220);
        UIDiamond(VW / 2 - sw / 2 - 9, 59, (Color) { 204, 48, 48, da });
        UIDiamond(VW / 2 + sw / 2 + 8, 59, (Color) { 204, 48, 48, da });
    }

    // 篝火旁烤肉提示（魂灯青小徽章）
    bool cookHint = !P.dead && !craftOpen && P.rawMeat > 0 && W.NearCampfire(P.x, P.y, 46) >= 0;
    if (cookHint) {
        const char* ck = "按 G 烤肉";
        int cw = ZhWidth(ck, 12);
        UIPanel(VW / 2 - (cw + 18) / 2, VH - 58, cw + 18, 16);
        ZhText(ck, VW / 2 - cw / 2, VH - 54, 12, UIC::BloodHi);
    }

    // 残魂收鬼提示（附近有残魂时：有幡显示按键，无幡提示需要；与烤肉提示错开）
    if (!P.dead && !craftOpen && !cookHint && gs == GS::Play) {
        bool soulNear = false;
        for (const Soul& s : souls) {
            if (!s.on) continue;
            float dx = s.x - P.x, dy = s.y - P.y;
            if (dx * dx + dy * dy < 72.0f * 72.0f) { soulNear = true; break; }
        }
        if (soulNear) {
            const char* ck = P.captureLv > 0 ? "按 V 收鬼" : "需要摄魂幡";
            int cw = ZhWidth(ck, 12);
            UIPanel(VW / 2 - (cw + 18) / 2, VH - 58, cw + 18, 16);
            ZhText(ck, VW / 2 - cw / 2, VH - 54, 12,
                   P.captureLv > 0 ? UIC::BloodHi : (Color) { 255, 160, 120, 255 });   // 缺幡告警留橙，不与"可执行"同红
        }
    }

    // ---- 幡面板（B 键开关：幡等级 / 成功率 / 复刻度 / 容量 / 鬼仆名册全参数）----
    if (bannerOpen && gs == GS::Play && !craftOpen) {
        int lv = P.captureLv;
        int px0 = VW / 2 - 130, py0 = VH / 2 - 118;
        int ph = lv > 0 ? 150 + 16 * (int)ghosts.size() : 96;
        UIPanel(px0, py0, 260, ph);
        Texture2D fanIc = lv >= 3 ? A.toolFan3 : (lv == 2 ? A.toolFan2 : A.toolFan1);
        if (lv > 0) DrawTexture(fanIc, px0 + 12, py0 + 8, WHITE);
        ZhText(lv > 0 ? CapName(lv) : "未持有摄魂幡", px0 + 30, py0 + 8, 13,
               lv >= 3 ? (Color) { 255, 224, 138, 255 } : (Color) { 140, 255, 210, 255 });
        ZhText(lv > 0 ? TextFormat("Lv%d", lv) : "Lv0", px0 + 224, py0 + 8, 13, UIC::Mute);
        if (lv == 0) {
            ZhText("粗纸幡: 木3 石2 (1级工作台)", px0 + 14, py0 + 34, 11, UIC::Cream);
            ZhText("收鬼 / 复刻鬼仆的入门之器", px0 + 14, py0 + 52, 11, UIC::Mute);
        } else {
            // 参数区（两列：施术 / 收服）
            int soulRate = (int)(CapSoulRate(lv) * 100 + 0.5f);
            int liveBase = (int)(CapLiveBase(lv) * 100 + 0.5f);
            ZhText(TextFormat(L10N("施距%d  冷却%.1f秒"), (int)CapRange(lv), CapCooldown(lv)),
                   px0 + 14, py0 + 32, 11, UIC::Cream);
            ZhText(TextFormat(L10N("磁吸%d  阴气%d秒"), (int)CapMagnet(lv), (int)CapGhostLife(lv)),
                   px0 + 14, py0 + 50, 11, UIC::Cream);
            ZhText(TextFormat(L10N("残魂收服%d%%"), soulRate), px0 + 136, py0 + 32, 11,
                   lv >= 3 ? (Color) { 255, 224, 138, 255 } : UIC::BloodHi);
            ZhText(TextFormat(L10N("活鬼基率%d%%"), liveBase), px0 + 136, py0 + 50, 11, UIC::BloodHi);
            ZhText(TextFormat(L10N("基础复刻度%d%%  容量%d只"), (int)(CapFidBase(lv) * 100 + 0.5f), lv),
                   px0 + 14, py0 + 68, 11, (Color) { 140, 255, 210, 255 });
            ZhText("复刻度决定鬼仆生命/伤害/速度", px0 + 14, py0 + 84, 10, UIC::Mute);
            // 鬼仆名册：每只的复刻度 / 等级 / 生命 / 阴气
            if (!ghosts.empty()) {
                ZhText("— 鬼仆名册 —", px0 + 14, py0 + 102, 11, UIC::Mute);
                int ry = py0 + 120;
                for (const GhostAlly& g : ghosts) {
                    Texture2D gi = MobFrames(g.kind)[0];
                    DrawTexturePro(gi, { 0, 0, (float)gi.width, (float)gi.height },
                                   { (float)px0 + 14, (float)ry - 3, 12.0f, 12.0f }, { 0, 0 }, 0,
                                   (Color) { 150, 255, 215, 255 });
                    ZhText(TextFormat(L10N("%s 复刻%d%% Lv%d 血%d/%d 阴气%d秒"),
                                      L10N(GhostName(g.kind)), (int)(g.fid * 100 + 0.5f),
                                      g.level, g.hp, g.maxHp, (int)(g.life + 0.99f)),
                           px0 + 30, ry, 10, (Color) { 140, 255, 210, 255 });
                    ry += 16;
                }
            } else {
                ZhText("尚无鬼仆：V 收鬼 / 残魂", px0 + 14, py0 + 102, 11, UIC::Mute);
            }
            ZhText(TextFormat(L10N("H 精炼身边鬼仆 复刻+5%% (碎%d)   B 关闭"), P.gemShard),
                   px0 + 14, py0 + ph - 20, 10, UIC::BloodHi);
        }
    }

    // ---- 合成面板（TAB；须靠近工作台，等级决定可合成配方）----
    if (craftOpen && gs == GS::Play) {
        int bench = BenchLvNear();
        // 英文文案比中文长 1.5~2 倍，面板加宽并把列位整体右移（中文保持原样）
        const int PWD = gL10nEn ? 436 : 280;
        const int CMat = gL10nEn ? 164 : 92;             // 材料 / 线索列
        const int CEff = gL10nEn ? 298 : 196;            // 效果列
        const int CIdx = gL10nEn ? 402 : 248;            // 数字键角标列
        int px0 = VW / 2 - PWD / 2, py0 = VH / 2 - 136;  // 15 行配方 + 提示行
        DrawRectangle(0, 0, VW, VH, (Color) { 12, 4, 8, 150 });   // 暗血红压暗
        UIPanel(px0, py0, PWD, 272);
        UICrown(px0 + 12, py0 + 7);
        ZhText("炼器", px0 + 26, py0 + 6, 14, UIC::BloodHi);
        ZhText(TextFormat("%d/%d", craftPage + 1, (CRAFT_CNT + 11) / 12), px0 + 86, py0 + 9, 10, UIC::Mute);
        // 工作台等级徽记（0=无台：全部灰显）
        const char* benchTxt = bench > 0 ? TextFormat(L10N("工作台 Lv%d"), bench) : "无工作台";
        Color benchC = bench >= 3 ? (Color) { 255, 224, 138, 255 }
                      : bench >= 2 ? UIC::BloodHi
                      : bench >= 1 ? UIC::Cream : UIC::LackC;
        ZhText(benchTxt, px0 + PWD - 14 - ZhWidth(benchTxt, 10), py0 + 8, 10, benchC);   // 右对齐
        UIRule(px0 + PWD / 2, py0 + 23, PWD - 36);
        auto RecipeRow = [&](int n, Texture2D ic, const char* name, const char* mat,
                             bool matOk, const char* eff, bool owned, Color effC) {
            if (n / 12 != craftPage) return;              // 分页：每页 12 项
            int ry = py0 + 28 + (n % 12) * 16;
            bool known = Prog::RecipeKnown(n);               // 图鉴：未悟得只给线索
            bool sel = (craftSel == n);
            int need = RecipeBenchLv(n);
            bool lvOk = (bench >= need);
            if (sel) {                                    // 选中：血煞高亮框
                DrawRectangle(px0 + 6, ry - 2, PWD - 12, 15, (Color) { 58, 10, 16, 170 });
                DrawRectangleLines(px0 + 6, ry - 2, PWD - 12, 15, UIC::BloodHi);
            }
            unsigned char tint = lvOk ? 255 : 110;
            DrawTexture(ic, px0 + 12, ry, (Color) { tint, tint, tint, 255 });
            ZhText(known ? name : "？？？", px0 + 30, ry + 1, 10,
                   sel ? UIC::BloodHi : (owned ? UIC::Mute : (known ? UIC::Cream : UIC::Mute)));
            if (!known) {
                ZhText(Prog::RecipeLore(n), px0 + CMat, ry + 1, 10, UIC::Mute);   // 未悟得：给一句线索
                return;
            }
            if (owned)
                ZhText("已拥有", px0 + CEff, ry + 1, 10, UIC::Paper);
            else if (!lvOk)
                ZhText(TextFormat(L10N("需%d级台"), need), px0 + CEff, ry + 1, 10, UIC::LackC);
            else {
                ZhText(mat, px0 + CMat, ry + 1, 10, matOk ? UIC::OkC : UIC::LackC);
                ZhText(eff, px0 + CEff, ry + 1, 10, effC);
            }
            // 数字键角标仅在 idx<10 时显示（数字键只支持 0~9，对应配方 0~9；其余请用 ↑↓ 选择）
            if (n < 10)
                ZhText(TextFormat("[%d]", (n + 1) % 10), px0 + CIdx, ry + 1, 10,
                       (Color) { 168, 160, 144, 255 });
        };
        // ---- Lv1：基础工具 ----
        RecipeRow(0, A.dropIronOre, "辟邪桃木剑", TextFormat(L10N("铁%d/3 木%d/2"), P.iron, P.wood),
                  P.iron >= 3 && P.wood >= 2, "攻击+3", P.toolLv[0] > 0, (Color) { 180, 220, 255, 255 });
        RecipeRow(1, A.dropIronOre, "铁斧", TextFormat(L10N("铁%d/2 木%d/2"), P.iron, P.wood),
                  P.iron >= 2 && P.wood >= 2, "采集x2", P.toolLv[3] > 0, (Color) { 240, 210, 160, 255 });
        RecipeRow(2, A.toolBow, "猎弓", TextFormat(L10N("木%d/3 铁%d/1"), P.wood, P.iron),
                  P.wood >= 3 && P.iron >= 1, "远程", P.toolLv[1] > 0, (Color) { 200, 235, 180, 255 });
        RecipeRow(3, A.toolKnife, "猎刀", TextFormat(L10N("铁%d/2 木%d/1"), P.iron, P.wood),
                  P.iron >= 2 && P.wood >= 1, "攻速", P.toolLv[2] > 0, (Color) { 225, 225, 238, 255 });
        RecipeRow(4, A.toolHammer, "铁锤", TextFormat(L10N("铁%d/3 石%d/2"), P.iron, P.stone),
                  P.iron >= 3 && P.stone >= 2, "重击", P.toolLv[4] > 0, (Color) { 235, 210, 160, 255 });
        RecipeRow(5, A.toolPick, "铁镐", TextFormat(L10N("铁%d/2 木%d/2"), P.iron, P.wood),
                  P.iron >= 2 && P.wood >= 2, "采矿", P.toolLv[5] > 0, (Color) { 210, 225, 240, 255 });
        RecipeRow(6, A.toolFan1, "粗纸摄魂幡", TextFormat(L10N("木%d/3 石%d/2"), P.wood, P.stone),
                  P.wood >= 3 && P.stone >= 2, "收鬼Lv1", P.captureLv > 0, (Color) { 150, 255, 200, 255 });
        // ---- Lv2：防具 / 药剂 / 食物 ----
        RecipeRow(7, A.armorIron, "铁甲", TextFormat(L10N("铁%d/5 木%d/2"), P.iron, P.wood),
                  P.iron >= 5 && P.wood >= 2, "减伤38%", P.armorLv >= 1, (Color) { 200, 210, 230, 255 });
        RecipeRow(8, A.dropCrystal, TextFormat(L10N("萤晶药剂 x%d"), P.potion),
                  TextFormat(L10N("晶%d/1 莓%d/3"), P.crystal, P.berry),
                  P.crystal >= 1 && P.berry >= 3, "回血50", false, (Color) { 150, 255, 170, 255 });
        RecipeRow(9, A.dropTea, "萤晶茶", TextFormat(L10N("晶%d/1 莓%d/2"), P.crystal, P.berry),
                  P.crystal >= 1 && P.berry >= 2, "夜视90秒", false, (Color) { 150, 255, 200, 255 });
        RecipeRow(10, A.dropJam, "血莓酱", TextFormat(L10N("莓%d/3"), P.berry), P.berry >= 3,
                  "加速30秒", false, (Color) { 230, 130, 220, 255 });
        RecipeRow(11, A.dropStew, TextFormat(L10N("炖肉 x%d"), P.stew),
                  TextFormat(L10N("熟肉%d/1 莓%d/2"), P.cookedMeat, P.berry),
                  P.cookedMeat >= 1 && P.berry >= 2, "需长明灯", false, (Color) { 250, 190, 110, 255 });
        bool canUp = hotSel <= 5 && P.toolLv[hotSel] > 0 && P.toolLv[hotSel] < 2;
        RecipeRow(12, A.toolAxeOre, "工具升级",
                  canUp ? TextFormat(L10N("%s 铁%d/2 碎%d/1"), ToolName(hotSel), P.iron, P.gemShard)
                        : "先在环形栏选中工具",
                  canUp && P.iron >= 2 && P.gemShard >= 1,
                  canUp ? TextFormat("Lv%d", P.toolLv[hotSel] + 1) : "--",
                  false, UIC::BloodHi);
        RecipeRow(13, A.toolFan2, "铜铃摄魂幡",
                  TextFormat(L10N("铁%d/3 晶%d/1 碎%d/1"), P.iron, P.crystal, P.gemShard),
                  P.iron >= 3 && P.crystal >= 1 && P.gemShard >= 1,
                  "收鬼Lv2", P.captureLv > 1, (Color) { 150, 255, 200, 255 });
        // ---- Lv3：鬼界物品（需收服铁匠鬼）----
        RecipeRow(14, A.toolFan3, "鎏金摄魂幡",
                  TextFormat(L10N("晶%d/3 碎%d/3 心%d/1"), P.crystal, P.gemShard, P.heart),
                  P.crystal >= 3 && P.gemShard >= 3 && P.heart >= 1,
                  "收鬼Lv3", P.captureLv > 2, (Color) { 255, 224, 138, 255 });
        RecipeRow(15, A.toolFan2,   "傀儡丝",   TextFormat(L10N("木%d/20 铁%d/3"), P.wood, P.iron),
                  P.wood >= 20 && P.iron >= 3, "同控+1", false, (Color) { 150, 220, 255, 255 });
        RecipeRow(16, A.dropCrystal,"引魂灯",   TextFormat(L10N("木%d/10 晶%d/2"), P.wood, P.crystal),
                  P.wood >= 10 && P.crystal >= 2, "自带光环", gLantern, (Color) { 150, 255, 200, 255 });
        RecipeRow(17, A.dropIronOre,"镇魂钉",   TextFormat(L10N("铁%d/5 石%d/10"), P.iron, P.stone),
                  P.iron >= 5 && P.stone >= 10, "镇宅强化", gWardNail, (Color) { 235, 210, 160, 255 });
        RecipeRow(18, A.armorIron,  "萤晶甲",   TextFormat(L10N("晶%d/5 铁%d/5"), P.crystal, P.iron),
                  P.crystal >= 5 && P.iron >= 5, "减伤55%", P.armorLv >= 2, (Color) { 150, 255, 200, 255 });
        RecipeRow(19, A.toolFan1,   "辟邪符匣", TextFormat(L10N("木%d/15 铁%d/2"), P.wood, P.iron),
                  P.wood >= 15 && P.iron >= 2, "符上限+2", false, (Color) { 216, 212, 200, 255 });
        RecipeRow(20, A.toolFan2,   "摄魂铃",   TextFormat(L10N("铁%d/8 晶%d/3"), P.iron, P.crystal),
                  P.iron >= 8 && P.crystal >= 3, "收鬼CD-三成", gCapBell, (Color) { 150, 255, 200, 255 });
        RecipeRow(21, A.dropHeart,  "血月大剑", TextFormat(L10N("心%d/1 铁%d/10"), P.heart, P.iron),
                  P.heart >= 1 && P.iron >= 10, "攻击+8", P.toolLv[0] >= 3, (Color) { 255, 120, 120, 255 });
        RecipeRow(22, A.dropJam,    "还魂香",   TextFormat(L10N("晶%d/3 莓%d/5"), P.crystal, P.berry),
                  P.crystal >= 3 && P.berry >= 5, "替死一次", gRevive, (Color) { 230, 130, 220, 255 });

        ZhText("TAB/↑↓ 选配方 / CapsLock 合成 / 数字键直达 / ESC 关闭",
               px0 + 22, py0 + 258, 10, UIC::Mute);
    }

    // ---- 环形物品栏（F 开关；1-9/0 选槽，再按同槽使用）----
    if (hotbarOpen && gs == GS::Play && !craftOpen) {
        Texture2D ics[10] = {
            A.dropIronOre, A.toolBow, A.toolKnife, A.toolAxe, A.toolHammer, A.toolPick,
            A.dropCrystal, A.dropBerry, A.dropCooked, A.dropStew
        };
        // 工具 Lv2 换矿石升级贴图（血月大剑换专属贴图）
        if (P.toolLv[1] > 1) ics[1] = A.toolBowOre;
        if (P.toolLv[2] > 1) ics[2] = A.toolKnifeOre;
        if (P.toolLv[3] > 1) ics[3] = A.toolAxeOre;
        if (P.toolLv[4] > 1) ics[4] = A.toolHammerOre;
        if (P.toolLv[5] > 1) ics[5] = A.toolPickOre;
        if (P.toolLv[0] > 2) ics[0] = A.dropHeart;       // 血月大剑：猩红之心剑格
        int counts[10] = { P.toolLv[0], P.toolLv[1], P.toolLv[2],
                           P.toolLv[3], P.toolLv[4], P.toolLv[5],
                           P.potion, P.berry, P.cookedMeat, P.stew };
        const char* names[10] = { "辟邪桃木剑", "猎弓", "猎刀", "铁斧", "铁锤", "铁镐",
                                  "药剂", "血莓", "熟肉", "炖肉" };
        const float cx = VW / 2.0f, cy = VH - 56.0f, R = 48.0f;
        float nowT = (float)GetTime();       // DrawUI 无 nowT 参数，自取时间做呼吸相位

        // ---- 中心：朱砂符牌（当前选中项详情 + 八卦朱砂点 + 血雾光晕）----
        {
            float pulse = 0.5f + 0.5f * sinf(nowT * 2.1f);
            for (int gr = 27; gr >= 20; gr -= 3)         // 血雾：随呼吸涨落的暗红光晕
                DrawPixCircle(cx, cy, (float)gr,
                              (Color) { 122, 14, 22, (unsigned char)(30 * pulse) });
            DrawPixCircle(cx, cy, 17, (Color) { 8, 4, 6, 226 });   // 墨托
            DrawPixCircle(cx, cy, 15, UIC::BloodDk);                // 凝血符纸底
            UIPixRing((int)cx, (int)cy, 16, UIC::Blood);            // 血煞环
            for (int k = 0; k < 8; k++) {                           // 八卦朱砂点（中式鬼神元素，2px 横点）
                float a = -1.5707963f + k * 0.7853982f;
                int gx = (int)(cx + cosf(a) * 24.0f);
                int gy = (int)(cy + sinf(a) * 24.0f * 0.72f);
                Color c = (Color) { 176, 32, 32, (unsigned char)(190 + 65 * pulse) };
                DrawPixel(gx, gy, c);
                DrawPixel(gx + 1, gy, c);
            }
            if (counts[hotSel] > 0) {                               // 选中项：放大图标 + 等级/数量
                DrawTexturePro(ics[hotSel],
                               { 0, 0, (float)ics[hotSel].width, (float)ics[hotSel].height },
                               { cx - 6.0f, cy - 14.0f, 12.0f, 12.0f }, { 0, 0 }, 0, WHITE);
                const char* ns = (hotSel < 6) ? TextFormat("Lv%d", counts[hotSel])
                                              : TextFormat("x%d", counts[hotSel]);
                ZhText(ns, (int)(cx - ZhWidth(ns, 10) / 2), (int)(cy + 1), 10, UIC::Cream);
            } else {
                ZhText("空", (int)(cx - ZhWidth("空", 10) / 2), (int)(cy - 5), 10, UIC::Mute);
            }
        }

        for (int i = 0; i < 10; i++) {
            float a = -1.5707963f + i * 0.6283185f;      // 从正上方开始，每槽 36°
            float sx = cx + cosf(a) * R;
            float sy = cy + sinf(a) * R * 0.62f;
            bool has = counts[i] > 0;
            if (has) {
                // 已拥有：血煞符纸槽（凝血底 + 朱砂环）
                DrawPixCircle(sx, sy, 10, (Color) { 8, 4, 6, 232 });
                DrawPixCircle(sx, sy, 9, UIC::BloodDk);
                UIPixRing((int)sx, (int)sy, 10, UIC::Blood);
                DrawTexture(ics[i], (int)(sx - 6), (int)(sy - 6), WHITE);
                if (i < 6) {                             // 工具显示等级
                    const char* ls = TextFormat("Lv%d", counts[i]);
                    ZhText(ls, (int)(sx - ZhWidth(ls, 10) / 2), (int)(sy + 11), 10,
                           counts[i] > 1 ? UIC::BloodHi : UIC::Mute);
                } else {                                 // 消耗品显示数量
                    const char* ns = TextFormat("%d", counts[i]);
                    ZhText(ns, (int)(sx - ZhWidth(ns, 10) / 2), (int)(sy + 11), 10, UIC::Cream);
                }
            } else {
                // 空白槽：空白符纸（明确表达"这格空着"，不再画半透明物品图标）
                DrawPixCircle(sx, sy, 10, (Color) { 8, 4, 6, 200 });
                DrawPixCircle(sx, sy, 8, (Color) { 188, 174, 150, 70 });      // 符纸黄
                UIPixRing((int)sx, (int)sy, 9, (Color) { 96, 84, 74, 150 });
                for (int k = -1; k <= 1; k++)                                 // 朱砂竖纹（符纸质感）
                    DrawPixel((int)sx + k * 3, (int)sy, (Color) { 122, 14, 22, 88 });
            }
            if (hotSel == i) {                           // 选中：猩红双环 + 朱砂符点 + 红框徽章
                UIPixRing((int)sx, (int)sy, 12, UIC::BloodHi);
                UIPixRing((int)sx, (int)sy, 11, (Color) { 150, 20, 28, 170 });
                UIDiamond((int)sx, (int)(sy - 17), UIC::BloodHi);
                int nw = ZhWidth(names[i], 10);
                int bx = (int)sx - nw / 2 - 5, by = (int)(sy - 34);
                UIPanel(bx, by, nw + 10, 13);
                DrawRectangle(bx, by, nw + 10, 1, UIC::Cinna);        // 冥婚红框黑底
                DrawRectangle(bx, by + 12, nw + 10, 1, UIC::Cinna);
                DrawRectangle(bx, by, 1, 13, UIC::Cinna);
                DrawRectangle(bx + nw + 9, by, 1, 13, UIC::Cinna);
                ZhText(names[i], (int)(sx - nw / 2), (int)(sy - 31), 10,
                       has ? UIC::Cream : UIC::Mute);
            }
            // 注：原"槽号 1..9,0"被移除。椭圆环上相邻槽位垂直间距在水平方向只有 ~4px，
            //     10px 字号无法在不重叠的前提下放稳；按数字键时由中心牌+名称徽章反馈槽位
        }
    }

    // ---- 符咒环（X 开关）：5 槽环形，样式与物品栏一致 ----
    // 滚轮切换符咒，左键使用；槽下显示持有数，冷却中变暗
    if (talBarOpen && gs == GS::Play && !craftOpen) {
        const float tcx = VW / 2.0f, tcy = VH - 56.0f, tR = 38.0f;
        float nt = (float)GetTime();
        float pulse = 0.5f + 0.5f * sinf(nt * 2.1f);
        // 中心符牌（同源：血雾 + 八卦朱砂点）
        for (int gr = 27; gr >= 20; gr -= 3)
            DrawPixCircle(tcx, tcy, (float)gr,
                          (Color) { 122, 14, 22, (unsigned char)(30 * pulse) });
        DrawPixCircle(tcx, tcy, 17, (Color) { 8, 4, 6, 226 });
        DrawPixCircle(tcx, tcy, 15, UIC::BloodDk);
        UIPixRing((int)tcx, (int)tcy, 16, UIC::Blood);
        for (int k = 0; k < 8; k++) {
            float a = -1.5707963f + k * 0.7853982f;
            Color c = { 176, 32, 32, (unsigned char)(190 + 65 * pulse) };
            int gx = (int)(tcx + cosf(a) * 24.0f);
            int gy = (int)(tcy + sinf(a) * 24.0f * 0.72f);
            DrawPixel(gx, gy, c);
            DrawPixel(gx + 1, gy, c);
        }
        {   // 中心：当前符咒名 + 数量 + 符等级
            const char* nm = TAL_NAME[P.talSel];
            ZhText(nm, (int)(tcx - ZhWidth(nm, 10) / 2), (int)(tcy - 14), 10, UIC::Cream);
            const char* cn = TextFormat("x%d Lv%d", P.talN[P.talSel], P.talLv[P.talSel]);
            ZhText(cn, (int)(tcx - ZhWidth(cn, 10) / 2), (int)(tcy - 3), 10, UIC::BloodHi);
            const char* ds = TAL_DESC[P.talSel];
            ZhText(ds, (int)(tcx - ZhWidth(ds, 10) / 2), (int)(tcy + 8), 10, UIC::Mute);
            // 升级进度点（3 点 = 还差几次升级）
            int left = BindUpNeed() - P.talUse[P.talSel];
            if (P.talLv[P.talSel] < 3 && left > 0 && left <= 3) {
                const char* up = TextFormat(L10N("升级%d/%d"), 3 - left, 3);
                ZhText(up, (int)(tcx - ZhWidth(up, 10) / 2), (int)(tcy + 19), 10,
                       (Color) { 255, 224, 138, 200 });
            }
        }
        // 5 个槽位环绕
        for (int i = 0; i < 5; i++) {
            float a = -1.5707963f + i * 1.2566371f;              // 每 72 度
            float sx = tcx + cosf(a) * tR;
            float sy = tcy + sinf(a) * tR * 0.62f;
            bool sel = (i == P.talSel);
            bool cooling = (P.talCd[i] > 0.0f);
            bool has = (P.talN[i] > 0);
            // 槽底：有符=血煞符纸，无符=空白符纸，冷却中整体压暗
            DrawPixCircle(sx, sy, 10, (Color) { 8, 4, 6, 230 });
            if (has && !cooling) {
                DrawPixCircle(sx, sy, 9, UIC::BloodDk);
                UIPixRing((int)sx, (int)sy, 10, UIC::Blood);
                DrawTexture(A.talisman, (int)(sx - 6), (int)(sy - 6),
                            (Color) { 255, 240, 210, 255 });
            } else {
                DrawPixCircle(sx, sy, 8, (Color) { 188, 174, 150, (unsigned char)(cooling ? 45 : 70) });
                UIPixRing((int)sx, (int)sy, 9, (Color) { 96, 84, 74, 150 });
            }
            // 数量
            const char* qn = TextFormat("%d", P.talN[i]);
            ZhText(qn, (int)(sx - ZhWidth(qn, 10) / 2), (int)(sy + 11), 10,
                   has ? UIC::Cream : (Color) { 120, 112, 104, 255 });
            // 冷却：外圈暗环随时间收拢
            if (cooling) {
                float k = P.talCd[i] / TAL_CD[i];
                UIPixRing((int)sx, (int)sy, 12, (Color) { 80, 76, 92, (unsigned char)(150 * k) });
            }
            if (sel) {                                           // 选中：猩红双环 + 朱砂符点
                UIPixRing((int)sx, (int)sy, 12, UIC::BloodHi);
                UIPixRing((int)sx, (int)sy, 11, (Color) { 150, 20, 28, 170 });
                UIDiamond((int)sx, (int)(sy - 17), UIC::BloodHi);
                int nw = ZhWidth(TAL_NAME[i], 10);
                int bx = (int)sx - nw / 2 - 5, by = (int)(sy - 34);
                UIPanel(bx, by, nw + 10, 13);
                DrawRectangle(bx, by, nw + 10, 1, UIC::Cinna);
                DrawRectangle(bx, by + 12, nw + 10, 1, UIC::Cinna);
                DrawRectangle(bx, by, 1, 13, UIC::Cinna);
                DrawRectangle(bx + nw + 9, by, 1, 13, UIC::Cinna);
                ZhText(TAL_NAME[i], (int)(sx - nw / 2), (int)(sy - 31), 10, UIC::Cream);
            }
        }
        const char* hint = "滚轮 切换   左键 用符";
        ZhText(hint, (int)(tcx - ZhWidth(hint, 10) / 2), VH - 14, 10, UIC::Mute);
    }

    // ---- 声音系鬼：屏幕边缘方位提示（朱砂箭头 + 距离，持续 2.2 秒）----
    if (wailHintT > 0.0f && !P.dead) {
        float nt = (float)GetTime();
        float pulse = 0.5f + 0.5f * sinf(nt * 8.0f);
        float ex = wailHintX - camX, ey = wailHintY - camY;
        float cx = VW * 0.5f, cy = VH * 0.5f;
        float dx = ex - cx, dy = ey - cy;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.001f) {
            dx /= len; dy /= len;
            float mx = VW * 0.5f - 26.0f, my = VH * 0.5f - 26.0f;
            float tt = 1.0e9f;
            if (fabsf(dx) > 1.0e-4f) tt = fminf(tt, mx / fabsf(dx));
            if (fabsf(dy) > 1.0e-4f) tt = fminf(tt, my / fabsf(dy));
            float ax = cx + dx * tt, ay = cy + dy * tt;
            unsigned char aa = (unsigned char)(150 + 105 * pulse);
            // 朱砂三角箭头（贴屏幕边缘，指向声源）
            for (int k = 0; k < 5; k++) {
                DrawPixel((int)(ax - dx * k), (int)(ay - dy * k + (dy > 0.5f ? k * 0.3f : 0)),
                          (Color) { 226, 40, 46, aa });
                DrawPixel((int)(ax - dx * k + (fabsf(dy) > 0.5f ? 1 : 0)),
                          (int)(ay - dy * k), (Color) { 226, 40, 46, aa });
            }
            const char* wt = TextFormat(L10N("声 %dm"), (int)(len / 16.0f));
            ZhText(wt, (int)(ax - ZhWidth(wt, 10) / 2), (int)(ay + 8), 10,
                   (Color) { 226, 40, 46, aa });
        }
    }

    // ---- 铁匠铺面板（E 与铁匠交谈：锻造三阶器物 / 献鬼升级 / 献鬼雇佣）----
    if (smithOpen && gs == GS::Play) {
        const int w = gL10nEn ? 420 : 300, h = 252;     // 英文文案更长，面板加宽
        const int x0 = VW / 2 - w / 2, y0 = 26;
        int slv = SmithLevelOf();
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 2.4f);

        DrawRectangle(x0 - 2, y0 - 2, w + 4, h + 4, UIC::Ink);
        DrawRectangle(x0, y0, w, h, UIC::Panel);
        DrawRectangle(x0, y0, w, 1, UIC::PanelHi);
        DrawRectangle(x0, y0 + h - 1, w, 1, UIC::GoldDim);
        UIDiamond(x0 + 6, y0 + 12, (Color) { 255, 224, 138, (unsigned char)(180 + 70 * pulse) });
        UIDiamond(x0 + w - 7, y0 + 12, (Color) { 255, 224, 138, (unsigned char)(180 + 70 * pulse) });
        const char* ttl = SmithIsCompanion() ? "铁 匠 鬼 · 伴 生" : "铁 匠 铺（鬼）";
        ZhTextS(TXT_TITLE, ttl, x0 + (w - ZhWidth(ttl, 14)) / 2, y0 + 4, 14);

        // ---- 头部：铁匠等级 + 锻造等级（Z / X 调）----
        const char* hd = TextFormat(L10N("铁匠鬼 Lv%d"), slv);
        ZhTextS(TXT_GOLD, hd, x0 + 12, y0 + 24, 12);
        const char* fl = TextFormat(L10N("锻造等级 %d  (Z-%d/X+%d)"), forgeLv, forgeLv > 1 ? 1 : 0, forgeLv < slv ? 1 : 0);
        ZhText(fl, x0 + 12 + ZhWidth(hd, 12) + 12, y0 + 25, 10, UIC::Cream);   // 紧跟左侧标题

        // ---- 三件器物 ----
        const char* IN[3] = { "辟邪桃木剑", "收鬼葫芦", "镇鬼幡" };
        const char* KEY[5] = { "1", "2", "3", "4", "5" };
        unsigned char* CUR[3] = { &P.swordLv, &P.gourdLv, &P.captureLv };
        for (int i = 0; i < 3; i++) {
            int ry = y0 + 42 + i * 30;
            bool hov = GB_Hit(x0 + 6, ry, w - 12, 28);
            DrawRectangle(x0 + 6, ry, w - 12, 28, hov ? (Color) { 40, 34, 20, 242 } : (Color) { 20, 24, 30, 235 });
            DrawRectangle(x0 + 6, ry, 2, 28, UIC::Gold);
            ZhText(KEY[i], x0 + 14, ry + 9, 10, (Color) { 255, 224, 138, 255 });
            const char* nm = TextFormat("%s Lv%d", L10N(IN[i]), forgeLv);
            ZhText(nm, x0 + 28, ry + 9, 10, UIC::Cream);
            if (*CUR[i] >= (unsigned char)forgeLv) {
                const char* own = "已持有";
                ZhText(own, x0 + (gL10nEn ? 184 : 132), ry + 9, 10, UIC::OkC);
            }
            const ForgeCost& c = FORGE_COST[forgeLv - 1][i];
            bool ok = P.wood >= c.wood && P.stone >= c.stone && P.iron >= c.iron &&
                      P.crystal >= c.crystal && P.gemShard >= c.shard && P.heart >= c.heart;
            char cost[64]; cost[0] = 0;
            if (c.wood)    sprintf(cost + strlen(cost), L10N("木%d "), c.wood);
            if (c.stone)   sprintf(cost + strlen(cost), L10N("石%d "), c.stone);
            if (c.iron)    sprintf(cost + strlen(cost), L10N("铁%d "), c.iron);
            if (c.crystal) sprintf(cost + strlen(cost), L10N("晶%d "), c.crystal);
            if (c.shard)   sprintf(cost + strlen(cost), L10N("碎%d "), c.shard);
            if (c.heart)   sprintf(cost + strlen(cost), L10N("心%d "), c.heart);
            ZhText(cost, x0 + w - 12 - ZhWidth(cost, 10), ry + 9, 10, ok ? UIC::OkC : UIC::LackC);
        }
        // ---- 献鬼二选一 ----
        int need = slv;                                  // 需献上的鬼等级（1..3）
        bool has = GhostStock(slv - 1) > 0;
        for (int i = 0; i < 2; i++) {
            int ry = y0 + 136 + i * 30;
            bool hov = GB_Hit(x0 + 6, ry, w - 12, 28);
            DrawRectangle(x0 + 6, ry, w - 12, 28, hov ? (Color) { 44, 18, 24, 242 } : (Color) { 20, 24, 30, 235 });
            DrawRectangle(x0 + 6, ry, 2, 28, UIC::Blood);
            ZhText(KEY[3 + i], x0 + 14, ry + 9, 10, UIC::BloodHi);
            const char* nm = (i == 0)
                ? (slv >= 3 ? "他的手艺已到顶" : TextFormat(L10N("献鬼：铁匠升到 Lv%d"), slv + 1))
                : TextFormat(L10N("献鬼：雇一位 Lv%d 铁匠"), slv);
            ZhText(nm, x0 + 28, ry + 9, 10, UIC::Cream);
            const char* cs = TextFormat(L10N("需 %d 级鬼(%d)"), need, GhostStock(slv - 1));
            ZhText(cs, x0 + w - 12 - ZhWidth(cs, 10), ry + 9, 10, has ? UIC::OkC : UIC::LackC);
        }
        // ---- 资源行 ----
        const char* rs = TextFormat(L10N("木%d 石%d 铁%d 晶%d 碎%d | 葫芦 %d/%d 鬼仆 %d"),
                                    P.wood, P.stone, P.iron, P.crystal, P.gemShard,
                                    (int)gourd.size(), GourdCap(), (int)ghosts.size());
        ZhText(rs, x0 + (w - ZhWidth(rs, 10)) / 2, y0 + 200, 10, UIC::Mute);
        // ---- 提示 / 结果反馈 ----
        if (smithMsgT > 0.0f) {
            const char* m = smithMsg;
            ZhText(m, x0 + (w - ZhWidth(m, 10)) / 2, y0 + 216, 10, (Color) { 255, 224, 138, 255 });
        }
        const char* hint = "1~5 选择   Z/X 调等级   E 离开";
        ZhText(hint, x0 + (w - ZhWidth(hint, 10)) / 2, y0 + h - 16, 10, UIC::Mute);
    }

    // ---- 鬼仆册（F 开关）：列表式（数量不限，不再受 3 槽限制）----
    //   页 0 镇鬼幡鬼仆：点一行 = 放它的专属技能；页 1 葫芦暂存：点 = 倒进镇鬼幡 / 右键 = 放生
    if (ghostBarOpen && gs == GS::Play && !craftOpen) {
        // 英文文案比中文长：面板加宽、列位右移（中文保持原布局）
        const int w = gL10nEn ? 452 : 300, h = 284;
        const int x0 = VW / 2 - w / 2, y0 = 38;
        const int rw0 = y0 + 50, rwh = 26, rowsMax = 9;
        const int CTier = gL10nEn ? 198 : 96;
        const int CHp   = gL10nEn ? 236 : 122;
        const int CSkl  = gL10nEn ? 288 : 170;
        const int CCd   = gL10nEn ? 372 : 232;
        const int CB1X  = gL10nEn ? 236 : 168;
        const int CB2X  = gL10nEn ? 340 : 230;
        const int CBW   = gL10nEn ? 96 : 56;
        const int CTabW = gL10nEn ? 130 : 86;
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 2.1f);

        DrawRectangle(x0 - 2, y0 - 2, w + 4, h + 4, UIC::Ink);
        DrawRectangle(x0, y0, w, h, UIC::Panel);
        DrawRectangle(x0, y0, w, 1, UIC::PanelHi);
        DrawRectangle(x0, y0 + h - 1, w, 1, UIC::GoldDim);
        UIDiamond(x0 + 6, y0 + 14, UIC::Cinna);
        UIDiamond(x0 + w - 7, y0 + 14, UIC::Cinna);

        const char* ttl = "鬼 仆 册";
        ZhTextS(TXT_BLOOD, ttl, x0 + (w - ZhWidth(ttl, 14)) / 2, y0 + 6, 14);

        // ---- 两个页签（← → 切换）----
        for (int t = 0; t < 2; t++) {
            int tx = x0 + 10 + t * (CTabW + 6);
            bool sel = (ghostBarPage == t);
            DrawRectangle(tx, y0 + 26, CTabW, 18, sel ? UIC::BloodDk : (Color) { 22, 26, 32, 240 });
            DrawRectangle(tx, y0 + 26, CTabW, 1, sel ? UIC::BloodHi : UIC::GoldDim);
            const char* tn = t == 0 ? "镇鬼幡·鬼仆" : "葫芦·暂存";
            ZhText(tn, tx + (CTabW - ZhWidth(tn, 10)) / 2, y0 + 31, 10, sel ? UIC::BloodHi : UIC::Mute);
        }
        // 右侧状态：剑/葫芦/幡 三阶
        {
            const char* st = TextFormat(L10N("剑%d 葫芦%d/%d 幡%d"),
                                        (int)P.swordLv, (int)gourd.size(), GourdCap(), (int)P.captureLv);
            ZhText(st, x0 + w - 14 - ZhWidth(st, 10), y0 + 31, 10, UIC::Cream);   // 右对齐
        }

        if (ghostBarPage == 0) {
            const char* hd = TextFormat(L10N("镇鬼幡 Lv%d · 鬼仆 %d（不限）"), (int)P.captureLv, (int)ghosts.size());
            ZhText(hd, x0 + 10, y0 + 47 - 12 + 4, 10, UIC::Mute);
            if (ghosts.empty()) {
                const char* e = "幡中空空——先收一只鬼进来";
                ZhText(e, x0 + (w - ZhWidth(e, 12)) / 2, rw0 + 40, 12, UIC::Mute);
            }
            int shown = (int)ghosts.size(); if (shown > rowsMax) shown = rowsMax;
            for (int i = 0; i < shown; i++) {
                const GhostAlly& g = ghosts[(size_t)i];
                int ry = rw0 + i * rwh;
                bool hov = GB_Hit(x0 + 6, ry, w - 12, rwh - 2);
                DrawRectangle(x0 + 6, ry, w - 12, rwh - 2,
                              hov ? (Color) { 40, 20, 26, 240 } : (Color) { 20, 24, 30, 235 });
                DrawRectangle(x0 + 6, ry, 2, rwh - 2, g.out ? UIC::BloodHi : UIC::Blood);
                const Texture2D* fr = MobFrames(g.kind);
                DrawTexturePro(fr[0], { 0, 0, (float)fr[0].width, (float)fr[0].height },
                               { (float)(x0 + 10), (float)(ry + 2), 16, 16 }, { 0, 0 }, 0,
                               (Color) { 150, 245, 215, 255 });
                const char* nm = GhostName(g.kind);
                ZhText(nm, x0 + 30, ry + 6, 10, UIC::Cream);
                const char* lv = TextFormat(L10N("%d级"), (int)g.tier + 1);
                ZhText(lv, x0 + CTier, ry + 6, 10, UIC::Mute);
                PixelBar(x0 + CHp, ry + 9, 40, 4, (float)g.hp / (float)(g.maxHp > 0 ? g.maxHp : 1), UIC::HpC);
                const char* sk = SkillName(SkillOfKind(g.kind));
                Color skc = g.skCd > 0.0f ? UIC::Mute : (Color) { 77, 255, 184, (unsigned char)(190 + 60 * pulse) };
                ZhText(sk, x0 + CSkl, ry + 6, 10, skc);
                const char* st2 = g.skCd > 0.0f ? TextFormat("%ds", (int)(g.skCd + 0.99f)) : "就绪";
                ZhText(st2, x0 + CCd, ry + 6, 10, g.skCd > 0.0f ? UIC::Mute : UIC::OkC);
                if (g.out) UIDiamond(x0 + w - 14, ry + 10, UIC::BloodHi);
            }
            if ((int)ghosts.size() > rowsMax) {
                const char* more = TextFormat(L10N("……还有 %d 只"), (int)ghosts.size() - rowsMax);
                ZhText(more, x0 + 12, rw0 + rowsMax * rwh + 2, 10, UIC::Mute);
            }
        } else {
            const char* hd = TextFormat(L10N("葫芦 Lv%d · %d/%d（暂储，可献给铁匠）"),
                                        (int)P.gourdLv, (int)gourd.size(), GourdCap());
            ZhText(hd, x0 + 10, y0 + 47 - 12 + 4, 10, UIC::Mute);
            if (gourd.empty()) {
                const char* e = "葫芦是空的——按 Y 收一只鬼进去";
                ZhText(e, x0 + (w - ZhWidth(e, 12)) / 2, rw0 + 40, 12, UIC::Mute);
            }
            int shown = (int)gourd.size(); if (shown > rowsMax) shown = rowsMax;
            for (int i = 0; i < shown; i++) {
                const StoredGhost& s = gourd[(size_t)i];
                int ry = rw0 + i * rwh;
                bool hov = GB_Hit(x0 + 6, ry, w - 12, rwh - 2);
                DrawRectangle(x0 + 6, ry, w - 12, rwh - 2,
                              hov ? (Color) { 40, 20, 26, 240 } : (Color) { 20, 24, 30, 235 });
                DrawRectangle(x0 + 6, ry, 2, rwh - 2, UIC::Cinna);
                const Texture2D* fr = MobFrames(s.kind);
                DrawTexturePro(fr[0], { 0, 0, (float)fr[0].width, (float)fr[0].height },
                               { (float)(x0 + 10), (float)(ry + 2), 16, 16 }, { 0, 0 }, 0,
                               (Color) { 255, 224, 138, 255 });
                const char* nm = GhostName(s.kind);
                ZhText(nm, x0 + 30, ry + 6, 10, UIC::Cream);
                const char* lv = TextFormat(L10N("%d级"), (int)s.tier + 1);
                ZhText(lv, x0 + CTier, ry + 6, 10, UIC::Mute);
                bool canT = (P.captureLv > 0 && (int)s.tier + 1 <= P.captureLv);
                DrawRectangle(x0 + CB1X, ry + 3, CBW, 18, canT ? UIC::BloodDk : (Color) { 30, 26, 26, 235 });
                DrawRectangle(x0 + CB1X, ry + 3, CBW, 1, canT ? UIC::BloodHi : UIC::Mute);
                const char* b1 = "倒进镇鬼幡";
                ZhText(b1, x0 + CB1X + (CBW - ZhWidth(b1, 10)) / 2, ry + 8, 10, canT ? UIC::Cream : UIC::Mute);
                DrawRectangle(x0 + CB2X, ry + 3, CBW, 18, (Color) { 30, 26, 26, 235 });
                DrawRectangle(x0 + CB2X, ry + 3, CBW, 1, UIC::Mute);
                const char* b2 = "放生";
                ZhText(b2, x0 + CB2X + (CBW - ZhWidth(b2, 10)) / 2, ry + 8, 10, UIC::Cream);
            }
        }
        // 底部操作提示
        const char* hint = ghostBarPage == 0
            ? "点一行 放技能   面板外左键 全体出击   右键 召回   ← → 翻页"
            : "左键 倒进镇鬼幡   右键 放生   ← → 翻页";
        ZhText(hint, x0 + (w - ZhWidth(hint, 10)) / 2, y0 + h - 16, 10, UIC::Mute);
    }

    // ---- 傀儡操控 HUD（左下角）：主控鬼 / 同控组 / 按键提示 ----
    if (puppetOn && gs == GS::Play && !craftOpen) {
        const int px0 = 8, pw = 216;
        int rows = (int)ghosts.size(); if (rows > 6) rows = 6;
        const int ph = 34 + rows * 15 + 14;
        const int py0 = VH - ph - 8;
        DrawRectangle(px0 - 2, py0 - 2, pw + 4, ph + 4, UIC::Ink);
        DrawRectangle(px0, py0, pw, ph, UIC::Panel);
        DrawRectangle(px0, py0, pw, 1, UIC::PanelHi);
        const char* ttl = "傀 儡 操 控";
        ZhTextS(TXT_BLOOD, ttl, px0 + (pw - ZhWidth(ttl, 12)) / 2, py0 + 4, 12);
        for (int i = 0; i < rows; i++) {
            const GhostAlly& g = ghosts[(size_t)i];
            bool main2 = (i == puppetMain);
            bool grp = (i < (int)puppetGrp.size() && puppetGrp[(size_t)i] != 0);
            int ry = py0 + 20 + i * 15;
            if (main2) DrawRectangle(px0 + 4, ry - 1, pw - 8, 13, (Color) { 40, 20, 26, 240 });
            const char* nm = GhostName(g.kind);
            ZhText(nm, px0 + 10, ry + 1, 10, main2 ? UIC::BloodHi : UIC::Cream);
            if (main2) ZhText("主", px0 + 108, ry + 1, 10, UIC::Gold);
            if (grp)    ZhText("傀", px0 + 128, ry + 1, 10, (Color) { 150, 220, 255, 255 });
            const char* sk = SkillName(SkillOfKind(g.kind));
            Color skc = g.skCd > 0.0f ? UIC::Mute : (Color) { 77, 255, 184, 255 };
            ZhText(g.skCd > 0.0f ? TextFormat("%s %ds", sk, (int)(g.skCd + 0.99f)) : sk,
                   px0 + 150, ry + 1, 10, skc);
        }
        if ((int)ghosts.size() > rows) {
            const char* more = TextFormat(L10N("……还有 %d 只"), (int)ghosts.size() - rows);
            ZhText(more, px0 + 10, py0 + 20 + rows * 15, 10, UIC::Mute);
        }
        const char* hint = "WASD驾驶 J技能 ←→切换 空格同控 R退出";
        ZhText(hint, px0 + (pw - ZhWidth(hint, 10)) / 2, py0 + ph - 12, 10, UIC::Mute);
    }

    // ---- 合成动画（世界冻结 0.9s：暗化 + 金环扩散 + 图标上浮 + 环绕光点）----
    if (craftAnimOn) {
        // ===== 合成动画三阶段 =====
        //  P1 0.00~0.40  材料逐一飞到工作台周围环形就位
        //  P2 0.40~0.72  材料依次朝圆心相撞
        //  P3 0.72~1.00  闪光融合 -> 产物图标浮现放大
        float t = craftAnimT / CRAFT_ANIM_LEN;
        const float P1 = 0.40f, P2 = 0.72f;

        // 圆心：优先取废弃工作台（地牢合成限定站），无工作台时退化为玩家身前
        float cx = P.x, cy = P.y - 22.0f;
        int wi = W.NearWorkbench(P.x, P.y, 64.0f);
        if (wi >= 0) {
            const WorldObj& wo = CurObjs()[(size_t)wi];
            cx = wo.tx * 16.0f + 8.0f;
            cy = wo.ty * 16.0f + 4.0f;
        }
        float sx = cx - camX, sy = cy - camY;

        // 背景压暗（中段最深，两端淡出，避免突兀）
        unsigned char dimA = (unsigned char)(96 * sinf(t * 3.14159f));
        DrawRectangle(0, 0, VW, VH, (Color) { 14, 4, 8, dimA });

        constexpr float PI2 = 6.28318531f;
        constexpr float RING_R = 30.0f;                  // 环形摆放半径
        const int N = craftMatN;

        // ---- P1 环形摆放：每个材料错开启动，沿抛物线飞向自己的环位 ----
        if (t < P2) {
            float p1 = (t < P1) ? t / P1 : 1.0f;         // P1 段进度（P2 期间保持就位/开始撞）
            for (int i = 0; i < N; i++) {
                float ang = -1.5707963f + (float)i * (PI2 / (float)N);   // 从正上方均分
                float fx = sx + cosf(ang) * RING_R;
                float fy = sy + sinf(ang) * RING_R * 0.62f;              // 椭圆：俯视透视
                // 逐一：第 i 个延迟 i*0.16 的标准化时长后启动
                float lt = (p1 - (float)i * 0.16f) / (1.0f - (float)(N - 1) * 0.16f);
                if (lt < 0.0f) lt = 0.0f; else if (lt > 1.0f) lt = 1.0f;
                if (lt <= 0.0f) continue;
                float ease = 1.0f - (1.0f - lt) * (1.0f - lt) * (1.0f - lt);   // ease-out cubic
                float ix = sx + (fx - sx) * ease;
                float iy = sy + 14.0f + (fy - sy - 14.0f) * ease - sinf(lt * 3.14159f) * 11.0f;
                float sz = 11.0f + 3.0f * sinf(lt * 3.14159f);
                // 青绿魂光晕底：地牢极暗，纯 8x8 图标会被背景吞掉，垫光晕保证可辨
                DrawCircle((int)ix, (int)iy, sz * 0.80f, (Color) { 77, 255, 184, 66 });
                DrawTexturePro(craftMat[i],
                               { 0, 0, (float)craftMat[i].width, (float)craftMat[i].height },
                               { ix - sz / 2, iy - sz / 2, sz, sz }, { 0, 0 }, 0, WHITE);
            }
        }

        // ---- P2 依次相撞：每个材料错开朝圆心加速冲刺，撞上即消失并溅火花 ----
        if (t >= P1 && t < P2) {
            float p2 = (t - P1) / (P2 - P1);
            for (int i = 0; i < N; i++) {
                float ang = -1.5707963f + (float)i * (PI2 / (float)N);
                float fx = sx + cosf(ang) * RING_R;
                float fy = sy + sinf(ang) * RING_R * 0.62f;
                float lt = (p2 - (float)i * 0.14f) / (1.0f - (float)(N - 1) * 0.14f);
                if (lt < 0.0f) lt = 0.0f; else if (lt > 1.0f) lt = 1.0f;
                float ease = lt * lt;                                     // ease-in 加速撞击
                float ix = fx + (sx - fx) * ease;
                float iy = fy + (sy - fy) * ease;
                if (lt >= 1.0f) continue;                                 // 已撞入中心
                float sz = 11.0f - 3.0f * lt;                             // 越近越小（透视冲入）
                DrawCircle((int)ix, (int)iy, sz * 0.80f, (Color) { 77, 255, 184, 66 });
                DrawTexturePro(craftMat[i],
                               { 0, 0, (float)craftMat[i].width, (float)craftMat[i].height },
                               { ix - sz / 2, iy - sz / 2, sz, sz }, { 0, 0 }, 0, WHITE);
            }
            // 中心蓄能光点（随相撞推进变亮）
            unsigned char ga = (unsigned char)(200 * p2);
            DrawCircleLines((int)sx, (int)sy, (int)(10 - 6 * p2), (Color) { 160, 255, 220, ga });
        }

        // ---- P3 融合：中心爆闪 -> 产物图标隆起放大 -> 青绿魂光环扩散 ----
        if (t >= P2) {
            float p3 = (t - P2) / (1.0f - P2);
            unsigned char fa = (unsigned char)(255 * (1.0f - p3));        // 闪光衰减
            DrawCircle((int)sx, (int)sy, 4.0f + 16.0f * p3, (Color) { 200, 255, 235, fa });
            for (int k = 0; k < 2; k++) {                                 // 两道扩散青绿环
                float rt = p3 - (float)k * 0.20f;
                if (rt > 0.0f && rt < 1.0f) {
                    float rr = 8.0f + rt * 54.0f;
                    unsigned char ra = (unsigned char)(210 * (1.0f - rt));
                    DrawCircleLines((int)sx, (int)sy, (int)rr, (Color) { 120, 230, 170, ra });
                }
            }
            float bump = (p3 < 0.25f) ? (1.0f + 0.55f * sinf(p3 * 4.0f * 3.14159f)) : 1.0f;
            float isz = (14.0f + 9.0f * p3) * bump;                       // 产物浮现放大
            float iy = sy - p3 * 15.0f + 2.0f * sinf(p3 * 12.6f);
            // 产物下垫一圈朱砂符纹小点（警示与仪式感）
            unsigned char sa2 = (unsigned char)(200 * (1.0f - p3 * 0.5f));
            for (int k = 0; k < 8; k++) {
                float a2 = (float)k * 0.7853982f;
                DrawPixel((int)(sx + cosf(a2) * (isz * 0.7f + 3.0f)),
                          (int)(iy + isz * 0.5f + sinf(a2) * 3.0f + 2.0f),
                          (Color) { 204, 48, 48, sa2 });
            }
            DrawTexturePro(craftAnimIcon,
                           { 0, 0, (float)craftAnimIcon.width, (float)craftAnimIcon.height },
                           { sx - isz / 2, iy - isz / 2, isz, isz }, { 0, 0 }, 0, WHITE);
            for (int k = 0; k < 6; k++) {                                 // 环绕光点收拢
                float a = p3 * 12.56f + (float)k * 1.047f;
                float gx2 = sx + cosf(a) * (24.0f - p3 * 12.0f);
                float gy2 = iy + sinf(a) * (14.0f - p3 * 7.0f);
                DrawRectangle((int)gx2, (int)gy2, 2, 2, (Color) { 150, 255, 200, 220 });
            }
        }
    }

    // ---- 成就解锁弹窗（白幡横幅：墨黑底 + 铜扣边 + 四角铜扣菱钻）----
    if (achvShowT > 0 && achvShowIdx >= 0) {
        float k = achvShowT > 2.8f ? (3.2f - achvShowT) / 0.4f          // 滑入
                                   : (achvShowT < 0.4f ? achvShowT / 0.4f : 1.0f);  // 滑出
        int bx = (int)(VW / 2 - 128), by = 36 + (int)((1.0f - k) * -30.0f);
        unsigned char a = (unsigned char)(235 * k);
        DrawRectangle(bx - 2, by - 2, 260, 48, (Color) { 8, 10, 16, a });
        DrawRectangle(bx - 1, by - 1, 258, 46, (Color) { 176, 32, 32, a });
        DrawRectangle(bx, by, 256, 44, (Color) { 16, 19, 24, a });
        DrawRectangle(bx, by, 256, 1, (Color) { 78, 94, 98, a });
        Color gd = { 176, 32, 32, a };
        UIDiamond(bx - 1, by - 1, gd); UIDiamond(bx + 256, by - 1, gd);
        UIDiamond(bx - 1, by + 44, gd); UIDiamond(bx + 256, by + 44, gd);
        DrawTexture(A.starIcon, bx + 8, by + 6, Fade(WHITE, k));
        ZhText("阴德 +1", bx + 20, by + 6, 10, (Color) { 216, 212, 200, a });
        ZhText(Prog::AchvName(achvShowIdx), bx + 78, by + 4, 12, (Color) { 216, 212, 200, a });
        ZhText(Prog::AchvDesc(achvShowIdx), bx + 10, by + 25, 10, (Color) { 107, 101, 96, a });
    }

    // 暂停界面（朱砂符箓面板 + 两侧鬼眼，暗血红背景）
    if (gs == GS::Pause) {
        DrawRectangle(0, 0, VW, VH, (Color) { 14, 4, 8, 170 });
        int pw = 196, ph = 160, px = VW / 2 - pw / 2, py = VH / 2 - ph / 2;
        UIPanel(px, py, pw, ph);
        UICrown(VW / 2 - 4, py + 8);
        UIGhostEye(VW / 2 - 20, py + 10);        // 两侧赤红鬼眼：中式恐怖的"被注视"感
        UIGhostEye(VW / 2 + 12, py + 10);
        ZhText("已暂停", VW / 2 - ZhWidth("已暂停", 26) / 2, py + 18, 26, UIC::Cream);
        UIRule(VW / 2, py + 50, 140);
        ZhText("点击设置项调整 · ESC 继续（暂停即存档）",
               VW / 2 - ZhWidth("点击设置项调整 · ESC 继续（暂停即存档）", 10) / 2, py + ph - 13, 10, UIC::Mute);
        // ---- 设置行（点击循环调整；悬停魂灯青高亮）----
        const char* rowL[6] = { "音乐音量", "总音量", "伤害飘字", "屏幕震动", "全屏", "语言" };
        char rowV[6][16];
        snprintf(rowV[0], 16, "%d%%", (int)(AU.bgmVol * 100 + 0.5f));
        snprintf(rowV[1], 16, "%d%%", (int)(gOptMaster * 100 + 0.5f));
        snprintf(rowV[2], 16, "%s", gOptDmg ? L10N("开") : L10N("关"));
        snprintf(rowV[3], 16, "%s", gOptShake ? L10N("开") : L10N("关"));
        snprintf(rowV[4], 16, "%s", IsWindowFullscreen() ? L10N("开 (F11)") : L10N("关 (F11)"));
        snprintf(rowV[5], 16, "%s", gL10nEn ? "English" : "中文");   // 语言行：显示目标语言名
        for (int i = 0; i < 6; i++) {
            int ry = py + 56 + i * 15;
            optRows[i] = { (float)(px + 10), (float)ry, (float)(pw - 20), 14.0f };
            bool hov = CheckCollisionPointRec(MouseGame(), optRows[i]);
            UIPanel((int)optRows[i].x, (int)optRows[i].y, (int)optRows[i].width, 14);
            if (hov)
                DrawRectangle((int)optRows[i].x + 2, (int)optRows[i].y + 2,
                              (int)optRows[i].width - 4, 10, (Color) { 58, 10, 16, 170 });
            Color tc = hov ? UIC::BloodHi : UIC::Cream;
            ZhText(rowL[i], (int)optRows[i].x + 5, ry + 3, 10, tc);
            const char* v = rowV[i];
            ZhText(v, (int)(optRows[i].x + optRows[i].width - 5 - ZhWidth(v, 10)), ry + 3, 10, tc);
        }
        // 兼容旧的全屏点击矩形（暂停分支输入处理仍引用它）
        fsBtnRect = optRows[4];
    }

    // 死亡结算页（白幡素带 + 符箓结算单）
    if (gs == GS::Dead) {
        DrawRectangle(0, 0, VW, VH, (Color) { 14, 4, 8, 200 });
        DrawRectangle(0, 0, VW, 30, (Color) { 216, 212, 200, 46 });     // 上白幡素带（纸白半透明）
        DrawRectangle(0, VH - 30, VW, 30, (Color) { 216, 212, 200, 46 });
        int pw = 250, ph = 170, px = VW / 2 - pw / 2, py = VH / 2 - ph / 2;
        UIPanel(px, py, pw, ph);
        UICrown(VW / 2 - 4, py + 10);
        UIGhostEye(VW / 2 - 20, py + 12);        // 鬼眼：死亡时盯着玩家
        UIGhostEye(VW / 2 + 12, py + 12);
        ZhText("魂 断", VW / 2 - ZhWidth("魂 断", 32) / 2, py + 22, 32, UIC::Ruby);
        UIRule(VW / 2, py + 62, 180);
        // 战绩行（纸白菱钻夹注）
        int dday = (int)(deathTime / DAY_LEN) + 1;
        auto Line = [&](const char* s, int y, Color c) {
            int w = ZhWidth(s, 12);
            ZhText(s, VW / 2 - w / 2, y, 12, c);
            UIDiamond(VW / 2 - w / 2 - 9, y + 5, UIC::Cream);
            UIDiamond(VW / 2 + w / 2 + 8, y + 5, UIC::Cream);
        };
        Line(TextFormat(L10N("存活 %d 天   等级 %d"), dday, P.level), py + 70, UIC::Cream);
        Line(TextFormat(L10N("击杀 %d   收鬼 %d"), P.kills, P.ghostCaught), py + 88, UIC::Cream);
        int got = 0;
        for (int i = 0; i < Prog::AchvCount(); i++) if (Prog::AchvGot(i)) got++;
        Line(TextFormat(L10N("成就 %d/%d   残影 %d/12"), got, Prog::AchvCount(), Prog::ShardCount()),
             py + 106, UIC::Paper);      // 符纸黄：不抢"魂 断"标题的朱砂
        float pulse = 0.55f + 0.45f * sinf((float)GetTime() * 4.0f);    // 魂灯青呼吸
        ZhText("按 R 还魂再世", VW / 2 - ZhWidth("按 R 还魂再世", 12) / 2, py + 148, 12,
               Fade(UIC::GoldHi, pulse));
    }
}

// ---------------- 手册总册（L 键）：命途 / 成就 / 图鉴 / 残卷 ----------------
static const char* BOOK_PAGE[4] = { "命 途", "成 就", "图 鉴", "残 卷" };
static const Color TIER_COL[4] = {
    { 190, 186, 176, 255 },    // 凡：纸灰
    { 120, 200, 170, 255 },    // 奇：魂灯青
    { 176, 146, 240, 255 },    // 秘：幽紫
    { 226,  64,  64, 255 },    // 绝：朱砂
};
static const char* TIER_NAME[4] = { "凡", "奇", "秘", "绝" };

// 面板框：墨黑底 + 鎏金铜扣边（与合成面板同款语言）
static void BookFrame(int px, int py, int pw, int ph) {
    DrawRectangle(px, py, pw, ph, UIC::Panel);
    DrawRectangleLines(px, py, pw, ph, UIC::Gold);
    DrawRectangle(px + 2, py + 2, pw - 4, 1, UIC::PanelHi);
    for (int i = 0; i < 4; i++) {                       // 四角铜扣
        int cx = (i & 1) ? px + pw - 3 : px + 1;
        int cy = (i & 2) ? py + ph - 3 : py + 1;
        DrawRectangle(cx, cy, 3, 3, UIC::GoldDim);
    }
}
static void Dot(int x, int y, bool on, Color c) {
    DrawRectangle(x, y, 5, 5, on ? c : (Color) { 60, 58, 54, 255 });
    DrawRectangle(x + 1, y + 1, 3, 3, on ? c : (Color) { 34, 34, 36, 255 });
}

// 手册内的按键（开启时独占，WASD 不再驱动角色）
static void BookKeys() {
    if (scrollOn) return;                  // 卷轴在上：把它让给卷轴
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_L)) { bookOpen = false; PlaySound(AU.pickup); return; }
    if (IsKeyPressed(KEY_LEFT))  { bookPage = (bookPage + 3) % 4; bookSel = 0; PlaySound(AU.pickup); return; }
    if (IsKeyPressed(KEY_RIGHT)) { bookPage = (bookPage + 1) % 4; bookSel = 0; PlaySound(AU.pickup); return; }
    if (bookPage == 0) {
        if (IsKeyPressed(KEY_UP))   { bookSel = (bookSel + PATH_N - 1) % PATH_N; PlaySound(AU.pickup); }
        if (IsKeyPressed(KEY_DOWN)) { bookSel = (bookSel + 1) % PATH_N; PlaySound(AU.pickup); }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (Prog::Invest(bookSel)) {
                PlaySound(AU.craft);
                FloatText(P.x, P.y - 40, TextFormat(L10N("%s 又深一层"), Prog::PathFullName(bookSel)), 150, 235, 190);
            } else {
                PlaySound(AU.hurt);
                FloatText(P.x, P.y - 40, "悟点不够——去杀点什么", 255, 160, 120);
            }
        }
        if (IsKeyPressed(KEY_W)) {                      // 洗点：耗木，成本随次数递增
            int cost = Prog::WipeCost();
            if (Prog::InsightSpent() <= 0) {
                FloatText(P.x, P.y - 40, "尚无已投之悟", 255, 200, 120); PlaySound(AU.hurt);
            } else if (P.wood < cost) {
                FloatText(P.x, P.y - 40, TextFormat(L10N("需木 %d"), cost), 255, 160, 120); PlaySound(AU.hurt);
            } else {
                P.wood -= cost;
                Prog::Wipe(nullptr);
                PlaySound(AU.craft);
                FloatText(P.x, P.y - 40, "重择其道", 150, 235, 190);
            }
        }
    } else if (bookPage == 3) {
        if (IsKeyPressed(KEY_UP))   { bookSel = (bookSel + CHAPTER_N - 1) % CHAPTER_N; PlaySound(AU.pickup); }
        if (IsKeyPressed(KEY_DOWN)) { bookSel = (bookSel + 1) % CHAPTER_N; PlaySound(AU.pickup); }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            OpenScroll(0, bookSel); PlaySound(AU.pickup);
        }
    }
}

static void DrawBook() {
    if (!bookOpen) return;
    DrawRectangle(0, 0, VW, VH, (Color) { 10, 6, 8, 178 });      // 压暗世界
    const int px = gL10nEn ? 110 : 152, py = 40;
    const int pw = gL10nEn ? 420 : 336, ph = 280;
    BookFrame(px, py, pw, ph);

    // ---- 页签 ----
    int tx = px + 10;
    for (int i = 0; i < 4; i++) {
        int w = ZhWidth(BOOK_PAGE[i], 12) + 14;
        bool sel = (i == bookPage);
        if (sel) DrawRectangle(tx, py + 7, w, 18, (Color) { 40, 26, 26, 255 });
        ZhText(BOOK_PAGE[i], tx + 7, py + 11, 12, sel ? UIC::GoldHi : UIC::Mute);
        if (sel) DrawRectangle(tx + 4, py + 25, w - 8, 1, UIC::Gold);
        tx += w + 4;
    }
    ZhText(TextFormat(L10N("悟 %d"), Prog::Insight()),
           px + pw - 10 - ZhWidth(TextFormat(L10N("悟 %d"), Prog::Insight()), 12), py + 11, 12,
           insightFlashT > 0 ? Fade(UIC::GoldHi, 0.5f + 0.5f * sinf((float)GetTime() * 9.0f)) : UIC::XpC);

    const int lx = px + 12, ly = py + 38;      // 列表起点（留出页计数器一行）
    const int rowH = 16;

    // ================= 页 0：命途 =================
    if (bookPage == 0) {
        for (int k = 0; k < PATH_N; k++) {
            int y = ly + k * 44;
            bool sel = (bookSel == k);
            if (sel) DrawRectangle(lx - 4, y - 2, pw - 16, 40, (Color) { 32, 30, 40, 220 });
            // 单字 + 道名 + 等级点
            if (gL10nEn) {                                     // 英文：单字道名换成全名，避免重叠
                ZhText(Prog::PathFullName(k), lx, y, 12, sel ? UIC::GoldHi : UIC::Cream);
            } else {
                ZhText(Prog::PathName(k), lx, y, 12, sel ? UIC::GoldHi : UIC::Cream);
                ZhText(Prog::PathFullName(k), lx + 16, y + 1, 10, sel ? UIC::Cream : UIC::Mute);
            }
            int lv = Prog::PathLv(k);
            const int dotX = lx + (gL10nEn ? 150 : 96);
            for (int d = 0; d < PATH_MAX_LV; d++) Dot(dotX + d * 8, y + 3, d < lv, UIC::GoldHi);
            // 下一级效果（或已满级）
            const char* eff = (lv >= PATH_MAX_LV) ? "—— 此道已通 ——"
                            : Prog::PathLine(k, lv + 1);
            ZhText(eff, lx + 4, y + 18, 10, (lv >= PATH_MAX_LV) ? UIC::Mute : UIC::Cream);
            // 本级花费
            if (lv < PATH_MAX_LV) {
                const char* cs = TextFormat(L10N("耗悟%d"), lv + 1);
                ZhText(cs, lx + pw - 34 - ZhWidth(cs, 10), y + 1, 10,
                       Prog::Insight() >= lv + 1 ? UIC::OkC : UIC::Mute);
            }
        }
        const char* h = "↑↓ 选道   Enter 投入一悟   W 洗点   ←→ 翻页   L/Esc 合册";
        ZhText(h, px + (pw - ZhWidth(h, 10)) / 2, py + ph - 15, 10, UIC::Mute);
        const char* wc = TextFormat(L10N("洗点：木 x%d（已洗 %d 次）"), Prog::WipeCost(), (int)Prog::Save().wipeCount);
        ZhText(wc, px + (pw - ZhWidth(wc, 10)) / 2, py + ph - 28, 10,
               (P.wood >= Prog::WipeCost() && Prog::InsightSpent() > 0) ? UIC::OkC : UIC::Mute);
    }
    // ================= 页 1：成就 =================
    else if (bookPage == 1) {
        int got = 0;
        for (int i = 0; i < Prog::AchvCount(); i++) if (Prog::AchvGot(i)) got++;
        const char* hd = TextFormat(L10N("已得 %d / %d"), got, Prog::AchvCount());
        ZhText(hd, px + pw - 12 - ZhWidth(hd, 10), ly - 13, 10, UIC::Mute);
        int n = 0;
        for (int i = 0; i < Prog::AchvCount(); i++) {
            int y = ly + n * rowH;
            if (y > py + ph - 24) break;
            int tier = Prog::AchvTierOf(i);
            bool has = Prog::AchvGot(i);
            DrawRectangle(lx, y + 3, 6, 6, has ? TIER_COL[tier] : (Color) { 52, 50, 48, 255 });
            ZhText(TIER_NAME[tier], lx + 10, y, 10, has ? TIER_COL[tier] : UIC::Mute);
            ZhText(Prog::AchvName(i), lx + (gL10nEn ? 62 : 24), y, 10,
                   has ? UIC::Cream : (Color) { 96, 92, 88, 255 });
            // 进度 / 奖励
            int cur = 0, need = 0;
            ProgStat dummy; Prog::AchvProgress(i, dummy, &cur, &need);
            if (!has && need > 0) {
                const char* pg = TextFormat("%d/%d", cur, need);
                ZhText(pg, lx + 214, y, 10, UIC::Mute);
            } else if (has) {
                const char* rw = TextFormat(L10N("悟+%d"), TIER_INSIGHT[tier]);
                if (TIER_INSIGHT[tier] > 0) ZhText(rw, lx + 214, y, 10, UIC::XpC);
            }
            n++;
        }
        const char* h = "成就即玩法旁证：奇 +1 悟  秘 +2 悟  绝 +3 悟   ←→ 翻页";
        ZhText(h, px + (pw - ZhWidth(h, 10)) / 2, py + ph - 15, 10, UIC::Mute);
    }
    // ================= 页 2：图鉴 =================
    else if (bookPage == 2) {
        int known = 0;
        for (int i = 0; i < RECIPE_N; i++) if (Prog::RecipeKnown(i)) known++;
        const char* hd = TextFormat(L10N("已悟 %d / %d"), known, RECIPE_N);
        ZhText(hd, px + pw - 12 - ZhWidth(hd, 10), ly - 13, 10, UIC::Mute);
        for (int i = 0; i < RECIPE_N; i++) {
            int y = ly + i * 10;
            if (y > py + ph - 12) break;
            bool kn = Prog::RecipeKnown(i);
            DrawRectangle(lx, y + 3, 4, 4, kn ? UIC::GoldHi : (Color) { 52, 50, 48, 255 });
            ZhText(kn ? Prog::RecipeName(i) : "？？？", lx + 9, y, 10,
                   kn ? UIC::Cream : (Color) { 88, 84, 80, 255 });
            ZhText(Prog::RecipeLore(i), lx + (gL10nEn ? 126 : 96), y, 10,
                   kn ? UIC::Mute : (Color) { 70, 66, 62, 255 });
        }
        const char* h = "配方不是列表，是图鉴：做到那一步，自然就懂了";
        ZhText(h, px + (pw - ZhWidth(h, 10)) / 2, py + ph - 15, 10, UIC::Mute);
    }
    // ================= 页 3：残卷 =================
    else {
        // 残影十二格
        ZhText(TextFormat(L10N("残影 %d / 12"), Prog::ShardCount()), lx, ly, 10, UIC::GoldHi);
        const int shardX = lx + (gL10nEn ? 146 : 82);
        for (int i = 0; i < SHARD_N; i++)
            Dot(shardX + i * 9, ly + 3, Prog::ShardGot(i), (Color) { 176, 146, 240, 255 });
        int y = ly + 16;
        for (int c = 0; c < CHAPTER_N; c++) {
            bool open = (c == 0) || (Prog::ShardCount() >= c * 2) || (c == 6 && Prog::ShardCount() >= 12);
            bool sel = (bookSel == c);
            if (sel) DrawRectangle(lx - 4, y - 2, pw - 16, 14, (Color) { 32, 30, 40, 220 });
            DrawRectangle(lx, y + 3, 5, 5, open ? UIC::Ruby : (Color) { 52, 50, 48, 255 });
            ZhText(open ? Prog::ChapterTitle(c) : "？？？", lx + 10, y, 10,
                   open ? (sel ? UIC::GoldHi : UIC::Cream) : (Color) { 88, 84, 80, 255 });
            if (open && c == (int)Prog::Chapter()) {
                const char* cur = "（当下）";
                ZhText(cur, lx + (gL10nEn ? 176 : 120), y, 10, UIC::Ruby);
            }
            y += 14;
        }
        // 选中章节的正文（至多 6 行）
        int sc = bookSel;
        bool open = (sc == 0) || (Prog::ShardCount() >= sc * 2) || (sc == 6 && Prog::ShardCount() >= 12);
        if (open && sc >= 0 && sc < CHAPTER_N) {
            DrawRectangle(lx - 4, y + 2, pw - 16, 1, UIC::GoldDim);
            const char* t = Prog::ChapterText(sc);
            char buf[256]; int bi = 0, li = 0;
            for (int ci = 0; t[ci] && li < 7; ci++) {
                if (t[ci] == '\n') {
                    buf[bi] = 0; bi = 0;
                    ZhText(buf, lx, y + 8 + li * 12, 10, UIC::Cream);
                    li++;
                } else if (bi < 250) buf[bi++] = t[ci];
            }
            if (bi > 0 && li < 7) { buf[bi] = 0; ZhText(buf, lx, y + 8 + li * 12, 10, UIC::Cream); }
        }
        const char* h = "↑↓ 翻章   Enter 重读   ←→ 翻页";
        ZhText(h, px + (pw - ZhWidth(h, 10)) / 2, py + ph - 15, 10, UIC::Mute);
    }
}

// ---------------- 剧情卷轴（章节 / 结局）：半透明浮层 + 打字机 ----------------
static void DrawScroll() {
    if (!scrollOn) return;
    float k = scrollT < 0.25f ? scrollT / 0.25f : 1.0f;         // 淡入
    unsigned char a = (unsigned char)(235 * k);
    DrawRectangle(0, 0, VW, VH, (Color) { 8, 6, 10, (unsigned char)(205 * k) });
    const int px = 110, py = 74, pw = 420, ph = 212;
    BookFrame(px, py, pw, ph);                                   // 同款符箓框
    // 标题（朱砂 / 鎏金）
    bool isEnd = (scrollKind == 1);
    Color tc = isEnd ? (Color) { 226, 64, 64, a } : (Color) { 216, 212, 200, a };
    const char* title = ScrollTitle();
    int tw = ZhWidth(title, 20);
    ZhText(title, px + (pw - tw) / 2, py + 16, 20, tc);
    DrawRectangle(px + 30, py + 42, pw - 60, 1, (Color) { 139, 105, 20, a });

    // 正文：按已显示字符数截断（打字机）
    const char* full = ScrollText();
    static char line[16][80];
    int lines = 0, ci = 0, bi = 0;
    int shown = scrollChars;
    for (ci = 0; full[ci] && lines < 16; ci++) {
        if (full[ci] == '\n') {
            line[lines][bi] = 0; lines++; bi = 0;
            continue;
        }
        if (shown <= 0) continue;
        if (bi < 78) line[lines][bi++] = full[ci];
        shown--;
    }
    if (bi > 0 && lines < 16) { line[lines][bi] = 0; lines++; }
    for (int i = 0; i < lines; i++) {
        if (!line[i][0]) continue;
        int lw = ZhWidth(line[i], 12);
        ZhText(line[i], px + (pw - lw) / 2, py + 58 + i * 18, 12, (Color) { 216, 212, 200, a });
    }
    // 打完：提示按键
    if (scrollDone) {
        float pl = 0.55f + 0.45f * sinf((float)GetTime() * 4.0f);
        const char* h = isEnd ? "按 Enter 结束这一世" : "按 Enter 收卷";
        ZhText(h, px + (pw - ZhWidth(h, 11)) / 2, py + ph - 22, 11, Fade(UIC::GoldHi, pl));
    }
}

// ---------------- 夜晚光照合成 ----------------
// 经典两遍乘法方案（不依赖多采样器自定义 Shader，规避不同驱动下采样器绑定差异）：
//   1) sceneRT 原样拷入 reflRT；
//   2) lightRT = 环境微光底色 + 加法混合的玩家/篝火圆形光圈；
//   3) 将 lightRT 以 BLEND_MULTIPLIED 叠到场景上 -> RGB 逐通道相乘，亮圈处保持原亮度。
static float LightAmbient(float darkF) {          // 光图底色灰度：夜越深越暗（保留 ~12% 微光）
    return 1.0f - darkF * 0.88f;
}

// ---------------- 天空天体（太阳/月亮/星星/血月，屏幕空间） ----------------

static float celSX = -1, celSY = -1;   // 天体屏幕坐标（供水面倒影 Shader）
static int   celType = 0;              // 0 无 1 太阳 2 月亮 3 血月
#ifdef DEBUG_AUTO_SHOT
static bool  dbgNoCelestial = false;   // A/B 验证：关闭日月/血月光柱倒影
static bool  dbgNoStar = false;        // A/B 验证：单独关闭星辰倒影（与光柱解耦）
#endif

static void DrawSky(float nowT) {
    float p = DayPhase(gameTime);
    celType = 0;
    if (!IsNight(gameTime)) {
        float t = p / 0.6667f;                        // 昼间进度 0..1
        celSX = 46 + (VW - 92) * t;
        celSY = 56 - 34 * sinf(t * 3.14159f);         // 正午最高
        celType = 1;
        bool dusk = p > 0.50f;                        // 黄昏改灰青冷调（取消暖橙夕阳）
        Color rim = dusk ? (Color) { 130, 152, 158, 190 } : (Color) { 168, 196, 205, 185 };
        DrawPixCircle(celSX, celSY, 7.0f, rim);       // 淡青晕
        DrawPixCircle(celSX, celSY, 4.5f, dusk ? (Color) { 205, 214, 216, 215 }
                                               : (Color) { 228, 234, 235, 225 });  // 惨白薄日
    } else {
        float t = (p - 0.6667f) / 0.3333f;            // 夜间进度 0..1
        celSX = 46 + (VW - 92) * t;
        celSY = 50 - 30 * sinf(t * 3.14159f);
        bool blood = IsBloodMoon(gameTime);
        celType = blood ? 3 : 2;
        // 星星：夜越深越亮，格哈希散布 + 各自闪烁（冷白，偶有一颗偏青）
        float darkK = NightDarkness(gameTime) / 228.0f;
        for (int i = 0; i < 44; i++) {
            unsigned h = (unsigned)i * 2654435761u;
            h ^= h >> 13;
            float sxx = (float)(h % (VW - 8)) + 4;
            float syy = (float)((h >> 8) % 190) + 4;
            float tw = 0.55f + 0.45f * sinf(nowT * (2.0f + (h & 7)) + i);
            unsigned char sa = (unsigned char)(darkK * 170 * tw);
            if (sa > 12) {
                Color sc = ((h & 15) == 3) ? (Color) { 170, 235, 220, sa }   // 偶偏青
                                           : (Color) { 218, 226, 240, sa };  // 冷白
                DrawRectangle((int)sxx, (int)syy, 1, 1, sc);
            }
        }
        Color rim = blood ? (Color) { 240, 70, 55, 235 } : (Color) { 226, 232, 240, 225 };  // 幽白冷光
        DrawPixCircle(celSX, celSY, 6.5f, rim);
        if (blood) {
            DrawPixCircle(celSX, celSY, 4.4f, (Color) { 255, 120, 100, 240 });
        } else {                                      // 半月：偏移阴影盘咬出月牙
            DrawPixCircle(celSX + 3.4f, celSY - 1.6f, 5.0f, (Color) { 10, 16, 28, 230 });
        }
        if (blood) DrawRectangle(0, 0, VW, VH, (Color) { 160, 30, 25, (unsigned char)(34 * darkK) });  // 血月红晕
    }
}

// ---------------- 开场叙事卡（纸符样式，首次进入显示） ----------------
// 伏笔保密：只写「一道阴冷的声音救了我」，不透露国王转世 / 公主心鬼设定
static void DrawIntroCard() {
    DrawRectangle(0, 0, VW, VH, (Color) { 14, 4, 8, 235 });       // 暗血红底
    int cw = 300, ch = 190, cx = VW / 2 - cw / 2, cy = VH / 2 - ch / 2;
    DrawRectangle(cx - 2, cy - 2, cw + 4, ch + 4, (Color) { 176, 32, 32, 255 });    // 朱砂细边
    DrawRectangle(cx, cy, cw, ch, (Color) { 240, 236, 224, 255 });                  // 纸白卡
    DrawRectangle(cx, cy, cw, 2, (Color) { 216, 212, 200, 255 });                   // 纸面纹理线
    DrawRectangle(cx, cy + ch - 2, cw, 2, (Color) { 216, 212, 200, 255 });
    // 右上角朱砂印章
    DrawRectangle(cx + cw - 24, cy + 8, 16, 16, (Color) { 204, 48, 48, 230 });
    ZhText("印", cx + cw - 21, cy + 11, 10, (Color) { 240, 236, 224, 255 });
    // 正文（墨黑，竖排感横排）
    static const char* LINES[6] = {
        "大明成化五年，海路断绝，人声俱灭。",
        "再睁眼时，我躺在孤岛滩涂上，四下无一活人。",
        "一道阴冷的声音说：",
        "这世上人人心中都藏着一只鬼。你的心……已被鬼破了。",
        "是那声音救了我。它不肯说名字，只说——",
        "活下去，找到那个破了心的人。",
    };
    int ly = cy + 22;
    for (int i = 0; i < 6; i++) {
        ZhText(LINES[i], cx + 18, ly, 12, (Color) { 26, 29, 36, 255 });
        ly += (i == 1 || i == 3) ? 26 : 22;
    }
    float pulse = 0.55f + 0.45f * sinf((float)GetTime() * 3.0f);
    const char* tip = "—— 按任意键 入岛 ——";
    ZhText(tip, VW / 2 - ZhWidth(tip, 10) / 2, cy + ch - 20, 10,
           Fade((Color) { 107, 101, 96, 255 }, pulse));
}

// ---------------- 新手按键指导（左侧逐条浮入：半透明底 + 像素黑描边，按过即消，P 重唤重演） ----------------
static void DrawTutorial() {
    // 逐条按键提示：只显示未掌握的键，一条接一条从屏幕左缘漂浮进来
    struct TK { const char* key; const char* note; int w; };
    static const TK ROWS[18] = {
        { "W", "上移", 13 }, { "A", "左移", 13 }, { "S", "下移", 13 }, { "D", "右移", 13 },
        { "Shift", "奔跑", 27 }, { "J", "攻击 / 傀儡技能", 13 }, { "E", "交谈", 13 },
        { "Tab", "炼器", 21 }, { "C", "长明灯", 13 }, { "G", "烤肉", 13 },
        { "T", "驯服", 13 }, { "V", "收鬼", 13 }, { "U", "葫芦", 13 }, { "B", "幡册", 13 },
        { "H", "精炼", 13 }, { "Esc", "暂停", 21 }, { "F", "鬼仆册", 13 }, { "R", "傀儡操控", 13 },
    };
    const int x0 = 10, y0 = 30, rh = 19, gap = 3;
    int shown = 0;
    for (int i = 0; i < 18; i++) {
        if (tutMastered[i]) continue;                        // 已掌握：整条隐去
        // 一条接一条浮入：每条比上一条晚 0.32s，0.38s 内滑入 + 淡入
        float t = (tutAnimT - (float)shown * 0.32f) / 0.38f;
        shown++;
        if (t <= 0.0f) continue;                             // 还没轮到它
        if (t > 1.0f) t = 1.0f;
        float e = 1.0f - (1.0f - t) * (1.0f - t);            // ease-out
        unsigned char a = (unsigned char)(255 * e);
        int rx = x0 + (int)((1.0f - e) * -30.0f);            // 从屏外左侧滑入
        int ry = y0 + (shown - 1) * (rh + gap) + (int)((1.0f - e) * -6.0f);  // 微微下飘落位
        int cw = ROWS[i].w + ZhWidth(ROWS[i].note, 8) + 18;
        int ch = rh;
        // 半透明底（隔着也能看见后面的世界）+ 像素黑描边（四边 1px 纯黑，像素风硬边）
        DrawRectangle(rx, ry, cw, ch, (Color) { 14, 17, 24, (unsigned char)(a * 55 / 100) });
        DrawRectangle(rx - 1, ry - 1, cw + 2, 1, (Color) { 4, 5, 8, a });          // 上
        DrawRectangle(rx - 1, ry + ch, cw + 2, 1, (Color) { 4, 5, 8, a });         // 下
        DrawRectangle(rx - 1, ry, 1, ch, (Color) { 4, 5, 8, a });                  // 左
        DrawRectangle(rx + cw, ry, 1, ch, (Color) { 4, 5, 8, a });                 // 右
        // 键帽：像素黑方块 + 1px 高光 + 素白标签
        DrawRectangle(rx + 3, ry + 3, ROWS[i].w, ch - 6, (Color) { 8, 10, 14, (unsigned char)(a * 90 / 100) });
        DrawRectangle(rx + 3, ry + 3, ROWS[i].w, 1, (Color) { 240, 236, 224, (unsigned char)(a * 45 / 100) });
        ZhText(ROWS[i].key, rx + 3 + (ROWS[i].w - ZhWidth(ROWS[i].key, 8)) / 2, ry + 6, 8,
               (Color) { 240, 236, 224, a });
        ZhText(ROWS[i].note, rx + ROWS[i].w + 9, ry + 6, 8, (Color) { 220, 214, 198, a });
    }
    // 底部一行小提示（同样半透明 + 黑描边），全部掌握后整版隐去
    if (shown > 0) {
        const char* hint = "按过即消 · 按 P 重新演示一遍";
        int hw = ZhWidth(hint, 8) + 10;
        int hy = y0 + shown * (rh + gap) + 2;
        float t = (tutAnimT - (float)shown * 0.32f) / 0.38f;
        if (t > 1.0f) t = 1.0f;
        if (t > 0.0f) {
            unsigned char a = (unsigned char)(200 * t);
            DrawRectangle(x0 - 2, hy, hw + 4, 13, (Color) { 14, 17, 24, (unsigned char)(a * 45 / 100) });
            DrawRectangle(x0 - 3, hy - 1, hw + 6, 1, (Color) { 4, 5, 8, a });
            DrawRectangle(x0 - 3, hy + 13, hw + 6, 1, (Color) { 4, 5, 8, a });
            ZhText(hint, x0 + 3, hy + 3, 8, (Color) { 168, 160, 144, a });
        }
    }
}

// ---------------- 渲染 ----------------

// ============================================================
// ---- 开始界面 ----
// ============================================================
static int  titleSel = 0;                     // 0 新游戏 1 继续 2 创建联机 3 加入联机 4 退出
// titleHasSave 定义见联机全局区（文件前部）
static const char* TITLE_ITEM[5] = { "新 游 戏", "继 续 游 戏", "创 建 联 机 房 间", "加 入 联 机 房 间", "退 出 游 戏" };

static void DrawTitleScreen() {
    // 世界画面之上压暗 + 青蓝夜幕
    DrawRectangle(0, 0, VW, VH, (Color) { 5, 8, 16, 206 });
    float tnow = (float)GetTime();
    // 画面底部一缕雾
    for (int k = 0; k < 26; k++) {
        float ph = fmodf(tnow * 0.06f + (float)k * 0.041f, 1.0f);
        int fx = (int)((float)k / 26.0f * VW + sinf(tnow * 0.3f + (float)k) * 12.0f);
        int fy = VH - 26 + (int)(sinf(tnow * 0.5f + (float)k * 1.7f) * 8.0f);
        DrawPixCircle((float)fx, (float)fy, 16.0f + 8.0f * ph, (Color) { 40, 70, 110, 22 });
    }
    // 标题
    const char* t1 = "还 魂 人";
    int tw = ZhWidth(t1, 32);
    float pulse = 0.5f + 0.5f * sinf(tnow * 1.6f);
    DrawRectangle(VW / 2 - tw / 2 - 14, 62, tw + 28, 40, (Color) { 8, 12, 22, 200 });
    ZhText(t1, VW / 2 - tw / 2, 66, 32, (Color) { 232, 216, 176, 255 });
    // 标题下朱砂横线
    DrawRectangle(VW / 2 - tw / 2 - 14, 101, tw + 28, 1,
                  (Color) { 176, 40, 44, (unsigned char)(160 + 90 * pulse) });

    const char* sub = "鬼杀不死，只能被限制";
    ZhText(sub, VW / 2 - ZhWidth(sub, 12) / 2, 112, 12, (Color) { 150, 158, 172, 220 });

    // 加入联机：IP 输入框
    if (titleJoin) {
        int pw = 240, ph = 74, px = VW / 2 - pw / 2, py = 170;
        UIPanel(px, py, pw, ph);
        ZhText("输入房主 IP 后按回车", px + pw / 2 - ZhWidth("输入房主 IP 后按回车", 12) / 2, py + 10, 12, UIC::Cream);
        char buf[24];
        snprintf(buf, sizeof(buf), "%s_", joinIp);
        ZhText(buf, px + pw / 2 - ZhWidth(buf, 14) / 2, py + 32, 14, UIC::GoldHi);
        ZhText("ESC 返回", px + pw / 2 - ZhWidth("ESC 返回", 10) / 2, py + 56, 10, UIC::Mute);
    }
    // 菜单
    for (int i = 0; i < 5; i++) {
        bool sel = (i == titleSel);
        bool dis = (i == 1 && !titleHasSave);
        int iw = 148, ix = VW / 2 - iw / 2, iy = 148 + i * 30;
        DrawRectangle(ix - 2, iy - 2, iw + 4, 26, (Color) { 6, 10, 18, 220 });
        DrawRectangle(ix, iy, iw, 22, sel ? (Color) { 28, 46, 74, 240 } : (Color) { 14, 20, 32, 230 });
        if (sel) DrawRectangle(ix, iy, 3, 22, (Color) { 176, 40, 44, (unsigned char)(180 + 70 * pulse) });
        Color tc = dis ? (Color) { 92, 96, 104, 200 }
                       : (sel ? (Color) { 240, 232, 208, 255 } : (Color) { 186, 190, 200, 255 });
        const char* it = TITLE_ITEM[i];
        ZhText(it, ix + (iw - ZhWidth(it, 12)) / 2, iy + 5, 12, tc);
        if (dis) {
            const char* no = "（无存档）";
            ZhText(no, ix + iw + 8, iy + 6, 10, (Color) { 120, 120, 128, 200 });
        }
    }
    if (titleMsgT > 0.0f) {
        titleMsgT -= GetFrameTime();
        ZhText(titleMsg, VW / 2 - ZhWidth(titleMsg, 12) / 2, 320, 12, (Color) { 226, 90, 80, 255 });
    }
    const char* hint = "W/S 选择    Enter 确认    鼠标可直接点击";
    ZhText(hint, VW / 2 - ZhWidth(hint, 10) / 2, VH - 22, 10, (Color) { 130, 138, 150, 220 });
}

static void Render() {
    float nowT = GetTime();
    playerSailing = CalcSailing();     // 每帧重算（调试传送/上下文切换后立即生效）

    // 相机：跟随 + 屏幕震动（坐标取整保证像素完美）
    float sx = 0, sy = 0;
    if (shakeT > 0 && gOptShake) {
        float k = shakeT / shakeDur;
        sx = (((float)rand() / RAND_MAX) * 2 - 1) * 3.0f * k;
        sy = (((float)rand() / RAND_MAX) * 2 - 1) * 3.0f * k;
    }
    camX = (int)(P.x - VW / 2 + sx);
    camY = (int)(P.y - VH / 2 + sy);
    int worldW = MAP_W * TILE;
    int worldH = MAP_H * TILE;
    if (camX < 0) camX = 0;
    if (camY < 0) camY = 0;
    if (camX > worldW - VW) camX = worldW - VW;
    if (camY > worldH - VH) camY = worldH - VH;

    Camera2D cam;
    cam.offset = { VW / 2.0f, VH / 2.0f };
    cam.target = { camX + VW / 2.0f, camY + VH / 2.0f };
    cam.rotation = 0;
    cam.zoom = 1;

    // ===== 场景层 =====
    BeginTextureMode(sceneRT);
    ClearBackground((Color) { 10, 15, 20, 255 });   // 幽冥青黑底色（#0a0f14，白天也带灰青雾感）
    BeginMode2D(cam);

    // 预渲染地面（只取视口源矩形；地牢为石砖墙地面）
    const Texture2D& gnd = W.CurGround();
    DrawTexturePro(gnd, { (float)camX, (float)camY, VW, VH },
                   { (float)camX, (float)camY, VW, VH }, { 0, 0 }, 0, WHITE);

    // ---- 屋内压暗（立体感：屋内比屋外暗一档，前墙与屋顶才"立"得起来）----
    for (const World::House& h : W.houses) {
        float hx0 = (h.tx - h.w) * 16.0f, hy0 = (h.ty - h.h) * 16.0f;
        float hw2 = (h.w * 2 + 1) * 16.0f, hh2 = (h.h * 2 + 1) * 16.0f;
        if (hx0 + hw2 < camX || hx0 > camX + VW || hy0 + hh2 < camY || hy0 > camY + VH) continue;
        DrawRectangle((int)hx0, (int)hy0, (int)hw2, (int)hh2, (Color) { 10, 12, 20, 40 });
    }

    // 水面统一由 WaterRenderer 在场景层之后整屏绘制（连续水面，无瓦片拼接）

    // ---- y 深度排序绘制（物体 + 生物 + 玩家；按上下文路由）----
    vis.clear();
    std::vector<WorldObj>& cobjs = CurObjs();
    int maxTX = MAP_W - 1;
    int maxTY = MAP_H - 1;
    int tx0 = (camX - 32) / TILE, tx1 = (camX + VW + 32) / TILE;
    int ty0 = (camY - 64) / TILE, ty1 = (camY + VH + 16) / TILE;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    if (tx1 > maxTX) tx1 = maxTX;
    if (ty1 > maxTY) ty1 = maxTY;
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            int idx = W.ObjIndexAt(tx, ty);
            if (idx < 0) continue;
            const WorldObj& o = cobjs[(size_t)idx];
            float syk = o.ty * 16.0f + ((o.kind == ObjKind::Tree || o.kind == ObjKind::Rock) ? 14.0f
                        : (o.kind == ObjKind::Campfire ? 16.0f : 15.0f));
            vis.push_back({ syk, 1, idx });
        }
    for (size_t i = 0; i < mobs.size(); i++) {
        // 视口剔除：远处鬼不进入排序与绘制
        if (mobs[i].state == AState::Dead) continue;
        float mx = mobs[i].x, my = mobs[i].y;
        if (mx < camX - 32 || mx > camX + VW + 32 || my < camY - 32 || my > camY + VH + 32) continue;
        vis.push_back({ my, 2, (int)i });
    }
    if (PET.on) {
        if (PET.x >= camX - 32 && PET.x <= camX + VW + 32 && PET.y >= camY - 32 && PET.y <= camY + VH + 32)
            vis.push_back({ PET.y, 3, 0 });
    }
    for (size_t i = 0; i < ghosts.size(); i++) {
        float gx = ghosts[i].x, gy = ghosts[i].y;
        if (gx < camX - 32 || gx > camX + VW + 32 || gy < camY - 32 || gy > camY + VH + 32) continue;
        vis.push_back({ gy, 4, (int)i });          // 鬼仆
    }
    for (size_t i = 0; i < npcs.size(); i++) {
        const Npc& n = npcs[i];
        if (!n.on) continue;
        if (n.x < camX - 32 || n.x > camX + VW + 32 || n.y < camY - 32 || n.y > camY + VH + 32) continue;
        vis.push_back({ n.y, 5, (int)i });            // NPC（铁匠/幸存者）
    }
    vis.push_back({ P.y, 0, 0 });
    std::sort(vis.begin(), vis.end(),
              [](const VisItem& a, const VisItem& b) { return a.y < b.y; });
    // ---- 鬼域：独立空间的幽蓝底色（画在实体之前 —— 天地被整个换掉，外面的一切都被隔绝）----
    // 注意：本段在 BeginMode2D(cam) 内，坐标是【世界坐标】。曾经错写成 dx = gdCX - camX（屏幕坐标），
    // 结果整个结界被画到屏幕外，肉眼看什么都没有 —— 世界层一律不要减 camX/camY。
    if (gdOn) {
        DrawRectangle(camX, camY, VW, VH, (Color) { 6, 10, 22, 208 }); // 域外：幽蓝墨雾（铺满视口）
        float dx = gdCX, dy = gdCY;
        DrawCircle((int)dx, (int)dy, (int)gdR + 2, (Color) { 10, 18, 38, 255 });
        DrawCircle((int)dx, (int)dy, (int)gdR - 6, (Color) { 16, 28, 56, 255 });
        // 符阵：同心圈 + 放射线（一眼看出"这是个独立的场"）
        for (int rr = 40; rr < (int)gdR; rr += 44)
            DrawCircleLines((int)dx, (int)dy, rr, (Color) { 34, 66, 120, 70 });
        for (int k = 0; k < 12; k++) {
            float a = (float)k * 0.5235988f + nowT * 0.06f;
            DrawRectangle((int)(dx + cosf(a) * 40), (int)(dy + sinf(a) * 40 * 0.72f), 1, 1,
                          (Color) { 60, 110, 180, 90 });
        }
        float pulse = 0.5f + 0.5f * sinf(nowT * 1.6f);
        DrawCircle((int)dx, (int)dy, (int)(gdR * 0.5f),
                   (Color) { 30, 62, 124, (unsigned char)(22 + 16 * pulse) });
    }

    // ---- 普通鬼：被选中范围（以鬼为圆心、合适长度作半径，直接画出来）----
    for (size_t i = 0; i < mobs.size(); i++) {
        const Creature& c = mobs[i];
        if (c.state == AState::Dead || c.companion) continue;
        // 「人」不该有选中范围：玩家与幸存者本就不画；伪装成人形的鬼（蛰伏的无面鬼/所有人）
        // 以及已混进队伍的无面鬼也一律不画 —— 画了等于当众报身份，伪装就破了（无破绽原则）。
        if (c.infiltrated) continue;
        if ((c.kind == CreatureKind::MimicAll || c.kind == CreatureKind::Faceless) &&
            !c.triggered) continue;
        float sr = MobSightRadius(c.kind);
        if (sr <= 0.0f) continue;
        float dx = c.x, dy = c.y;                       // 世界坐标（本段在 BeginMode2D 内）
        if (dx < camX - sr || dx > camX + VW + sr || dy < camY - sr || dy > camY + VH + sr) continue;
        bool hot = c.triggered;
        unsigned char aa = hot ? (unsigned char)(190 + 60 * (0.5f + 0.5f * sinf(nowT * 5.0f)))
                               : (unsigned char)(165 + 40 * (0.5f + 0.5f * sinf(nowT * 2.0f)));
        Color col = hot ? (Color) { 244, 86, 86, aa } : (Color) { 126, 176, 240, aa };
        // 虚线圈（俯视压扁成椭圆）
        // 历史 bug：原来每 24° 只画 2px 一个点（点距 60px+、alpha 只有 40）→ 肉眼完全看不到。
        // 现在按弧长 3px 采样、2 点画 1 点空、点粗 2px → 是条一眼能读出来的虚线椭圆。
        {
            float step = 3.0f / (sr > 1.0f ? sr : 1.0f);
            int n = (int)(6.2831855f / step);
            for (int k = 0; k < n; k++) {
                if (k % 3 == 2) continue;
                float a = (float)k * step;
                int x0 = (int)(dx + cosf(a) * sr);
                int y0 = (int)(dy + sinf(a) * sr * 0.72f);
                DrawRectangle(x0, y0 - 1, 2, c.marked ? 3 : 2, col);
            }
        }
        // 圆心标记（圈到底在圈谁，一眼看得出）
        DrawRectangle((int)dx - 3, (int)dy, 7, 1, col);
        DrawRectangle((int)dx, (int)dy - 3, 1, 7, col);
    }

    for (const VisItem& v : vis) {
        if (v.type == 0) { if (!playerSailing) DrawPlayer();   // 泛舟时推迟到水面层之上
                           DrawRemotePlayers(); }              // 联机：其他玩家（跟本地玩家同层，层序近似）
        else if (v.type == 1) DrawObj(cobjs[(size_t)v.idx], nowT);
        else if (v.type == 3) DrawPet();
        else if (v.type == 4) DrawGhostAlly(ghosts[(size_t)v.idx]);
        else if (v.type == 5) DrawNpc(npcs[(size_t)v.idx]);
        else DrawMob(mobs[(size_t)v.idx]);
    }

    // ---- 屋顶层（遮挡）：盖住屋内一切，玩家进屋时该屋顶渐隐 ----
    DrawHouseRoofs();

    // ---- 鬼域结界（幽蓝）：范围一目了然 / 出不去 / 代理游戏鬼（画在实体之上）----
    // 同样在 BeginMode2D(cam) 内 → 一律用世界坐标（历史 bug：这里减了 camX/camY，结界被画到屏幕外）
    if (gdOn) {
        float dx = gdCX, dy = gdCY;
        float pulse = 0.5f + 0.5f * sinf(nowT * 2.2f);
        // 结界环：多层幽蓝（内亮外淡）→ 加粗成 3px 亮带，确保"一眼看出出不去"
        for (int r = 0; r < 3; r++) {
            float rad = gdR - r * 7.0f;
            DrawCircleLines((int)dx, (int)dy, (int)rad,
                            (Color) { 96, 176, 255, (unsigned char)(160 + 70 * pulse - r * 40) });
        }
        for (int r = -1; r <= 1; r++)                       // 亮带加粗（±1px 各补一圈）
            DrawCircleLines((int)dx, (int)dy, (int)gdR + r, (Color) { 150, 210, 255,
                            (unsigned char)(150 + 80 * pulse) });
        DrawCircleLines((int)dx, (int)dy, (int)gdR + 5, (Color) { 40, 92, 172, 130 });
        // 环上符牌：20 块贴着结界缓缓转动（"这是符阵锁死的场"）
        for (int k = 0; k < 20; k++) {
            float a = (float)k * 0.3141593f + nowT * 0.28f;
            int x0 = (int)(dx + cosf(a) * gdR);
            int y0 = (int)(dy + sinf(a) * gdR * 0.72f);
            unsigned char aa2 = (unsigned char)(150 + 100 * (0.5f + 0.5f * sinf(nowT * 3.0f + (float)k)));
            DrawRectangle(x0 - 2, y0 - 3, 4, 7, (Color) { 26, 54, 112, aa2 });
            DrawRectangle(x0 - 2, y0 - 3, 4, 2, (Color) { 170, 224, 255, aa2 });
        }
        // 域主本体：幽蓝核心环
        for (size_t i = 0; i < mobs.size(); i++) {
            if (mobs[i].kind != CreatureKind::GhostDomain || mobs[i].state == AState::Dead) continue;
            float ox = mobs[i].x, oy = mobs[i].y;
            DrawCircleLines((int)ox, (int)oy, 24 + (int)(5 * pulse), (Color) { 150, 216, 255, 170 });
            DrawPixCircle(ox, oy - 26, 2.0f, (Color) { 190, 235, 255, 220 });
        }
        // 代理游戏鬼：幽蓝鬼影 + 头顶挂着它要玩的游戏名
        for (const GdProxy& pr : gdProxies) {
            float px2 = pr.x, py2 = pr.y;
            if (px2 < camX - 40.0f || px2 > camX + VW + 40.0f ||
                py2 < camY - 40.0f || py2 > camY + VH + 40.0f) continue;
            float bob = sinf(pr.animT * 3.0f) * 2.0f;
            DrawCircleLines((int)px2, (int)(py2 - 4 + bob), pr.active ? 14 : 10,
                            pr.active ? (Color) { 255, 128, 128, 180 } : (Color) { 120, 190, 255, 130 });
            DrawPixCircle(px2, py2 - 6 + bob, 6.0f, (Color) { 34, 62, 118, 205 });
            DrawPixCircle(px2, py2 - 6 + bob, 4.5f, (Color) { 92, 156, 246, 215 });
            DrawPixel((int)px2 - 2, (int)(py2 - 8 + bob), (Color) { 235, 250, 255, 255 });
            DrawPixel((int)px2 + 1, (int)(py2 - 8 + bob), (Color) { 235, 250, 255, 255 });
            const char* tag = GD_GAME_NAME[pr.game];
            ZhText(tag, (int)(px2 - ZhWidth(tag, 10) / 2), (int)(py2 - 24 + bob), 10,
                   (Color) { 176, 218, 255, 255 });
        }
    }

    // ---- 收鬼特效演出（世界空间：牵引束 / 身份金环 / 护主盾环）----
    for (const CapFx& f : capFxs) {
        if (!f.on) continue;
        float k = f.t / f.dur;                        // 0..1 进度
        if (f.kind == 0) {
            // 摄魂牵引束：玩家→目标 的青绿珠链向目标汇聚 + 目标端收口漩涡 + 起手符光
            float dx = f.tx - f.x, dy = f.ty - f.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len < 0.001f) len = 1;
            float ux = dx / len, uy = dy / len;
            constexpr int BEADS = 7;
            for (int i = 0; i < BEADS; i++) {
                float ph = fmodf(k * 1.6f + (float)i / BEADS, 1.0f);   // 各珠错相汇聚
                float d = len * (1.0f - ph);
                float px = f.x + ux * d + sinf(ph * 12.0f + i) * 3.0f * (1.0f - ph);
                float py = (f.y - 6.0f) + uy * d + cosf(ph * 12.0f + i) * 3.0f * (1.0f - ph);
                unsigned char a = (unsigned char)(200 * (1.0f - k * 0.4f));
                DrawPixCircle(px, py, 1.6f + (1.0f - ph) * 1.4f, (Color) { 120, 255, 200, a });
            }
            // 目标端收口漩涡（魂火被拽进幡里的旋转收束）
            float sw = f.t * 10.0f;
            for (int i = 0; i < 3; i++) {
                float a2 = sw + (float)i * 2.094f;
                float r = 8.0f * (1.0f - k * 0.5f);
                DrawPixel((int)(f.tx + cosf(a2) * r), (int)(f.ty - 8.0f + sinf(a2) * r * 0.6f),
                          (Color) { 160, 255, 220, 200 });
            }
            // 起手符光（玩家端符纸张开的白闪，仅前 30%）
            if (k < 0.3f) {
                unsigned char fa = (unsigned char)(180 * (1.0f - k / 0.3f));
                DrawPixCircle(f.x, f.y - 10.0f, 5.0f, (Color) { 220, 255, 240, fa });
            }
        } else if (f.kind == 1) {
            // 身份归位金环：双环扩散 + 符文光点上升（经验从鬼身上被搜出还给你）
            float rr = 6.0f + 30.0f * k;
            unsigned char ra = (unsigned char)(210 * (1.0f - k));
            DrawCircleLines((int)f.x, (int)(f.y - 8), (int)rr, (Color) { 255, 235, 130, ra });
            DrawCircleLines((int)f.x, (int)(f.y - 8), (int)(rr * 0.6f), (Color) { 200, 255, 170, ra });
            for (int i = 0; i < 5; i++) {
                float ph = fmodf(k * 1.4f + (float)i * 0.2f, 1.0f);
                DrawPixCircle(f.x + sinf((float)i * 2.4f + ph * 5.0f) * (10.0f - ph * 6.0f),
                              f.y - 8.0f - ph * 22.0f, 1.4f,
                              (Color) { 255, 240, 160, (unsigned char)(220 * (1.0f - ph)) });
            }
        } else {
            // 护主盾环：青绿双环在玩家周身一闪（自家的鬼替你护住记忆）
            float rr = 14.0f + 10.0f * k;
            unsigned char ra = (unsigned char)(220 * (1.0f - k));
            DrawCircleLines((int)f.x, (int)(f.y - 8), (int)rr, (Color) { 120, 255, 200, ra });
            DrawCircleLines((int)f.x, (int)(f.y - 8), (int)(rr + 4.0f), (Color) { 200, 255, 235, (unsigned char)(ra / 2) });
        }
    }

    // 落叶（空中旋转飘摆，落地渐隐）
    for (const Leaf& L : leaves)
        if (L.on) {
            unsigned char la = (unsigned char)(255 * (L.fade < 1.0f ? L.fade : 1.0f));
            Color lc = ((int)(L.ph * 3.0f) & 1) ? (Color) { 96, 168, 66, la }
                                                : (Color) { 124, 192, 82, la };
            float rot = (L.y < L.ground) ? sinf(L.ph) * 50.0f : 0.0f;   // 度
            DrawRectanglePro({ L.x, L.y, 4, 3 }, { 2, 1.5f }, rot, lc);
        }

    // 攻击弧光
    if (P.atkTime > 0 && !P.dead) {
        float k = P.atkTime / 0.18f;
        float rot = P.dir == 0 ? 90.0f : P.dir == 1 ? 270.0f : P.dir == 2 ? 0.0f : 180.0f;
        DrawTexturePro(A.slashArc, { 0, 0, 56, 56 },
                       { P.x - 12, P.y - 42, 56, 56 }, { 12, 28 }, rot,
                       { 255, 255, 255, (unsigned char)(230 * k) });
    }

    // 掉落物（悬浮动画；按上下文路由）
    for (const Drop& d : CurDrops()) {
        float bob = sinf(d.age * 5.0f) * 1.6f;
        Texture2D t = DropTex(d.kind);
        Color dt = (d.kind == ItemKind::GemShard) ? (Color) { 220, 150, 255, 255 } : WHITE;
        DrawTexturePro(t, { 0, 0, 8, 8 }, { d.x - 4, d.y - 4 + bob, 8, 8 }, { 0, 0 }, 0, dt);
    }

    // 残魂（可收服的魂火：按来源层级着色，漂浮明灭 + 上升光点）
    for (const Soul& s : souls) {
        if (!s.on) continue;
        float bob = sinf(s.age * 2.6f) * 2.0f;
        float pulse = 0.5f + 0.5f * sinf(s.age * 5.0f);
        float fade = s.age > 9.0f ? 1.0f - (s.age - 9.0f) / 3.0f : 1.0f;
        if (fade < 0) fade = 0;
        Color glow, core;
        if (s.tier == 0)      { glow = { 77, 255, 184, 0 };  core = { 200, 255, 235, 0 }; }
        else if (s.tier == 1) { glow = { 102, 204, 255, 0 }; core = { 205, 240, 255, 0 }; }
        else                  { glow = { 190, 130, 255, 0 }; core = { 240, 215, 255, 0 }; }
        glow.a = (unsigned char)((90 + 50 * pulse) * fade);
        core.a = (unsigned char)(235 * fade);
        float sy2 = s.y - 10.0f + bob;
        DrawPixCircle(s.x, sy2, 3.6f + pulse * 1.4f, glow);
        DrawPixCircle(s.x, sy2, 1.8f, core);
        float ph = fmodf(s.age * 0.8f, 1.0f);
        unsigned char pa = (unsigned char)(160 * (1.0f - ph) * fade);
        if (pa > 8)
            DrawPixCircle(s.x + sinf(ph * 6.0f) * 2.0f, sy2 - ph * 14.0f, 1.0f,
                          (Color) { glow.r, glow.g, glow.b, pa });
    }

    // 怪物投射物（酸弹绿 / 火球橙）
    for (const Spit& s : spits) {
        if (!s.on) continue;
        if (s.fire) {
            float pul = 0.5f + 0.5f * sinf((float)GetTime() * 14.0f);
            DrawPixCircle(s.x, s.y, 3.4f + pul, (Color) { 255, 130, 40, 240 });
            DrawPixCircle(s.x - s.vx * 0.02f, s.y - s.vy * 0.02f, 2.0f, (Color) { 255, 220, 120, 190 });
        } else {
            DrawPixCircle(s.x, s.y, 3.2f, (Color) { 150, 225, 80, 235 });
            DrawPixCircle(s.x - s.vx * 0.02f, s.y - s.vy * 0.02f, 1.8f, (Color) { 190, 245, 130, 180 });
        }
    }

    // 玩家箭矢：白色箭头 + 木杆拖尾（沿速度方向）
    for (const Arrow& a : arrows) {
        if (!a.on) continue;
        float l = sqrtf(a.vx * a.vx + a.vy * a.vy);
        if (l < 0.001f) continue;
        float nx = a.vx / l, ny = a.vy / l;
        DrawPixCircle(a.x, a.y, 1.6f, (Color) { 235, 235, 240, 240 });          // 箭头
        DrawPixCircle(a.x - nx * 3.0f, a.y - ny * 3.0f, 1.1f,
                      (Color) { 150, 108, 60, 220 });                           // 杆
        DrawPixCircle(a.x - nx * 6.0f, a.y - ny * 6.0f, 0.8f,
                      (Color) { 150, 108, 60, 140 });                           // 尾
    }

    // 未发现遗迹的指引箭头（环绕主角旋转指向最近目标，400px 内显示）
    {
        int nearest = -1;
        float nd = 400.0f * 400.0f;
        for (size_t i = 0; i < W.ruins.size() && i < 4; i++) {
            if (ruinFound[i]) continue;
            float dx = W.ruins[i].x - P.x, dy = W.ruins[i].y - P.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < nd) { nd = d2; nearest = (int)i; }
        }
        if (nearest >= 0) {
            float ang = atan2f(W.ruins[nearest].y - P.y, W.ruins[nearest].x - P.x) * 57.29578f;
            float bobA = sinf(arrowPh * 4.0f) * 5.0f;
            DrawTexturePro(A.arrowHint, { 0, 0, 12, 12 },
                           { P.x - 6, P.y - 6 - 34 - bobA, 12, 12 },
                           { 6, 18 }, ang + 90.0f,
                           { 255, 240, 120, (unsigned char)(150 + 60 * sinf(arrowPh * 5.0f)) });
        }
    }

    // 粒子
    for (const Particle& p : parts) {
        unsigned char a = (unsigned char)(255 * p.life / p.maxLife);
        DrawRectangle((int)p.x, (int)p.y, 2, 2, (Color) { p.r, p.g, p.b, a });
    }

    // 飘字
    for (const DmgText& d : dmgs) {
        unsigned char a = (unsigned char)(255 * d.life / 0.8f);
        // 弹出手感：出现前 0.15s 用大号字（14），落定后回到 10 —— 伤害数字有"砸出来"的分量
        float age = 0.8f - d.life;
        int sz = (age < 0.15f) ? 14 : 10;
        ZhText(d.txt, (int)d.x - ZhWidth(d.txt, sz) / 2, (int)d.y, sz, (Color) { d.r, d.g, d.b, a });
    }

    EndMode2D();

    // ---- 天空天体（太阳/月亮/星星/血月；屏幕空间覆盖，地牢无天空）----
    DrawSky(nowT);

    EndTextureMode();
    float warmK = DayWarm(gameTime);
    float nightK = NightDarkness(gameTime) / 228.0f;

    // ===== 水面：连续水面（墨青冷调 + 云状暗斑 + 短划/冷点 + 倒影 + 雨天涟漪）=====
    // 地牢无水面
    float objArr[WaterRenderer::MAX_OBJECTS * 4] = {};
    int nObj = CollectReflectObjects(objArr);
    // 天体倒影：DrawSky 已写入 celSX/celSY/celType（屏幕空间）。
    // 按天体类型给出光柱颜色与强度 —— 天空换日/月/血月，水面倒影同步改变。
    float cel[4]    = { celSX, celSY, (float)celType, 0.0f };
    float celCol[3] = { 1.0f, 1.0f, 1.0f };
    if (celType == 1) {                        // 惨白薄日：日月倒影统一冷色（取消暖橙）
        celCol[0] = 0.82f;
        celCol[1] = 0.90f;
        celCol[2] = 1.0f;
        cel[3]    = 0.85f;
    } else if (celType == 2) {                 // 半月：幽白偏蓝（月光柔弱但仍是夜水面唯一亮源）
        celCol[0] = 0.72f; celCol[1] = 0.80f; celCol[2] = 1.0f;
        cel[3]    = 0.78f;
    } else if (celType == 3) {                 // 血月：破碎暗红，强度居中
        celCol[0] = 1.0f; celCol[1] = 0.35f; celCol[2] = 0.28f;
        cel[3]    = 0.88f;
    }
    // 星辰倒影：仅夜间，夜越深越亮；白昼为 0
    float starTw = (celType >= 2) ? nightK * 0.95f : 0.0f;
#ifdef DEBUG_AUTO_SHOT
    if (dbgNoCelestial) cel[3] = 0.0f;    // 只关光柱
    if (dbgNoStar)      starTw = 0.0f;    // 只关星辰（与光柱解耦，便于分离验证）
#endif
    WATER.Draw(sceneRT, reflRT, camX, camY, VW, VH, nowT, warmK, nightK,
               rainAmt, objArr, nObj, cel, celCol, starTw);

    // 泛舟补绘：水面覆盖场景层，玩家与小舟在水面之上重画一次
    if (playerSailing) {
        BeginTextureMode(sceneRT);
        BeginMode2D(cam);
        DrawPlayer();
        EndMode2D();
        EndTextureMode();
    }

    // ===== 黄昏灰青冷调（乘法叠加，随 DayWarm 平滑渐入渐出；取消暖橙夕阳）=====
    float warm = warmK;
    if (warm > 0.01f) {
        BeginTextureMode(sceneRT);
        BeginBlendMode(BLEND_MULTIPLIED);
        DrawRectangle(0, 0, VW, VH, (Color) { (unsigned char)(255 - 95 * warm),
                      (unsigned char)(255 - 40 * warm),
                      (unsigned char)(255 - 8 * warm), 255 });
        EndBlendMode();
        EndTextureMode();
    }

    // ===== 雨幕（画在光照前：雨丝同样被夜色压暗）=====
    if (rainAmt > 0.02f) {
        BeginTextureMode(sceneRT);
        int rn = (int)(150 * rainAmt);
        Color rcol = (Color) { 168, 198, 232, 135 };
        for (int i = 0; i < rn; i++) {
            const RainDrop& d = rainDrops[i];
            for (int k = 0; k < 4; k++)              // 4px 斜向雨丝
                DrawRectangle((int)(d.x - k * 1.5f), (int)(d.y - k * 3.0f), 1, 2, rcol);
        }
        DrawRectangle(0, 0, VW, VH, (Color) { 40, 55, 80, (unsigned char)(46 * rainAmt) });   // 雨天冷色
        EndTextureMode();
    }

    // ===== 受击红边闪 =====
    BeginTextureMode(sceneRT);
    if (hurtVin > 0)
        DrawRectangle(0, 0, VW, VH, (Color) { 190, 30, 30, (unsigned char)(hurtVin * 0.45f * 255) });
    EndTextureMode();

    // ===== 光照贴图（环境底色 + 加法混合圆形光圈：白=全亮，灰=夜色微光）=====
    float darkF = nightK;                            // 0..1
    // 萤晶茶夜视 buff：视野暗度大幅降低（夜视 90 秒）
    if (P.buffNightT > 0) darkF *= 0.35f;
#ifdef DEBUG_AUTO_SHOT
    { static int rc = 0; if (++rc % 120 == 0)
        TraceLog(LOG_INFO, "DBG-R: darkF=%.3f fires=%zu mobs=%zu", darkF,
                 W.campfires.size(), mobs.size()); }
#endif
    BeginTextureMode(lightRT);
    {
        unsigned char amb = (unsigned char)(255 * LightAmbient(darkF) + 0.5f);
        // 环境微光底色：白昼保持全亮，夜色沉入深青黑（#06090d 系）而非纯黑
        float lk = (amb - 31.0f) / 224.0f;
        if (lk < 0.0f) lk = 0.0f; else if (lk > 1.0f) lk = 1.0f;
        ClearBackground((Color) { (unsigned char)(6 + lk * 249),
                                  (unsigned char)(9 + lk * 246),
                                  (unsigned char)(13 + lk * 242), 255 });
        if (darkF > 0.02f) {
            BeginBlendMode(BLEND_ADDITIVE);
            BeginMode2D(cam);
            // 玩家照明：以主角为中心的圆形光圈，半径随等级成长
            // Lv1 ≈ 68px，每级 +12px，上限 160px
            float lr = 68.0f + 12.0f * (P.level - 1) + (gLantern ? 46.0f : 0.0f);   // 引魂灯：自带光环
            if (lr > 160.0f) lr = 160.0f;
            float flick = 1.0f + 0.03f * sinf(nowT * 8.0f) * sinf(nowT * 3.3f);   // 呼吸闪烁
            DrawTexturePro(A.lightGrad, { 0, 0, 128, 128 },
                           { P.x - 6 - lr * flick, (P.y - 8) - lr * flick,
                             lr * 2 * flick, lr * 2 * flick }, { 0, 0 }, 0, WHITE);
            // 鬼仆魂火光圈（青绿小光晕：收鬼系统，夜间自带照明）
            for (size_t i = 0; i < ghosts.size(); i++) {
                const GhostAlly& g = ghosts[i];
                float fr = 26.0f * (1.0f + 0.08f * sinf(nowT * 9.0f + (float)i * 2.1f));
                DrawTexturePro(A.lightGrad, { 0, 0, 128, 128 },
                               { g.x - fr, (g.y - 6) - fr, fr * 2, fr * 2 },
                               { 0, 0 }, 0, (Color) { 140, 255, 210, 150 });
            }
            // 残魂微光（弱光点，标记可收之魂）
            for (const Soul& s : souls) {
                if (!s.on) continue;
                float sr = 16.0f + 3.0f * sinf(nowT * 5.0f + s.x * 0.1f);
                Color sc = s.tier == 0 ? (Color) { 120, 255, 200, 110 }
                        : s.tier == 1 ? (Color) { 130, 220, 255, 110 }
                                      : (Color) { 210, 160, 255, 110 };
                DrawTexturePro(A.lightGrad, { 0, 0, 128, 128 },
                               { s.x - sr, (s.y - 10) - sr, sr * 2, sr * 2 },
                               { 0, 0 }, 0, sc);
            }
            // 篝火光圈（闪烁，地表专属）
            for (int fi : W.campfires) {
                const WorldObj& o = W.objs[(size_t)fi];
                float fr = 88.0f * (1.0f + 0.06f * sinf(nowT * 13.0f + fi));
                DrawTexturePro(A.lightGrad, { 0, 0, 128, 128 },
                               { o.tx * 16 + 8 - fr, o.ty * 16 + 8 - fr, fr * 2, fr * 2 },
                               { 0, 0 }, 0, WHITE);
            }
        }
    }
    EndTextureMode();

    // ===== 场景 × 光照 合成（乘法混合 -> reflRT），UI 同目标绘制 =====
    BeginTextureMode(reflRT);
    ClearBackground(BLACK);
    DrawTexturePro(sceneRT.texture, { 0, 0, VW, -VH }, { 0, 0, VW, VH }, { 0, 0 }, 0, WHITE);
    if (darkF > 0.02f) {
        BeginBlendMode(BLEND_MULTIPLIED);
        DrawTexturePro(lightRT.texture, { 0, 0, VW, -VH }, { 0, 0, VW, VH }, { 0, 0 }, 0, WHITE);
        EndBlendMode();
        // 夜晚冷色调罩层（近似原 shader 的偏蓝处理）
        unsigned char coldA = (unsigned char)(70 * darkF + 0.5f);
        DrawRectangle(0, 0, VW, VH, (Color) { 34, 50, 105, coldA });
    }

    // ===== UI（画在合成结果之上）=====
    if (gs == GS::Title) {
        DrawTitleScreen();                       // 开始界面（盖在世界画面之上）
    } else {
        DrawUI();
        GdDrawHud();                             // 鬼域横幅 + 并行小游戏卡片
    }

    // ===== 开场叙事卡 / 新手按键指导（最顶层覆盖）=====
    if (introOn) DrawIntroCard();
    else if (tutOn && gs == GS::Play && !craftOpen && !hotbarOpen) {
        DrawTutorial();
    }
    DrawBook();                                       // 手册总册（L）：命途/成就/图鉴/残卷
    DrawScroll();                                     // 剧情卷轴（章节/结局，最上层）
    EndTextureMode();

#ifdef DEBUG_AUTO_SHOT
    // 联机/新 UI 验证导出：reflRT 此刻已含全部 UI 层（标题/暂停/游戏都走这里）
    if (fno == 1774) { Image img = LoadImageFromTexture(reflRT.texture);
                       ImageFlipVertical(&img);
                       ExportImage(img, "shot_i_title.png"); UnloadImage(img); }
    if (fno == 1777) { Image img = LoadImageFromTexture(reflRT.texture);
                       ImageFlipVertical(&img);
                       ExportImage(img, "shot_i_join.png"); UnloadImage(img); }
    if (fno == 1786) { Image img = LoadImageFromTexture(reflRT.texture);
                       ImageFlipVertical(&img);
                       ExportImage(img, "shot_i_bounty.png"); UnloadImage(img); }
    if (fno == 1789) { Image img = LoadImageFromTexture(reflRT.texture);
                       ImageFlipVertical(&img);
                       ExportImage(img, "shot_i_pause.png"); UnloadImage(img); }
#endif

    // ===== 主窗口：整数倍放大（按实际客户区自适应 + 信箱居中，窗口缩小时不再裁剪画面）=====
#ifdef DEBUG_HEADLESS
    // 无头验证模式：不做窗口 blit（避免最小化遮挡时驱动交换阻塞），直接从纹理导出
#else
    BeginDrawing();
    ClearBackground(BLACK);
    {
        int sw = GetScreenWidth(), sh = GetScreenHeight();
        int sc = (sw / VW < sh / VH) ? sw / VW : sh / VH;
        if (sc < 1) sc = 1;
        float dw = (float)(VW * sc), dh = (float)(VH * sc);
        DrawTexturePro(reflRT.texture, { 0, 0, VW, -VH },
                       { (sw - dw) / 2.0f, (sh - dh) / 2.0f, dw, dh }, { 0, 0 }, 0, WHITE);
    }
    Zh_Flush();                                  // 帧末补字：把本帧遇到的缺字一次性烘进图集
    EndDrawing();
#endif
}

// ---------------- 新游戏 ----------------

void NewGame(unsigned seed) {
    gSeed = seed;
    W.Generate(seed, A);
    WATER.BakeMask(W);       // 烧录岸线 Chamfer 距离场 -> 连续灰度遮罩（POINT，随地图尺寸自适应）
    P = Player();
    P.x = W.spawn.x;
    P.y = W.spawn.y;
    InitCreatures(mobs, W, P.x, P.y, gameTime);

    // ---- POI 鬼放置（固定驻点：世界生成时埋好的鬼）----
    {
        // 鬼域核心（鬼游戏：第一个乱葬岗中心，区域即鬼）
        if (W.domainPos.x > 0.0f) {
            Creature c;
            c.kind = CreatureKind::GhostDomain;
            c.x = W.domainPos.x; c.y = W.domainPos.y;
            c.hp = MobBaseHp(c.kind);
            c.state = AState::Idle;
            mobs.push_back(c);
        }
        // 闹鬼屋：潜伏鬼（「所有人」伪装屋主 / 无脸鬼伪装路人）
        // —— 放在屋外门口而不是屋内：屋顶会盖住屋内，玩家根本看不见（旧的"偶尔不可见"根因之一）
        for (const World::House& h : W.houses) {
            if (!h.haunted) continue;
            Creature g;
            g.kind = (rand() & 1) ? CreatureKind::MimicAll : CreatureKind::Faceless;
            float a = (float)(rand() % 360) * 0.0174533f;
            g.x = h.Center().x + cosf(a) * 26.0f;
            g.y = h.Center().y + (float)(h.h * 16 + 20) + sinf(a) * 10.0f;
            if (!W.CircleFree(g.x, g.y, 7.0f)) { g.x = h.Center().x; g.y = h.Center().y + h.h * 16 + 20; }
            g.hp = MobBaseHp(g.kind);
            g.state = AState::Idle;
            mobs.push_back(g);
        }
        // 野外游荡的规则鬼：房屋与出生点之外散布 10 只（鬼不该全躲在屋里）
        {
            static const CreatureKind WP[6] = { CreatureKind::GhostChild, CreatureKind::GhostTeen,
                                                CreatureKind::Faceless,   CreatureKind::ManyFaces,
                                                CreatureKind::MimicAll,   CreatureKind::NineFace };
            int placedW = 0;
            for (int attempt = 0; attempt < 1200 && placedW < 10; attempt++) {
                float x = 32.0f + (float)(rand() % (MAP_W * 16 - 64));
                float y = 32.0f + (float)(rand() % (MAP_H * 16 - 64));
                float dxs = x - W.spawn.x, dys = y - W.spawn.y;
                if (dxs * dxs + dys * dys < 300.0f * 300.0f) continue;    // 离出生点远些
                bool nearHouse = false;
                for (const World::House& h : W.houses) {
                    float hx = x - h.Center().x, hy = y - h.Center().y;
                    if (hx * hx + hy * hy < 90.0f * 90.0f) { nearHouse = true; break; }
                }
                if (nearHouse) continue;                                  // 别贴着屋子（要和屋内鬼区分开）
                if (!W.CircleFree(x, y, 8.0f)) continue;
                Creature c;
                c.kind = WP[rand() % 6];
                c.x = x; c.y = y;
                c.hp = MobBaseHp(c.kind);
                c.tier = (rand() % 100 < 30) ? 1 : 0;
                c.dmgMul = 1.0f + 0.1f * c.tier;
                c.state = AState::Idle;
                c.timer = (float)(rand() % 200) / 100.0f;
                RollGhostRule(c);
                mobs.push_back(c);
                placedW++;
            }
        }
        // 野外铁匠鬼：守在废弃铁匠铺门外（旧版在屋内被屋顶盖住 → 看不见）
        if (W.smithGhostPos.x > 0.0f) {
            Creature c;
            c.kind = CreatureKind::SmithGhost;
            c.x = W.smithGhostPos.x; c.y = W.smithGhostPos.y;
            c.hp = MobBaseHp(c.kind);
            c.state = AState::Idle;
            mobs.push_back(c);
        }
        // 伴生铁匠鬼：从开局起就飘在玩家身侧（永远看得见，不会死、不会散）
        {
            Creature c;
            c.kind = CreatureKind::SmithGhost;
            c.companion = true;
            c.x = P.x + 26.0f; c.y = P.y - 34.0f;     // 开局就悬在玩家侧上方（右肩斜上）
            c.hp = MobBaseHp(c.kind);
            c.state = AState::Idle;
            mobs.push_back(c);
        }
        // 其余乱葬岗：游荡的规则鬼（每个岗 2 只）
        for (size_t gi = 1; gi < W.graveyards.size(); gi++) {
            const Vector2& g = W.graveyards[gi];
            static const CreatureKind pool[4] = { CreatureKind::GhostTeen, CreatureKind::ManyFaces,
                                                  CreatureKind::NineFace, CreatureKind::GhostChild };
            for (int k = 0; k < 2; k++) {
                Creature c;
                c.kind = pool[rand() % 4];
                float a = (float)(rand() % 360) * 0.0174533f;
                float r = 20.0f + (float)(rand() % 60);
                c.x = g.x + cosf(a) * r; c.y = g.y + sinf(a) * r;
                if (!W.CircleFree(c.x, c.y, 8.0f)) { c.x = g.x; c.y = g.y; }
                c.hp = MobBaseHp(c.kind);
                c.state = AState::Idle;
                mobs.push_back(c);
            }
        }
    }

    // ---- NPC：铁匠（1）+ 幸存者（5，随机职业，分布野外；不显示名字——真假自己分辨）----
    npcs.clear();
    {
        Npc smith;
        smith.kind = 0; smith.job = 0; smith.state = 0;
        smith.x = W.smithPos.x; smith.y = W.smithPos.y;
        npcs.push_back(smith);
        int made = 0;
        for (int attempt = 0; attempt < 400 && made < 5; attempt++) {
            int tx = 6 + rand() % (MAP_W - 12);
            int ty = 6 + rand() % (MAP_H - 12);
            if (W.TileAt(tx, ty) == Tile::Water) continue;
            float x = tx * 16.0f + 8.0f, y = ty * 16.0f + 8.0f;
            if (!W.CircleFree(x, y, 6.0f)) continue;
            float dxs = x - P.x, dys = y - P.y;
            if (dxs * dxs + dys * dys < 150.0f * 150.0f) continue;   // 别贴出生点
            Npc n;
            n.kind = 1; n.job = made % 5; n.state = 0;   // 职业齐全（医师/猎人/农夫/工匠/哨兵）
            n.x = x; n.y = y;
            npcs.push_back(n);
            made++;
        }
    }

    parts.clear();
    dmgs.clear();
    for (Spit& s : spits) s.on = false;      // 酸弹清空
    for (Arrow& a : arrows) a.on = false;    // 箭矢清空
    for (Soul& s : souls) s.on = false;      // 残魂清空
    for (CapFx& f : capFxs) f.on = false;    // 收鬼特效清空
    ghosts.clear();                          // 鬼仆清空
    gourd.clear();                           // 葫芦清空
    banished.clear();                        // 被击溃的鬼清空
    ghostBanished = 0;
    P.swordLv = 0; P.gourdLv = 0; P.captureLv = 0;
    P.shieldT = 0.0f;
    smithOpen = false; smithIdx = -1; forgeLv = 1; smithMsgT = 0; smithMsg[0] = 0;
    smithOwned = false; ghostBarPage = 0; gourdCd = 0.0f;
    objActive.clear();                       // 活跃物体表清空
    PET.on = false;                          // 宠物重置
    for (bool& f : ruinFound) f = false;     // 遗迹发现重置
    for (Achv& a : achvs) a.got = false;     // 成就重置
    achvShowIdx = -1; achvShowT = 0;
    Prog::Reset();                          // 元系统：命途/成就/图鉴/残影全清（新的一世）
    gPuppetMax = 0; gBanished = 0; gGraveDug = 0; gBowKill = false; gStalkSurvived = false;
    gBountyDay = -1; gNCook = 0; gNCraft = 0;
    gGlowRain = false; gTide = false; gTideT = 0; gWasNight = false;
    for (int i = 0; i < 3; i++) { gBounty[i].type = 0; gBounty[i].need = 1; gBounty[i].base = 0; gBounty[i].done = 0; }
    bookOpen = false; bookPage = 0; bookSel = 0;
    scrollOn = false; scrollChars = 0; scrollDone = false;
    craftOpen = false;
    craftSel = 0; craftAnimOn = false; craftAnimT = 0;
    hotbarOpen = false; hotSel = 0;
    puppetOn = false; puppetMain = -1; puppetGrp.clear(); puppetMx = 0.0f; puppetMy = 0.0f;
    gameTime = 0; deathTime = 0;
    hitStop = 0; shakeT = 0; hurtVin = 0;
    hungerT = 0; starveT = 0; regenT = 0; healT = 0;
    // ---- 以下字段此前遗漏：重开后残留会把上一局的状态带进新档 ----
    bossSlain = false;                       // 击杀成就标记
    bossHowlT = 0; bossHowlTxt[0] = '\0';    // 招式字幕
    ResetCreatureSpawnState();               // 清空 creature 模块的重生记录与补怪计时
    // ---- 新系统状态重置 ----
    rpsState = 0; rpsGhostIdx = -1; rpsDraws = 0; rpsHintT = 0; rpsHintTxt = "";
    dealState = 0; dealGhostIdx = -1;
    campBuilt = false; campLv = 1; campCenter = {}; dayCampMark = -1;
    // ---- 领地 / 同伴 / 无面鬼同化 重置 ----
    gCompTotal = 0; gCompFace = 0;
    askLeft = 2; askDay = -1; askType = -1; askIdx = -1;
    askWinT = 0; askLineT = 0; askLine[0] = 0;
    smithMet = false; smithOwned = false;
    gSmithGhostLv = 1; gSmithCompanionIdx = -1;
    campHealT = 0;
    // ---- 大鬼域重置 ----
    gdOn = false; gdCX = 0; gdCY = 0; gdR = 200.0f; gdT = 0; gdOwnerIdx = -1;
    gdSpawnT = 0.0f; gdCap = 1; gdProxyN = 0; gdInfoT = 0; gdInfo[0] = 0; gdSealT = 0;
    gdProxies.clear();
    for (GdChallenge& gch : gdChal) gch.on = false;
    gdWin = 0; gdFail = 0;
    domainPurged = false;
    captureCd = 0; playerAttackedFrame = false; bannerOpen = false;
    knockT = 25.0f; knockSummonT = 0.0f;
    gs = GS::Play;
#ifdef DEBUG_SHOT_NIGHT
    gameTime = DAY_LEN * 0.75f;      // 调试：直接进入深夜验证光圈
#endif
#ifdef DEBUG_SHOT_CRAFT
    P.iron = 5; P.crystal = 2; P.berry = 9; P.wood = 8;
    craftOpen = true;                // 调试：打开合成面板
#endif
}

// ============================================================
// ---- 存档：二进制全量快照 ----
// 世界地形由 seed 确定性重建；物体改动、实体、玩家、旗标全部落盘。
// ============================================================
static const char* SAVE_PATH = "save.dat";
static const unsigned SAVE_MAGIC = 0x56535750u;   // "PWSV"
static const unsigned SAVE_VER   = 5u;   // v4：+ 每日悬赏/计数/选项；v5：+ Creature.netId/netTx/netTy、Drop.netId（联机同步）
static bool SaveExists() { return FileExists(SAVE_PATH); }

// ---- 语言偏好：独立小文件（不写进存档 —— 新游戏/删档都保持玩家的语言选择）----
static const char* LANG_PATH = "lang.cfg";
static void SaveLangPref() {
    FILE* f = fopen(LANG_PATH, "wb");
    if (!f) return;
    const char* v = gL10nEn ? "en" : "zh";
    fwrite(v, 1, 2, f);
    fclose(f);
}
static void LoadLangPref() {
    FILE* f = fopen(LANG_PATH, "rb");
    if (!f) { gL10nEn = false; return; }            // 无配置：默认中文
    char b[8] = {};
    size_t n = fread(b, 1, 7, f);
    fclose(f);
    gL10nEn = (n >= 2 && b[0] == 'e' && b[1] == 'n');
}
static void ApplyWindowTitle() {                    // 窗口标题跟随语言
    SetWindowTitle(gL10nEn ? "Huanhun Ren" : "Huanhun Ren - 还魂人");
}
static bool gQuitNoSave = false;      // 从开始界面直接退出：不覆盖存档
// lastSaveDay 定义见联机全局区（文件前部）

static void WrRaw(FILE* f, const void* p, size_t n) { if (n) fwrite(p, 1, n, f); }
static void RdRaw(FILE* f, void* p, size_t n) { if (n) { size_t got = fread(p, 1, n, f); (void)got; } }
// 读向量前先校验"剩余字节数够不够"：损坏/过期的存档曾在这里按垃圾长度 resize → 直接闪退
static long RdRemain(FILE* f) {
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long rem = ftell(f) - cur;
    fseek(f, cur, SEEK_SET);
    return rem;
}
#define SAVE_VEC(V, T) do { unsigned _n = (unsigned)(V).size(); WrRaw(f, &_n, sizeof(_n)); \
                            if (_n) WrRaw(f, (V).data(), (size_t)_n * sizeof(T)); } while (0)
#define LOAD_VEC(V, T) do { unsigned _n = 0; RdRaw(f, &_n, sizeof(_n)); (V).clear(); \
                            if ((long)_n * (long)sizeof(T) > RdRemain(f)) { fclose(f); return false; } \
                            if (_n) { (V).resize(_n); RdRaw(f, (V).data(), (size_t)_n * sizeof(T)); } } while (0)

static bool SaveGame() {
    if (netOn && !netHost) return true;      // 客人无世界所有权，不覆盖房主存档
    FILE* f = fopen(SAVE_PATH, "wb");
    if (!f) return false;
    unsigned magic = SAVE_MAGIC, ver = SAVE_VER;
    WrRaw(f, &magic, sizeof(magic)); WrRaw(f, &ver, sizeof(ver));
    unsigned sd = (unsigned)gSeed;
    WrRaw(f, &sd, sizeof(sd));
    WrRaw(f, &gameTime, sizeof(gameTime));
    WrRaw(f, &P, sizeof(P));                        // Player：全 POD
    WrRaw(f, &PET, sizeof(PET));
    SAVE_VEC(W.objs, WorldObj);
    SAVE_VEC(W.objAt, int);
    SAVE_VEC(W.campfires, int);
    { unsigned n = (unsigned)W.houses.size(); WrRaw(f, &n, sizeof(n));
      for (const World::House& h : W.houses) WrRaw(f, &h.roofA, sizeof(h.roofA)); }
    SAVE_VEC(W.drops, Drop);
    SAVE_VEC(mobs, Creature);
    SAVE_VEC(npcs, Npc);
    SAVE_VEC(ghosts, GhostAlly);
    SAVE_VEC(gourd, StoredGhost);
    for (int i = 0; i < 8; i++) WrRaw(f, &souls[i], sizeof(Soul));
    unsigned char fl[5] = { (unsigned char)campBuilt, (unsigned char)smithMet, (unsigned char)smithOwned,
                            (unsigned char)domainPurged, (unsigned char)bossSlain };
    WrRaw(f, fl, 5);
    int ic[6] = { campLv, dayCampMark, gSmithGhostLv, forgeLv, ghostBanished, gdWin };
    WrRaw(f, ic, sizeof(ic));
    WrRaw(f, &campCenter, sizeof(campCenter));
    float fc[4] = { bossHowlT, knockT, knockSummonT, rpsHintT };
    WrRaw(f, fc, sizeof(fc));
    unsigned char rf[4]; for (int i = 0; i < 4; i++) rf[i] = ruinFound[i] ? 1 : 0;
    WrRaw(f, rf, 4);
    unsigned char ag[11]; for (int i = 0; i < 11; i++) ag[i] = achvs[i].got ? 1 : 0;
    WrRaw(f, ag, 11);
    { const ProgSave& ps = Prog::Save(); WrRaw(f, &ps, sizeof(ps)); }   // 元系统：命途/成就/图鉴/残影
    WrRaw(f, gBounty, sizeof(gBounty));                 // 每日悬赏
    WrRaw(f, &gBountyDay, sizeof(gBountyDay));
    WrRaw(f, &gNCook, sizeof(gNCook));
    WrRaw(f, &gNCraft, sizeof(gNCraft));
    { unsigned char op[2] = { (unsigned char)gOptDmg, (unsigned char)gOptShake };
      WrRaw(f, op, sizeof(op)); }
    WrRaw(f, &gOptMaster, sizeof(gOptMaster));
    fclose(f);
    return true;
}

static bool LoadGame() {
    FILE* f = fopen(SAVE_PATH, "rb");
    if (!f) return false;
    unsigned magic = 0, ver = 0;
    RdRaw(f, &magic, sizeof(magic)); RdRaw(f, &ver, sizeof(ver));
    if (magic != SAVE_MAGIC || ver != SAVE_VER) { fclose(f); return false; }
    unsigned sd = 0;
    RdRaw(f, &sd, sizeof(sd));
    NewGame(sd);                                    // 用同一 seed 重建世界与派生数据
    RdRaw(f, &gameTime, sizeof(gameTime));
    RdRaw(f, &P, sizeof(P));
    RdRaw(f, &PET, sizeof(PET));
    LOAD_VEC(W.objs, WorldObj);
    LOAD_VEC(W.objAt, int);
    LOAD_VEC(W.campfires, int);
    { unsigned n = 0; RdRaw(f, &n, sizeof(n));
      for (unsigned i = 0; i < n && i < W.houses.size(); i++)
          RdRaw(f, &W.houses[i].roofA, sizeof(float)); }
    LOAD_VEC(W.drops, Drop);
    LOAD_VEC(mobs, Creature);
    LOAD_VEC(npcs, Npc);
    LOAD_VEC(ghosts, GhostAlly);
    LOAD_VEC(gourd, StoredGhost);
    for (int i = 0; i < 8; i++) RdRaw(f, &souls[i], sizeof(Soul));
    unsigned char fl[5] = {};
    RdRaw(f, fl, 5);
    campBuilt = fl[0] != 0; smithMet = fl[1] != 0; smithOwned = fl[2] != 0;
    domainPurged = fl[3] != 0; bossSlain = fl[4] != 0;
    int ic[6] = {};
    RdRaw(f, ic, sizeof(ic));
    campLv = ic[0]; dayCampMark = ic[1]; gSmithGhostLv = ic[2]; forgeLv = ic[3];
    ghostBanished = ic[4]; gdWin = ic[5];
    RdRaw(f, &campCenter, sizeof(campCenter));
    float fc[4] = {};
    RdRaw(f, fc, sizeof(fc));
    bossHowlT = fc[0]; knockT = fc[1]; knockSummonT = fc[2]; rpsHintT = fc[3];
    unsigned char rf[4] = {};
    RdRaw(f, rf, 4);
    for (int i = 0; i < 4; i++) ruinFound[i] = rf[i] != 0;
    unsigned char ag[11] = {};
    RdRaw(f, ag, 11);
    for (int i = 0; i < 11; i++) achvs[i].got = ag[i] != 0;
    {   // 元系统：旧档没有这一段 —— 读不到就保持初始状态，绝不因旧档崩
        ProgSave ps;
        if (RdRemain(f) >= (long)sizeof(ProgSave)) RdRaw(f, &ps, sizeof(ps));
        Prog::Load(ps);
    }
    {   // 每日悬赏 / 选项（读不到就保持默认，绝不因缺段崩）
        long needSz = (long)(sizeof(gBounty) + sizeof(gBountyDay) + sizeof(gNCook) +
                             sizeof(gNCraft) + 2 + sizeof(gOptMaster));
        if (RdRemain(f) >= needSz) {
            RdRaw(f, gBounty, sizeof(gBounty));
            RdRaw(f, &gBountyDay, sizeof(gBountyDay));
            RdRaw(f, &gNCook, sizeof(gNCook));
            RdRaw(f, &gNCraft, sizeof(gNCraft));
            unsigned char op[2] = {};
            RdRaw(f, op, sizeof(op));
            gOptDmg = op[0] != 0; gOptShake = op[1] != 0;
            RdRaw(f, &gOptMaster, sizeof(gOptMaster));
            SetMasterVolume(gOptMaster);
        }
    }
    fclose(f);
    // ---- 读档后数据消毒（损坏/旧版存档的越界内容曾导致游玩中不定时闪退）----
    // 1) objAt 必须恰好是 MAP_W*MAP_H，且每个下标都在 objs 范围内，否则按垃圾下标取 objs 即崩
    if ((int)W.objAt.size() != MAP_W * MAP_H) return false;
    for (int& v : W.objAt) if (v < -1 || v >= (int)W.objs.size()) v = -1;
    // 2) campfires 是 objs 下标缓存：不信存档，直接重扫重建
    W.campfires.clear();
    for (size_t i = 0; i < W.objs.size(); i++)
        if (W.objs[i].kind == ObjKind::Campfire) W.campfires.push_back((int)i);
    // 3) 生物/鬼仆种类与数值夹回合法域（MobFrames/Stat 按种类取下标，垃圾种类 = 越界）
    for (Creature& c : mobs) {
        if ((unsigned)c.kind > (unsigned)CreatureKind::SmithGhost) c.kind = CreatureKind::Rabbit;
        if (c.tier > 2) c.tier = 2;
        if (c.hp < 0) c.hp = 0;
    }
    for (GhostAlly& g : ghosts) {
        if ((unsigned)g.kind > (unsigned)CreatureKind::SmithGhost) g.kind = CreatureKind::GhostChild;
        if (g.tier > 2) g.tier = 2;
        if (g.maxHp < 1) g.maxHp = 1;
        if (g.hp < 1) g.hp = 1;
    }
    // 4) 伴生铁匠鬼兜底：旧存档里没有它 → 立即补一只（玩家侧上方），杜绝"铁匠鬼消失"
    {
        bool has = false;
        for (const Creature& c : mobs) if (c.companion && c.kind == CreatureKind::SmithGhost) { has = true; break; }
        if (!has) {
            Creature c;
            c.kind = CreatureKind::SmithGhost;
            c.companion = true;
            c.x = P.x + 26.0f; c.y = P.y - 34.0f;
            c.hp = MobBaseHp(c.kind);
            c.state = AState::Idle;
            mobs.push_back(c);
        }
    }
    gs = GS::Play;
    return true;
}
#undef SAVE_VEC
#undef LOAD_VEC

// ---------------- 入口 ----------------

#ifdef _WIN32
// 闪退取证：未处理异常落地前写 crash.log（异常码 + 地址 + 运行时长），不再无声消失
static LONG WINAPI CrashLogHandler(EXCEPTION_POINTERS* ep) {
    FILE* f = fopen("crash.log", "a");
    if (f) {
        unsigned long code = ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionCode : 0;
        void* addr = ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionAddress : nullptr;
        fprintf(f, "CRASH code=0x%08lX addr=%p tick=%lu\n", code, addr, GetTickCount());
        fclose(f);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

// ---------------- 宣传素材录制（DEBUG_CAPTURE）----------------
// 逐帧导出真实游戏画面，供宣传片剪辑使用。没有定义 DEBUG_CAPTURE 时这段代码
// 完全不参与编译，正式版与现有验证构建都不受影响。
// 配置全部走环境变量，避免为它改命令行解析：
//   PW_CAP_OUT      输出目录（需已存在）
//   PW_CAP_WINDOWS  帧区间，如 "540-660;1050-1120"，分号分隔；留空则不录
//   PW_CAP_EVERY    每 N 帧导一张（默认 1）
//   PW_CAP_FMT      png / bmp（默认 png）
// 帧号与 DEBUG_AUTO_SHOT 的 fno 对齐，所以可直接用已有的验证分镜帧表来选段。
#ifdef DEBUG_CAPTURE
static int  gCapFrom[32] = { 0 }, gCapTo[32] = { 0 };
static int  gCapN = 0, gCapEvery = 1, gCapDumped = 0, gCapLastEnd = -1;
#ifdef DEBUG_AUTO_SHOT
// 帧号直接沿用分镜脚本的 fno（两者每帧同增），所以窗口帧表与已有验证帧表通用
#else
static int  gCapFno = 0;
#endif
static char gCapOut[512] = "cap";
static char gCapFmt[8] = "png";
static bool gCapInit = false;

static void CapInit() {
    gCapInit = true;
    const char* o = getenv("PW_CAP_OUT");
    if (o && o[0]) snprintf(gCapOut, sizeof(gCapOut), "%s", o);
    const char* e = getenv("PW_CAP_EVERY");
    if (e && e[0]) { int v = atoi(e); if (v > 0) gCapEvery = v; }
    const char* f = getenv("PW_CAP_FMT");
    if (f && f[0]) snprintf(gCapFmt, sizeof(gCapFmt), "%s", f);
    const char* w = getenv("PW_CAP_WINDOWS");
    if (w && w[0]) {
        const char* p = w;
        while (*p && gCapN < 32) {
            int a = 0, b = 0;
            if (sscanf(p, "%d-%d", &a, &b) == 2 && b >= a) {
                gCapFrom[gCapN] = a; gCapTo[gCapN] = b; gCapN++;
            }
            const char* sc = strchr(p, ';');
            if (!sc) break;
            p = sc + 1;
        }
    }
    for (int i = 0; i < gCapN; i++) if (gCapTo[i] > gCapLastEnd) gCapLastEnd = gCapTo[i];
    printf("CAP: out=%s fmt=%s every=%d windows=%d lastEnd=%d\n",
           gCapOut, gCapFmt, gCapEvery, gCapN, gCapLastEnd);
    fflush(stdout);
}

// 返回 true = 所有窗口都已录完，主循环可以退出
static bool CapTick(int fno, const RenderTexture2D& rt) {
    if (!gCapInit) CapInit();
    if (gCapN == 0) return false;
    bool inWin = false;
    for (int i = 0; i < gCapN; i++)
        if (fno >= gCapFrom[i] && fno <= gCapTo[i]) { inWin = true; break; }
    if (inWin && gCapEvery > 0 && (fno % gCapEvery) == 0) {
        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);          // RT 纹理 Y 向下，导出前必须翻正
        char name[768];
        snprintf(name, sizeof(name), "%s/cap_%05d.%s", gCapOut, fno, gCapFmt);
        ExportImage(img, name);
        UnloadImage(img);
        gCapDumped++;
        if (gCapDumped % 120 == 0) { printf("CAP: %d frames (fno=%d)\n", gCapDumped, fno); fflush(stdout); }
    }
    return gCapLastEnd > 0 && fno > gCapLastEnd;
}
#endif

int main() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(CrashLogHandler);
#endif
#ifdef DEBUG_AUTO_SHOT
    setvbuf(stdout, nullptr, _IONBF, 0);     // 调试日志即时刷新（重定向场景）
#endif
    InitWindow(VW * SCALE, VH * SCALE, "Huanhun Ren - 还魂人");
    LoadLangPref();                // 语言偏好先于任何绘制生效（默认中文）
    ApplyWindowTitle();
    SetExitKey(KEY_NULL);          // 禁用 ESC 直接关窗（ESC 一律走游戏内暂停/关面板逻辑）
    SetTargetFPS(60);

    // 窗口图标：优先读取游戏目录 icon.png（用户自定义），否则程序生成像素风图标
    {
        Image ic = {};
        if (FileExists("icon.png")) ic = LoadImage("icon.png");
        if (ic.data == nullptr) {              // 生成 32x32 像素图标：草地 + 大树
            ic = GenImageColor(32, 32, (Color) { 92, 152, 64, 255 });
            for (int i = 0; i < 30; i++)
                ImageDrawPixel(&ic, rand() % 32, rand() % 32, (Color) { 74, 128, 50, 255 });
            for (int y = -8; y <= 8; y++)      // 树冠（双层像素圆）
                for (int x = -8; x <= 8; x++) {
                    if (x * x + y * y > 64) continue;
                    bool rim = (x * x + y * y > 49);
                    ImageDrawPixel(&ic, 16 + x, 12 + y, rim ? (Color) { 40, 92, 40, 255 }
                                                             : (Color) { 64, 138, 58, 255 });
                }
            for (int y = 20; y < 27; y++)      // 树干
                for (int x = 14; x <= 18; x++)
                    ImageDrawPixel(&ic, x, y, (Color) { 112, 74, 42, 255 });
            ImageDrawPixel(&ic, 15, 21, (Color) { 150, 104, 60, 255 });   // 树干高光
        }
        SetWindowIcon(ic);
        UnloadImage(ic);
    }
    InitAudioDevice();
    AU.Init();                                    // 合成全部音效（含 BGM）
    if (AU.bgm.ctxData != nullptr) {              // 循环 BGM（默认音量 0.45，[ / ] 调节）
        SetMusicVolume(AU.bgm, AU.bgmVol);
        PlayMusicStream(AU.bgm);
    }

    A.Load();
    AU.Init();
    WATER.Init();            // 编译水面 Shader（云斑 + 短划/橙点 + 倒影 + 雨涟漪）
    Zh_LoadAll();             // 加载 5 档系统中文字体（10/12/14/26/32）
    InitPixHalfW();           // 半径查表：DrawPixCircle 的 sqrtf 折半
    Zh_Reindex();             // 建立码点->字形映射（替代逐次线性查找）
    vis.reserve(800);         // 预分配可见项缓冲（主循环零分配）

    sceneRT = LoadRenderTexture(VW, VH);
    lightRT = LoadRenderTexture(VW, VH);
    reflRT = LoadRenderTexture(VW, VH);
    SetTextureFilter(sceneRT.texture, TEXTURE_FILTER_POINT);   // 最近邻：像素锐利
    SetTextureFilter(lightRT.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(reflRT.texture, TEXTURE_FILTER_POINT);    // 倒影严禁双线性
    SetTextureFilter(lightRT.texture, TEXTURE_FILTER_BILINEAR); // 光照图双线性 -> 光圈边缘柔和

    // 预分配（主循环零 new/delete）
    mobs.reserve(64);
    parts.reserve(512);
    dmgs.reserve(32);
    vis.reserve(512);

    NewGame(gSeed);
    // ---- 直接进开始界面（世界已生成，正好当背景）----
    titleHasSave = SaveExists();
    titleSel = titleHasSave ? 1 : 0;
    introOn = false;
    // 新手教程只在该设备第一次玩时自动出现（学过一轮后写 tut_seen.dat；之后按 P 手动重唤重演）
    if (FileExists(TUT_MARK)) tutOn = false;
#if defined(DEBUG_HEADLESS) || defined(DEBUG_AUTO_SHOT) || defined(DEBUG_CAPTURE)
    gs = GS::Play;                  // 无头/截图脚本模式：跳过开始界面，直接进游戏
#else
    gs = GS::Title;
#endif

    // 测试夹具的收尾标志：绝不在循环内调 CloseWindow()（程序尾部还会再关一次 → 二次关闭会段错误）
    bool harnessStop = false;
    while (!WindowShouldClose() && !harnessStop) {
        float rawDt = GetFrameTime();
#ifdef DEBUG_AUTO_SHOT
        fno++;
        // 联机/新 UI 验证状态机：切换必须发生在分支逻辑之前（标题/暂停分支带 continue）
        if (fno == 1770) { gs = GS::Title; titleSel = 2; titleJoin = false; titleMsgT = 0;
                           craftOpen = false; hotbarOpen = false; bookOpen = false; }
        if (fno == 1775) titleJoin = true;
        if (fno == 1778) { titleJoin = false; gs = GS::Play;
                           P.x = W.spawn.x; P.y = W.spawn.y;
                           gameTime = DAY_LEN * 0.05f;              // 清晨
                           gBountyDay = -1; }                       // 触发重新派发（BountyUpdate 下一帧跑）
        if (fno == 1787) gs = GS::Pause;
        if (fno == 1790) gs = GS::Play;
#endif
#ifdef DEBUG_HEADLESS
        rawDt = 1.0f / 60.0f;                // 无头模式跳过 EndDrawing 时 GetFrameTime 不推进，用固定步长
#endif
        if (rawDt > 0.05f) rawDt = 0.05f;
        if (achvShowT > 0) achvShowT -= rawDt;   // 成就弹窗计时（与游戏状态无关）

        // 全屏切换：F11 随时可用（暂停界面另有可点击按钮）
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        // ================= 开始界面 =================
        if (gs == GS::Title) {
            if (titleJoin) {                                      // 输入房主 IP
                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= 32 && key < 127 && (int)strlen(joinIp) < 15) {
                        int l = (int)strlen(joinIp);
                        joinIp[l] = (char)key;
                        joinIp[l + 1] = 0;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && strlen(joinIp) > 0)
                    joinIp[strlen(joinIp) - 1] = 0;
                if (IsKeyPressed(KEY_ESCAPE)) titleJoin = false;
                else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                    if (NetJoin(joinIp)) titleJoin = false;
                }
                Render();
                continue;
            }
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) titleSel = (titleSel + 4) % 5;
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) titleSel = (titleSel + 1) % 5;
            bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
            {   // 鼠标悬停选中 / 点击确认
                Vector2 mg = MouseGame();
                const int iw = 148, ix = VW / 2 - iw / 2;
                for (int i = 0; i < 5; i++) {
                    int iy = 148 + i * 30;
                    Rectangle rc = { (float)(ix - 2), (float)(iy - 2), (float)(iw + 4), 26.0f };
                    if (CheckCollisionPointRec(mg, rc)) {
                        titleSel = i;
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) confirm = true;
                    }
                }
            }
            if (confirm) {
                if (titleSel == 0) {                                  // 新游戏
                    unsigned ns = (unsigned)(GetTime() * 1000.0) ^ (unsigned)(rand() * 2654435761u);
                    NewGame(ns);
                    titleHasSave = SaveExists();
                    introOn = true;
                    gs = GS::Play;
                    lastSaveDay = (int)(gameTime / DAY_LEN);
                } else if (titleSel == 1) {                           // 继续游戏
                    if (titleHasSave) {
                        introOn = false;
                        if (LoadGame()) lastSaveDay = (int)(gameTime / DAY_LEN);
                        else { titleHasSave = false; titleSel = 0; }
                    }
                } else if (titleSel == 2) {                           // 创建联机房间（做房主）
                    if (NetHostStart()) {
                        FloatText(P.x, P.y - 60,
                                  TextFormat(L10N("房间已建：%s（朋友选「加入联机房间」输此 IP）"), Net::LocalIp()),
                                  150, 235, 200);
                    } else {
                        titleMsgT = 3.0f;
                        strncpy(titleMsg, "建房失败：端口被占用", 47);
                    }
                } else if (titleSel == 3) {                           // 加入联机房间
                    titleJoin = true;
                    joinIp[0] = 0;
                } else {                                              // 退出（不要覆盖存档）
                    gQuitNoSave = true;
                    break;
                }
            }
            Render();
            continue;
        }

        // 开场叙事卡：首次进入显示，任意键/鼠标关闭（世界冻结）
        if (introOn) {
            if (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                introOn = false;
                if (!Prog::ChapterSeen(0)) { Prog::MarkChapterSeen(0); OpenScroll(0, 0); }   // 序章卷轴
            }
            Render();
            continue;
        }

        // ---- 每日自动存档（换日的那一刻落盘）----
        if (gs == GS::Play) {
            int curD = (int)(gameTime / DAY_LEN);
            if (lastSaveDay >= 0 && curD != lastSaveDay) {
                lastSaveDay = curD;
                if (SaveGame()) {
                    titleHasSave = true;
                    FloatText(P.x, P.y - 56, "已自动存档", 150, 235, 200);
                }
            }
        }

        if (gs == GS::Play) {
            // 合成动画计时（必须独立于下方按键分支：面板仍开时走不到 else 分支，动画会冻结在 t=0）
            if (craftAnimOn) {
                craftAnimT += rawDt;
                if (craftAnimT >= CRAFT_ANIM_LEN) craftAnimOn = false;
            }
            // 新手按键指导：P 键重唤并从头再演示一遍；按下对应键即视为掌握
            // （必须放主循环——按 TAB/ESC 的那一帧 UpdateGame 不执行，放里面会导致这两键永不"掌握"）
            if (IsKeyPressed(KEY_P)) { tutOn = true; tutAnimT = 0.0f; for (int i = 0; i < 18; i++) tutMastered[i] = false; }
            if (tutOn) {
                tutAnimT += rawDt;                       // 逐条浮入动画计时
                static const KeyboardKey TUTKEYS[18] = { KEY_W, KEY_A, KEY_S, KEY_D, KEY_LEFT_SHIFT,
                    KEY_J, KEY_E, KEY_TAB, KEY_C, KEY_G, KEY_T, KEY_V, KEY_U, KEY_B, KEY_H, KEY_ESCAPE,
                    KEY_F, KEY_R };
                for (int i = 0; i < 18; i++)
                    if (IsKeyPressed(TUTKEYS[i])) tutMastered[i] = true;
                bool all = true;
                for (int i = 0; i < 18; i++) if (!tutMastered[i]) { all = false; break; }
                if (all) {
                    tutOn = false;
                    FILE* tf = fopen(TUT_MARK, "wb");    // 全部掌握：记下"已不再是第一次玩"，以后不再自动弹
                    if (tf) { fputc(1, tf); fclose(tf); }
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (craftOpen) craftOpen = false;
                else {
                    gs = GS::Pause;
                    if (SaveGame()) titleHasSave = true;    // 暂停即存档（手动存盘点）
                }
            } else if (IsKeyPressed(KEY_TAB)) {
                if (!craftOpen) { craftOpen = true; craftSel = 0; craftPage = 0; }   // 首次按 TAB：打开合成面板并重置分页
                else {
                    craftSel = (craftSel + 1) % CRAFT_CNT;            // TAB：逐配方循环
                    PlaySound(AU.pickup);                             // 短促切换反馈
                }
            } else if (craftOpen) {                          // 面板中：仅响应合成键，世界冻结
                // ↑↓ 选配方，CapsLock/Enter 合成选中项，数字键直达
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
                    craftSel = (craftSel + CRAFT_CNT - 1) % CRAFT_CNT;
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
                    craftSel = (craftSel + 1) % CRAFT_CNT;
                if (IsKeyPressed(KEY_ONE))   CraftMenu(0);
                if (IsKeyPressed(KEY_TWO))   CraftMenu(1);
                if (IsKeyPressed(KEY_THREE)) CraftMenu(2);
                if (IsKeyPressed(KEY_FOUR))  CraftMenu(3);
                if (IsKeyPressed(KEY_FIVE))  CraftMenu(4);
                if (IsKeyPressed(KEY_SIX))   CraftMenu(5);
                if (IsKeyPressed(KEY_SEVEN)) CraftMenu(6);
                if (IsKeyPressed(KEY_EIGHT)) CraftMenu(7);
                if (IsKeyPressed(KEY_NINE))  CraftMenu(8);
                if (IsKeyPressed(KEY_ZERO))  CraftMenu(9);
                if (IsKeyPressed(KEY_CAPS_LOCK) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
                    CraftMenu(craftSel);
                craftPage = craftSel / 12;                        // 选中项跨页时自动翻页
            } else {
                float dt = rawDt;
                if (craftAnimOn) dt = 0;    // 合成动画：世界冻结（计时已在上方统一推进）
                if (hitStop > 0) {          // 打击停顿：世界冻结但仍渲染
                    hitStop -= rawDt;
                    dt = 0;
                }
                if (scrollOn) dt = 0;               // 卷轴：世界冻结（UpdateGame 内亦有兜底）
                UpdateMusicStream(AU.bgm);          // 驱动 BGM 流（循环播放）
            UpdateGame(dt);
            }
        } else if (gs == GS::Pause) {
            if (IsKeyPressed(KEY_ESCAPE)) gs = GS::Play;
            // 设置行点击（矩形由 DrawUI 每帧写入 optRows）
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = MouseGame();
                for (int i = 0; i < 6; i++) {
                    if (!CheckCollisionPointRec(m, optRows[i])) continue;
                    switch (i) {
                    case 0:
                        AU.bgmVol += 0.1f;
                        if (AU.bgmVol > 1.0f) AU.bgmVol = 0.0f;
                        if (AU.bgm.ctxData != nullptr) SetMusicVolume(AU.bgm, AU.bgmVol);
                        break;
                    case 1:
                        gOptMaster += 0.1f;
                        if (gOptMaster > 1.0f) gOptMaster = 0.0f;
                        SetMasterVolume(gOptMaster);
                        break;
                    case 2: gOptDmg = !gOptDmg; break;
                    case 3: gOptShake = !gOptShake; break;
                    case 4: ToggleFullscreen(); break;
                    case 5: gL10nEn = !gL10nEn; SaveLangPref(); ApplyWindowTitle(); break;   // 语言：中 <-> 英
                    }
                    PlaySound(AU.pickup);
                    break;
                }
            }
        } else { // Dead
            if (IsKeyPressed(KEY_R)) {
                if (netOn && !netHost) {              // 客人还魂：原地复活请求
                    Net::Buf b;
                    b.d[0] = 'R'; b.n = 1;
                    Net::UdpSend(netSock, netAddrHost, netPortHost, b);
                    P.dead = false; P.hp = 100; P.deadT = 0;
                    P.x = W.spawn.x; P.y = W.spawn.y;
                    P.hunger = 60;
                    gs = GS::Play;
                } else NewGame(gSeed + 1);
            }
            else { UpdateMusicStream(AU.bgm); UpdateGame(scrollOn ? 0.0f : rawDt); }   // 世界继续（尸体淡出）
        }

        // ---- 元系统：卷轴打字机推进 + 事件表现（放在渲染前，保证当帧可见）----
        if (scrollOn) {
            scrollT += rawDt;
            int total = (int)strlen(ScrollText());
            if (scrollChars < total) {
                scrollCharT += rawDt;
                while (scrollCharT >= 0.026f && scrollChars < total) { scrollCharT -= 0.026f; scrollChars += 2; }
            } else if (!scrollDone) scrollDone = true;
            if (scrollDone && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE))) {
                scrollOn = false;
                if (scrollKind == 1) { gs = GS::Title; titleHasSave = SaveExists(); }   // 结局收卷：回到开始界面
            }
        }
        if (insightFlashT > 0) insightFlashT -= rawDt;
        ConsumeProgEvents();

        Render();

#ifdef DEBUG_CAPTURE
        {   // 宣传录制：本帧刚从 GPU 渲染完，直接落盘
#ifdef DEBUG_AUTO_SHOT
            int capF = fno;                 // 有分镜脚本时与 fno 对齐，便于按帧表选段
#else
            int capF = ++gCapFno;
#endif
            if (CapTick(capF, reflRT)) {
                printf("CAP: done, %d frames dumped\n", gCapDumped);
                fflush(stdout);
                harnessStop = true;
            }
        }
#endif

#ifdef DEBUG_STRESS
        // 长时压测：狂点攻击 + 反复重开局/进鬼域/存读档/跨图传送，跑满 STRES_SECS 秒
        {
            static int sf = 0;
            sf++;
            if (sf % 5 == 0)  DoAttack();                       // 攻击（含挥砍粒子/命中判定）
            if (sf % 17 == 0) TryInteract();                    // 交互（铁匠/建造/采集）
            if (sf % 23 == 0) { P.atkCd = 0; P.dir = (unsigned char)(rand() % 4); }
            if (sf % 41 == 0) {                                 // 跨图随机传送（触发流式加载/水域烘焙）
                P.x = 200.0f + (float)(rand() % 2800);
                P.y = 200.0f + (float)(rand() % 2800);
            }
            if (sf % 300 == 0) {                                // 反复重开局：世界生成压力测试
                NewGame((unsigned)(20260818u + sf));
                printf("[stress] f=%d NewGame ok  mobs=%d objs=%d\n",
                       sf, (int)mobs.size(), (int)W.objs.size());
            }
            if (sf % 450 == 0) {                                // 存读档往返压力测试
                P.hp = 60; SaveGame();
                if (!LoadGame()) printf("[stress] f=%d LoadGame FAILED\n", sf);
            }
            if (sf % 600 == 0) {                                // 强制进/出鬼域
                if (!gdOn && !mobs.empty()) GdEnter(0);
                else GdExit();
                printf("[stress] f=%d gdOn=%d proxies=%d\n", sf, (int)gdOn, (int)gdProxies.size());
            }
            if (sf % 60 == 0) printf("[stress] f=%d hp=%d dead=%d t=%.1f\n",
                                     sf, P.hp, (int)P.dead, gameTime);
            fflush(stdout);
            if (sf >= 60 * STRES_SECS) {
                printf("STRESS DONE: %d frames / %d s, no crash\n", sf, STRES_SECS);
                fflush(stdout);
                harnessStop = true;          // 走正常退出路径（不要在这里 CloseWindow）
            }
        }
#endif

#ifdef DEBUG_VERIFY
        // 针对性回归验证：对本轮修复项做行为断言（独立宏，不参与正常构建）
        // 编译：g++ ... -DDEBUG_HEADLESS -DDEBUG_VERIFY，运行后核对 stdout 的 PASS/FAIL
        {
            static int vf = 0;
            vf++;
            // ---- 1. 死亡重开后必须能正常移动（回归：防止状态残留把玩家卡死）----
            if (vf == 60) {
                P.dead = true; gs = GS::Dead;
                NewGame(gSeed + 1);                   // 等价于按 R
                Vector2 before = { P.x, P.y };
                Vector2 after = W.MoveCircle(P.x, P.y, 5.0f, 24.0f, 0.0f);
                printf("VERIFY1 %s: moved=%.2fpx (want >0)\n",
                       (after.x - before.x > 0.0f) ? "PASS" : "FAIL",
                       after.x - before.x);
            }
            // 夹具：回到干净存活状态（隔离用例 1 的死亡/地牢残留）
            if (vf == 70) { NewGame(gSeed + 2); gs = GS::Play; }
            // ---- 2. 铁斧 Lv1（pow=2）砍树：修复前 hp 3->1->255 回绕，树永不倒 ----
            if (vf == 80) {
                int idx = -1;
                for (size_t i = 0; i < W.objs.size(); i++)
                    if (IsTreeKind(W.objs[i].kind)) { idx = (int)i; break; }
                if (idx < 0) printf("VERIFY2 FAIL: 地图上找不到树\n");
                else {
                    P.toolLv[3] = 1;
                    int hits = 0;
                    while (W.objs[idx].kind != ObjKind::Stump && hits < 10) { HitObject(idx); hits++; }
                    bool ok = (W.objs[idx].kind == ObjKind::Stump) && hits == 2;
                    printf("VERIFY2 %s: axeLv1 hits=%d finalHp=%d (want Stump / hits=2)\n",
                           ok ? "PASS" : "FAIL", hits, (int)W.objs[idx].hp);
                    P.toolLv[3] = 0;
                }
            }
            // ---- 3. 合成面板仍开着时，合成动画必须继续推进（修复前冻结在 t=0）----
            if (vf == 100) { craftAnimOn = true; craftAnimT = 0; craftOpen = true; }
            if (vf == 106) {
                printf("VERIFY3 %s: craftAnimT=%.3f (want >0.05)\n",
                       (craftAnimT > 0.05f) ? "PASS" : "FAIL", craftAnimT);
                craftOpen = false; craftAnimOn = false; craftAnimT = 0;
            }
            // ---- 4. 开着环形栏死亡：必须能自动关闭（修复前永久盖住死亡结算页）----
            if (vf == 120) { hotbarOpen = true; P.dead = true; gs = GS::Dead; }
            if (vf == 123) printf("VERIFY4 %s: hotbarOpen=%d (want 0)\n",
                                  hotbarOpen ? "FAIL" : "PASS", (int)hotbarOpen);
            // ---- 5. NewGame 必须清干净上一局残留 ----
            if (vf == 130) {
                bossSlain = true;                          // 夹具：制造脏状态
                NewGame(gSeed + 3);
                printf("VERIFY5 %s: bossSlain=%d (want 0)\n",
                       (!bossSlain) ? "PASS" : "FAIL", (int)bossSlain);
            }
            // ---- 6/7. 神秘复苏：鬼童「犯之则反」（不主动、打它才还手）----
            auto PlaceProbe = [&](CreatureKind k) {
                mobs.clear();
                Creature c;
                c.kind = k;
                c.x = P.x + 20.0f; c.y = P.y + 8.0f;
                c.hp = MobBaseHp(k);
                c.state = AState::Idle; c.triggered = false;
                c.phase = 0;
                mobs.push_back(c);
                P.dir = 2;                      // 面朝右（正对探针）
                P.atkCd = 0.0f;
            };
            if (vf >= 180 && vf <= 195) PlaceProbe(CreatureKind::GhostChild);   // 持续压住位置
            if (vf == 190) {
                bool trig = false;
                for (const Creature& c2 : mobs) if (c2.triggered) trig = true;
                printf("VERIFY6 %s: 鬼童满血贴脸 triggered=%d (want 0，犯之则反/不主动)\n",
                       trig ? "FAIL" : "PASS", (int)trig);
            }
            if (vf == 195) {
                DoAttack();                                // 打它一下
                bool trig = false;
                for (const Creature& c2 : mobs) if (c2.triggered) trig = true;
                printf("VERIFY7 %s: 打鬼童一下后 triggered=%d (want 1)\n",
                       trig ? "PASS" : "FAIL", (int)trig);
            }
            // ---- 8. 凡器难伤：玩家攻击鬼，鬼 hp 必须不变 ----
            if (vf == 205) PlaceProbe(CreatureKind::GhostChild);
            if (vf == 206) {
                int before = mobs.empty() ? -1 : mobs[0].hp;
                DoAttack();
                int after = mobs.empty() ? -2 : mobs[0].hp;
                printf("VERIFY8 %s: 鬼 hp %d -> %d (want 不变)\n",
                       (before == after) ? "PASS" : "FAIL", before, after);
            }
            // ---- 9. 单鬼「时间重启」：第一次击杀原地满血复活，第二次才真死 ----
            if (vf == 212) {
                Creature g;
                g.kind = CreatureKind::LoneGhost;
                g.phase = 0; g.hp = 0;
                int maxHp = MobBaseHp(CreatureKind::LoneGhost);
                bool r1 = MobPreDeath(g);                  // 第一次：应重启
                int hp1 = g.hp;
                g.hp = 0;
                bool r2 = MobPreDeath(g);                  // 第二次：应真死
                bool ok = r1 && (hp1 == maxHp) && !r2;
                printf("VERIFY9 %s: 首杀重启=%d 回血=%d/%d 二杀重启=%d (want 1 / 满 / 0)\n",
                       ok ? "PASS" : "FAIL", (int)r1, hp1, maxHp, (int)r2);
            }
            // ================= 本轮新增项回归（需求一~六） =================
            // ---- 10. 需求一：伴生铁匠鬼开局即在玩家身侧（症状：旧版被困屋顶遮挡看不见）----
            if (vf == 240) { NewGame(gSeed + 7); }
            if (vf == 242) {
                bool found = false; float d = -1.0f;
                for (const Creature& c : mobs)
                    if (c.kind == CreatureKind::SmithGhost && c.companion) {
                        found = true;
                        d = sqrtf((c.x - P.x) * (c.x - P.x) + (c.y - P.y) * (c.y - P.y));
                    }
                printf("VERIFY10 %s: 伴生铁匠鬼 companion=%d dist=%.1f (want 1 / <80)\n",
                       (found && d < 80.0f) ? "PASS" : "FAIL", (int)found, d);
            }
            // ---- 11. 需求三：必须有鬼刷在野外（不在任何屋子范围内）----
            if (vf == 244) {
                int wild = 0;
                for (const Creature& c : mobs) {
                    if (c.companion || c.kind == CreatureKind::SmithGhost) continue;
                    bool inHouse = false;
                    for (const World::House& h : W.houses) {
                        Vector2 hc = h.Center();
                        if (fabsf(c.x - hc.x) < h.w * 16.0f + 40.0f &&
                            fabsf(c.y - hc.y) < h.h * 16.0f + 40.0f) { inHouse = true; break; }
                    }
                    if (!inHouse) wild++;
                }
                printf("VERIFY11 %s: 野外鬼 %d 只 (want >=5)\n",
                       (wild >= 5) ? "PASS" : "FAIL", wild);
            }
            // ---- 12. 需求五：每类鬼都有可画的选中半径 ----
            if (vf == 246) {
                float r1 = MobSightRadius(CreatureKind::GhostChild);
                float r2 = MobSightRadius(CreatureKind::GhostDomain);
                float r3 = MobSightRadius(CreatureKind::SmithGhost);
                bool ok = r1 > 0 && r2 > 0 && r3 > 0 && r2 > r1;
                printf("VERIFY12 %s: 半径 僵尸=%.0f 游戏鬼=%.0f 铁匠鬼=%.0f (want 全>0 且 游戏鬼>僵尸)\n",
                       ok ? "PASS" : "FAIL", r1, r2, r3);
            }
            // ---- 13. 需求四：鬼域是独立空间——进得去、锁得住、收服后能出去 ----
            if (vf == 250) {
                domainPurged = false;
                W.domainPos = { P.x + 300.0f, P.y };     // 造一个鬼域中心
                mobs.clear();
                Creature g;
                g.kind = CreatureKind::GhostDomain;
                g.x = P.x + 40.0f; g.y = P.y;
                g.hp = MobBaseHp(CreatureKind::GhostDomain);
                g.state = AState::Idle;
                mobs.push_back(g);
                GdEnter(0);
            }
            if (vf == 252) {
                printf("VERIFY13a %s: gdOn=%d 结界半径=%.0f (want 1 / >0)\n",
                       (gdOn && gdR > 0.0f) ? "PASS" : "FAIL", (int)gdOn, gdR);
            }
            // 结界锁死：把玩家硬推出圈外，GdUpdate 必须把他拉回来
            if (vf == 254) {
                P.x = gdCX + gdR + 120.0f;
                P.y = gdCY;
            }
            if (vf == 256) {
                float d = sqrtf((P.x - gdCX) * (P.x - gdCX) + (P.y - gdCY) * (P.y - gdCY));
                printf("VERIFY13b %s: 越界后距圆心 %.1f (want <=%.0f，即被结界拉回)\n",
                       (d <= gdR) ? "PASS" : "FAIL", d, gdR);
            }
            if (vf == 258) {
                GdExit();
                printf("VERIFY13c %s: 退出后 gdOn=%d 代理数=%d (want 0 / 0)\n",
                       (!gdOn && gdProxies.empty()) ? "PASS" : "FAIL",
                       (int)gdOn, (int)gdProxies.size());
            }
            // ---- 14. 需求六：二进制存档往返（存 -> 改 -> 读，必须还原）----
            if (vf == 264) {
                NewGame(gSeed + 11);
                P.x += 137.0f; P.y += 53.0f; P.hp = 42;
                float sx = P.x, sy = P.y;
                int   shp = P.hp;
                bool  saved = SaveGame();
                P.x = 0; P.y = 0; P.hp = 1;             // 弄脏
                bool ok = LoadGame();
                bool same = saved && ok &&
                            fabsf(P.x - sx) < 0.5f && fabsf(P.y - sy) < 0.5f && P.hp == shp;
                printf("VERIFY14 %s: save=%d load=%d (%.1f,%.1f) hp=%d (want 1/1 (%.1f,%.1f) %d)\n",
                       same ? "PASS" : "FAIL", (int)saved, (int)ok, P.x, P.y, P.hp, sx, sy, shp);
            }
            // ---- 15. 需求六：开始界面默认停在标题页，且能识别存档 ----
            if (vf == 270) {
                titleHasSave = SaveGame();
                gs = GS::Title; titleSel = 0;
                printf("VERIFY15 %s: 标题态 gs=%d titleHasSave=%d\n",
                       (gs == GS::Title && titleHasSave) ? "PASS" : "FAIL",
                       (int)gs, (int)titleHasSave);
                gs = GS::Play;
            }
            if (vf == 272) { NewGame(gSeed + 13); }

            // ================= 本轮新增项回归（横扫多目标 / 铁匠鬼不死 / 无面鬼渗透 / 领地） =================
            static int vTree[3] = { -1, -1, -1 };
            static int vTreeHp[3] = { 0, 0, 0 };
            static int vOpenX = -1, vOpenY = -1;
            // 夹具：手工在瓦片上放一棵树（含瓦片索引，横扫走 ObjIndexAt 查询）
            auto PlaceTreeAt = [&](int tx, int ty) {
                WorldObj o;
                o.kind = ObjKind::Tree; o.tx = (unsigned char)tx; o.ty = (unsigned char)ty;
                o.hp = 3; o.shake = 0; o.harvested = false; o.regrow = 0;
                W.objs.push_back(o);
                int idx = (int)W.objs.size() - 1;
                W.objAt[(size_t)(ty * MAP_W + tx)] = idx;
                return idx;
            };
            // 夹具：找一块干净陆地（非水、无物体、站得开）—— 立界碑的前置条件
            auto FindOpenTile = [&](int& otx, int& oty) {
                otx = -1; oty = -1;
                for (int ty = 20; ty < MAP_H - 20 && otx < 0; ty++)
                    for (int tx = 20; tx < MAP_W - 20; tx++) {
                        if (W.TileAt(tx, ty) == Tile::Water) continue;
                        if (W.ObjIndexAt(tx, ty) >= 0) continue;
                        if (!W.CircleFree(tx * 16.0f + 8.0f, ty * 16.0f + 8.0f, 12.0f)) continue;
                        otx = tx; oty = ty; return;
                    }
            };
            // ---- 16. 横扫：一次攻击必须同时结算多个目标（修复前只砍最近的一个）----
            if (vf == 300) {
                NewGame(gSeed + 17); gs = GS::Play;
                mobs.clear(); npcs.clear();
                int ptx = (int)(P.x / TILE), pty = (int)(P.y / TILE);
                P.x = ptx * TILE + 8.0f; P.y = pty * TILE + 8.0f;
                hotSel = 6; P.dir = 2; P.atkCd = 0.0f;          // 空手朝右，避免弓/工具分支
                vTree[0] = PlaceTreeAt(ptx + 1, pty);
                vTree[1] = PlaceTreeAt(ptx + 1, pty + 1);
                vTree[2] = PlaceTreeAt(ptx + 2, pty);
                for (int i = 0; i < 3; i++) vTreeHp[i] = W.objs[(size_t)vTree[i]].hp;
            }
            if (vf == 302) {
                DoAttack();
                int hit = 0;
                for (int i = 0; i < 3; i++) if (W.objs[(size_t)vTree[i]].hp < vTreeHp[i]) hit++;
                printf("VERIFY16 %s: 一次横扫命中 %d/3 棵树 (want >=2)\n",
                       (hit >= 2) ? "PASS" : "FAIL", hit);
            }
            // ---- 17. 铁匠鬼不死：玩家砍 + 鬼仆打，都不掉血、不会死 ----
            if (vf == 310) {
                NewGame(gSeed + 19); gs = GS::Play;
                mobs.clear(); npcs.clear();
                Creature s;
                s.kind = CreatureKind::SmithGhost;
                s.x = P.x + 16.0f; s.y = P.y;
                s.hp = MobBaseHp(CreatureKind::SmithGhost);
                s.state = AState::Idle; s.triggered = false; s.phase = 0;
                mobs.push_back(s);
                hotSel = 6; P.dir = 2; P.atkCd = 0.0f;
            }
            if (vf == 312) {
                int before = mobs.empty() ? -1 : mobs[0].hp;
                DoAttack();                                              // 凡器
                if (!mobs.empty())                                       // 鬼仆（以鬼制鬼）
                    HitMobByGhost(mobs[0], CreatureKind::GhostChild, 9999.0f, 1.0f, 0.0f);
                int  after = mobs.empty() ? -2 : mobs[0].hp;
                bool alive = !mobs.empty() && mobs[0].state != AState::Dead;
                printf("VERIFY17 %s: 铁匠鬼 hp %d->%d 存活=%d (want 不掉血 / 1)\n",
                       (before == after && alive) ? "PASS" : "FAIL", before, after, (int)alive);
            }
            // ---- 18. 无面鬼混入：身边已有同伴 + 贴身蛰伏够久 -> 变成随行同伴 ----
            if (vf == 320) {
                NewGame(gSeed + 23); gs = GS::Play;
                mobs.clear(); npcs.clear();
                Npc n; n.on = true; n.kind = 1; n.state = 1;
                n.x = P.x + 60.0f; n.y = P.y; npcs.push_back(n);
                Creature f;
                f.kind = CreatureKind::Faceless;
                f.x = P.x + 30.0f; f.y = P.y;
                f.hp = MobBaseHp(CreatureKind::Faceless);
                f.state = AState::Idle; f.triggered = false; f.phase = 0;
                f.infiltrated = false; f.infilT = 5.90f;      // 差一点点就够 6 秒
                mobs.push_back(f);
            }
            if (vf == 334) {
                bool got = false;
                for (const Creature& c : mobs)
                    if (c.kind == CreatureKind::Faceless && c.infiltrated) got = true;
                printf("VERIFY18 %s: 贴身蛰伏后 混入同伴=%d (want 1)\n",
                       got ? "PASS" : "FAIL", (int)got);
            }
            // ---- 19. 无面鬼同化：混入者定时顶替掉身边的一名真同伴 ----
            if (vf == 340) {
                NewGame(gSeed + 29); gs = GS::Play;
                mobs.clear(); npcs.clear();
                Npc n; n.on = true; n.kind = 1; n.state = 1;
                n.x = P.x + 60.0f; n.y = P.y; npcs.push_back(n);
                Creature f;
                f.kind = CreatureKind::Faceless;
                f.x = P.x + 40.0f; f.y = P.y;
                f.hp = MobBaseHp(CreatureKind::Faceless);
                f.state = AState::Idle; f.infiltrated = true; f.convertT = 0.001f;
                mobs.push_back(f);
            }
            if (vf == 344) {
                bool npcGone = !npcs.empty() && !npcs[0].on;
                int faceN = 0;
                for (const Creature& c : mobs)
                    if (c.infiltrated && c.state != AState::Dead) faceN++;
                printf("VERIFY19 %s: 同化后 原同伴离开=%d 无面鬼数=%d (want 1 / >=2)\n",
                       (npcGone && faceN >= 2) ? "PASS" : "FAIL", (int)npcGone, faceN);
            }
            // ---- 20. 死亡阈值：身边同伴 >5 且无面鬼占比 >=50% ----
            if (vf == 350) {
                NewGame(gSeed + 31); gs = GS::Play;
                mobs.clear(); npcs.clear();
                for (int i = 0; i < 3; i++) {                 // 3 名真同伴
                    Npc n; n.on = true; n.kind = 1; n.state = 1;
                    n.x = P.x + 60.0f + i * 10.0f; n.y = P.y; npcs.push_back(n);
                }
                for (int i = 0; i < 3; i++) {                 // 3 只混入的无面鬼（占比 50%）
                    Creature f;
                    f.kind = CreatureKind::Faceless;
                    f.x = P.x + 30.0f + i * 10.0f; f.y = P.y;
                    f.hp = MobBaseHp(CreatureKind::Faceless);
                    f.state = AState::Idle; f.infiltrated = true; f.convertT = 999.0f;
                    mobs.push_back(f);
                }
            }
            if (vf == 356) {
                printf("VERIFY20 %s: 同伴6(无面3) 角色死亡=%d (want 1)\n",
                       P.dead ? "PASS" : "FAIL", (int)P.dead);
            }
            // ---- 21. 领地：材料/同伴双重门槛 ----
            if (vf == 360) {
                NewGame(gSeed + 37); gs = GS::Play;
                mobs.clear(); npcs.clear();
                FindOpenTile(vOpenX, vOpenY);
                if (vOpenX >= 0) { P.x = vOpenX * 16.0f + 8.0f; P.y = vOpenY * 16.0f + 8.0f; }
                P.wood = 0; P.stone = 0; P.iron = 0;
                TryBuildTerritory();
                printf("VERIFY21a %s: 材料不足 领地已立=%d (want 0)\n",
                       (!campBuilt) ? "PASS" : "FAIL", (int)campBuilt);
            }
            if (vf == 362) {
                P.x = vOpenX * 16.0f + 8.0f; P.y = vOpenY * 16.0f + 8.0f;
                P.wood = 100; P.stone = 100;
                TryBuildTerritory();
                printf("VERIFY21b %s: 材料充足 领地已立=%d Lv=%d (want 1 / 1)\n",
                       (campBuilt && campLv == 1) ? "PASS" : "FAIL", (int)campBuilt, campLv);
            }
            if (vf == 364) {                                  // 材料足但同伴不够：不得升级
                P.x = campCenter.x; P.y = campCenter.y;
                P.wood = 100; P.stone = 100; P.iron = 50;
                TryInteract();
                printf("VERIFY21c %s: 同伴0人 材料足 Lv=%d (want 1)\n",
                       (campLv == 1) ? "PASS" : "FAIL", campLv);
            }
            if (vf == 366) {                                  // 同伴够了：升级成功
                npcs.clear();
                for (int i = 0; i < 2; i++) {
                    Npc n; n.on = true; n.kind = 1; n.state = 1;
                    n.x = P.x + 120.0f + i * 10.0f; n.y = P.y; npcs.push_back(n);
                }
                P.wood = 100; P.stone = 100;
                TryInteract();
                printf("VERIFY21d %s: 同伴2人 材料足 Lv=%d (want 2)\n",
                       (campLv == 2) ? "PASS" : "FAIL", campLv);
            }
            if (vf == 368) {
                printf("VERIFY21e %s: Lv2 无面鬼阈值=%d%% 每日问询=%d (want 30 / 3)\n",
                       (FacelessPct() == 30 && AskPerDay() == 3) ? "PASS" : "FAIL",
                       FacelessPct(), AskPerDay());
            }

            // ---- VERIFY22 联机回归：回环 UDP 上跑通 加入/世界/快照/攻击/拾取/离开 ----
            if (vf == 300) {
                Net::Init();
                unsigned short hs = Net::UdpOpen(Net::PORT);
                unsigned short cs = Net::UdpOpen(0);
                bool okA = hs != (unsigned short)-1 && cs != (unsigned short)-1;
                printf("VERIFY22a %s: hostSock=%d cliSock=%d\n", okA ? "PASS" : "FAIL", (int)hs, (int)cs);
                netSock = hs; netOn = true; netHost = true; netId = 0; netJoined = true;
                netNextMobId = 1; netDropSeq = 1;
                WDropIdCb = NetDropIdCb;
                for (auto& p : netPeers) p = PeerView{};
                netPeers[0].on = true;
                Net::Buf jb;
                jb.d[0] = 'J'; jb.n = 1;
                Net::WStr(jb, "测试客", 16);
                Net::UdpSend(cs, Net::ResolveIp("127.0.0.1"), Net::PORT, jb);
                NetHostService(0.05f);
                bool okB = netPeers[1].on;
                printf("VERIFY22b %s: join -> peer1=%d name=%s\n", okB ? "PASS" : "FAIL",
                       (int)netPeers[1].on, netPeers[1].name);
                Net::Buf rb; unsigned ra; unsigned short rp;
                bool gotW = false; int chunks = 0;
                while (Net::UdpRecv(cs, ra, rp, rb)) {
                    if (rb.d[0] == 'W') gotW = true;
                    if (rb.d[0] == 'n') chunks++;
                }
                bool okC = gotW && chunks > 0;
                printf("VERIFY22c %s: welcome=%d worldChunks=%d\n", okC ? "PASS" : "FAIL", (int)gotW, chunks);
                // 客人上报状态 + 装备 → 攻击兔子 → 掉血
                float px0 = netPeers[1].x, py0 = netPeers[1].y;
                Creature rab;
                rab.kind = CreatureKind::Rabbit;
                rab.x = px0 + 20.0f; rab.y = py0;
                rab.hp = MobBaseHp(CreatureKind::Rabbit);
                mobs.push_back(rab);
                Net::Buf eb;
                eb.d[0] = 'E'; eb.n = 1;
                for (int i = 0; i < 6; i++) Net::W8(eb, 0);
                Net::W8(eb, 0); Net::W8(eb, 1); Net::W8(eb, 0);
                Net::UdpSend(cs, Net::ResolveIp("127.0.0.1"), Net::PORT, eb);
                Net::Buf ab;
                ab.d[0] = 'A'; ab.n = 1;
                Net::W8(ab, 2);                       // 朝右打
                Net::UdpSend(cs, Net::ResolveIp("127.0.0.1"), Net::PORT, ab);
                NetHostService(0.05f);
                int rabHp = -1;
                for (const Creature& c : mobs) if (c.kind == CreatureKind::Rabbit) { rabHp = c.hp; break; }
                bool okD = rabHp >= 0 && rabHp < MobBaseHp(CreatureKind::Rabbit);
                printf("VERIFY22d %s: rabbitHp=%d (base=%d, want <)\n", okD ? "PASS" : "FAIL",
                       rabHp, MobBaseHp(CreatureKind::Rabbit));
                // 拾取：生成带网络 id 的掉落 → 客人请求 → 房主移除并广播
                W.SpawnDrop(ItemKind::Wood, px0, py0, 0, 0);
                unsigned did = W.drops.back().netId;
                size_t dropsB4 = W.drops.size();
                Net::Buf pb;
                pb.d[0] = 'P'; pb.n = 1;
                Net::W32(pb, did);
                Net::UdpSend(cs, Net::ResolveIp("127.0.0.1"), Net::PORT, pb);
                NetHostService(0.05f);
                bool okE = W.drops.size() == dropsB4 - 1;
                printf("VERIFY22e %s: drops %d->%d\n", okE ? "PASS" : "FAIL", (int)dropsB4, (int)W.drops.size());
                // 离开
                Net::Buf lb;
                lb.d[0] = 'L'; lb.n = 1;
                Net::UdpSend(cs, Net::ResolveIp("127.0.0.1"), Net::PORT, lb);
                NetHostService(0.05f);
                bool okF = !netPeers[1].on;
                printf("VERIFY22f %s: leave -> peer1=%d\n", okF ? "PASS" : "FAIL", (int)netPeers[1].on);
                Net::UdpClose(hs);
                Net::UdpClose(cs);
                NetStop();
            }
            if (vf == 400) { fflush(stdout); harnessStop = true; }
        }
#endif

#ifdef DEBUG_AUTO_SHOT
        // 无头自动验证：不依赖窗口可见性，直接从 GPU 纹理导出 PNG
        {
            if (fno % 120 == 0)
                TraceLog(LOG_INFO, "DBG: fno=%d gs=%d dead=%d p=%.3f darkK=%.3f gameTime=%.1f",
                         fno, (int)gs, (int)P.dead,
                         DayPhase(gameTime), NightDarkness(gameTime) / 228.0f, gameTime);
            auto DumpRT = [](const RenderTexture2D& rt, const char* name) {
                Image img = LoadImageFromTexture(rt.texture);
                ImageFlipVertical(&img);   // reflRT 纹理 Y 向下；主窗口 blit 用负高翻转，导出同样要翻
                ExportImage(img, name);
                UnloadImage(img);
                TraceLog(LOG_INFO, "DBG: exported %s", name);
            };
            // 传送到湖边北岸：保证视野内有大片水面（验证水面倒影用）
            auto GotoWater = []() {
                for (int ty = 4; ty < MAP_H - 4; ty++)
                    for (int tx = 4; tx < MAP_W - 4; tx++) {
                        if (W.TileAt(tx, ty) != Tile::Water) continue;
                        for (int k = 1; k <= 3; k++) {          // 向北找落脚点
                            if (W.TileAt(tx, ty - k) == Tile::Water) continue;
                            if (W.ObjIndexAt(tx, ty - k) >= 0) continue;   // 避开树石
                            P.x = tx * 16.0f + 8.0f;
                            P.y = (ty - k) * 16.0f + 8.0f;
                            return;
                        }
                    }
            };
            // 天体倒影 A/B：同机位隔 2 帧各导出一张（波浪动画近乎同步）
            auto CelDump = [&](int f, const char* onName, const char* offName) {
                if (fno == f)         { DumpRT(reflRT, onName);
                    TraceLog(LOG_INFO, "DBG-CEL: celSX=%.1f celType=%d phase=%.4f "
                             "P=(%.1f,%.1f) cam=(%d,%d)",
                             celSX, celType, DayPhase(gameTime), P.x, P.y, camX, camY); }
                if (fno == f + 1)   { dbgNoCelestial = true; dbgNoStar = true; }
                if (fno == f + 3)     DumpRT(reflRT, offName);
                if (fno == f + 4)   { dbgNoCelestial = false; dbgNoStar = false; }
            };
            // ---- 深夜湖边：验证月亮 + 星辰的水面倒影 ----
            if (fno == 540) {
                // 深夜 0.877：使月亮屏幕 X 落在 ~392（与白天验证过的水面区间重合）
                gameTime = DAY_LEN * 0.877f;
                rainTarget = 0; rainAmt = 0;                     // 锁晴：排除雨幕涟漪干扰
                GotoWater();
            }
            if (fno == 600) DumpRT(lightRT, "shot_a_lightmap.png");   // 夜间光照图
            CelDump(600, "shot_a_night.png", "shot_w_night_nocel.png");
            // 验证期间持续清怪：怪物击退会让玩家(及相机)漂移，污染 A/B 逐像素对比
            if (fno >= 540 && fno <= 620 && fno % 6 == 0) mobs.clear();
            // 夜间分解验证：仅月光柱 / 仅星辰 / 全关（分离两者各自的贡献）
            if (fno == 606) DumpRT(reflRT, "shot_n_all.png");            // 全开
            if (fno == 607) dbgNoStar = true;                            // 只留月光柱
            if (fno == 609) DumpRT(reflRT, "shot_n_moon.png");
            if (fno == 610) { dbgNoCelestial = true; dbgNoStar = false; }// 只留星辰
            if (fno == 612) DumpRT(reflRT, "shot_n_star.png");
            if (fno == 613) { dbgNoCelestial = true; dbgNoStar = true; } // 全关
            if (fno == 615) DumpRT(reflRT, "shot_n_none.png");
            if (fno == 616) { dbgNoCelestial = false; dbgNoStar = false; }
            // ---- 立体屋子验证：传送到最近民居南门外（屋顶/前墙/屋内压暗）----
            if (fno == 618 && !W.houses.empty()) {
                const World::House& h = W.houses[0];
                P.x = h.tx * 16.0f + 8.0f;
                P.y = (h.ty + h.h + 3) * 16.0f + 8.0f;   // 屋南门外
                P.dir = 1;                                 // 面朝屋子
            }
            if (fno == 640) DumpRT(reflRT, "shot_house3d.png");
            // ---- 白天正午湖边：验证太阳的水面倒影 ----
            if (fno == 660) {
                gameTime = DAY_LEN * 0.42f;
                rainTarget = 0; rainAmt = 0;
                GotoWater();
            }
            CelDump(700, "shot_b_day.png", "shot_w_nocel.png");
            if (fno == 740) { P.wood = 99; P.stone = 99; craftOpen = true; }   // 打开合成面板验证
            if (fno == 800) DumpRT(reflRT, "shot_c_craft.png");   // 单页 15 行（无台：等级灰显）
            if (fno == 802) craftSel = 12;                         // 模拟 TAB 连按
            if (fno == 806) DumpRT(reflRT, "shot_c_sel3.png");     // 高亮应移到第 13 行
            if (fno == 814) { craftSel = 0; craftOpen = false; }
            // ---- 新系统验证：鬼域夜景 / 工作台配方 / 环形物品栏 / 合成动画 ----
            if (fno == 830) {                                    // 置于鬼域深夜：验证红域侵蚀圈 + 鬼条
                P.wood = 30; P.iron = 20; P.stone = 20; P.gemShard = 3;
                P.crystal = 5; P.berry = 10;
                gameTime = DAY_LEN * 0.75f;                      // 深夜
                if (W.domainPos.x > 0.0f) {
                    P.x = W.domainPos.x; P.y = W.domainPos.y + 130.0f;   // 域边缘：触发近核心
                }
                InitCreatures(mobs, W, P.x, P.y, gameTime);
                for (Spit& s : spits) s.on = false;
                craftOpen = false;
            }
            if (fno == 900) DumpRT(reflRT, "shot_d_domain.png");  // 鬼域红圈 + 鬼条 + 指针
            if (fno == 910) {                                    // 传送到最近的工作台旁 + 打开炼器面板
                int wi = -1;
                float bd = 1e9f;
                for (size_t i = 0; i < W.objs.size(); i++)
                    if (W.objs[i].kind == ObjKind::Workbench) {
                        float dx = W.objs[i].tx * 16.0f + 8.0f - P.x;
                        float dy = W.objs[i].ty * 16.0f + 8.0f - P.y;
                        float q = dx * dx + dy * dy;
                        if (q < bd) { bd = q; wi = (int)i; }
                    }
                if (wi >= 0) {
                    P.x = W.objs[(size_t)wi].tx * 16.0f + 8.0f;
                    P.y = W.objs[(size_t)wi].ty * 16.0f + 24.0f;
                }
                craftOpen = true;
                TraceLog(LOG_INFO, "DBG-BENCH: benchLv=%d", BenchLvNear());
            }
            if (fno == 970) DumpRT(reflRT, "shot_e_workbench.png");
            if (fno == 980) { craftOpen = false; hotbarOpen = true; }   // 环形物品栏
            if (fno == 1040) DumpRT(reflRT, "shot_f_hotbar.png");
            // 合成动画三阶段：每帧锁定计时器（主循环会推进 craftAnimT）
            if (fno == 1044) {                                   // 重新贴回工作台：怪推挤会让玩家离开合成范围
                int wi3 = W.NearWorkbench(P.x, P.y, 400.0f);
                if (wi3 >= 0) {
                    const WorldObj& wo = CurObjs()[(size_t)wi3];
                    P.x = wo.tx * 16.0f + 8.0f;
                    P.y = wo.ty * 16.0f + 24.0f;
                }
                mobs.clear();                                    // 清怪：避免击退位移污染取帧
            }
            if (fno == 1046) {
                CraftMenu(2);                                    // 合成猎弓（触发动画）
                TraceLog(LOG_INFO, "DBG-CRAFT: animOn=%d wood=%d iron=%d toolLv1=%d",
                         (int)craftAnimOn, P.wood, P.iron, P.toolLv[1]);
            }
            if (fno == 1047) {
                craftAnimT = 0.34f;
                float acx = P.x, acy = P.y - 22.0f;
                int wi2 = W.NearWorkbench(P.x, P.y, 64.0f);
                if (wi2 >= 0) {
                    const WorldObj& wo = CurObjs()[(size_t)wi2];
                    acx = wo.tx * 16.0f + 8.0f; acy = wo.ty * 16.0f + 4.0f;
                }
                TraceLog(LOG_INFO, "DBG-ANIM: center=(%.1f,%.1f) wi=%d P=(%.1f,%.1f) cam=(%d,%d)",
                         acx - camX, acy - camY, wi2, P.x, P.y, camX, camY);
                TraceLog(LOG_INFO, "DBG-MAT: n=%d ids=%d,%d,%d wh=%dx%d,%dx%d,%dx%d",
                         craftMatN, craftMat[0].id, craftMat[1].id, craftMat[2].id,
                         craftMat[0].width, craftMat[0].height,
                         craftMat[1].width, craftMat[1].height,
                         craftMat[2].width, craftMat[2].height);
            }
            if (fno == 1048) DumpRT(reflRT, "shot_g1_place.png");   // P1 材料环绕摆放
            if (fno == 1050) craftAnimT = 0.58f;
            if (fno == 1051) DumpRT(reflRT, "shot_g2_crash.png");   // P2 依次相撞
            if (fno == 1053) craftAnimT = 0.95f;
            if (fno == 1054) DumpRT(reflRT, "shot_g3_fuse.png");    // P3 融合产物
            // ---- 新系统验证：护甲/Buff/伙伴 HUD + 环形栏 10 槽 ----
            if (fno == 1100) {                                   // 回出生点：合成铁甲 + Buff + 伙伴
                P.x = W.spawn.x; P.y = W.spawn.y;
                InitCreatures(mobs, W, P.x, P.y, gameTime);
                craftOpen = false;
                P.iron = 20; P.wood = 99; P.stone = 99; P.berry = 12;
                W.PlaceWorkbench((int)(P.x / 16) + 2, (int)(P.y / 16), 3);   // 调试台：3 级
                CraftMenu(7);                                    // 合成铁甲 -> HUD 护甲图标
                P.buffSpdT = 26.0f; P.buffNightT = 85.0f;        // 直接施加 Buff 验证 HUD
                PET.on = true; PET.kind = 0; PET.hp = 47; PET.level = 3;   // 预置伙伴（带等级）
                PET.x = P.x - 18; PET.y = P.y + 6;
            }
            if (fno == 1108) { introOn = false; tutOn = false; }  // 关掉开场卡/新手指导，否则会盖住面板
            if (fno == 1110) gs = GS::Pause;                      // 提前进暂停：验证设置页英文布局
            if (fno == 1116) DumpRT(reflRT, "shot_k_pause.png");
            if (fno == 1120) { gs = GS::Play; KillPlayer(); }     // 死亡结算页英文布局
            if (fno == 1130) DumpRT(reflRT, "shot_l_death.png");
            if (fno == 1132) { gs = GS::Play; bookOpen = true; bookPage = 0; }  // 手册·命途页英文
            if (fno == 1136) DumpRT(reflRT, "shot_o_book_path.png");
            if (fno == 1138) bookPage = 1;                          // 手册·成就页英文
            if (fno == 1142) DumpRT(reflRT, "shot_o_book_achv.png");
            if (fno == 1146) bookPage = 2;                          // 手册·图鉴页英文
            if (fno == 1154) DumpRT(reflRT, "shot_o_book_best.png");
            if (fno == 1156) bookPage = 3;                          // 手册·残卷页英文
            if (fno == 1158) DumpRT(reflRT, "shot_o_book_scroll.png");
            if (fno == 1162) { bookOpen = false; gs = GS::Play; bookPage = 1; }
            if (fno == 1170) DumpRT(reflRT, "shot_h_hud.png");   // 护甲/Buff/伙伴状态栏（关册后）
            if (fno == 1180) { hotbarOpen = true; P.stew = 3; }  // 环形栏 10 槽（含炖肉）
            if (fno == 1240) DumpRT(reflRT, "shot_i_hotbar10.png");

            // ---- 小舟泛舟验证：拥有小舟后站上水面，玩家与小舟应画在水面之上 ----
            if (fno == 1235) {
                hotbarOpen = false;
                PET.on = false;
                P.boat = true;
                P.dir = 0;
                gameTime = DAY_LEN * 0.30f;                    // 白天取帧，画面清晰
                for (int ty = 4; ty < MAP_H - 4; ty++)
                    for (int tx = 4; tx < MAP_W - 4; tx++) {
                        if (W.TileAt(tx, ty) != Tile::Water) continue;
                        P.x = tx * 16.0f + 8.0f;
                        P.y = ty * 16.0f + 8.0f;
                        ty = MAP_H; tx = MAP_W;                // 跳出双层循环
                    }
                TraceLog(LOG_INFO, "DBG-BOAT: sailing=%d P=(%.1f,%.1f)",
                         (int)CalcSailing(), P.x, P.y);
            }
            if (fno == 1238) DumpRT(reflRT, "shot_boat.png");
            // ---- 铁匠屋验证：传送到铁匠旁（NPC + 交互提示 + 工作台授予流程）----
            if (fno == 1240) {
                hotbarOpen = false;
                P.boat = false;
                P.dead = false; P.hp = P.maxHp;
                gameTime = DAY_LEN * 0.30f;
                if (W.smithPos.x > 0.0f) {
                    P.x = W.smithPos.x;
                    P.y = W.smithPos.y + 14.0f;
                    mobs.clear();
                    // 预置一只鬼仆：验证 E 交互换工作台
                    P.captureLv = 1;
                    GhostAlly g;
                    g.kind = CreatureKind::GhostChild;
                    g.maxHp = g.hp = 8; g.dmg = 9; g.spd = 60; g.radius = 6;
                    g.life = 300.0f;
                    g.x = P.x - 20; g.y = P.y;
                    ghosts.push_back(g);
                    TraceLog(LOG_INFO, "DBG-SMITH: pos=(%.0f,%.0f) benchLv(before)=%d",
                             W.smithPos.x, W.smithPos.y, SmithBenchLv());
                }
            }
            if (fno == 1244) TryInteract();                     // E：给鬼仆换 1 级工作台
            if (fno == 1246)
                TraceLog(LOG_INFO, "DBG-SMITH: benchLv(after)=%d ghosts=%zu (want 1 / 0)",
                         SmithBenchLv(), ghosts.size());
            if (fno == 1250) DumpRT(reflRT, "shot_s_smith.png"); // 铁匠 NPC + 交互提示 + 台
            // ---- 树木碰撞体积验证：从树正南 60px 处以 1px 步推向树，测最小可达距离 ----
            if (fno == 1290) {
                const ObjKind ks[4] = { ObjKind::Tree, ObjKind::TreePine,
                                        ObjKind::TreeBirch, ObjKind::TreePalm };
                float res[4] = { -1, -1, -1, -1 };
                for (int ki = 0; ki < 4; ki++)
                    for (size_t oi = 0; oi < W.objs.size(); oi++) {
                        if (W.objs[oi].kind != ks[ki]) continue;
                        const WorldObj& o = W.objs[oi];
                        if (W.ObjIndexAt(o.tx, o.ty) != (int)oi) continue;
                        bool isolated = true;
                        for (int dy = -2; dy <= 2 && isolated; dy++)
                            for (int dx = -2; dx <= 2; dx++) {
                                if (!dx && !dy) continue;
                                int j = W.ObjIndexAt(o.tx + dx, o.ty + dy);
                                if (j >= 0 && W.ObjSolid(W.objs[(size_t)j].kind)) {
                                    isolated = false; break;
                                }
                            }
                        if (!isolated) continue;
                        float tx = o.tx * 16.0f + 8.0f, ty = o.ty * 16.0f + 8.0f;
                        float py2 = ty + 40.0f;                  // 起点：树南 40px
                        if (!W.CircleFree(tx, py2, 6.0f)) continue;   // 起点被挡则换一棵
                        for (int st = 0; st < 200; st++) {       // 向正北推进直到被挡住
                            Vector2 np = W.MoveCircle(tx, py2, 6.0f, 0.0f, -1.0f);
                            if (np.y >= py2 - 0.001f) break;
                            py2 = np.y;
                        }
                        res[ki] = py2 - ty;
                        break;
                    }
                TraceLog(LOG_INFO, "DBG-TREE: min-approach dist (player r=6) "
                         "oak=%.2f pine=%.2f birch=%.2f palm=%.2f",
                         res[0], res[1], res[2], res[3]);
            }
            // ---- 营地验证：建庇护屋 + 升级扩容 + 幸存者入住（领地系统）----
            if (fno == 1400) {
                if (W.campsite.x > 0.0f) {
                    P.x = W.campsite.x;
                    P.y = W.campsite.y + 14.0f;
                    P.wood = 200; P.stone = 100; P.iron = 20;
                }
            }
            if (fno == 1404) TryInteract();                     // E：建庇护屋（消耗 50 木）
            if (fno == 1406) {
                TraceLog(LOG_INFO, "DBG-CAMP: built=%d lv=%d wood=%d (want 1/1/150)",
                         (int)campBuilt, campLv, P.wood);
                // 把最近的幸存者拉到营地旁（模拟带回）
                for (Npc& n : npcs)
                    if (n.kind == 1 && n.state == 0) {
                        n.state = 1;
                        n.x = campCenter.x + 20.0f;
                        n.y = campCenter.y + 10.0f;
                        break;
                    }
            }
            if (fno == 1420) UpdateNpcs(1.0f / 60.0f);           // 推进入住判定
            if (fno == 1422) {
                int settled = 0;
                for (const Npc& n : npcs) if (n.kind == 1 && n.state == 2) settled++;
                TraceLog(LOG_INFO, "DBG-CAMP: settled=%d cap=%d (want 1/2)", settled, CampCap());
                // 升级营地两次：Lv1 -> Lv3（收容 6 人）
                TryInteract(); TryInteract();
                TraceLog(LOG_INFO, "DBG-CAMP: lvAfter=%d cap=%d (want 3/6)", campLv, CampCap());
            }
            if (fno == 1430) DumpRT(reflRT, "shot_m_camp.png");  // 营地 HUD（等级+入住）+ 幸存者
            // ---- 无脸鬼还原验证：记忆篡改（假血条）+ 窃取经验 + 染血人脸覆盖 ----
            if (fno == 1450) {
                gs = GS::Play; P.dead = false; P.hp = P.maxHp;
                P.x = W.spawn.x; P.y = W.spawn.y;
                mobs.clear();
                // 放一只无脸鬼贴脸（直接命中触发记忆篡改）
                Creature fc;
                fc.kind = CreatureKind::Faceless;
                fc.x = P.x; fc.y = P.y - 20.0f;
                fc.hp = MobBaseHp(CreatureKind::Faceless);
                fc.state = AState::Chase;
                fc.triggered = true;
                fc.atkCd = 0; fc.timer = 0.1f;
                fc.state = AState::Windup;
                mobs.push_back(fc);
                P.xp = 40;
                TraceLog(LOG_INFO, "DBG-FACE: xpBefore=%d hp=%d", P.xp, P.hp);
            }
            if (fno == 1460) {                                   // 无脸鬼前摇结束，命中生效
                TraceLog(LOG_INFO, "DBG-FACE: hacked=%d fakeHp=%d faceCover=%.1f (want 1/150/4.0)",
                         (int)P.hacked, P.fakeHp, P.faceCoverT);
            }
            if (fno == 1464) DumpRT(reflRT, "shot_face_cover.png");   // 血脸覆盖演出
            if (fno == 1470) {                                   // 血条显示假值：截图核对
                TraceLog(LOG_INFO, "DBG-FACE: realHp=%d fakeHp=%d hacked=%d", P.hp, P.fakeHp, (int)P.hacked);
            }
            if (fno == 1474) DumpRT(reflRT, "shot_fakebar.png");     // 假血条 HUD
            // ---- 无脸鬼收鬼工具链验证：收服即追回被偷经验 + 解除篡改 ----
            if (fno == 1476) {
                // 记下当前经验；把那只无脸鬼打残（活体收服：越残越好收）
                int xpBefore = P.xp;
                int stolenRec = P.stolenXp;
                P.captureLv = 3;
                captureCd = 0;
                for (Creature& c : mobs)
                    if (c.kind == CreatureKind::Faceless && c.state != AState::Dead) {
                        c.hp = 1;                                 // 打残：成功率 ~78%
                        break;
                    }
                TraceLog(LOG_INFO, "DBG-FACETOL: xp=%d stolen=%d hacked=%d (want hacked=1)",
                         xpBefore, stolenRec, (int)P.hacked);
            }
            // 循环尝试收服（失败会定住重试；78%/次，8 次内基本必成）
            if (fno >= 1478 && fno <= 1495) {
                captureCd = 0;                                    // 测试加速
                bool still = false;
                for (const Creature& c : mobs)
                    if (c.kind == CreatureKind::Faceless && c.state != AState::Dead) still = true;
                if (still) TryCapture();
            }
            // 牵引束演出验证：施法帧截屏（珠链+漩涡应清晰可见）
            if (fno == 1482) {
                int beamOn = 0;
                for (const CapFx& f : capFxs) if (f.on && f.kind == 0) beamOn++;
                TraceLog(LOG_INFO, "DBG-CAPFX: beamOn=%d (want 1)", beamOn);
                DumpRT(reflRT, "shot_capbeam.png");
            }
            // 身份归位金环验证：收服成功帧附近截屏
            if (fno == 1490) {
                int ringOn = 0;
                for (const CapFx& f : capFxs) if (f.on && f.kind == 1) ringOn++;
                TraceLog(LOG_INFO, "DBG-CAPFX: goldRingOn=%d (may be 0/1, timing-dependent)", ringOn);
            }
            if (fno == 1496) {
                bool hasFace = false;
                for (const GhostAlly& g : ghosts)
                    if (g.kind == CreatureKind::Faceless) { hasFace = true; break; }
                TraceLog(LOG_INFO, "DBG-FACETOL: ghostFaceless=%d hacked=%d xp=%d stolen=%d "
                         "(want 1/0/xp增/stolen=0)",
                         (int)hasFace, (int)P.hacked, P.xp, P.stolenXp);
            }
            if (fno == 1498) DumpRT(reflRT, "shot_face_ally.png"); // 无脸鬼鬼仆 HUD
            // ---- 错乱特技验证：无脸鬼鬼仆撕咬 → 敌鬼记忆错乱（15%/击，循环打必触发）----
            if (fno == 1500) {
                // 放一只厚血靶鬼贴着无脸鬼鬼仆
                float gx0 = 0, gy0 = 0;
                for (const GhostAlly& g : ghosts)
                    if (g.kind == CreatureKind::Faceless) { gx0 = g.x; gy0 = g.y; break; }
                Creature tgt;
                tgt.kind = CreatureKind::GhostAdult;
                tgt.x = gx0 + 20.0f; tgt.y = gy0;
                tgt.hp = 5000; tgt.state = AState::Idle; tgt.timer = 9.0f;
                tgt.triggered = true;
                mobs.push_back(tgt);
            }
            if (fno >= 1502 && fno <= 1506) {
                // 直接驱动撕咬（绕过攻击冷却；每帧 10 击，15%/击 → 5 帧 ~99.9% 必触发）
                for (int hits = 0; hits < 10; hits++)
                    for (const GhostAlly& g : ghosts)
                        if (g.kind == CreatureKind::Faceless)
                            for (Creature& c : mobs)
                                if (c.kind == CreatureKind::GhostAdult && c.state != AState::Dead &&
                                    c.confuseT <= 0.0f)
                                    HitMobByGhost(c, CreatureKind::Faceless, 2.0f, 1.0f, 0.0f);
            }
            if (fno == 1507) {
                float conf = -1.0f;
                for (const Creature& c : mobs)
                    if (c.kind == CreatureKind::GhostAdult && c.state != AState::Dead) conf = c.confuseT;
                TraceLog(LOG_INFO, "DBG-CONFUSE: confuseT=%.1f (want >0 = 错乱已触发)", conf);
            }
            if (fno == 1508) DumpRT(reflRT, "shot_confuse.png");   // 头顶问号 + 迷尘
            if (fno == 1510) gs = GS::Pause;                     // 暂停界面（后期场景，与 1110 的早期版区分文件名）
            if (fno == 1520) DumpRT(reflRT, "shot_r_pause_late.png");
            if (fno == 1530) { gs = GS::Play; KillPlayer(); }    // 死亡结算页
            if (fno == 1540) DumpRT(reflRT, "shot_r_death_late.png");
            // ---- 收鬼系统验证：摄魂幡 HUD + 残魂收服 + 活体收容（凭操作收鬼）----
            if (fno == 1560) {                                   // 复活并授予鎏金摄魂幡
                gs = GS::Play; P.dead = false; P.hp = P.maxHp;
                P.captureLv = 3;
                mobs.clear();                                    // 清场：排除游荡鬼吞魂干扰
                SpawnSoulAt(CreatureKind::GhostTeen, P.x + 26, P.y, 0, 1.0f);
                SpawnSoulAt(CreatureKind::ManyFaces, P.x - 30, P.y + 6, 1, 1.2f);
                TraceLog(LOG_INFO, "DBG-CAP: banner=%d souls near player", (int)P.captureLv);
            }
            if (fno == 1570) TryCapture();                       // 收服残魂（Lv3 必成）
            if (fno == 1572) {
                TraceLog(LOG_INFO, "DBG-CAP: ghosts=%zu caught=%d hp=%d",
                         ghosts.size(), P.ghostCaught,
                         ghosts.empty() ? -1 : ghosts[0].hp);
                // 放一只被打残的活鬼在鬼仆旁：验证活体收容（越残血越好收）
                Creature d;
                d.kind = CreatureKind::GhostAdult;
                d.x = ghosts.empty() ? P.x + 24 : ghosts[0].x + 22;
                d.y = ghosts.empty() ? P.y : ghosts[0].y;
                d.hp = 4; d.state = AState::Idle; d.timer = 3.0f;
                d.triggered = true;                              // 正在追杀你（被追杀时凭操作收容）
                mobs.push_back(d);
            }
            // 测试隔离：冻结鬼仆攻击（避免抢先击杀测试目标，专测活体收容）
            if (fno >= 1572 && fno <= 1625)
                for (GhostAlly& g : ghosts) g.atkCd = 5.0f;
            // 连续尝试活体收容：失败会震退定住 1.6s，冷却后重试（拉扯重试）
            if (fno >= 1574 && fno <= 1620 && (fno % 10) == 0) {
                captureCd = 0;                                   // 测试加速：无视冷却
                bool still = false;
                for (const Creature& c : mobs)
                    if (c.kind == CreatureKind::GhostAdult && c.state != AState::Dead) still = true;
                if (still) TryCapture();
            }
            if (fno == 1625) {
                bool still = false; int hp = -1;
                for (const Creature& c : mobs)
                    if (c.kind == CreatureKind::GhostAdult && c.state != AState::Dead) { still = true; hp = c.hp; }
                TraceLog(LOG_INFO, "DBG-LIVECAP: ghosts=%zu aliveGhost=%d hp=%d (want >=2 / 0 / -)",
                         ghosts.size(), (int)still, hp);
            }
            if (fno == 1580) DumpRT(reflRT, "shot_q_ghost.png"); // 鬼仆跟随 + 幡/鬼仆 HUD
            // ---- 幡等级/复刻度验证：Lv1 粗纸幡收 65% 残次品 → H 精炼 → B 幡面板 ----
            if (fno == 1626) {
                P.captureLv = 1;                               // 换粗纸幡：基础复刻度 65%
                ghosts.clear();                                // 容量=幡等级：清场给 Lv1 腾位
                mobs.clear();                                  // 防游荡鬼抢占最近目标
                for (Soul& s : souls) s.on = false;            // 清残魂：防旧高阶残魂挡住幡等级检查
                SpawnSoulAt(CreatureKind::GhostChild, P.x + 22, P.y - 4, 0, 1.0f);
            }
            if (fno == 1630) {
                // 同帧循环收服直至成功（Lv1 65%/次；失败残魂溃散 → 重放再试，60 次内必成）
                for (int tries = 0; tries < 60; tries++) {
                    captureCd = 0;
                    bool soulNear = false;
                    for (const Soul& s : souls) if (s.on) { soulNear = true; break; }
                    if (!soulNear) SpawnSoulAt(CreatureKind::GhostChild, P.x + 22, P.y - 4, 0, 1.0f);
                    size_t before = ghosts.size();
                    TryCapture();
                    if (ghosts.size() > before) break;
                }
            }
            if (fno == 1650) {
                float fid = -1; int hp = -1;
                for (const GhostAlly& g : ghosts) if (g.fid < 0.99f) { fid = g.fid; hp = g.maxHp; break; }
                TraceLog(LOG_INFO, "DBG-FID: lv1Ghost fid=%d%% maxHp=%d (want 65 / >0)",
                         (int)(fid * 100 + 0.5f), hp);
            }
            if (fno >= 1652 && fno <= 1672) { P.gemShard = 9; TryRefine(); }   // +5%/次 → 65 到 100
            if (fno == 1675) {
                float fid = -1; int hp = -1; float dmg = 0;
                for (const GhostAlly& g : ghosts)
                    if (g.kind == CreatureKind::GhostChild && g.fid > 0.99f) { fid = g.fid; hp = g.maxHp; dmg = g.dmg; break; }
                TraceLog(LOG_INFO, "DBG-REFINE: fid=%d%% (want 100) maxHp=%d dmg=%.1f shards=%d",
                         (int)(fid * 100 + 0.5f), hp, dmg, P.gemShard);
            }
            if (fno == 1678) bannerOpen = true;               // B 幡面板：全参数 + 鬼仆名册
            if (fno == 1680) DumpRT(reflRT, "shot_banner.png");
            // ================= 本轮需求截图（开始界面 / 伴生铁匠鬼 / 选中范围 / 鬼域 / 房屋比例）=================
            // 注意：不能把 gs 切成 Title 就完事 —— 主循环的标题分支带 continue，会跳过本夹具导致帧号冻结。
            // 正确做法：临时切 gs 就地 Render() 一次 → 导出 → 立刻切回 Play。
            if (fno == 1692) {
                GS keep = gs;
                gs = GS::Title; titleSel = 0; titleHasSave = true;
                introOn = false; tutOn = false;
                Render();
                DumpRT(reflRT, "shot_r13_title.png");           // 需求六：开始界面
                gs = keep;
            }

            if (fno == 1700) { gs = GS::Play; introOn = false; tutOn = false; NewGame(20260818u); }
            // 清掉树石（只为出图：伴生铁匠鬼的青白鬼体与树冠同色，容易被看花）
            if (fno == 1708) { W.objs.clear(); for (auto& q : W.objAt) q = -1; }
            if (fno == 1712) DumpRT(reflRT, "shot_r13_companion.png");      // 需求一：伴生铁匠鬼在玩家身侧

            if (fno == 1716) { NewGame(20260818u); }                        // 复原世界（后面几帧要用树石）

            // 需求五：普通鬼的"被选中范围"（虚线圈；一只已触发画红、一只蛰伏画蓝）
            if (fno == 1718) {
                mobs.clear();
                Creature a;                                     // 蛰伏鬼童（蓝圈）
                a.kind = CreatureKind::GhostChild;
                a.x = P.x + 96.0f; a.y = P.y - 26.0f;
                a.hp = MobBaseHp(a.kind); a.state = AState::Idle; a.triggered = false;
                mobs.push_back(a);
                Creature b;                                     // 已触发鬼童（红圈）
                b.kind = CreatureKind::GhostChild;
                b.x = P.x - 104.0f; b.y = P.y + 34.0f;
                b.hp = MobBaseHp(b.kind); b.state = AState::Idle; b.triggered = true;
                mobs.push_back(b);
                Creature c;                                     // 游戏鬼（更大半径）
                c.kind = CreatureKind::GameGhost;
                c.x = P.x + 10.0f; c.y = P.y - 96.0f;
                c.hp = MobBaseHp(c.kind); c.state = AState::Idle; c.triggered = false;
                mobs.push_back(c);
            }
            if (fno == 1723) {
                TraceLog(LOG_INFO, "DBG-SIGHT: mobs=%d cam=(%d,%d) P=(%.0f,%.0f) camClamp=%d",
                         (int)mobs.size(), camX, camY, P.x, P.y,
                         (camX <= 0 || camY <= 0 || camX >= MAP_W * TILE - VW || camY >= MAP_H * TILE - VH));
                for (size_t i = 0; i < mobs.size(); i++)
                    TraceLog(LOG_INFO, "DBG-SIGHT  mob%d kind=%d pos=(%.0f,%.0f) scr=(%.0f,%.0f) sr=%.0f trig=%d dead=%d",
                             (int)i, (int)mobs[i].kind, mobs[i].x, mobs[i].y,
                             mobs[i].x - camX, mobs[i].y - camY,
                             MobSightRadius(mobs[i].kind), (int)mobs[i].triggered,
                             (int)(mobs[i].state == AState::Dead));
            }
            if (fno == 1724) DumpRT(reflRT, "shot_r13_sight.png");

            // 需求四：大鬼域（幽蓝结界 + 域主 + 漂移增多的代理游戏鬼）
            if (fno == 1730) {
                domainPurged = false;
                W.domainPos = { P.x + 24.0f, P.y - 8.0f };
                mobs.clear();
                Creature g;
                g.kind = CreatureKind::GhostDomain;
                g.x = P.x + 40.0f; g.y = P.y - 6.0f;
                g.hp = MobBaseHp(g.kind); g.state = AState::Idle;
                mobs.push_back(g);
                GdEnter(0);
                gdSpawnT = 0.35f;                               // 加速代理鬼生成，便于出图
            }
            if (fno == 1744) {                                  // 直接摆 3 只代理鬼 + 同时开 2 场小游戏
                gdT = 20.0f; gdCap = 4;
                gdProxies.clear();
                for (int k = 0; k < 3; k++) {
                    GdProxy pr;
                    float a = (float)k * 2.1f + 0.4f;
                    pr.x = gdCX + cosf(a) * 96.0f;
                    pr.y = gdCY + sinf(a) * 74.0f;
                    pr.game = (unsigned char)(k % GD_GAME_N);
                    pr.selCd = 1.4f; pr.driftA = a; pr.driftT = 1.0f;
                    gdProxies.push_back(pr);
                }
                GdOpenChallenge(0);
                GdOpenChallenge(1);
            }
            // A/B：只差 gdOn 一个变量，连续两帧各导一张 —— 用于确定鬼域到底画了什么
            if (fno == 1746) { gdOn = false; Render(); DumpRT(reflRT, "shot_r13_nodomain.png"); gdOn = true; }
            if (fno == 1747) {              Render(); DumpRT(reflRT, "shot_r13_domainab.png"); }
            if (fno == 1749) {
                TraceLog(LOG_INFO, "DBG-GD: on=%d owner=%d mobs=%d prox=%d gdCX=%.0f gdCY=%.0f "
                         "cam=(%d,%d) dx=%.0f dy=%.0f R=%.0f P=(%.0f,%.0f) dead=%d",
                         (int)gdOn, gdOwnerIdx, (int)mobs.size(), (int)gdProxies.size(),
                         gdCX, gdCY, camX, camY, gdCX - camX, gdCY - camY, gdR, P.x, P.y, (int)P.dead);
            }
            if (fno == 1750) DumpRT(reflRT, "shot_r13_domain.png");         // 结界/代理鬼/小游戏卡片

            // 需求二：房屋比例（屋顶不再是"一整块"）
            if (fno == 1760) {
                GdExit();
                W.domainPos = { 0.0f, 0.0f };
                if (!W.houses.empty()) {
                    const World::House& h = W.houses[0];
                    Vector2 hc = h.Center();
                    P.x = hc.x;
                    P.y = hc.y + (float)(h.h * 16 + 54);        // 站到屋外南侧，整座房子入镜
                    P.dir = 1;
                }
            }
            if (fno == 1766) DumpRT(reflRT, "shot_r13_house.png");

            if (fno >= 1792) { TraceLog(LOG_INFO, "DBG: done"); break; }
        }
#else
        if (IsKeyPressed(KEY_F12)) TakeScreenshot("shot_manual.png");     // 手动截图（F12）
#endif
    }

    // ---- 退出前自动存档（从开始界面直接退出则不动存档）----
    if (!gQuitNoSave && gs != GS::Title) SaveGame();

    W.Unload();
    A.Unload();
    WATER.Unload();
    AU.Unload();
    for (int i = 0; i < 5; i++)
        if (ZhFonts[i].texture.id != 0 && ZhFonts[i].texture.id != GetFontDefault().texture.id)
            UnloadFont(ZhFonts[i]);
    UnloadRenderTexture(sceneRT);
    UnloadRenderTexture(lightRT);
    UnloadRenderTexture(reflRT);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
