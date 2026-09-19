#pragma once
#include "raylib.h"

// ============================================================
// 资源模块：全部纹理由「调色板 + 像素字符串」程序化生成，零外部素材
// ============================================================

struct Assets {
    // ---- 玩家 ----
    // player[方向][帧]：方向 0=下 1=上 2=右（左=右镜像）
    // 帧：0-1 待机 | 2-5 行走 | 6-8 攻击（3 帧，含木剑）
    Texture2D player[3][9];
    Texture2D playerWhite[3];       // 受击白闪剪影（站立姿势）

    // ---- 生物 ----
    // 帧：0-1 待机 | 2-3 移动
    Texture2D rabbit[4];  Texture2D rabbitWhite;
    Texture2D deer[4];    Texture2D deerWhite;
    Texture2D zombie[4];  Texture2D zombieWhite;
    Texture2D runner[4];  Texture2D runnerWhite;   // 疾奔者（变异犬，快速低血）
    Texture2D spitter[4]; Texture2D spitterWhite;  // 泡泡怪（远程吐酸，保持距离）
    Texture2D brute[4];   Texture2D bruteWhite;    // 尸王（夜间巨型坦克）

    // ---- 地牢专属怪物（7 种，深度越深越强）----
    Texture2D skeleton[4];  Texture2D skeletonWhite;   // 骷髅兵（近战）
    Texture2D bat[4];       Texture2D batWhite;        // 洞穴蝠（快速突袭）
    Texture2D spider[4];    Texture2D spiderWhite;     // 穴蛛（高速毒咬）
    Texture2D gargoyle[4];  Texture2D gargoyleWhite;   // 石像鬼（伪装突进）
    Texture2D wraith[4];    Texture2D wraithWhite;     // 幽魂（穿墙近战）
    Texture2D golem[4];     Texture2D golemWhite;      // 岩傀（重甲坦克，守宝箱）
    Texture2D warlock[4];   Texture2D warlockWhite;    // 缚灵术士（远程火球）
    Texture2D boss[4];      Texture2D bossWhite;       // 血月尸王（Boss，渲染层放大 1.7x）

    // ---- 瓦片（16x16，多随机变体）----
    Texture2D grass[4];
    Texture2D forest[3];
    Texture2D sand[3];
    Texture2D water[4];              // 4 帧波光动画（浅色通透 + 反光高光）

    // ---- 世界物体 ----
    Texture2D tree;                  // 32x44 橡树
    Texture2D treePine;              // 24x54 松树（针叶锥形）
    Texture2D treeBirch;             // 26x40 白桦（白干黑纹）
    Texture2D treePalm;              // 32x38 棕榈（沙滩弯干）
    Texture2D stump;                 // 16x12 树桩
    Texture2D rock;                  // 20x14
    Texture2D bushFull;              // 有果浆果丛
    Texture2D bushEmpty;             // 采空浆果丛
    Texture2D tallGrass;             // 高草
    Texture2D flower[2];             // 小花（白/黄）
    Texture2D campfire[2];           // 篝火 2 帧火焰动画
    Texture2D boat;                  // 小舟 30x14（泛舟渡海；墨黑船身 + 鎏金铜缘 + 船头魂灯）

    // ---- 地牢 ----
    Texture2D dunFloor[2];           // 地牢地面 2 变体（16x16 石砖）
    Texture2D dunWall;               // 地牢墙面（16x16，绘制在地板上方）
    Texture2D dunStairsDown;         // 下行楼梯 16x16（地牢已移除，贴图保留备用）
    Texture2D dunStairsUp;           // 上行楼梯 16x16（地牢已移除，贴图保留备用）
    Texture2D dunEntrance;           // 地表洞口 20x20（地牢已移除，贴图保留备用）
    Texture2D workbench;             // 义庄祭台 20x16（高级配方合成站，聚于乱葬岗）
    Texture2D oreRock;               // 矿脉 18x14（散落乱葬岗，含金属闪光）
    Texture2D graveMound;            // 坟冢 18x16（乱葬岗地标：土坟包 + 石碑 + 朱砂碑文 + 纸幡）
    Texture2D wallWood;              // 木墙 16x16（建造：暗木横条 + 朱砂符纹）
    Texture2D bedStraw;              // 草席床 16x14（建造：睡眠点）
    Texture2D talisman;              // 符咒 12x12（黄符纸 + 朱砂符文）
    Texture2D toolBenchGlow;         // 祭台可合成提示光点

    // ---- 掉落物图标（8x8）----
    Texture2D dropWood, dropStone, dropBerry, dropRawMeat, dropCooked, dropRotten;
    Texture2D dropIronOre, dropCrystal;

    // ---- 工具图标（12x12，合成动画与环形物品栏共用）----
    Texture2D toolBow, toolKnife, toolAxe, toolHammer, toolPick;         // 基础（木石）
    Texture2D toolBowOre, toolKnifeOre, toolAxeOre, toolHammerOre,
              toolPickOre;                                               // 矿石升级版
    Texture2D toolFan1, toolFan2, toolFan3;      // 收魂幡三阶（粗纸/铜铃/鎏金，收鬼系统）
    Texture2D arrowItem;             // 箭矢投射物（10x4，默认朝右）

    // ---- 护甲 / 新食物 / Boss 材料（8x8~12x12 图标）----
    Texture2D armorIron;             // 铁甲（胸甲图标）
    Texture2D armorCrystal;          // 萤晶甲（青紫晶甲图标）
    Texture2D dropJam;               // 浆果酱（紫色果酱罐）
    Texture2D dropTea;               // 萤晶茶（发光茶杯）
    Texture2D dropStew;              // 炖肉（木碗炖锅）
    Texture2D dropHeart;             // 血月之心（猩红心脏，Boss 专属掉落）
    Texture2D boatIcon;              // 小舟图标 12x12（合成面板/合成动画共用）

    // ---- 村庄遗迹 ----
    Texture2D ruinWall[2];           // 残墙 2 变体（16x16 碎石砖）
    Texture2D chestClosed;           // 宝箱（未开启）
    Texture2D chestOpen;             // 宝箱（已开启）
    Texture2D arrowHint;             // 探索指引箭头（黄色）

    // ---- UI ----
    Texture2D heart;                 // 生命图标
    Texture2D drumstick;             // 饥饿图标
    Texture2D sunIcon, moonIcon;     // 昼夜图标
    Texture2D starIcon;              // 经验星标（等级条）

    // ---- 特效 ----
    Texture2D lightGrad;             // 光圈径向渐变（白->黑，减法混合用）
    Texture2D lightBeam;             // 手电筒聚光锥（默认朝右，原点在光源处）
    Texture2D slashArc;              // 挥砍弧光（默认朝右）

    void Load();                     // 生成全部纹理
    void Unload();
};
