#pragma once
#include "raylib.h"

// ============================================================
// 音频模块：全部音效由手动填充 Wave 结构体 PCM 数据合成
// （方波 / 锯齿波 / 白噪声 + 指数包络），零外部素材
// ============================================================

struct AudioBank {
    Sound swing;       // 挥砍破空声
    Sound hit;         // 命中敌人（清脆）
    Sound chop;        // 砍树
    Sound mine;        // 采石
    Sound pickup;      // 拾取
    Sound eat;         // 进食
    Sound hurt;        // 玩家受击
    Sound zombieHit;   // 僵尸受击
    Sound zombieDie;   // 僵尸死亡
    Sound treeFall;    // 树倒下
    Sound craft;       // 合成成功（琶音）
    Sound place;       // 放置篝火
    Sound playerDie;   // 玩家死亡
    Sound roar;        // Boss 咆哮（低频锯齿下滑 + 颤音）
    Sound shock;       // 震地冲击（低频轰鸣 + 噪声）
    Sound capture;     // 收鬼铃音（双音铜铃，收服成功）
    Sound capturePull; // 摄魂牵引啸音（收鬼施法：上滑啸声 + 符纸破空）
    Sound memoryRestore; // 记忆恢复钟音（身份归位：清澈双音上行）
    Sound confuseWarble; // 记忆错乱咕哝（失谐双振荡拍频，鬼在耳边絮语）
    // ---- 背景音乐（循环流，程序化合成，无外部素材）----
    Music  bgm {};       // 循环氛围 BGM
    float  bgmVol = 0.45f;   // 音量 0..1（[ / ] 键调节，步进 0.1）

    void Init();       // 合成全部音效
    void Unload();
};
