#pragma once
#include "raylib.h"
#include "world.hpp"

// ============================================================
// 水面渲染器 v4.2 —— 像素水面（GLSL 330）
// 视觉特征：
//  1. 深蓝半透明基底 + 块状深浅色斑（低频噪声，2px 像素量化）；
//  2. 阶梯状滚动波浪带（量化正弦，随水流方向漂移）；
//  3. 稀疏闪烁高光像素（阳光亮闪，格位随时间跳动）；
//  4. 岸线湿暗带（沙滩缘 2~4px 暗水带，圆润距离场过渡）；
//  5. 菲涅尔倒影保留：D/H 比值判定，场景快照 Nearest + 整数波扭曲；
//  6. 雨天动画：像素涟漪扩散环 + 中心水花闪白 + 外溅水珠；
//  7. 像素感来自 640x360 低分辨率 RT + 整数 2 倍 POINT 上采样。
// ============================================================

struct WaterParams {
    float waveSpeed     = 0.35f;  // 波速（海面近乎静止，仅缓慢漂移）
    float waveAmplitude = 0.5f;   // 倒影波扭曲幅度（整数像素）
    float threshold     = 0.40f;  // 菲涅尔倒影比值阈值 D/H
    float maxDist       = 130.0f; // 倒影最大有效距离（树倒影下延 2.8H≈118px，不截断）
    float flowDirX      = 1.0f;   // 水流方向 X
    float flowDirY      = 0.22f;  // 水流方向 Y
};

class WaterRenderer {
public:
    bool       valid = false;
    Shader     sh    = {};
    Texture2D  mask  = {};     // 岸线连续距离遮罩（1280x1280，POINT）
    WaterParams par;

    // uniform 位置缓存
    int loc_time = 0, loc_resolution = 0, loc_cam = 0, loc_flow_dir = 0;
    int loc_wave_speed = 0, loc_wave_amplitude = 0;
    int loc_objects = 0, loc_object_count = 0;
    int loc_shoreline_mask = 0, loc_scene_texture = 0;
    int loc_deep_color = 0, loc_shallow_color = 0;
    int loc_crest_color = 0, loc_speck_color = 0;
    int loc_threshold = 0, loc_max_dist = 0;
    int loc_world_size = 0, loc_rain = 0;
    // 天体倒影（太阳/月亮/血月/星辰实时映在水面）
    int loc_celestial = 0, loc_cel_color = 0, loc_star_twinkle = 0;

    static constexpr int MAX_OBJECTS = 24;

    void Init();                    // 编译内嵌 GLSL
    void BakeMask(const World& w);  // Chamfer 距离场 -> 连续灰度遮罩（POINT）
    // cel：{ 屏幕X, 屏幕Y, 类型(0无/1日/2月/3血月), 强度0..1 }
    // celColor：天体倒影颜色；starTwinkle：星辰倒影强度 0..1（夜越深越亮）
    void Draw(const RenderTexture2D& scene, const RenderTexture2D& snapshot,
              int camX, int camY, int resW, int resH,
              float nowT, float warmK, float nightK, float rainAmt,
              const float* objs, int objCount,
              const float cel[4], const float celColor[3], float starTwinkle);
    void Unload();
};
