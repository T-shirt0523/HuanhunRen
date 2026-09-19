#include "assets.hpp"
#include <map>
#include <vector>
#include <cmath>
#include <cstring>

// ============================================================
// 资源生成实现：调色板像素串 + 程序化像素图形，零外部素材
// 画风：「幽灯冥火」中式微恐（深青黑为底，鬼火磷光为光源，
//       白为丧仪点缀，红只留给危险；见 2026-08-29 设计文档 §1~§3）
// ============================================================

namespace {

// ---- 固定种子伪随机（仅初始化阶段使用）----
unsigned RSeed = 20260818u;
unsigned Rnd() { RSeed = RSeed * 1664525u + 1013904223u; return RSeed >> 8; }
int RndN(int m) { return (int)(Rnd() % (m > 0 ? m : 1)); }

Color CC(int r, int g, int b, int a = 255) {
    Color c; c.r = (unsigned char)r; c.g = (unsigned char)g; c.b = (unsigned char)b; c.a = (unsigned char)a; return c;
}

// ---- 幽灯冥火全局色板（设计文档 §1.1，每组暗→亮四阶）----
const Color QH0 = CC(10, 15, 20),   QH1 = CC(13, 26, 31),   QH2 = CC(18, 34, 40),    QH3 = CC(26, 47, 51);     // 幽冥青黑（底色）
const Color LH0 = CC(26, 74, 58),   LH1 = CC(45, 107, 90),  LH2 = CC(77, 255, 184),  LH3 = CC(160, 255, 224);  // 鬼火磷光（主光）
const Color YH0 = CC(13, 34, 51),   YH1 = CC(26, 74, 107),  YH2 = CC(102, 204, 255), YH3 = CC(176, 224, 255);  // 幽蓝魂火（辅光）
const Color DH0 = CC(74, 16, 16),   DH1 = CC(139, 32, 32),  DH2 = CC(204, 48, 48),   DH3 = CC(255, 96, 80);    // 灯笼赤红（警示）
const Color LJ0 = CC(90, 64, 16),   LJ1 = CC(139, 105, 20), LJ2 = CC(212, 160, 23),  LJ3 = CC(255, 224, 138);  // 鎏金铜色（点缀）
const Color ZH0 = CC(58, 56, 53),   ZH1 = CC(107, 101, 96), ZH2 = CC(168, 160, 144), ZH3 = CC(224, 216, 200);  // 纸灰白骨（中性）
const Color SW0 = CC(138, 134, 124), SW1 = CC(184, 180, 168), SW2 = CC(216, 212, 200), SW3 = CC(240, 236, 224); // 素白·丧仪之色
const Color MH0 = CC(3, 4, 7),      MH1 = CC(8, 10, 16),    MH2 = CC(16, 19, 24),    MH3 = CC(26, 29, 36);     // 墨黑·深渊之色

// 区域搬移（用于程序化动画变体：把一块像素整体偏移）
struct Shift { int x0, y0, x1, y1, dx, dy; };

// 把像素字符串网格化后按参数绘制到 Image
// cutMask: bit0=裁掉左腿末行  bit1=裁掉右腿末行（玩家行走动画用）
void Blit(Image& img, const char* const* rows, int nRows, const std::map<char, Color>& pal,
          int ox, int oy, int dyAll, const std::vector<Shift>* shifts, int cutMask)
{
    // 1. 解析到临时网格
    int w = 0;
    for (int i = 0; i < nRows; i++) { int L = (int)strlen(rows[i]); if (L > w) w = L; }
    int h = nRows;
    std::vector<Color> grid((size_t)w * h, CC(0, 0, 0, 0));
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w && x < (int)strlen(rows[y]); x++) {
            auto it = pal.find(rows[y][x]);
            if (it != pal.end()) grid[(size_t)y * w + x] = it->second;
        }
    // 2. 区域搬移（腿/手臂动画）
    if (shifts)
        for (const Shift& s : *shifts) {
            std::vector<Color> tmp((size_t)w * h, CC(0, 0, 0, 0));
            for (int y = s.y0; y <= s.y1 && y < h; y++)
                for (int x = s.x0; x <= s.x1 && x < w; x++) {
                    tmp[(size_t)y * w + x] = grid[(size_t)y * w + x];
                    grid[(size_t)y * w + x] = CC(0, 0, 0, 0);
                }
            for (int y = s.y0; y <= s.y1 && y < h; y++)
                for (int x = s.x0; x <= s.x1 && x < w; x++) {
                    int nx = x + s.dx, ny = y + s.dy;
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h)
                        grid[(size_t)ny * w + nx] = tmp[(size_t)y * w + x];
                }
        }
    // 3. 写入 Image
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            Color c = grid[(size_t)y * w + x];
            if (c.a == 0) continue;
            if ((cutMask & 1) && y == h - 1 && x < 8) continue;   // 左腿末行裁掉
            if ((cutMask & 2) && y == h - 1 && x >= 8) continue;  // 右腿末行裁掉
            ImageDrawPixel(&img, ox + x, oy + y + dyAll, c);
        }
}

// 基础像素串 -> 纹理
Texture2D TexFromRows(const char* const* rows, int nRows, const std::map<char, Color>& pal,
                      int outW, int outH, int ox = 0, int oy = 0, int dyAll = 0,
                      const std::vector<Shift>* shifts = nullptr)
{
    Image img = GenImageColor(outW, outH, CC(0, 0, 0, 0));
    Blit(img, rows, nRows, pal, ox, oy, dyAll, shifts, 0);
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(t, TEXTURE_FILTER_POINT);
    return t;
}

// 像素填充圆（硬边像素风）
void FillPixCircle(Image& img, int cx, int cy, int r, Color c) {
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r)
                ImageDrawPixel(&img, cx + x, cy + y, c);
}

// 竖直辟邪桃木剑模板（7x10）与横剑（旋转 90 度，指向 +x）
const char* SWORD_V[10] = {
    "...W...",
    "..WWW..",
    "..WWW..",
    "..WWW..",
    "..WWW..",
    "..WWW..",
    ".GGGGG.",
    "...M...",
    "...M...",
    "...M...",
};
std::map<char, Color> SwordPal() {
    // 辟邪桃木剑：黄褐木剑身（鎏金暗色代木）+ 朱砂护手 + 墨木剑柄
    return { {'W', LJ1}, {'G', DH2}, {'M', MH2} };
}
void DrawSword(Image& img, bool horizontal, int x, int y) {
    auto pal = SwordPal();
    for (int py = 0; py < 10; py++)
        for (int px = 0; px < 7; px++) {
            auto it = pal.find(SWORD_V[py][px]);
            if (it == pal.end()) continue;
            if (!horizontal) ImageDrawPixel(&img, x + px, y + py, it->second);
            else ImageDrawPixel(&img, x + (9 - py), y + px, it->second); // 旋转：指向 +x
        }
}

// 玩家身体像素串（16x24，含站立腿）—— 渡魂人：深斗笠 + 墨色道袍 + 腰挂朱砂葫芦 + 纸白肤色
const char* BODY_DOWN[24] = {
    "................",
    "................",
    ".....OOOOOO.....",
    "....OHHHHHHO....",
    "...OHHHHHHHHO...",
    "..OHHHHHHHHHHO..",
    ".OHHHHHHHHHHHHO.",
    "..OSSSSSSSSSSO..",
    "..OSESSSSSSESO..",
    "..OSSSSSSSSSSO..",
    "...OSSSSSSSSO...",
    "....OOOOOOOO....",
    "...OCCCCCCCCO...",
    "..OSCDDDDDDCSO..",
    "..OSCDDDDDDCSO..",
    "..OSCCDDDDCCSO..",
    "...OCDDDDDDCOR..",
    "....ODDDDDDO.R..",
    ".....OOOOOO.....",
    "....OPPOOPPO....",
    "....OPP..PPO....",
    "....OPP..PPO....",
    "....OBB..BBO....",
    "....OOO..OOO....",
};
const char* BODY_UP[24] = {
    "................",
    "................",
    ".....OOOOOO.....",
    "....OHHHHHHO....",
    "...OHHHHHHHHO...",
    "..OHHHHHHHHHHO..",
    ".OHHHHHHHHHHHHO.",
    "..OHHHHHHHHHHO..",
    "..OHHHHHHHHHHO..",
    "..OHHHHHHHHHHO..",
    "...OHHHHHHHHO...",
    "....OOOOOOOO....",
    "...OCCCCCCCCO...",
    "..OSCDDDDDDCSO..",
    "..OSCDDDDDDCSO..",
    "..OSCCDDDDCCSO..",
    "...OCDDDDDDCOR..",
    "....ODDDDDDO.R..",
    ".....OOOOOO.....",
    "....OPPOOPPO....",
    "....OPP..PPO....",
    "....OPP..PPO....",
    "....OBB..BBO....",
    "....OOO..OOO....",
};
const char* BODY_RIGHT[24] = {
    "................",
    "................",
    ".....OOOOOO.....",
    "....OHHHHHHO....",
    "...OHHHHHHHHO...",
    "..OHHHHHHHHHHO..",
    "...OHSSSSSSEO...",
    "...OHSSSSSSSO...",
    "....OSSSSSSO....",
    ".....OOOOOO.....",
    "....OCCCCCCO....",
    "...OCCCCCCSSO...",
    "...OCCCCCCSSO...",
    "...OCCCCCCCCO...",
    "....OCCCCCO.....",
    "....ODDDDDOR....",
    ".....OOOOO.R....",
    "....OPPOOPPO....",
    "....OPP..PPO....",
    "....OPP..PPO....",
    "....OPP..PPO....",
    "....OBB..BBO....",
    "....OBB..BBO....",
    "....OOO..OOO....",
};

// 生物像素串
// 素兔：纯白 + 青磷描边（眼睛改黑，调色板中处理）
const char* RABBIT[12] = {
    "..OO......OO..",
    "..OWO....OWWO.",
    "..OWO....OWWO.",
    "...OWO..OWWWO.",
    "..OWWWOOWWWWO.",
    ".OWWWWWEWWWWO.",
    ".OWWWWWWWWWWO.",
    ".PWWWWWWWWWWO.",
    "..OWWWWWWWWO..",
    "..OWW.WWW.WO..",
    "..OWO.OOO.WO..",
    "..OOO.....OOO.",
};
// 引魂鹿：灰白毛 + 骨白鹿角，右角挂一盏白灯笼（W 像素，第 2~3 行）
const char* DEER[16] = {
    "....OO....OO......",
    "....OAO...OAO.....",
    "....OAOO.OOAO.W...",
    ".....OAOOAO...W...",
    ".....OOOOO........",
    "....OFFFSO........",
    "....OFFSESO.......",
    "....OFFFSOO.......",
    ".....OSSO.........",
    "...OFFFFFFO.......",
    "..OFSFFFFFSFO.....",
    "..OFSFFFFFSFO.....",
    "..OFFFFFFFFFO.....",
    "..OLF.FFF.FLO.....",
    "..OLL.FFF.LLO.....",
    "..OOO.....OOO.....",
};
// 符尸：额贴黄符（Y）缀朱砂（X），腕缠灰白断链（L 在腕部），红眼保留
const char* ZOMBIE[20] = {
    "................",
    ".....OOOOOO.....",
    "....OYYXXYYO....",
    "...OGYYYYYYGO...",
    "...OGRGGGGRGO...",
    "...OGGGGGGGGO...",
    "....OGGMMGGO....",
    ".....OOOOOO.....",
    "...OTTTTTTTO....",
    "..OTTTTTTTTTO...",
    "..OGTTTTTTTGGO..",
    "..OLTTTTTTTLLO..",
    "...OTTTTTTTTO...",
    "....OTTTTTTO....",
    ".....OOOOOO.....",
    "....OPP.PPO.....",
    "....OPP.PPO.....",
    "....OGG.GGO.....",
    "....OGG.GGO.....",
    "....OOO.OOO.....",
};

// 掉落物 / UI 像素串
// 黑褐断木段（H=黑褐木，Q=断面暗青）
const char* DROP_WOOD[8] = {
    "........",
    "..OOOO..",
    ".OHHHHO.",
    ".OHQHHO.",
    "..OOOO..",
    "........",
    "........",
    "........",
};
const char* DROP_STONE[8] = {
    "........",
    "...OO...",
    "..OGGO..",
    ".OGGGGO.",
    ".OGGGO..",
    "..OOO...",
    "........",
    "........",
};
const char* DROP_BERRY[8] = {
    "........",
    "...O....",
    "..OOO...",
    ".ORRRO..",
    ".ORRRO..",
    "..ORO...",
    "........",
    "........",
};
const char* DROP_MEAT[8] = {
    "........",
    "..OOO...",
    ".OMMMO..",
    ".OMMWO..",
    "..OWO...",
    "...O....",
    "........",
    "........",
};
const char* DROP_COOKED[8] = {
    "........",
    "..OOO...",
    ".OCCCO..",
    ".OCCWO..",
    "..OWO...",
    "...O....",
    "........",
    "........",
};
const char* DROP_ROTTEN[8] = {
    "........",
    "..OOO...",
    ".ONNNO..",
    ".ONKWO..",
    "..OWO...",
    "...O....",
    "........",
    "........",
};
// 朱砂心符
const char* UI_HEART[8] = {
    ".OO.OO..",
    "ORRORRO.",
    "ORRRRRO.",
    "ORRRRRO.",
    ".ORRRO..",
    "..ORO...",
    "...O....",
    "........",
};
// 粗陶碗（饥饿图标，保持 8x8；顶行两缕白汽）
const char* UI_DRUM[8] = {
    "..W..W..",
    "........",
    ".OWWWWO.",
    ".OWWWWO.",
    "..OWWO..",
    "..OWWO..",
    "...OO...",
    "........",
};

} // namespace

// ============================================================

void Assets::Load() {
    // ---------- 玩家（渡魂人） ----------
    // O=墨黑轮廓  S=纸白肤色  H=深斗笠  E=墨黑眼  C=道袍青缘
    // D=墨色道袍  P=下裳  B=布鞋  R=腰挂朱砂葫芦
    std::map<char, Color> palP = {
        {'O', MH0}, {'S', SW3}, {'H', QH2},
        {'E', MH0}, {'C', QH3}, {'D', MH2},
        {'P', MH1}, {'B', ZH0}, {'R', DH2},
    };
    // 攻击剑姿态表：{mode(0竖1横), x, y}（画布 24x28，身体绘制于 (4,4)）
    static const int SWORD[3][3][3] = {
        { {0,13,0}, {1,3,20}, {0,2,14} },   // 下：举剑/挥下/收左下
        { {0,5,0}, {1,3,1},  {0,15,9} },    // 上：举剑/挥上/收右上
        { {0,14,0},{1,13,12},{0,16,15} },   // 右：举剑/前刺/前下收
    };

    for (int d = 0; d < 3; d++) {
        const char* const* body = (d == 0) ? BODY_DOWN : (d == 1) ? BODY_UP : BODY_RIGHT;
        // 待机 2 帧（呼吸下沉）
        for (int f = 0; f < 2; f++) {
            Image img = GenImageColor(24, 28, CC(0, 0, 0, 0));
            Blit(img, body, 24, palP, 4, 4, f, nullptr, 0);
            player[d][f] = LoadTextureFromImage(img); UnloadImage(img);
        }
        // 行走 4 帧：左腿抬 / 站 / 右腿抬 / 站（身体交替下沉 1px 产生弹跳）
        for (int f = 0; f < 4; f++) {
            Image img = GenImageColor(24, 28, CC(0, 0, 0, 0));
            int cut = (f == 0) ? 1 : (f == 2) ? 2 : 0;
            int dy = (f == 1 || f == 3) ? -1 : 0;
            Blit(img, body, 24, palP, 4, 4, dy, nullptr, cut);
            player[d][2 + f] = LoadTextureFromImage(img); UnloadImage(img);
        }
        // 攻击 3 帧：站立身体 + 剑三姿态
        for (int f = 0; f < 3; f++) {
            Image img = GenImageColor(24, 28, CC(0, 0, 0, 0));
            Blit(img, body, 24, palP, 4, 4, 0, nullptr, 0);
            DrawSword(img, SWORD[d][f][0] == 1, SWORD[d][f][1], SWORD[d][f][2]);
            player[d][6 + f] = LoadTextureFromImage(img); UnloadImage(img);
        }
    }
    // 全部设 NEAREST + 白闪剪影
    for (int d = 0; d < 3; d++) {
        for (int f = 0; f < 9; f++) SetTextureFilter(player[d][f], TEXTURE_FILTER_POINT);
        Image img = LoadImageFromTexture(player[d][0]);
        unsigned char* px = (unsigned char*)img.data;
        for (int i = 0; i < img.width * img.height; i++)
            if (px[i * 4 + 3] > 0) { px[i * 4] = 255; px[i * 4 + 1] = 255; px[i * 4 + 2] = 255; }
        playerWhite[d] = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(playerWhite[d], TEXTURE_FILTER_POINT);
    }

    // ---------- 生物 ----------
    // 素兔：纯白毛 + 青磷描边 + 黑眼（唯一不带鬼气的活物）
    std::map<char, Color> palR = { {'O',LH1}, {'W',SW3}, {'P',SW1}, {'E',MH0} };
    rabbit[0] = TexFromRows(RABBIT, 12, palR, 14, 12);
    rabbit[1] = TexFromRows(RABBIT, 12, palR, 14, 12, 0, 0, 1);
    rabbit[2] = TexFromRows(RABBIT, 12, palR, 14, 12);
    {   // walk 帧：后腿收起（腿区上移 1px）
        std::vector<Shift> sh = { {2,9,11,11,0,-1} };
        rabbit[3] = TexFromRows(RABBIT, 12, palR, 14, 12, 0, 0, 0, &sh);
    }
    // 引魂鹿：灰白毛 + 骨白鹿角（如白灯笼）+ 空洞黑眼 + 角挂白灯笼
    std::map<char, Color> palD = { {'O',MH1}, {'F',SW1}, {'S',SW2},
                                   {'E',MH0}, {'A',SW3}, {'L',ZH0}, {'W',SW3} };
    deer[0] = TexFromRows(DEER, 16, palD, 18, 16);
    deer[1] = TexFromRows(DEER, 16, palD, 18, 16, 0, 0, 1);
    deer[2] = TexFromRows(DEER, 16, palD, 18, 16);
    {   // walk 帧：左前腿抬起
        std::vector<Shift> sh = { {2,13,8,15,0,-1} };
        deer[3] = TexFromRows(DEER, 16, palD, 18, 16, 0, 0, 0, &sh);
    }
    // 符尸：尸绿肤 + 黄符朱砂 + 红眼 + 灰白断链缠腕
    std::map<char, Color> palZ = { {'O',MH1}, {'G',LH1}, {'R',DH3},
                                   {'M',MH0}, {'T',MH3}, {'P',MH2},
                                   {'Y',LJ2}, {'X',DH2}, {'L',ZH2} };
    zombie[0] = TexFromRows(ZOMBIE, 20, palZ, 16, 20);
    zombie[1] = TexFromRows(ZOMBIE, 20, palZ, 16, 20, 0, 0, 1);
    zombie[2] = TexFromRows(ZOMBIE, 20, palZ, 16, 20);
    {   // walk 帧：前伸手臂摆动 + 身体下沉
        std::vector<Shift> sh = { {10,10,14,11,1,0} };
        zombie[3] = TexFromRows(ZOMBIE, 20, palZ, 16, 20, 0, 0, 1, &sh);
    }
    // 白闪剪影（站立帧）
    struct WhiteMaker { static Texture2D Make(Texture2D src) {
        Image img = LoadImageFromTexture(src);
        unsigned char* px = (unsigned char*)img.data;
        for (int i = 0; i < img.width * img.height; i++)
            if (px[i * 4 + 3] > 0) { px[i * 4] = 255; px[i * 4 + 1] = 255; px[i * 4 + 2] = 255; }
        Texture2D t = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(t, TEXTURE_FILTER_POINT); return t;
    } };
    rabbitWhite = WhiteMaker::Make(rabbit[0]);
    deerWhite = WhiteMaker::Make(deer[0]);
    zombieWhite = WhiteMaker::Make(zombie[0]);

    // ---------- 新怪物（程序化像素绘制，形状风格差异化） ----------
    auto MobFrame = [&](Texture2D* out, int w, int h, void (*paint)(Image&, int)) {
        Image img = GenImageColor(w, h, CC(0, 0, 0, 0));
        paint(img, -1);
        out[0] = LoadTextureFromImage(img);                       // 待机 A
        UnloadImage(img);
        img = GenImageColor(w, h, CC(0, 0, 0, 0));
        paint(img, 1);                                            // 待机 B（呼吸）
        out[1] = LoadTextureFromImage(img); UnloadImage(img);
        img = GenImageColor(w, h, CC(0, 0, 0, 0));
        paint(img, -1);
        out[2] = LoadTextureFromImage(img); UnloadImage(img);     // 移动 A
        img = GenImageColor(w, h, CC(0, 0, 0, 0));
        paint(img, 2);                                            // 移动 B（迈步）
        out[3] = LoadTextureFromImage(img); UnloadImage(img);
        for (int i = 0; i < 4; i++) SetTextureFilter(out[i], TEXTURE_FILTER_POINT);
    };
    // 丧犬（疾奔者）：墨黑犬 + 青绿鬼火眼窝 + 断裂白绫项圈 + 背脊露骨刺
    auto PaintRunner = [](Image& im, int fr) {
        int dy = (fr == 1) ? 1 : 0;
        // 躯干（横放椭圆，墨黑）
        FillPixCircle(im, 9, 7 + dy, 4, MH2);
        FillPixCircle(im, 8, 8 + dy, 3, QH3);                    // 背部青灰微光
        // 尾巴
        ImageDrawPixel(&im, 3, 5 + dy, MH2); ImageDrawPixel(&im, 4, 5 + dy, MH3);
        // 背脊露骨刺
        ImageDrawPixel(&im, 7, 4 + dy, ZH1); ImageDrawPixel(&im, 9, 4 + dy, ZH1);
        ImageDrawPixel(&im, 11, 5 + dy, ZH0);
        // 头部（右侧）+ 竖耳 + 吻部
        FillPixCircle(im, 15, 5 + dy, 3, MH3);
        ImageDrawPixel(&im, 13, 1 + dy, MH2); ImageDrawPixel(&im, 14, 1 + dy, MH3);
        ImageDrawPixel(&im, 17, 6 + dy, ZH1);                    // 灰白吻部
        ImageDrawPixel(&im, 16, 4 + dy, LH2);                    // 青绿鬼火眼窝
        ImageDrawPixel(&im, 15, 4 + dy, LH1);                    // 眼窝余焰
        // 白绫项圈（颈系断裂白绫）
        ImageDrawPixel(&im, 12, 4 + dy, SW3); ImageDrawPixel(&im, 12, 5 + dy, SW2);
        ImageDrawPixel(&im, 12, 6 + dy, SW1);
        // 四腿（fr==2 时前后交叉）
        int ph = (fr == 2) ? 1 : 0;
        ImageDrawRectangle(&im, 5 - ph, 10 + dy, 2, 4, MH1);
        ImageDrawRectangle(&im, 11, 10 + dy, 2, 4, MH1);
        ImageDrawRectangle(&im, 7 + ph, 10 + dy, 2, 4, MH3);
        ImageDrawRectangle(&im, 13 - ph, 10 + dy, 2, 4, MH3);
    };
    MobFrame(runner, 20, 15, PaintRunner);
    // 溺尸（泡泡怪）：灰绿肿胀躯体 + 幽绿腹囊 + 瘴气泡
    auto PaintSpitter = [](Image& im, int fr) {
        int sq = (fr == 1) ? 1 : 0;
        FillPixCircle(im, 8, 8 + sq, 6, LH1);                    // 灰绿肿胀主体
        FillPixCircle(im, 6, 6 + sq, 3, ZH2);                    // 泡水胀光高光
        for (int i = 0; i < 6; i++)                              // 暗色气孔（固定伪随机位）
            ImageDrawPixel(&im, 5 + (i * 5) % 7, 7 + (i * 3) % 5 + sq, LH0);
        ImageDrawPixel(&im, 6, 9 + sq, ZH3);                     // 浑浊白眼点 x2
        ImageDrawPixel(&im, 10, 9 + sq, ZH3);
        ImageDrawRectangle(&im, 5, 13 + sq, 6, 2, LH0);          // 腹囊（底部阴影袋）
        ImageDrawPixel(&im, 6, 13 + sq, LH2);                    // 腹囊幽绿光
        ImageDrawPixel(&im, 9, 14 + sq, LH2);
        if (fr != 1) {                                           // 滴落瘴气泡（移动帧收起）
            ImageDrawPixel(&im, 7, 15, LH2);
            ImageDrawPixel(&im, 9, 14, LH2);
        }
    };
    MobFrame(spitter, 16, 16, PaintSpitter);
    // 缚链尸王（尸王）：灰青尸身 + 铁链灰白斜纹 + 暗红魂晶胸口 + 肩扛破白灯笼
    auto PaintBrute = [](Image& im, int fr) {
        int dy = (fr == 1) ? 1 : 0;
        int ax = (fr == 2) ? 1 : 0;                              // 手臂前摆相位
        FillPixCircle(im, 10, 8 + dy, 6, ZH1);                   // 灰青尸身躯干
        FillPixCircle(im, 10, 6 + dy, 4, ZH2);                   // 上身亮面
        for (int i = 0; i < 5; i++)                              // 铁链灰白斜纹
            ImageDrawPixel(&im, 6 + i, 5 + i + dy, (i % 2) ? ZH3 : SW1);
        FillPixCircle(im, 10, 2 + dy, 3, ZH1);                   // 头
        ImageDrawPixel(&im, 8, 2 + dy, DH3);                     // 红眼
        ImageDrawPixel(&im, 12, 2 + dy, DH3);
        // 暗红魂晶（胸口）
        ImageDrawPixel(&im, 10, 8 + dy, DH2);
        ImageDrawPixel(&im, 9, 8 + dy, DH1); ImageDrawPixel(&im, 11, 8 + dy, DH1);
        ImageDrawPixel(&im, 10, 7 + dy, DH1); ImageDrawPixel(&im, 10, 9 + dy, DH0);
        // 肩扛破白灯笼（右肩，右下角破损缺口）
        ImageDrawPixel(&im, 15, 2 + dy, ZH0);                    // 灯杆钩
        ImageDrawPixel(&im, 15, 3 + dy, SW2); ImageDrawPixel(&im, 16, 3 + dy, SW1);
        ImageDrawPixel(&im, 15, 4 + dy, SW3);
        // 双臂前伸（左右各一，随迈步交错）
        ImageDrawRectangle(&im, 0, 6 + dy, 5, 3, QH3);
        ImageDrawRectangle(&im, 15 - ax, 7 + dy, 5, 3, QH3);
        ImageDrawRectangle(&im, 14, 6 + dy, 2, 2, ZH0);
        // 粗腿（碎布裤）
        ImageDrawRectangle(&im, 6, 13 + dy, 3, 5, MH3);
        ImageDrawRectangle(&im, 11, 13 + dy, 3, 5, MH3);
        ImageDrawRectangle(&im, 6, 17 + dy, 3, 2, MH1);
        ImageDrawRectangle(&im, 11, 17 + dy, 3, 2, MH1);
    };
    MobFrame(brute, 20, 20, PaintBrute);
    runnerWhite = WhiteMaker::Make(runner[0]);
    spitterWhite = WhiteMaker::Make(spitter[0]);
    bruteWhite = WhiteMaker::Make(brute[0]);

    // ---------- 地牢专属怪物（7 种） ----------
    // 陪葬骨俑（骷髅兵）：白骨缠朽布 + 持断刃 + 幽蓝眼窝
    auto PaintSkeleton = [](Image& im, int fr) {
        int dy = (fr == 1) ? 1 : 0;
        int ax = (fr == 2) ? 1 : 0;
        FillPixCircle(im, 10, 3 + dy, 3, ZH3);                   // 头骨（纸灰白骨）
        ImageDrawPixel(&im, 8, 3 + dy, YH2);                     // 幽蓝眼窝
        ImageDrawPixel(&im, 12, 3 + dy, YH2);
        ImageDrawRectangle(&im, 9, 5 + dy, 2, 1, ZH1);           // 下颌缝
        for (int y = 7; y <= 13; y += 2)                         // 肋骨
            ImageDrawRectangle(&im, 7, y + dy, 6, 1, ZH3);
        ImageDrawRectangle(&im, 7, 7 + dy, 1, 7, ZH1);           // 脊柱
        ImageDrawRectangle(&im, 6 - ax, 7 + dy, 2, 5, ZH2);      // 双臂
        ImageDrawRectangle(&im, 12 + ax, 7 + dy, 2, 5, ZH2);
        ImageDrawRectangle(&im, 7, 13 + dy, 6, 1, QH2);          // 朽布腰缠
        ImageDrawRectangle(&im, 7, 14 + dy, 2, 4, ZH1);          // 腿骨
        ImageDrawRectangle(&im, 11, 14 + dy, 2, 4, ZH1);
        // 断刃（左手斜下）
        ImageDrawPixel(&im, 5 - ax, 7 + dy, ZH2);
        ImageDrawPixel(&im, 4 - ax, 8 + dy, ZH1);
        ImageDrawPixel(&im, 4 - ax, 9 + dy, ZH0);
    };
    MobFrame(skeleton, 20, 19, PaintSkeleton);
    // 尸蝠（洞穴蝠）：墨色翼膜透青脉络 + 幽蓝眼 + 白獠牙
    auto PaintBat = [](Image& im, int fr) {
        int wy = (fr == 1) ? -1 : (fr == 2) ? 1 : 0;             // 翼高相位
        FillPixCircle(im, 10, 6, 4, MH3);                        // 墨色圆身
        FillPixCircle(im, 10, 4, 2, QH3);
        ImageDrawPixel(&im, 8, 6, YH2);                          // 幽蓝眼
        ImageDrawPixel(&im, 12, 6, YH2);
        ImageDrawPixel(&im, 9, 9, SW3);                          // 白獠牙
        ImageDrawPixel(&im, 11, 9, SW3);
        // 双翼（墨色翼膜 + 青脉络相间，上/中/下三相位拍动）
        for (int k = 0; k < 7; k++) {
            int wl = (k < 3) ? k : (6 - k);
            int yy = 5 + wl + wy - 1;
            Color vein = (k % 2 == 0) ? YH1 : MH2;               // 青脉络 / 墨翼膜
            ImageDrawPixel(&im, 3 - k, yy, vein);
            ImageDrawPixel(&im, 17 + k, yy, vein);
        }
        ImageDrawPixel(&im, 9, 1, MH3); ImageDrawPixel(&im, 11, 1, MH3);    // 耳
    };
    MobFrame(bat, 20, 13, PaintBat);
    // 人面蛛（穴蛛）：墨黑躯体 + 背部苍白人面纹 + 八足细长 + 红眼群
    auto PaintSpider = [](Image& im, int fr) {
        int ph = (fr == 2) ? 1 : 0;
        FillPixCircle(im, 12, 8, 5, MH3);                        // 墨黑腹部
        FillPixCircle(im, 12, 6, 3, SW2);                        // 苍白人面底
        ImageDrawPixel(&im, 11, 6, MH0);                         // 人面眼窝
        ImageDrawPixel(&im, 13, 6, MH0);
        ImageDrawPixel(&im, 12, 7, MH1);                         // 人面口
        FillPixCircle(im, 6, 8, 3, MH3);                         // 头胸
        for (int i = 0; i < 4; i++) {                            // 每侧 4 腿（细长）
            int lx = 4 + i * 3;
            int dy = ((i + ph) % 2 == 0) ? -1 : 1;
            ImageDrawPixel(&im, lx, 11 + dy, MH1); ImageDrawPixel(&im, lx - (dy > 0 ? 1 : -1), 12, MH1);
            ImageDrawPixel(&im, 20 - lx + 2, 11 + dy, MH1);
        }
        ImageDrawPixel(&im, 4, 7, DH2);                          // 红眼群（敌意警报）
        ImageDrawPixel(&im, 6, 6, DH2);
        ImageDrawPixel(&im, 7, 8, DH2);
        ImageDrawPixel(&im, 10, 12, SW0);                        // 腹部灰纹
        ImageDrawPixel(&im, 13, 11, SW0);
    };
    MobFrame(spider, 18, 14, PaintSpider);
    // 石傩像（石像鬼）：傩戏面具石像，伪装静止，突进时面具裂开露红光
    auto PaintGargoyle = [](Image& im, int fr) {
        int dy = (fr == 1) ? 1 : 0;
        int wing = (fr == 2) ? 2 : 0;
        bool rush = (fr == 2);                                   // 突进相位
        FillPixCircle(im, 10, 8 + dy, 5, ZH1);                   // 青黑石躯
        FillPixCircle(im, 10, 5 + dy, 3, ZH2);                   // 傩面具头
        ImageDrawPixel(&im, 8, 5 + dy, rush ? DH3 : QH3);        // 面具眼窝（突进露红光）
        ImageDrawPixel(&im, 12, 5 + dy, rush ? DH3 : QH3);
        ImageDrawPixel(&im, 9, 7 + dy, ZH0); ImageDrawPixel(&im, 11, 7 + dy, ZH0);   // 面具鼻孔
        if (rush) {                                              // 面具裂纹
            ImageDrawPixel(&im, 10, 3 + dy, DH1);
            ImageDrawPixel(&im, 10, 4 + dy, DH1);
            ImageDrawPixel(&im, 8, 6 + dy, DH0); ImageDrawPixel(&im, 12, 6 + dy, DH0);
        }
        for (int k = 0; k < 4; k++) {                            // 双翼（石膜）
            ImageDrawPixel(&im, 4 - k, 6 + k / 2 - wing + dy, ZH0);
            ImageDrawPixel(&im, 16 + k, 6 + k / 2 - wing + dy, ZH0);
        }
        ImageDrawRectangle(&im, 7, 12 + dy, 2, 4, ZH0);          // 蹲爪
        ImageDrawRectangle(&im, 11, 12 + dy, 2, 4, ZH0);
        ImageDrawPixel(&im, 6, 16 + dy, ZH1); ImageDrawPixel(&im, 14, 16 + dy, ZH1);
        ImageDrawPixel(&im, 10, 2 + dy, ZH2);                    // 面具冠角
    };
    MobFrame(gargoyle, 20, 18, PaintGargoyle);
    // 白衣怨灵（幽魂）：素白长发白衣，下摆化青烟（alpha 由 DrawMob 控制半透明）
    auto PaintWraith = [](Image& im, int fr) {
        int dy = (fr == 1) ? 1 : 0;
        int wob = (fr == 2) ? 1 : 0;
        for (int y = 2; y <= 10; y++) {                          // 白衣主体（上圆下散）
            int hw = (y <= 6) ? (y) : (11 - y);
            for (int x = 9 - hw; x <= 9 + hw; x++)
                ImageDrawPixel(&im, x, y + dy, SW2);
        }
        for (int y = 4; y <= 7; y++)                             // 白衣高光
            for (int x = 7; x <= 11; x++)
                ImageDrawPixel(&im, x, y + dy, SW3);
        // 墨黑长发（覆面垂落）
        ImageDrawPixel(&im, 7, 3 + dy, MH1); ImageDrawPixel(&im, 8, 2 + dy, MH1);
        ImageDrawPixel(&im, 10, 2 + dy, MH1); ImageDrawPixel(&im, 11, 3 + dy, MH1);
        ImageDrawPixel(&im, 6, 5 + dy, MH1); ImageDrawPixel(&im, 12, 5 + dy, MH1);
        ImageDrawPixel(&im, 7, 6 + dy, MH0);                     // 空洞眼（透出发丝）
        ImageDrawPixel(&im, 12, 6 + dy, MH0);
        for (int k = 0; k < 3; k++)                              // 下摆化青烟（锯齿）
            ImageDrawPixel(&im, 6 + k * 3 + wob, 11 + dy + (k % 2), (k % 2) ? LH1 : YH1);
    };
    MobFrame(wraith, 18, 15, PaintWraith);
    // 守墓俑（岩傀）：陶灰重甲 + 胸口篆纹 + 关节透青光
    auto PaintGolem = [](Image& im, int fr) {
        int dy = (fr == 1) ? 1 : 0;
        int ax = (fr == 2) ? 1 : 0;
        ImageDrawRectangle(&im, 3, 4 + dy, 14, 12, ZH1);         // 陶灰方躯
        ImageDrawRectangle(&im, 4, 5 + dy, 12, 3, ZH2);          // 顶部亮
        ImageDrawRectangle(&im, 3, 14 + dy, 14, 2, ZH0);
        ImageDrawRectangle(&im, 6, 1 + dy, 8, 4, ZH1);           // 头
        ImageDrawPixel(&im, 8, 3 + dy, YH2);                     // 幽蓝符文眼
        ImageDrawPixel(&im, 12, 3 + dy, YH2);
        // 胸口刻篆纹（幽蓝刻痕）
        ImageDrawPixel(&im, 9, 8 + dy, YH1); ImageDrawPixel(&im, 10, 8 + dy, YH1);
        ImageDrawPixel(&im, 11, 8 + dy, YH1);
        ImageDrawPixel(&im, 10, 9 + dy, YH1); ImageDrawPixel(&im, 10, 10 + dy, YH1);
        ImageDrawPixel(&im, 9, 11 + dy, YH1); ImageDrawPixel(&im, 11, 11 + dy, YH1);
        ImageDrawRectangle(&im, 0 - ax, 6 + dy, 4, 6, ZH1);      // 巨臂
        ImageDrawRectangle(&im, 16 + ax, 6 + dy, 4, 6, ZH1);
        ImageDrawRectangle(&im, 5, 16 + dy, 4, 4, ZH0);          // 短腿
        ImageDrawRectangle(&im, 11, 16 + dy, 4, 4, ZH0);
        // 关节透青光
        ImageDrawPixel(&im, 3, 12 + dy, LH2); ImageDrawPixel(&im, 18, 12 + dy, LH2);
        ImageDrawPixel(&im, 6, 16 + dy, LH2); ImageDrawPixel(&im, 15, 16 + dy, LH2);
    };
    MobFrame(golem, 22, 21, PaintGolem);
    // 招魂祭司（缚灵术士）：墨黑袍 + 骨冠 + 手挑白灯笼 + 幽绿魂火
    auto PaintWarlock = [](Image& im, int fr) {
        int dy = (fr == 1) ? 1 : 0;
        int hb = (fr == 2) ? -1 : 0;
        for (int y = 4; y <= 15; y++) {                          // 墨黑长袍（下摆散开）
            int hw = (y <= 8) ? 4 : (4 + (y - 8) / 2);
            for (int x = 9 - hw; x <= 9 + hw; x++)
                ImageDrawPixel(&im, x, y + dy, MH2);
        }
        ImageDrawRectangle(&im, 6, 6 + dy, 8, 2, QH3);           // 腰带（青黑）
        FillPixCircle(im, 9, 3 + dy, 3, MH2);                    // 兜帽
        ImageDrawPixel(&im, 7, 4 + dy, DH3);                     // 兜帽阴影红眼
        ImageDrawPixel(&im, 11, 4 + dy, DH3);
        // 骨冠（冠顶骨白三点）
        ImageDrawPixel(&im, 7, 1 + dy, ZH3); ImageDrawPixel(&im, 9, 0 + dy, ZH3);
        ImageDrawPixel(&im, 11, 1 + dy, ZH3);
        ImageDrawPixel(&im, 5 + hb, 6 + dy, QH3);                // 施法手 + 幽绿魂火
        ImageDrawPixel(&im, 5 + hb, 5 + dy, LH2);
        ImageDrawPixel(&im, 14 - hb, 7 + dy, QH3);               // 挑灯手
        // 手挑白灯笼（右手上方）
        ImageDrawPixel(&im, 14 - hb, 5 + dy, SW2); ImageDrawPixel(&im, 15 - hb, 5 + dy, SW3);
        ImageDrawPixel(&im, 14 - hb, 6 + dy, SW1); ImageDrawPixel(&im, 15 - hb, 6 + dy, SW2);
        ImageDrawPixel(&im, 4, 1 + dy, LH2);                     // 环绕幽绿魂火
        ImageDrawPixel(&im, 15, 2 + dy, LH2);
        ImageDrawPixel(&im, 3, 10 + dy, LH1);
    };
    MobFrame(warlock, 18, 17, PaintWarlock);
    skeletonWhite = WhiteMaker::Make(skeleton[0]);
    batWhite = WhiteMaker::Make(bat[0]);
    spiderWhite = WhiteMaker::Make(spider[0]);
    gargoyleWhite = WhiteMaker::Make(gargoyle[0]);
    wraithWhite = WhiteMaker::Make(wraith[0]);
    golemWhite = WhiteMaker::Make(golem[0]);
    warlockWhite = WhiteMaker::Make(warlock[0]);

    // 血月尸王（Boss）：24x28 —— 缚链尸王放大版 + 头顶血月轮红环 + 周身红雾
    auto PaintBoss = [](Image& im, int fr) {
        int dy = (fr == 1) ? 1 : 0;
        int ax = (fr == 2) ? 1 : 0;                              // 迈步相位
        // 灰青尸身
        FillPixCircle(im, 12, 12 + dy, 8, QH3);                  // 巨躯
        FillPixCircle(im, 12, 9 + dy, 5, ZH1);                   // 上身亮面
        FillPixCircle(im, 12, 20 + dy, 6, MH2);                  // 下身沉暗
        for (int i = 0; i < 7; i++)                              // 铁链灰白斜纹
            ImageDrawPixel(&im, 7 + i, 6 + i + dy, (i % 2) ? ZH3 : SW1);
        // 暗红魂晶（胸口）
        ImageDrawPixel(&im, 12, 10 + dy, DH2);
        ImageDrawPixel(&im, 10, 11 + dy, DH1); ImageDrawPixel(&im, 14, 11 + dy, DH1);
        ImageDrawPixel(&im, 11, 13 + dy, DH1); ImageDrawPixel(&im, 13, 13 + dy, DH1);
        // 头部 + 双角 + 猩红双瞳
        FillPixCircle(im, 12, 3 + dy, 3, ZH1);
        ImageDrawPixel(&im, 9, 0 + dy, MH2); ImageDrawPixel(&im, 9, 1 + dy, QH2);
        ImageDrawPixel(&im, 15, 0 + dy, MH2); ImageDrawPixel(&im, 15, 1 + dy, QH2);
        ImageDrawPixel(&im, 10, 3 + dy, DH3);                    // 猩红瞳
        ImageDrawPixel(&im, 14, 3 + dy, DH3);
        // 血月轮（头顶红环，绕头一周）
        ImageDrawPixel(&im, 8, 0 + dy, DH1); ImageDrawPixel(&im, 16, 0 + dy, DH1);
        ImageDrawPixel(&im, 7, 1 + dy, DH2); ImageDrawPixel(&im, 17, 1 + dy, DH2);
        ImageDrawPixel(&im, 7, 3 + dy, DH3); ImageDrawPixel(&im, 17, 3 + dy, DH3);
        ImageDrawPixel(&im, 7, 5 + dy, DH2); ImageDrawPixel(&im, 17, 5 + dy, DH2);
        ImageDrawPixel(&im, 8, 6 + dy, DH1); ImageDrawPixel(&im, 16, 6 + dy, DH1);
        // 巨爪双臂（随迈步交错摆动）
        ImageDrawRectangle(&im, 1 - ax, 9 + dy, 6, 4, QH3);
        ImageDrawRectangle(&im, 17, 9 + dy, 6, 4, QH3);
        ImageDrawPixel(&im, 0 - ax, 13 + dy, SW3);               // 白骨爪尖
        ImageDrawPixel(&im, 1 - ax, 14 + dy, SW3);
        ImageDrawPixel(&im, 23, 13 + dy, SW3);
        ImageDrawPixel(&im, 22, 14 + dy, SW3);
        // 粗腿
        ImageDrawRectangle(&im, 7, 24 + dy - ax, 4, 3, MH2);
        ImageDrawRectangle(&im, 13, 24 + dy, 4, 3 - ax, MH2);
        if (fr != 1) {                                           // 移动帧红雾滴落
            ImageDrawPixel(&im, 5, 16, DH1);
            ImageDrawPixel(&im, 19, 18, DH1);
        }
    };
    MobFrame(boss, 24, 28, PaintBoss);
    bossWhite = WhiteMaker::Make(boss[0]);

    // ---------- 瓦片 ----------
    for (int v = 0; v < 4; v++) {   // 草原：4 变体（墨绿底 + 青灰草点 + 零星幽蓝磷光像素）
        Image img = GenImageColor(16, 16, LH0);
        for (int i = 0; i < 10; i++) ImageDrawPixel(&img, RndN(15), RndN(14), QH2),
                                     ImageDrawPixel(&img, RndN(15), RndN(14) + 1, QH2);
        for (int i = 0; i < 5; i++) ImageDrawPixel(&img, RndN(16), RndN(16), LH1);
        if (v % 2 == 0) ImageDrawPixel(&img, RndN(16), RndN(16), YH2);   // 低密度磷光
        grass[v] = LoadTextureFromImage(img); UnloadImage(img);
    }
    for (int v = 0; v < 3; v++) {   // 森林地：3 变体（深墨绿腐叶地，偶见骨屑白石点）
        Image img = GenImageColor(16, 16, QH2);
        for (int i = 0; i < 12; i++) ImageDrawPixel(&img, RndN(15), RndN(14), (i % 2) ? MH2 : LH0),
                                     ImageDrawPixel(&img, RndN(15), RndN(14) + 1, (i % 2) ? MH2 : LH0);
        for (int i = 0; i < 3; i++) ImageDrawPixel(&img, RndN(16), RndN(16), LH1);
        ImageDrawPixel(&img, RndN(16), RndN(16), SW1);                   // 骨屑白点
        forest[v] = LoadTextureFromImage(img); UnloadImage(img);
    }
    for (int v = 0; v < 3; v++) {   // 沙岸：3 变体（冷调灰白 + 暗青阴影）
        Image img = GenImageColor(16, 16, ZH2);
        for (int i = 0; i < 8; i++) ImageDrawPixel(&img, RndN(15), RndN(15), ZH1);
        for (int i = 0; i < 4; i++) ImageDrawPixel(&img, RndN(16), RndN(16), ZH3);
        ImageDrawPixel(&img, RndN(16), RndN(16), QH3);                   // 暗青阴影
        sand[v] = LoadTextureFromImage(img); UnloadImage(img);
    }
    for (int f = 0; f < 4; f++) {   // 水面 4 帧：墨青深水 + 幽蓝冷光高光
        Image img = GenImageColor(16, 16, YH0);
        for (int i = 0; i < 7; i++) ImageDrawPixel(&img, RndN(16), RndN(16), QH0);   // 暗杂斑
        for (int i = 0; i < 5; i++) ImageDrawPixel(&img, RndN(16), RndN(16), YH1);   // 亮杂斑
        int y0 = (3 + f * 4) % 16;      // 每帧仅 1 条幽蓝微光短划（随帧移位 -> 安静闪烁）
        int sx = (RndN(11) + f * 2) % 13;
        for (int k = 0; k < 3; k++) ImageDrawPixel(&img, sx + k, y0, YH2);
        water[f] = LoadTextureFromImage(img); UnloadImage(img);
    }

    // ---------- 世界物体 ----------
    {   // 树 32x44：鬼槐（墨绿叶冠带青灰雾边 + 黑褐虬曲树干 + 褪色符纸）
        Image img = GenImageColor(32, 44, CC(0, 0, 0, 0));
        FillPixCircle(img, 16, 15, 13, QH2);                     // 青灰雾边
        FillPixCircle(img, 16, 15, 11, LH0);                     // 墨绿叶冠
        FillPixCircle(img, 11, 18, 7, LH0);
        FillPixCircle(img, 12, 10, 5, LH1);
        FillPixCircle(img, 21, 13, 4, LH1);
        for (int y = 26; y < 44; y++) {   // 黑褐树干
            for (int x = 13; x <= 18; x++) ImageDrawPixel(&img, x, y, MH3);
            ImageDrawPixel(&img, 12, y, MH0);
            ImageDrawPixel(&img, 19, y, MH0);
        }
        for (int x = 13; x <= 18; x++) ImageDrawPixel(&img, x, 26, MH0);
        ImageDrawPixel(&img, 15, 32, LJ2);                       // 褪色符纸
        ImageDrawPixel(&img, 16, 33, LJ1);
        tree = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 松树 24x54：守灵松（深墨绿针叶锥形 + 灰白雾凇，瘦长如立碑）
        Image img = GenImageColor(24, 54, CC(0, 0, 0, 0));
        for (int ly = 0; ly < 3; ly++) {                    // 三层锥叶（上窄下宽）
            int cy = 8 + ly * 11, halfW = 4 + ly * 4;
            for (int y = -6; y <= 6; y++) {
                int wq = halfW - abs(y) / 2;
                for (int x = 12 - wq; x <= 11 + wq; x++) {
                    Color c = (y < 0) ? LH0 : ((x + y) % 3 == 0) ? QH2 : LH0;
                    if ((x + y + ly) % 7 == 0) c = ZH2;          // 灰白雾凇点
                    ImageDrawPixel(&img, x, cy + y, c);
                }
            }
        }
        for (int y = 38; y < 54; y++) {                     // 墨黑细干
            for (int x = 10; x <= 13; x++) ImageDrawPixel(&img, x, y, MH3);
            ImageDrawPixel(&img, 9, y, MH1);
        }
        treePine = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 白桦 26x40：挂幡桦（惨白树干黑纹 + 枝头挂白幡两缕）
        Image img = GenImageColor(26, 40, CC(0, 0, 0, 0));
        FillPixCircle(img, 13, 11, 10, LH0);                     // 墨绿树冠
        FillPixCircle(img, 9, 8, 5, LH1);
        FillPixCircle(img, 17, 12, 5, LH1);
        FillPixCircle(img, 13, 7, 4, QH3);
        for (int y = 20; y < 40; y++) {                     // 惨白桦干
            for (int x = 11; x <= 14; x++) ImageDrawPixel(&img, x, y, SW3);
            ImageDrawPixel(&img, 10, y, SW0);
        }
        ImageDrawRectangle(&img, 11, 24, 4, 1, MH1);             // 黑色横纹
        ImageDrawRectangle(&img, 12, 30, 3, 1, MH1);
        ImageDrawRectangle(&img, 11, 35, 4, 1, MH1);
        // 白幡（枝头白绫，左右各一缕）
        ImageDrawPixel(&img, 4, 13, SW3); ImageDrawPixel(&img, 4, 14, SW2);
        ImageDrawPixel(&img, 4, 15, SW3); ImageDrawPixel(&img, 4, 16, SW1);
        ImageDrawPixel(&img, 22, 13, SW3); ImageDrawPixel(&img, 22, 14, SW2);
        ImageDrawPixel(&img, 22, 15, SW3); ImageDrawPixel(&img, 22, 16, SW1);
        treeBirch = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 棕榈 32x38：断魂椰（枯灰褐弯干 + 残破下垂叶 + 挂一盏熄灭白灯笼）
        Image img = GenImageColor(32, 38, CC(0, 0, 0, 0));
        for (int y = 14; y < 38; y++) {                     // 弯干（向右弯，枯灰褐）
            int x0 = 12 + (y - 14) / 4;
            ImageDrawPixel(&img, x0, y, ZH0);
            ImageDrawPixel(&img, x0 + 1, y, ZH1);
            ImageDrawPixel(&img, x0 + 2, y, MH3);
        }
        int tx = 12 + (38 - 14) / 4 + 1, ty = 14;           // 干顶叶簇
        for (int k = 0; k < 6; k++) {                       // 6 片放射叶（枯败下垂）
            float a = -3.05f + k * 0.62f;
            int ex = tx + (int)(cosf(a) * 11.0f), ey = ty + (int)(sinf(a) * 7.0f);
            for (int s = 0; s <= 10; s++) {
                int px = tx + (int)(cosf(a) * s * 1.1f), py = ty + (int)(sinf(a) * s * 0.7f) + s * s / 22;
                ImageDrawPixel(&img, px, py, (s < 6) ? QH3 : MH3);
            }
            (void)ex; (void)ey;
        }
        // 熄灭白灯笼（挂于叶簇之下）
        ImageDrawPixel(&img, tx + 1, ty + 2, ZH0);          // 吊绳
        ImageDrawPixel(&img, tx + 1, ty + 3, SW2);          // 灯笼身（素白）
        ImageDrawPixel(&img, tx + 1, ty + 4, SW3);
        ImageDrawPixel(&img, tx + 1, ty + 5, SW1);
        treePalm = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 树桩 16x12：断木桩（黑褐断面 + 年轮发暗青 + 边缘青苔磷点）
        Image img = GenImageColor(16, 12, CC(0, 0, 0, 0));
        FillPixCircle(img, 8, 6, 5, MH2);
        FillPixCircle(img, 8, 6, 4, QH2);                    // 年轮暗青
        FillPixCircle(img, 8, 6, 2, QH3);
        for (int x = 3; x <= 12; x++) ImageDrawPixel(&img, x, 9, MH0),
                                      ImageDrawPixel(&img, x, 10, MH3);
        ImageDrawPixel(&img, 4, 8, LH1);                     // 青苔磷点
        ImageDrawPixel(&img, 12, 7, LH1);
        stump = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 石头 20x14：青黑石（冷灰石体 + 青苔痕 + 顶部一线白雾高光）
        Image img = GenImageColor(20, 14, CC(0, 0, 0, 0));
        FillPixCircle(img, 6, 8, 6, ZH0);
        FillPixCircle(img, 13, 8, 5, ZH0);
        FillPixCircle(img, 6, 8, 5, ZH1);
        FillPixCircle(img, 13, 8, 4, ZH1);
        FillPixCircle(img, 5, 6, 2, ZH2);
        FillPixCircle(img, 12, 6, 1, ZH2);
        FillPixCircle(img, 10, 11, 3, ZH0);
        ImageDrawPixel(&img, 5, 4, SW2);                     // 顶部白雾高光
        ImageDrawPixel(&img, 6, 4, SW3); ImageDrawPixel(&img, 12, 5, SW2);
        ImageDrawPixel(&img, 4, 9, LH0);                     // 青苔痕
        ImageDrawPixel(&img, 14, 9, LH0);
        rock = LoadTextureFromImage(img); UnloadImage(img);
    }
    auto MakeBush = [&](bool withBerry) {
        // 血莓丛：黑叶 + 暗红果串（采空后剩黑枝）
        Image img = GenImageColor(16, 12, CC(0, 0, 0, 0));
        FillPixCircle(img, 5, 8, 5, MH3);
        FillPixCircle(img, 11, 8, 5, MH3);
        FillPixCircle(img, 8, 5, 5, MH3);
        FillPixCircle(img, 8, 8, 5, QH3);
        FillPixCircle(img, 6, 6, 2, LH0);
        if (withBerry) {
            ImageDrawPixel(&img, 5, 7, DH1); ImageDrawPixel(&img, 6, 7, DH2);
            ImageDrawPixel(&img, 11, 6, DH1); ImageDrawPixel(&img, 12, 6, DH2);
            ImageDrawPixel(&img, 8, 9, DH1); ImageDrawPixel(&img, 9, 9, DH2);
        }
        Texture2D t = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(t, TEXTURE_FILTER_POINT); return t;
    };
    bushFull = MakeBush(true);
    bushEmpty = MakeBush(false);
    {   // 高草 12x10：墨绿高草（穗尖带青灰）
        Image img = GenImageColor(12, 10, CC(0, 0, 0, 0));
        int bx[4] = { 2, 5, 8, 10 };
        for (int k = 0; k < 4; k++) {
            Color leaf = (k % 2 == 0) ? LH0 : QH3;
            int h = 6 + (k % 3) * 2;
            for (int i = 0; i < h; i++) {
                int x = bx[k] + ((i > h - 3) ? (k % 2 == 0 ? 1 : -1) : 0); // 顶部弯折
                ImageDrawPixel(&img, x, 9 - i, (i == h - 1) ? ZH1 : leaf);  // 穗尖青灰
            }
        }
        tallGrass = LoadTextureFromImage(img); UnloadImage(img);
    }
    for (int v = 0; v < 2; v++) {   // 小花：白菊（保留白）/ 幽蓝冥花（低饱和）
        Image img = GenImageColor(6, 6, CC(0, 0, 0, 0));
        Color petal = (v == 0) ? SW3 : YH1;
        Color core = (v == 0) ? LJ2 : YH2;
        ImageDrawPixel(&img, 2, 1, petal); ImageDrawPixel(&img, 3, 1, petal);
        ImageDrawPixel(&img, 1, 2, petal); ImageDrawPixel(&img, 2, 2, core);
        ImageDrawPixel(&img, 3, 2, core); ImageDrawPixel(&img, 4, 2, petal);
        ImageDrawPixel(&img, 2, 3, petal); ImageDrawPixel(&img, 3, 3, petal);
        flower[v] = LoadTextureFromImage(img); UnloadImage(img);
    }
    for (int f = 0; f < 2; f++) {   // 篝火 2 帧：长明灯阵（黑木三脚架 + 青绿魂焰 + 朱砂符纹地圈）
        Image img = GenImageColor(16, 16, CC(0, 0, 0, 0));
        // 朱砂符纹地圈
        for (int x = 3; x <= 12; x++) ImageDrawPixel(&img, x, 12, (x % 2) ? DH1 : DH0),
                                      ImageDrawPixel(&img, x, 14, (x % 2) ? DH0 : DH1);
        ImageDrawPixel(&img, 2, 13, DH1); ImageDrawPixel(&img, 13, 13, DH1);
        // 黑木三脚架
        ImageDrawPixel(&img, 4, 11, MH2); ImageDrawPixel(&img, 5, 10, MH2); ImageDrawPixel(&img, 6, 9, MH3);
        ImageDrawPixel(&img, 11, 11, MH2); ImageDrawPixel(&img, 10, 10, MH2); ImageDrawPixel(&img, 9, 9, MH3);
        ImageDrawPixel(&img, 7, 10, MH3); ImageDrawPixel(&img, 8, 10, MH3);
        int cx = 8 - f;    // 焰心随帧偏移
        for (int y = 4; y <= 11; y++) {   // 外焰（青绿鬼火）
            int w = (y - 3) / 2;
            for (int x = cx - w; x <= cx + w; x++)
                if (x >= 0 && x < 16) ImageDrawPixel(&img, x, y, LH1);
        }
        for (int y = 7; y <= 11; y++) {   // 内焰（磷光亮芯）
            int w = (y - 6) / 2;
            for (int x = cx - w; x <= cx + w; x++)
                if (x >= 0 && x < 16) ImageDrawPixel(&img, x, y, LH2);
        }
        ImageDrawPixel(&img, cx, 3, LH3);
        ImageDrawPixel(&img, cx + 2, 5, LH2);                  // 魂火火星
        ImageDrawPixel(&img, cx - 2, 2 + f, LH3);
        campfire[f] = LoadTextureFromImage(img); UnloadImage(img);
    }

    // ---------- 地牢贴图（墓穴化） ----------
    for (int v = 0; v < 2; v++) {   // 地牢地面 16x16：青黑石砖 + 冷灰青砖缝，v2 带暗绿苔痕
        Image img = GenImageColor(16, 16, QH2);
        for (int row = 0; row < 4; row++)
            ImageDrawRectangle(&img, 0, row * 4 + 3, 16, 1, MH2);
        for (int row = 0; row < 4; row++)
            ImageDrawRectangle(&img, ((row % 2) ? 5 : 11), row * 4, 1, 4, MH2);
        for (int i = 0; i < 4; i++)
            ImageDrawPixel(&img, RndN(16), RndN(16), QH3);
        if (v == 1) {
            ImageDrawPixel(&img, 4, 10, LH0); ImageDrawPixel(&img, 5, 10, LH1);
            ImageDrawPixel(&img, 11, 3, LH0);
        }
        dunFloor[v] = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(dunFloor[v], TEXTURE_FILTER_POINT);
    }
    {   // 地牢墙 16x16：墓砖墙（深黑青砖 + 顶部一线幽蓝微光，壁缝透光感）
        Image img = GenImageColor(16, 16, MH2);
        ImageDrawRectangle(&img, 0, 0, 16, 1, YH1);            // 顶棱幽蓝微光
        ImageDrawRectangle(&img, 0, 1, 16, 1, QH3);
        ImageDrawRectangle(&img, 0, 14, 16, 2, MH0);           // 底部阴影
        ImageDrawRectangle(&img, 6, 4, 4, 3, QH1);             // 墓砖缝
        ImageDrawRectangle(&img, 1, 9, 5, 3, QH1);
        ImageDrawRectangle(&img, 10, 9, 5, 3, QH1);
        ImageDrawPixel(&img, 13, 5, YH1);                      // 壁缝透光
        dunWall = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(dunWall, TEXTURE_FILTER_POINT);
    }
    {   // 下行楼梯 16x16：青石台阶（下行口边缘渗幽蓝光提示深渊）
        Image img = GenImageColor(16, 16, CC(0, 0, 0, 0));
        Color stepC[4] = { ZH1, QH3, QH2, MH2 };
        for (int s = 0; s < 4; s++)
            ImageDrawRectangle(&img, 2 + s, 2 + s * 3, 12 - s * 2, 3, stepC[s]);
        ImageDrawRectangle(&img, 6, 13, 4, 3, MH0);            // 底部深渊黑洞
        ImageDrawPixel(&img, 5, 13, YH1); ImageDrawPixel(&img, 10, 13, YH1);   // 边缘渗幽蓝光
        ImageDrawPixel(&img, 6, 12, YH1); ImageDrawPixel(&img, 9, 12, YH1);
        dunStairsDown = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(dunStairsDown, TEXTURE_FILTER_POINT);
    }
    {   // 上行楼梯 16x16：青石台阶（顶部一线阳世冷光）
        Image img = GenImageColor(16, 16, CC(0, 0, 0, 0));
        Color stepC[4] = { MH2, QH2, QH3, ZH1 };
        for (int s = 0; s < 4; s++)
            ImageDrawRectangle(&img, 2 + s, 12 - s * 3, 12 - s * 2, 3, stepC[s]);
        ImageDrawRectangle(&img, 6, 0, 4, 3, YH3);             // 顶部冷光
        dunStairsUp = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(dunStairsUp, TEXTURE_FILTER_POINT);
    }
    {   // 地表洞口 20x20：鬼门（青石墓门框 + 半开石门 + 门楣篆纹 + 门内纯墨黑）
        Image img = GenImageColor(20, 20, CC(0, 0, 0, 0));
        FillPixCircle(img, 10, 11, 9, ZH0);                    // 青石门框
        FillPixCircle(img, 10, 11, 8, ZH1);
        FillPixCircle(img, 10, 11, 6, MH0);                    // 门内墨黑
        FillPixCircle(img, 9, 10, 4, MH0);
        ImageDrawRectangle(&img, 12, 8, 3, 7, ZH0);            // 半开石门
        ImageDrawPixel(&img, 12, 8, ZH1);
        ImageDrawPixel(&img, 7, 3, YH1);                       // 门楣篆纹
        ImageDrawPixel(&img, 10, 2, YH1);
        ImageDrawPixel(&img, 13, 3, YH1);
        ImageDrawPixel(&img, 4, 6, ZH2);                       // 碎石高光
        ImageDrawPixel(&img, 16, 15, ZH2);
        ImageDrawPixel(&img, 6, 17, QH3);
        ImageDrawPixel(&img, 15, 5, QH3);
        dunEntrance = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(dunEntrance, TEXTURE_FILTER_POINT);
    }
    {   // 废弃工作台 20x16：镇魂案（黑木案台 + 铜香炉一点鎏金 + 炉口青烟）
        Image img = GenImageColor(20, 16, CC(0, 0, 0, 0));
        ImageDrawRectangle(&img, 2, 7, 16, 6, MH3);            // 黑木案面
        ImageDrawRectangle(&img, 2, 7, 16, 1, QH3);            // 顶棱青光
        ImageDrawRectangle(&img, 3, 13, 3, 3, MH1);            // 案腿
        ImageDrawRectangle(&img, 14, 13, 3, 3, MH1);
        ImageDrawRectangle(&img, 12, 4, 6, 3, LJ1);            // 铜香炉身
        ImageDrawRectangle(&img, 12, 4, 6, 1, LJ2);            // 鎏金炉沿
        ImageDrawPixel(&img, 15, 4, LJ3);                      // 炉沿金高光
        ImageDrawPixel(&img, 13, 7, LJ0); ImageDrawPixel(&img, 16, 7, LJ0);     // 炉足
        ImageDrawPixel(&img, 14, 2, ZH1); ImageDrawPixel(&img, 15, 1, ZH1);     // 炉口青烟
        ImageDrawPixel(&img, 5, 5, MH3);                       // 案上黑木段
        ImageDrawPixel(&img, 6, 5, ZH0);
        ImageDrawPixel(&img, 8, 4, QH3);                       // 刻篆青石片
        ImageDrawPixel(&img, 9, 5, ZH1);
        ImageDrawPixel(&img, 4, 9, LH0);                       // 苔痕（废弃感）
        ImageDrawPixel(&img, 16, 11, LH0);
        workbench = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(workbench, TEXTURE_FILTER_POINT);
    }
    {   // 矿脉 18x14：黑岩矿脉（黑岩体 + 幽蓝晶簇发光，萤晶改幽蓝）
        Image img = GenImageColor(18, 14, CC(0, 0, 0, 0));
        FillPixCircle(img, 6, 8, 6, MH2);
        FillPixCircle(img, 12, 8, 5, MH2);
        FillPixCircle(img, 6, 8, 5, MH3);
        FillPixCircle(img, 12, 8, 4, ZH0);
        // 幽蓝晶簇
        ImageDrawPixel(&img, 5, 6, YH1); ImageDrawPixel(&img, 6, 6, YH3);
        ImageDrawPixel(&img, 5, 7, YH2);
        ImageDrawPixel(&img, 12, 6, YH1); ImageDrawPixel(&img, 13, 7, YH3);
        ImageDrawPixel(&img, 12, 8, YH2);
        ImageDrawPixel(&img, 9, 10, YH1); ImageDrawPixel(&img, 10, 10, YH3);
        ImageDrawPixel(&img, 3, 10, MH1); ImageDrawPixel(&img, 15, 10, MH1);
        oreRock = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(oreRock, TEXTURE_FILTER_POINT);
    }
    {   // 坟冢 18x16：乱葬岗地标（土坟包 + 灰石碑 + 朱砂碑文 + 坟头纸幡 + 散落纸钱）
        Image img = GenImageColor(18, 16, CC(0, 0, 0, 0));
        FillPixCircle(img, 9, 11, 7, (Color) { 62, 48, 36, 255 });      // 坟包底（暗土）
        FillPixCircle(img, 9, 10, 6, (Color) { 92, 74, 56, 255 });      // 坟包
        FillPixCircle(img, 8, 9, 4, (Color) { 116, 94, 70, 255 });      // 受光面
        ImageDrawRectangle(&img, 7, 4, 5, 8, (Color) { 148, 146, 138, 255 });   // 石碑
        ImageDrawRectangle(&img, 7, 4, 5, 1, (Color) { 180, 178, 170, 255 });   // 碑顶棱
        ImageDrawRectangle(&img, 7, 11, 5, 1, (Color) { 104, 102, 96, 255 });   // 碑底阴影
        // 碑文：朱砂四点（远看如"墓"字笔画）
        ImageDrawPixel(&img, 9, 5, (Color) { 176, 32, 32, 255 });
        ImageDrawPixel(&img, 8, 7, (Color) { 176, 32, 32, 255 });
        ImageDrawPixel(&img, 10, 7, (Color) { 176, 32, 32, 255 });
        ImageDrawPixel(&img, 9, 9, (Color) { 176, 32, 32, 255 });
        ImageDrawRectangle(&img, 13, 1, 1, 7, (Color) { 120, 96, 60, 255 });    // 幡杆
        ImageDrawRectangle(&img, 11, 2, 4, 3, (Color) { 216, 212, 200, 230 });  // 纸幡
        ImageDrawPixel(&img, 11, 5, (Color) { 216, 212, 200, 200 });            // 飘带下摆
        ImageDrawPixel(&img, 13, 5, (Color) { 216, 212, 200, 200 });
        ImageDrawPixel(&img, 3, 13, (Color) { 188, 174, 150, 220 });            // 散落纸钱
        ImageDrawPixel(&img, 14, 14, (Color) { 188, 174, 150, 200 });
        ImageDrawPixel(&img, 5, 15, (Color) { 188, 174, 150, 180 });
        graveMound = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(graveMound, TEXTURE_FILTER_POINT);
    }
    {   // 木墙 16x24：立起来的一面墙（顶面 + 墙面 + 墙基），不再是纸片
        // 底部对齐瓦片底边，顶部高出瓦片 8px —— 从斜上方看有明确体量
        Image img = GenImageColor(16, 24, CC(0, 0, 0, 0));
        const Color ZHU = { 176, 32, 32, 240 };      // 朱砂
        // ---- 墙顶（y 0..5）：受光的顶面，亮一档，交代厚度 ----
        ImageDrawRectangle(&img, 0, 0, 16, 6, MH1);
        ImageDrawRectangle(&img, 0, 0, 16, 1, (Color) { 138, 110, 70, 255 });   // 顶棱更亮
        ImageDrawRectangle(&img, 0, 5, 16, 1, MH2);                             // 顶面/墙面交界暗线
        ImageDrawPixel(&img, 3, 2, ZH1); ImageDrawPixel(&img, 10, 3, ZH1);      // 顶面高光点
        // ---- 墙面（y 6..19）：砖砌 + 立柱 ----
        ImageDrawRectangle(&img, 0, 6, 16, 14, MH3);
        for (int y = 9; y < 20; y += 5)                                         // 横向砖缝
            ImageDrawRectangle(&img, 0, y, 16, 1, MH2);
        ImageDrawPixel(&img, 4, 7, MH2);  ImageDrawPixel(&img, 11, 7, MH2);     // 交错竖缝
        ImageDrawPixel(&img, 7, 12, MH2); ImageDrawPixel(&img, 14, 12, MH2);
        ImageDrawPixel(&img, 4, 17, MH2); ImageDrawPixel(&img, 11, 17, MH2);
        ImageDrawRectangle(&img, 0, 6, 2, 14, MH2);                             // 左右立柱
        ImageDrawRectangle(&img, 14, 6, 2, 14, MH2);
        ImageDrawPixel(&img, 2, 8, ZH0); ImageDrawPixel(&img, 13, 15, ZH0);     // 柱面高光
        // ---- 朱砂镇宅符（贴在墙面正中，竖笔 + 三道横）----
        for (int y = 8; y <= 16; y++) ImageDrawPixel(&img, 8, y, ZHU);
        ImageDrawPixel(&img, 7, 11, ZHU); ImageDrawPixel(&img, 9, 11, ZHU);
        ImageDrawPixel(&img, 7, 15, ZHU); ImageDrawPixel(&img, 9, 15, ZHU);
        ImageDrawPixel(&img, 8, 7, (Color) { 226, 60, 56, 255 });               // 符头朱点
        // ---- 墙基（y 20..23）：埋入地面，最暗 + 接地阴影 ----
        ImageDrawRectangle(&img, 0, 20, 16, 4, MH2);
        ImageDrawRectangle(&img, 0, 23, 16, 1, (Color) { 26, 19, 14, 210 });
        ImageDrawRectangle(&img, 1, 20, 14, 1, (Color) { 74, 58, 38, 190 });    // 基础上沿
        wallWood = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(wallWood, TEXTURE_FILTER_POINT);
    }
    {   // 草席床 16x14：竹席底 + 草编纹 + 一端枕头（夜间可入睡）
        Image img = GenImageColor(16, 14, CC(0, 0, 0, 0));
        ImageDrawRectangle(&img, 1, 4, 14, 9, (Color) { 150, 128, 74, 255 });   // 席面
        ImageDrawRectangle(&img, 1, 4, 14, 1, (Color) { 178, 154, 92, 255 });   // 席面上沿
        for (int x = 1; x < 15; x += 2)                                         // 草编竖纹
            ImageDrawRectangle(&img, x, 5, 1, 7, (Color) { 132, 112, 62, 255 });
        ImageDrawRectangle(&img, 1, 12, 14, 1, (Color) { 104, 88, 50, 255 });    // 席底阴影
        ImageDrawRectangle(&img, 2, 2, 5, 3, (Color) { 196, 190, 172, 255 });    // 枕头
        ImageDrawPixel(&img, 3, 3, (Color) { 168, 162, 148, 255 });
        bedStraw = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(bedStraw, TEXTURE_FILTER_POINT);
    }
    {   // 符咒 12x12：黄符纸 + 朱砂符文（竖线 + 三道横 + 顶部小三角）
        Image img = GenImageColor(12, 12, CC(0, 0, 0, 0));
        ImageDrawRectangle(&img, 2, 1, 8, 10, (Color) { 214, 196, 132, 255 });   // 符纸
        ImageDrawRectangle(&img, 2, 1, 8, 1, (Color) { 234, 218, 158, 255 });    // 纸上沿
        ImageDrawRectangle(&img, 2, 10, 8, 1, (Color) { 168, 150, 96, 255 });    // 纸下沿
        for (int y = 2; y <= 9; y++)                                             // 朱砂竖笔
            ImageDrawPixel(&img, 6, y, (Color) { 176, 32, 32, 240 });
        ImageDrawPixel(&img, 4, 4, (Color) { 176, 32, 32, 240 });                // 三道横
        ImageDrawPixel(&img, 5, 4, (Color) { 176, 32, 32, 240 });
        ImageDrawPixel(&img, 7, 6, (Color) { 176, 32, 32, 240 });
        ImageDrawPixel(&img, 8, 6, (Color) { 176, 32, 32, 240 });
        ImageDrawPixel(&img, 4, 8, (Color) { 176, 32, 32, 240 });
        ImageDrawPixel(&img, 5, 8, (Color) { 176, 32, 32, 240 });
        ImageDrawPixel(&img, 6, 2, (Color) { 226, 60, 56, 255 });                // 符头朱点
        talisman = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(talisman, TEXTURE_FILTER_POINT);
    }
    {   // 工作台可合成提示光点 6x6（魂灯青小星）
        Image img = GenImageColor(6, 6, CC(0, 0, 0, 0));
        ImageDrawPixel(&img, 2, 0, LH2);
        ImageDrawPixel(&img, 0, 2, LH2);
        ImageDrawPixel(&img, 5, 2, LH2);
        ImageDrawPixel(&img, 2, 5, LH2);
        ImageDrawPixel(&img, 1, 1, LH3); ImageDrawPixel(&img, 3, 1, LH3);
        ImageDrawPixel(&img, 1, 3, LH3); ImageDrawPixel(&img, 3, 3, LH3);
        ImageDrawPixel(&img, 2, 2, SW3);
        ImageDrawPixel(&img, 2, 4, LH3); ImageDrawPixel(&img, 4, 2, LH3);
        toolBenchGlow = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(toolBenchGlow, TEXTURE_FILTER_POINT);
    }

    // ---------- 工具图标（12x12）----------
    // 通用绘制：基础=黑木柄 + 青石刃；ore=true 时头换冷铁泛青光
    auto ToolTex = [](const char* const* rows, int nRows, bool ore) {
        std::map<char, Color> pal = {
            {'O', MH1},                                        // 轮廓
            {'H', MH3}, {'h', MH1},                            // 黑木柄
            {'W', ZH2}, {'w', ZH1},                            // 青石
            {'M', ore ? YH1 : ZH2},                            // 头（矿石=冷铁青光）
            {'m', ore ? QH3 : ZH1},
            {'g', ore ? YH2 : SW2},                            // 高光（矿石=幽蓝冷光）
            {'S', SW2},                                        // 弓弦
        };
        return TexFromRows(rows, nRows, pal, 12, 12);
    };
    {   // 弓：竖弧弓身 + 弦 + 箭
        static const char* BOW[12] = {
            ".....MM.....",
            "....M..M....",
            "...M....M...",
            "...M.....M..",
            "..M......M..",
            "..M..S...M..",
            ".M...S...M..",
            ".M...S..M...",
            ".M...S..M...",
            ".....S......",
            "....MM......",
            "............",
        };
        toolBow = ToolTex(BOW, 12, false);
        toolBowOre = ToolTex(BOW, 12, true);
    }
    {   // 刀：短刃斜置 + 短柄
        static const char* KNIFE[12] = {
            "............",
            ".........MM.",
            "........MMM.",
            ".......MMMm.",
            "......MMMm..",
            ".....MMMm...",
            "....MMMm....",
            "...HHmm.....",
            "..HHh.......",
            ".HHh........",
            "............",
            "............",
        };
        toolKnife = ToolTex(KNIFE, 12, false);
        toolKnifeOre = ToolTex(KNIFE, 12, true);
    }
    {   // 斧：竖柄 + 斧刃（侧视）
        static const char* AXE[12] = {
            "....MMMM....",
            "...MMMMMM...",
            "..MMMgMMM...",
            "..MMwMMM....",
            "...HHMM.....",
            "...HH.......",
            "...HH.......",
            "...HH.......",
            "...HH.......",
            "...HH.......",
            "..HHHh......",
            "............",
        };
        toolAxe = ToolTex(AXE, 12, false);
        toolAxeOre = ToolTex(AXE, 12, true);
    }
    {   // 锤：竖柄 + 方头
        static const char* HAMMER[12] = {
            "..MMMMMMM...",
            "..MMMMMMM...",
            "..MMgMMMM...",
            "..MMMMMMM...",
            "....HH......",
            "....HH......",
            "....HH......",
            "....HH......",
            "....HH......",
            "....HH......",
            "...HHHh.....",
            "............",
        };
        toolHammer = ToolTex(HAMMER, 12, false);
        toolHammerOre = ToolTex(HAMMER, 12, true);
    }
    {   // 镐：竖柄 + 尖镐头（采矿）
        static const char* PICK[12] = {
            "..M......M..",
            ".M........M.",
            ".M...HH...M.",
            "M...HH.....M",
            "M..HH......M",
            "...HH.......",
            "...HH.......",
            "...HH.......",
            "...HH.......",
            "...HH.......",
            "..HHHh......",
            "............",
        };
        toolPick = ToolTex(PICK, 12, false);
        toolPickOre = ToolTex(PICK, 12, true);
    }
    {   // ---- 收魂幡三阶（收鬼系统：粗纸 / 铜铃 / 鎏金）----
        // 竖杆 + 横杆悬幡：粗纸=素纸幡面 + 一点青绿鬼火；
        // 铜铃=横杆左端悬铜铃；鎏金=金顶金边幡面 + 铜铃 + 亮焰心
        std::map<char, Color> palFan = {
            {'O', MH0},                                    // 墨黑轮廓
            {'H', MH3}, {'h', MH1},                        // 黑木杆
            {'W', SW2}, {'w', ZH2},                        // 幡面素纸
            {'F', LH2}, {'f', LH3},                        // 鬼火 / 焰心
            {'B', LJ2}, {'b', LJ0},                        // 铜铃
            {'G', LJ1},                                    // 鎏金饰边
        };
        static const char* FAN1[12] = {
            "...OO.......",
            "...Hh.......",
            "...HhOOOOOO.",
            "...HhOWWWWO.",
            "...HhOWwWWO.",
            "...HhOWFWWO.",
            "...HhOWFWWO.",
            "...HhOWwWWO.",
            "...HhOOOOOO.",
            "...Hh.......",
            "...Hh.......",
            "............",
        };
        static const char* FAN2[12] = {
            "...OO.......",
            "...Hh.......",
            ".OOHhOOOOOO.",
            ".BbHhOWWWWO.",
            ".OOHhOWwWWO.",
            "...HhOWFWWO.",
            "...HhOWFWWO.",
            "...HhOWwWWO.",
            "...HhOOOOOO.",
            "...Hh.......",
            "...Hh.......",
            "............",
        };
        static const char* FAN3[12] = {
            "...GG.......",
            "...Hh.......",
            ".GGHhGGGGGG.",
            ".BbHhGWWWWG.",
            ".GGHhGWwWWG.",
            "...HhGWFfwG.",
            "...HhGWfFWG.",
            "...HhGWwWWG.",
            "...HhGGGGGG.",
            "...Hh.......",
            "...Hh.......",
            "............",
        };
        toolFan1 = TexFromRows(FAN1, 12, palFan, 12, 12);
        toolFan2 = TexFromRows(FAN2, 12, palFan, 12, 12);
        toolFan3 = TexFromRows(FAN3, 12, palFan, 12, 12);
    }
    {   // 箭矢 10x4（默认朝右）：黑杆 + 幽蓝箭头 + 素白尾羽
        Image img = GenImageColor(10, 4, CC(0, 0, 0, 0));
        for (int x = 0; x < 8; x++) ImageDrawPixel(&img, x, 1, MH3);
        ImageDrawPixel(&img, 8, 1, YH2); ImageDrawPixel(&img, 9, 1, YH3);
        ImageDrawPixel(&img, 8, 0, YH1); ImageDrawPixel(&img, 8, 2, YH1);
        ImageDrawPixel(&img, 0, 0, SW1); ImageDrawPixel(&img, 0, 2, SW1);   // 尾羽
        ImageDrawPixel(&img, 1, 1, SW1);
        arrowItem = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureFilter(arrowItem, TEXTURE_FILTER_POINT);
    }

    // ---------- 掉落物图标 ----------
    // O=墨黑轮廓 W=骨白 G=青黑石 R=血莓暗红 M=生肉暗红 C=酱褐熟肉
    // N=腐肉灰绿 K=青斑 H=黑褐木 Q=断面暗青
    std::map<char, Color> palDrop = {
        {'O',MH0}, {'W',SW3}, {'G',QH3}, {'R',DH1},
        {'M',DH1}, {'C',LJ0}, {'N',ZH1}, {'K',YH1},
        {'H',MH3}, {'Q',QH2},
    };
    dropWood = TexFromRows(DROP_WOOD, 8, palDrop, 8, 8);
    dropStone = TexFromRows(DROP_STONE, 8, palDrop, 8, 8);
    dropBerry = TexFromRows(DROP_BERRY, 8, palDrop, 8, 8);
    dropRawMeat = TexFromRows(DROP_MEAT, 8, palDrop, 8, 8);
    dropCooked = TexFromRows(DROP_COOKED, 8, palDrop, 8, 8);
    dropRotten = TexFromRows(DROP_ROTTEN, 8, palDrop, 8, 8);
    {   // 铁矿石：青灰矿块 + 金属冷光点
        Image im2 = GenImageColor(8, 8, CC(0, 0, 0, 0));
        FillPixCircle(im2, 4, 5, 3, ZH1);
        FillPixCircle(im2, 4, 4, 2, ZH2);
        ImageDrawPixel(&im2, 3, 4, YH3); ImageDrawPixel(&im2, 5, 5, QH3);
        ImageDrawPixel(&im2, 6, 6, MH2);
        dropIronOre = LoadTextureFromImage(im2); UnloadImage(im2);
        SetTextureFilter(dropIronOre, TEXTURE_FILTER_POINT);
    }
    {   // 萤晶：幽蓝晶簇（菱形 + 微光晕，辨识度保留）
        Image im2 = GenImageColor(8, 8, CC(0, 0, 0, 0));
        for (int y = 1; y <= 6; y++) {       // 菱形
            int wq = (y <= 3) ? y : (7 - y);
            for (int x = 4 - wq; x <= 3 + wq; x++)
                ImageDrawPixel(&im2, x, y, (x == 4 - wq || x == 3 + wq) ? YH1
                                                                        : YH2);
        }
        ImageDrawPixel(&im2, 4, 2, YH3);     // 晶面闪光
        ImageDrawPixel(&im2, 3, 4, YH3);
        dropCrystal = LoadTextureFromImage(im2); UnloadImage(im2);
        SetTextureFilter(dropCrystal, TEXTURE_FILTER_POINT);
    }

    // ---------- 护甲 / 新食物 / Boss 材料图标 ----------
    auto IconTex = [&](int w, int h, void (*paint)(Image&)) {
        Image im2 = GenImageColor(w, h, CC(0, 0, 0, 0));
        paint(im2);
        Texture2D t = LoadTextureFromImage(im2); UnloadImage(im2);
        SetTextureFilter(t, TEXTURE_FILTER_POINT);
        return t;
    };
    armorIron = IconTex(10, 10, [](Image& im2) {           // 铁甲：黑铁札甲
        ImageDrawRectangle(&im2, 1, 1, 8, 3, MH3);                 // 肩甲
        ImageDrawRectangle(&im2, 2, 4, 6, 5, MH2);                 // 胸甲主体
        ImageDrawRectangle(&im2, 4, 4, 2, 2, MH0);                 // 领口阴影
        ImageDrawPixel(&im2, 4, 6, ZH1);                           // 金属冷光
        ImageDrawPixel(&im2, 3, 8, MH0); ImageDrawPixel(&im2, 6, 8, MH0);
        ImageDrawRectangle(&im2, 2, 9, 6, 1, MH1);                 // 下摆
    });
    armorCrystal = IconTex(10, 10, [](Image& im2) {        // 萤晶甲：幽蓝晶鳞甲
        ImageDrawRectangle(&im2, 1, 1, 8, 3, YH1);                 // 肩甲
        ImageDrawRectangle(&im2, 2, 4, 6, 5, YH0);                 // 胸甲主体
        ImageDrawPixel(&im2, 4, 5, YH2);                           // 晶面
        ImageDrawPixel(&im2, 5, 6, YH3);                           // 发光高光
        ImageDrawPixel(&im2, 3, 7, YH1);
        ImageDrawPixel(&im2, 6, 5, YH1);
        ImageDrawRectangle(&im2, 2, 9, 6, 1, QH2);
    });
    dropJam = IconTex(8, 8, [](Image& im2) {               // 浆果酱：黑陶罐 + 血莓暗红酱
        ImageDrawRectangle(&im2, 2, 2, 4, 5, DH1);                 // 酱体
        ImageDrawRectangle(&im2, 2, 2, 4, 1, DH2);                 // 酱面高光
        ImageDrawRectangle(&im2, 2, 1, 4, 1, MH3);                 // 黑陶盖
        ImageDrawPixel(&im2, 1, 3, ZH1);                           // 陶身反光
        ImageDrawPixel(&im2, 6, 4, ZH1);
        ImageDrawRectangle(&im2, 2, 7, 4, 1, DH0);
    });
    dropTea = IconTex(8, 8, [](Image& im2) {               // 萤晶茶：青瓷杯 + 幽蓝茶光
        ImageDrawRectangle(&im2, 2, 3, 5, 4, ZH2);                 // 杯体
        ImageDrawRectangle(&im2, 3, 3, 3, 1, YH2);                 // 幽蓝茶面
        ImageDrawPixel(&im2, 4, 4, YH3);
        ImageDrawPixel(&im2, 7, 4, ZH2);                           // 杯柄
        ImageDrawPixel(&im2, 7, 5, ZH2);
        ImageDrawPixel(&im2, 3, 0, YH2);                           // 上升茶气
        ImageDrawPixel(&im2, 5, 1, YH2);
        ImageDrawRectangle(&im2, 2, 7, 5, 1, ZH1);                 // 杯托
    });
    dropStew = IconTex(8, 8, [](Image& im2) {              // 炖肉：黑陶碗 + 白汽
        ImageDrawRectangle(&im2, 1, 3, 6, 3, MH2);                 // 黑陶碗
        ImageDrawRectangle(&im2, 1, 6, 6, 1, MH1);
        ImageDrawRectangle(&im2, 2, 3, 4, 1, LJ0);                 // 酱褐汤面
        ImageDrawPixel(&im2, 3, 2, DH1);                           // 肉块
        ImageDrawPixel(&im2, 5, 2, SW2);                           // 白汽
        ImageDrawPixel(&im2, 4, 4, LJ1);                           // 热油光
    });
    dropHeart = IconTex(8, 8, [](Image& im2) {             // 血月之心：暗红心脏缠黑线
        ImageDrawPixel(&im2, 2, 1, DH1); ImageDrawPixel(&im2, 5, 1, DH1);
        ImageDrawPixel(&im2, 1, 2, DH1); ImageDrawPixel(&im2, 2, 2, DH2);
        ImageDrawPixel(&im2, 3, 2, DH1); ImageDrawPixel(&im2, 4, 2, DH1);
        ImageDrawPixel(&im2, 5, 2, DH1); ImageDrawPixel(&im2, 6, 2, DH1);
        for (int y = 3; y <= 5; y++)
            for (int x = y - 1; x <= 8 - y; x++)
                ImageDrawPixel(&im2, x, y, (x == 2 && y == 3) ? DH3 : DH1);
        ImageDrawPixel(&im2, 3, 6, DH0); ImageDrawPixel(&im2, 4, 6, DH0);
        ImageDrawPixel(&im2, 4, 7, MH1);                           // 黑线垂滴
        ImageDrawPixel(&im2, 2, 3, DH3);                           // 高光
        // 黑线缠绕
        ImageDrawPixel(&im2, 4, 3, MH1); ImageDrawPixel(&im2, 3, 4, MH1);
        ImageDrawPixel(&im2, 5, 4, MH1); ImageDrawPixel(&im2, 4, 5, MH1);
    });
    boatIcon = IconTex(12, 12, [](Image& im2) {           // 小舟图标：墨黑船身 + 鎏金铜缘 + 船头魂火
        ImageDrawRectangle(&im2, 2, 5, 8, 1, LJ1);                 // 上缘鎏金铜
        ImageDrawRectangle(&im2, 1, 6, 10, 2, MH3);                // 船身
        ImageDrawRectangle(&im2, 2, 8, 8, 1, MH2);                 // 船底
        ImageDrawRectangle(&im2, 3, 9, 6, 1, MH1);                 // 龙骨
        ImageDrawPixel(&im2, 1, 5, MH3); ImageDrawPixel(&im2, 10, 5, MH3);   // 船首尾翘起
        ImageDrawPixel(&im2, 9, 3, MH3); ImageDrawPixel(&im2, 9, 4, MH3);    // 灯杆
        ImageDrawPixel(&im2, 8, 2, LH2); ImageDrawPixel(&im2, 8, 3, LH3);    // 船头魂火
    });
    boat = IconTex(30, 14, [](Image& im2) {               // 小舟世界精灵（泛舟渡海）
        ImageDrawRectangle(&im2, 4, 5, 22, 1, LJ1);                 // 上缘鎏金铜
        ImageDrawRectangle(&im2, 2, 6, 26, 2, MH3);                // 船身
        ImageDrawRectangle(&im2, 3, 8, 24, 2, MH2);                // 船底
        ImageDrawRectangle(&im2, 6, 10, 18, 1, MH1);               // 龙骨
        ImageDrawPixel(&im2, 2, 5, MH3); ImageDrawPixel(&im2, 27, 5, MH3);   // 首尾翘起
        ImageDrawPixel(&im2, 28, 4, MH3);
        ImageDrawRectangle(&im2, 5, 6, 20, 1, MH0);                // 舱内暗影（玩家立于此）
        ImageDrawRectangle(&im2, 25, 2, 1, 3, MH3);                // 船头灯杆
        ImageDrawPixel(&im2, 24, 1, LH2); ImageDrawPixel(&im2, 24, 2, LH3); // 魂火幽灯
    });

    // ---------- 村庄遗迹 ----------
    for (int v = 0; v < 2; v++) {   // 残墙 16x16：坍塌青砖墙（砖缝青苔 + 墙头残破白幡）
        Image im2 = GenImageColor(16, 16, QH3);
        for (int row = 0; row < 4; row++)         // 砖缝（交错）
            ImageDrawRectangle(&im2, 0, row * 4 + 3, 16, 1, QH1);
        for (int row = 0; row < 4; row++)
            ImageDrawRectangle(&im2, ((row % 2) ? 4 : 10), row * 4, 1, 4, QH1);
        ImageDrawPixel(&im2, 3, 6, LH0); ImageDrawPixel(&im2, 4, 6, LH1);
        ImageDrawPixel(&im2, 11, 12, LH0);                         // 青苔
        ImageDrawPixel(&im2, 7, 1, MH3);                           // 风化斑
        // 墙头残破白幡
        ImageDrawPixel(&im2, 2, 0, SW3); ImageDrawPixel(&im2, 2, 1, SW2);
        ImageDrawPixel(&im2, 2, 2, SW3); ImageDrawPixel(&im2, 3, 1, SW1);
        if (v == 1) {           // 破损变体：右上缺块
            ImageDrawRectangle(&im2, 11, 0, 5, 5, CC(0, 0, 0, 0));
            ImageDrawRectangle(&im2, 13, 0, 3, 7, CC(0, 0, 0, 0));
        }
        ruinWall[v] = LoadTextureFromImage(im2); UnloadImage(im2);
        SetTextureFilter(ruinWall[v], TEXTURE_FILTER_POINT);
    }
    auto MakeChest = [&](bool open) {
        // 陪葬漆棺箱：黑漆木箱 + 鎏金铜扣（开启露暗红内衬）
        Image im2 = GenImageColor(16, 14, CC(0, 0, 0, 0));
        unsigned char la = open ? 255 : 255;
        (void)la;
        // 箱体（黑漆）
        ImageDrawRectangle(&im2, 2, 6, 12, 7, MH3);
        ImageDrawRectangle(&im2, 2, 6, 12, 1, MH0);
        ImageDrawRectangle(&im2, 3, 9, 10, 1, MH1);              // 漆板缝
        // 盖子：闭合平放 / 开启竖立（露出暗红内衬）
        if (!open) {
            ImageDrawRectangle(&im2, 1, 3, 14, 3, MH3);
            ImageDrawRectangle(&im2, 1, 3, 14, 1, QH3);
            ImageDrawRectangle(&im2, 7, 5, 2, 3, LJ2);           // 鎏金铜扣
        } else {
            ImageDrawRectangle(&im2, 1, 0, 14, 3, MH3);
            ImageDrawRectangle(&im2, 2, 6, 12, 2, DH0);          // 暗红内衬
            ImageDrawPixel(&im2, 7, 6, LJ2);                     // 铜扣垂落
        }
        return LoadTextureFromImage(im2);
    };
    chestClosed = MakeChest(false); SetTextureFilter(chestClosed, TEXTURE_FILTER_POINT);
    chestOpen = MakeChest(true);   SetTextureFilter(chestOpen, TEXTURE_FILTER_POINT);
    {   // 探索指引箭头 12x12：幽蓝魂火蝶（默认朝上，渲染时按方向旋转）
        Image im2 = GenImageColor(12, 12, CC(0, 0, 0, 0));
        static const int WP[14][2] = { {3,2},{2,3},{3,3},{4,3},{1,4},{2,4},{3,4},{4,4},
                                       {2,5},{3,5},{4,5},{3,6},{4,6},{4,7} };
        for (int i = 0; i < 14; i++) {                            // 双翼（左右镜像）
            ImageDrawPixel(&im2, WP[i][0], WP[i][1], YH2);
            ImageDrawPixel(&im2, 11 - WP[i][0], WP[i][1], YH2);
        }
        for (int y = 3; y <= 7; y++)                              // 蝶身
            ImageDrawPixel(&im2, 5, y, YH1), ImageDrawPixel(&im2, 6, y, YH1);
        ImageDrawPixel(&im2, 4, 1, YH2); ImageDrawPixel(&im2, 7, 1, YH2);    // 触须
        ImageDrawPixel(&im2, 1, 4, YH3); ImageDrawPixel(&im2, 10, 4, YH3);   // 翼尖魂光
        arrowHint = LoadTextureFromImage(im2); UnloadImage(im2);
        SetTextureFilter(arrowHint, TEXTURE_FILTER_POINT);
    }

    // ---------- UI 图标 ----------
    // O=墨黑轮廓  R=朱砂心符  W=粗陶碗（饥饿图标）
    std::map<char, Color> palUI = {
        {'O',MH0}, {'R',DH2}, {'W',ZH1},
    };
    heart = TexFromRows(UI_HEART, 8, palUI, 8, 8);
    drumstick = TexFromRows(UI_DRUM, 8, palUI, 8, 8);
    {   // 太阳 12x12：惨白薄日（白盘 + 极淡青晕）
        Image img = GenImageColor(12, 12, CC(0, 0, 0, 0));
        FillPixCircle(img, 6, 6, 3, SW3);
        ImageDrawPixel(&img, 6, 6, SW3);
        for (int k = 0; k < 8; k++) {   // 淡晕光线
            float a = k * 0.785398f;
            int x = 6 + (int)(round(cosf(a) * 5.0f)), y = 6 + (int)(round(sinf(a) * 5.0f));
            ImageDrawPixel(&img, x, y, SW1);
        }
        sunIcon = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 月亮 10x10：幽白弯月（圆减偏移圆=弯月）
        Image img = GenImageColor(10, 10, CC(0, 0, 0, 0));
        for (int y = 0; y < 10; y++) for (int x = 0; x < 10; x++) {
            int dx = x - 5, dy = y - 5;
            bool inFull = (dx * dx + dy * dy <= 16);
            int dx2 = x - 3, dy2 = y - 5;
            bool inCut = (dx2 * dx2 + dy2 * dy2 <= 12);
            if (inFull && !inCut) ImageDrawPixel(&img, x, y, SW2);
        }
        moonIcon = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 经验星标 8x8：幽蓝魂火（两个叠错菱形）
        Image im2 = GenImageColor(8, 8, CC(0, 0, 0, 0));
        for (int y = 1; y <= 7; y++) {
            int wq = (y <= 3) ? y - 1 : (y <= 5 ? 2 : (y == 6 ? 1 : 0));
            for (int x = 4 - wq; x <= 3 + wq; x++)
                ImageDrawPixel(&im2, x, y, y < 4 ? YH2 : YH1);
        }
        starIcon = LoadTextureFromImage(im2); UnloadImage(im2);
        SetTextureFilter(starIcon, TEXTURE_FILTER_POINT);
    }

    // ---------- 特效纹理 ----------
    {   // 光圈径向渐变 128x128（白->黑，RGB 与 alpha 同步渐变，供减法混合减暗用）
        // 注意：光照机制依赖灰度，颜色不可改
        Image img = GenImageColor(128, 128, CC(0, 0, 0, 0));
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) {
            float d = sqrtf((x - 64) * (x - 64) + (y - 64) * (y - 64) * 1.0f) / 64.0f;
            float a = 1.0f - d;
            if (a < 0) a = 0;
            a = a * a * (3.0f - 2.0f * a) * 1.15f;   // smoothstep 提亮中心
            if (a > 1) a = 1;
            int v = (int)(255 * a);
            ImageDrawPixel(&img, x, y, CC(v, v, v, v));
        }
        lightGrad = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 手电筒光束 144x64（朝右 +x，光源在左侧中点 (0,32)）
        // 由里向外逐渐变宽：近端半宽 3px -> 远端半宽 22px（最外约 44px = 3 个半主角）
        // 注意：光照机制依赖灰度，颜色不可改
        int BW = 144, BH = 64, cy = 32;
        float reach = 142.0f;
        Image img = GenImageColor(BW, BH, CC(0, 0, 0, 0));
        for (int y = 0; y < BH; y++) for (int x = 0; x < BW; x++) {
            float u = (float)x / reach;                    // 0 近 -> 1 远
            if (u > 1.0f) continue;
            float halfw = 3.0f + 19.0f * u;                // 半宽渐张
            float dy = fabsf((float)(y - cy));
            if (dy > halfw) continue;
            float across = dy / halfw;                     // 0 中轴 -> 1 边缘
            float af = 1.0f - across * across;             // 角度衰减（中心亮轴）
            float rf = 1.0f - 0.72f * u;                   // 径向衰减（远处稍暗）
            float a = af * af * rf;                        // 平方让光芯更聚
            if (a > 1) a = 1;
            if (a < 0) a = 0;
            int v = (int)(255 * a);
            ImageDrawPixel(&img, x, y, CC(v, v, v, v));
        }
        lightBeam = LoadTextureFromImage(img); UnloadImage(img);
    }
    {   // 挥砍弧光 56x56（默认朝右，旋转原点在弧心 (12,28)）—— 青绿魂火弧光
        Image img = GenImageColor(56, 56, CC(0, 0, 0, 0));
        for (int y = 0; y < 56; y++) for (int x = 0; x < 56; x++) {
            float dx = (float)(x - 12), dy = (float)(y - 28);
            float r = sqrtf(dx * dx + dy * dy);
            if (r < 16 || r > 38) continue;
            float ang = atan2f(dy, dx) * 57.29578f;   // 度
            float aa = fabsf(ang);
            if (aa > 62) continue;
            float edge = 1.0f - aa / 62.0f;           // 角度衰减
            float rad = (r - 16) / 22.0f;             // 径向位置
            float band = 1.0f - fabsf(rad - 0.5f) * 2.0f; // 中间亮
            if (band < 0) band = 0;
            int alpha = (int)(235 * edge * (0.35f + 0.65f * band));
            if (alpha > 0) {
                Color arc = (band > 0.5f) ? LH3 : LH2;     // 魂火亮芯 / 青绿弧光
                ImageDrawPixel(&img, x, y, CC(arc.r, arc.g, arc.b, alpha > 255 ? 255 : alpha));
            }
        }
        slashArc = LoadTextureFromImage(img); UnloadImage(img);
    }

    // 统一 NEAREST 过滤
    for (int i = 0; i < 4; i++) { SetTextureFilter(grass[i], TEXTURE_FILTER_POINT); if (i < 3) { SetTextureFilter(forest[i], TEXTURE_FILTER_POINT); SetTextureFilter(sand[i], TEXTURE_FILTER_POINT); } }
    for (int i = 0; i < 4; i++) SetTextureFilter(water[i], TEXTURE_FILTER_POINT);
    SetTextureFilter(tree, TEXTURE_FILTER_POINT); SetTextureFilter(stump, TEXTURE_FILTER_POINT);
    SetTextureFilter(rock, TEXTURE_FILTER_POINT); SetTextureFilter(bushFull, TEXTURE_FILTER_POINT);
    SetTextureFilter(bushEmpty, TEXTURE_FILTER_POINT); SetTextureFilter(tallGrass, TEXTURE_FILTER_POINT);
    SetTextureFilter(flower[0], TEXTURE_FILTER_POINT); SetTextureFilter(flower[1], TEXTURE_FILTER_POINT);
    SetTextureFilter(campfire[0], TEXTURE_FILTER_POINT); SetTextureFilter(campfire[1], TEXTURE_FILTER_POINT);
    SetTextureFilter(dropWood, TEXTURE_FILTER_POINT); SetTextureFilter(dropStone, TEXTURE_FILTER_POINT);
    SetTextureFilter(dropBerry, TEXTURE_FILTER_POINT); SetTextureFilter(dropRawMeat, TEXTURE_FILTER_POINT);
    SetTextureFilter(dropCooked, TEXTURE_FILTER_POINT); SetTextureFilter(dropRotten, TEXTURE_FILTER_POINT);
    SetTextureFilter(heart, TEXTURE_FILTER_POINT); SetTextureFilter(drumstick, TEXTURE_FILTER_POINT);
    SetTextureFilter(sunIcon, TEXTURE_FILTER_POINT); SetTextureFilter(moonIcon, TEXTURE_FILTER_POINT);
    SetTextureFilter(lightGrad, TEXTURE_FILTER_BILINEAR); SetTextureFilter(lightBeam, TEXTURE_FILTER_BILINEAR); SetTextureFilter(slashArc, TEXTURE_FILTER_POINT);
    for (int i = 0; i < 4; i++) { SetTextureFilter(rabbit[i], TEXTURE_FILTER_POINT); SetTextureFilter(deer[i], TEXTURE_FILTER_POINT); SetTextureFilter(zombie[i], TEXTURE_FILTER_POINT); }
}

void Assets::Unload() {
    for (int d = 0; d < 3; d++) {
        for (int f = 0; f < 9; f++) UnloadTexture(player[d][f]);
        UnloadTexture(playerWhite[d]);
    }
    for (int i = 0; i < 4; i++) { UnloadTexture(rabbit[i]); UnloadTexture(deer[i]); UnloadTexture(zombie[i]); }
    for (int i = 0; i < 4; i++) { UnloadTexture(runner[i]); UnloadTexture(spitter[i]); UnloadTexture(brute[i]); }
    UnloadTexture(rabbitWhite); UnloadTexture(deerWhite); UnloadTexture(zombieWhite);
    UnloadTexture(runnerWhite); UnloadTexture(spitterWhite); UnloadTexture(bruteWhite);
    for (int i = 0; i < 4; i++) { UnloadTexture(skeleton[i]); UnloadTexture(bat[i]); UnloadTexture(spider[i]); }
    for (int i = 0; i < 4; i++) { UnloadTexture(gargoyle[i]); UnloadTexture(wraith[i]); UnloadTexture(golem[i]); }
    for (int i = 0; i < 4; i++) UnloadTexture(warlock[i]);
    for (int i = 0; i < 4; i++) UnloadTexture(boss[i]);
    UnloadTexture(skeletonWhite); UnloadTexture(batWhite); UnloadTexture(spiderWhite);
    UnloadTexture(gargoyleWhite); UnloadTexture(wraithWhite); UnloadTexture(golemWhite);
    UnloadTexture(warlockWhite);
    UnloadTexture(bossWhite);
    for (int i = 0; i < 4; i++) UnloadTexture(grass[i]);
    for (int i = 0; i < 3; i++) { UnloadTexture(forest[i]); UnloadTexture(sand[i]); }
    for (int i = 0; i < 4; i++) UnloadTexture(water[i]);
    UnloadTexture(tree); UnloadTexture(treePine); UnloadTexture(treeBirch); UnloadTexture(treePalm);
    UnloadTexture(stump); UnloadTexture(rock);
    UnloadTexture(bushFull); UnloadTexture(bushEmpty); UnloadTexture(tallGrass);
    UnloadTexture(flower[0]); UnloadTexture(flower[1]);
    UnloadTexture(campfire[0]); UnloadTexture(campfire[1]);
    UnloadTexture(dunFloor[0]); UnloadTexture(dunFloor[1]); UnloadTexture(dunWall);
    UnloadTexture(dunStairsDown); UnloadTexture(dunStairsUp); UnloadTexture(dunEntrance);
    UnloadTexture(workbench); UnloadTexture(oreRock); UnloadTexture(toolBenchGlow);
    UnloadTexture(graveMound); UnloadTexture(wallWood); UnloadTexture(bedStraw);
    UnloadTexture(talisman);
    UnloadTexture(toolBow); UnloadTexture(toolKnife); UnloadTexture(toolAxe);
    UnloadTexture(toolHammer); UnloadTexture(toolPick);
    UnloadTexture(toolBowOre); UnloadTexture(toolKnifeOre); UnloadTexture(toolAxeOre);
    UnloadTexture(toolHammerOre); UnloadTexture(toolPickOre);
    UnloadTexture(toolFan1); UnloadTexture(toolFan2); UnloadTexture(toolFan3);
    UnloadTexture(arrowItem);
    UnloadTexture(dropWood); UnloadTexture(dropStone); UnloadTexture(dropBerry);
    UnloadTexture(dropRawMeat); UnloadTexture(dropCooked); UnloadTexture(dropRotten);
    UnloadTexture(dropIronOre); UnloadTexture(dropCrystal);
    UnloadTexture(armorIron); UnloadTexture(armorCrystal);
    UnloadTexture(dropJam); UnloadTexture(dropTea);
    UnloadTexture(dropStew); UnloadTexture(dropHeart);
    UnloadTexture(boat); UnloadTexture(boatIcon);
    UnloadTexture(ruinWall[0]); UnloadTexture(ruinWall[1]);
    UnloadTexture(chestClosed); UnloadTexture(chestOpen); UnloadTexture(arrowHint);
    UnloadTexture(heart); UnloadTexture(drumstick);
    UnloadTexture(sunIcon); UnloadTexture(moonIcon); UnloadTexture(starIcon);
    UnloadTexture(lightGrad); UnloadTexture(lightBeam); UnloadTexture(slashArc);
}
