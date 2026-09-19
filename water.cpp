#include "water.hpp"
#include <vector>
#include <cmath>

// ============================================================
// 水面渲染器 v4.2 实现：像素水面（块状波浪 + 雨天水花）
// v4.2：2px 像素量化波浪带 + 闪烁高光 + 像素涟漪环/飞溅水珠
// ============================================================

static const char* WATER_FS = R"GLSL(#version 330
// ============ 像素水面：块状波浪带 + 闪烁高光 + 像素涟漪 ============
out vec4 fragColor;

uniform float u_time;
uniform vec2  u_resolution;
uniform vec2  u_cam;
uniform vec2  u_flow_dir;
uniform float u_wave_speed;
uniform float u_wave_amplitude;
uniform float u_threshold;          // 菲涅尔倒影比值阈值 D/H
uniform float u_max_dist;           // 倒影最大距离
uniform vec4  u_objects[24];        // xy=位置 z=高度H w=宽度
uniform int   u_object_count;
uniform sampler2D u_shoreline_mask; // 连续距离遮罩（POINT）
uniform sampler2D u_scene;          // 场景快照（POINT，倒影源）
uniform vec4  u_deep_color;         // 深水蓝 + alpha
uniform vec4  u_shallow_color;      // 浅水/湿岸带色 + alpha
uniform vec4  u_crest_color;        // 浪尖高光色
uniform vec4  u_speck_color;        // 水花亮色
uniform float u_world_size;         // 世界边长（遮罩归一化）
uniform float u_rain;               // 雨强 0..1（驱动涟漪/水花）
// ---- 天体倒影（太阳/月亮/血月/星辰实时映在水面）----
uniform vec4  u_celestial;          // xy=天体屏幕坐标 z=类型(0无/1日/2月/3血月) w=强度0..1
uniform vec3  u_cel_color;          // 天体倒影色
uniform float u_star_twinkle;       // 星辰倒影强度 0..1（夜越深越亮）

float hash21(vec2 p) {
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

void main() {
    vec2 scr = vec2(gl_FragCoord.x, u_resolution.y - gl_FragCoord.y);
    vec2 wp  = u_cam + scr;                                   // 世界像素坐标（连续）
    float t  = u_time * u_wave_speed;

    // ---- 岸线连续距离遮罩（0=深水 0.5=岸线 1=陆地）----
    float shore = texture(u_shoreline_mask, wp / u_world_size).r;
    float waterAlpha = 1.0 - smoothstep(0.55, 0.88, shore);    // 岸缘圆润渐隐
    if (waterAlpha <= 0.003) { fragColor = vec4(0.0); return; }

    // ---- 2px 像素量化（块状水波的基础）----
    vec2 q  = floor(wp / 2.0) * 2.0;                           // 量化坐标
    vec2 qp = floor(wp / 2.0);                                 // 像素格 id

    // ---- 基底：深浅蓝随云状噪声缓慢过渡（量化后呈块状色斑）----
    float m1 = sin(q.x * 0.011 + t * 0.30) * sin(q.y * 0.013 - t * 0.22);
    float m2 = sin((q.x + q.y) * 0.007 + t * 0.17);
    float cloudF = m1 * 0.6 + m2 * 0.4;
    vec3 col = mix(u_deep_color.rgb, u_shallow_color.rgb,
                   smoothstep(-0.45, 0.55, cloudF) * 0.60);

    // ---- 岸线湿暗带：沙滩缘 2~4px 暗水带 ----
    float wet = smoothstep(0.56, 0.50, shore);
    col = mix(col, u_shallow_color.rgb * 0.72, wet * 0.50);

    // ---- 波浪带：量化坐标上的滚动正弦 -> 阶梯状亮纹（随水流方向漂移）----
    float wph  = dot(q, u_flow_dir) * 0.055 - t * 2.6;
    float band = sin(wph) * 0.6 + sin(wph * 0.47 + 1.7 + q.y * 0.02) * 0.4;
    float crest = smoothstep(0.42, 0.78, band);
    col = mix(col, u_crest_color.rgb, crest * 0.55);

    // ---- 稀疏闪烁高光像素（阳光洒在水面上的亮闪，格位随时间跳动）----
    float sp = hash21(qp + floor(vec2(t * 2.5, -t * 1.8)));
    float sparkle = step(0.982, sp) * (0.6 + 0.4 * sin(t * 8.0 + sp * 40.0));
    col = mix(col, u_crest_color.rgb * 1.25, clamp(sparkle, 0.0, 1.0));

    // ---- 雨天：像素涟漪环 + 中心水花 + 外溅水珠（雨强驱动，程序化网格）----
    if (u_rain > 0.02) {
        vec2  rg  = q / 18.0;                                  // 18px 量化涟漪网格
        vec2  rid = floor(rg);
        vec2  rf  = fract(rg);
        float rh  = hash21(rid + 3.17);
        float on  = step(1.0 - u_rain * 0.85, fract(rh * 29.3));   // 雨越大越密
        if (on > 0.0) {
            float phase = fract(t * (1.1 + rh * 0.7) + rh * 7.0);  // 环扩散相位
            vec2  cpos  = vec2(fract(rh * 13.7), fract(rh * 5.11)) * 0.6 + 0.2;
            float d     = length(rf - cpos);
            // 扩散环（量化坐标下呈像素方环）
            float ring  = smoothstep(0.16, 0.04, abs(d - phase * 0.45)) * (1.0 - phase);
            col = mix(col, u_crest_color.rgb, ring * u_rain * 0.60);
            // 中心水花亮点（环出现瞬间闪白）
            float splash = smoothstep(0.08, 0.0, d) * (1.0 - smoothstep(0.0, 0.16, phase));
            col = mix(col, u_speck_color.rgb, splash * u_rain * 0.80);
            // 外溅水珠：两颗像素沿对角飞出后消失
            vec2  off  = vec2(phase * 0.34);
            float drop = (1.0 - smoothstep(0.05, 0.09, length(rf - cpos + off)))
                       + (1.0 - smoothstep(0.05, 0.09, length(rf - cpos - off)));
            drop *= step(0.25, phase) * (1.0 - smoothstep(0.45, 0.70, phase));
            col = mix(col, u_speck_color.rgb, clamp(drop, 0.0, 1.0) * u_rain * 0.70);
        }
    }

    // ---- 菲涅尔倒影：D/H 比值判定 + 场景快照镜像（Nearest + 整数波扭曲）----
    vec3 reflSum = vec3(0.0); float totalA = 0.0;
    float wob = floor(sin(wp.y * 0.13 + u_time * 1.6) * u_wave_amplitude * 2.0) * 0.5;
    for (int i = 0; i < 24; i++) {
        if (i >= u_object_count) break;
        vec4 o = u_objects[i];
        float D = distance(wp, o.xy);
        if (D > u_max_dist) continue;
        float H = max(o.z, 0.1);
        float ratio = D / H;                                   // 视角越平比值越大
        if (ratio <= u_threshold) continue;                    // 正上方透光无倒影
        float a = smoothstep(u_threshold, u_threshold + 0.8, ratio);
        a *= smoothstep(u_max_dist, u_max_dist * 0.5, D);      // 距离衰减
        float below = wp.y - o.y;
        if (below < 0.0) continue;                             // 倒影只出现在物体下方
        a *= 1.0 - smoothstep(H * 1.4, H * 2.8, below);        // 垂直伸展限制
        a *= 1.0 - smoothstep(o.w * 0.5, o.w * 1.2, abs(wp.x - o.x));
        if (a < 0.01) continue;
        vec2 rp = floor(vec2(wp.x + wob, 2.0 * o.y - wp.y)) - u_cam;  // 整数像素对齐
        if (rp.x < 0.0 || rp.x > u_resolution.x || rp.y < 0.0 || rp.y > u_resolution.y) continue;
        reflSum += texture(u_scene, vec2(rp.x / u_resolution.x, 1.0 - rp.y / u_resolution.y)).rgb * a;
        totalA += a;
    }
    if (totalA > 0.0) {
        vec3 refl = reflSum / totalA;
        col = mix(col, col * 0.35 + refl * 0.80, min(totalA, 0.70) * waterAlpha);
    }

    // ---- 天体倒影：日月在水面形成破碎光柱，星辰撒下稀疏闪烁 ----
    // 天体为屏幕空间元素（不随相机平移），故光柱按屏幕 X 对齐；
    // 纵向用世界坐标 wp.y 做波动，使光带随水波破碎、随时间抖动下滑。
    if (u_celestial.w > 0.01 && u_celestial.z > 0.5) {
        float celType = u_celestial.z;
        // 横向摆动：双频正弦 -> 光柱边缘自然蛇形
        float wobX = sin(wp.y * 0.28 + u_time * 2.6) * 2.2
                   + sin(wp.y * 0.09 - u_time * 1.3) * 3.0;
        float dx   = abs(scr.x - (u_celestial.x + wobX));
        // 光柱半宽：太阳最宽最亮，血月居中，月亮最窄最柔
        float halfW = (celType < 1.5) ? 11.0 : (celType < 2.5 ? 9.0 : 9.5);
        float band  = 1.0 - smoothstep(halfW * 0.25, halfW, dx);
        // 纵向破碎：3px 量化格 + 逐格时间抖动 -> 断续像素光斑
        float brk = hash21(floor(vec2(wp.x / 3.0, wp.y / 2.0 + floor(u_time * 7.0))));
        band *= 0.35 + 0.65 * step(0.30, brk);
        // 整体呼吸 + 沿光柱流动的高光
        float shimmer = 0.55 + 0.45 * sin(u_time * 2.1 - wp.y * 0.35 + brk * 6.0);
        float inten = band * shimmer * u_celestial.w;
        // wet：深水=1、靠岸=0（smoothstep 反向），故直接用它做"深水满强度、浅滩渐隐"
        inten *= wet * waterAlpha;
        if (celType > 2.5) inten *= 0.75;                // 血月压暗，避免盖过夜色
        col = mix(col, u_cel_color, clamp(inten, 0.0, 1.0) * 0.85);
    }

    // ---- 星辰倒影：夜间水面稀疏冷白闪烁点（5px 量化格 + 各自频率跳动）----
    if (u_star_twinkle > 0.01) {
        vec2  sg = floor(wp / 5.0);
        float sh = hash21(sg + 11.3);
        if (step(0.82, sh) > 0.0) {                      // 稀疏：约 18% 格子有星
            vec2  sf = fract(wp / 5.0);
            float sd = length(sf - vec2(0.5));
            float core = smoothstep(0.34, 0.05, sd);
            float tw   = 0.45 + 0.55 * sin(u_time * (1.6 + sh * 3.0) + sh * 40.0);
            float inten = core * tw * u_star_twinkle * wet * waterAlpha;
            col = mix(col, vec3(0.86, 0.90, 1.0), clamp(inten, 0.0, 1.0) * 0.85);
        }
    }

    // ---- 半透明输出：深水更不透明，岸缘渐隐 ----
    float depthK = smoothstep(0.04, 0.46, shore);
    fragColor = vec4(col, waterAlpha * mix(u_deep_color.a, u_shallow_color.a, depthK));
}
)GLSL";

// ---------------- 初始化 ----------------
void WaterRenderer::Init() {
    sh = LoadShaderFromMemory(nullptr, WATER_FS);
    valid = (sh.id != 0);
    if (!valid) { TraceLog(LOG_WARNING, "WATER: shader compile failed"); return; }

    loc_time = GetShaderLocation(sh, "u_time");
    loc_resolution = GetShaderLocation(sh, "u_resolution");
    loc_cam = GetShaderLocation(sh, "u_cam");
    loc_flow_dir = GetShaderLocation(sh, "u_flow_dir");
    loc_wave_speed = GetShaderLocation(sh, "u_wave_speed");
    loc_wave_amplitude = GetShaderLocation(sh, "u_wave_amplitude");
    loc_objects = GetShaderLocation(sh, "u_objects");
    loc_object_count = GetShaderLocation(sh, "u_object_count");
    loc_shoreline_mask = GetShaderLocation(sh, "u_shoreline_mask");
    loc_scene_texture = GetShaderLocation(sh, "u_scene");
    loc_deep_color = GetShaderLocation(sh, "u_deep_color");
    loc_shallow_color = GetShaderLocation(sh, "u_shallow_color");
    loc_crest_color = GetShaderLocation(sh, "u_crest_color");
    loc_speck_color = GetShaderLocation(sh, "u_speck_color");
    loc_threshold = GetShaderLocation(sh, "u_threshold");
    loc_max_dist = GetShaderLocation(sh, "u_max_dist");
    loc_world_size = GetShaderLocation(sh, "u_world_size");
    loc_rain = GetShaderLocation(sh, "u_rain");
    // 天体倒影
    loc_celestial = GetShaderLocation(sh, "u_celestial");
    loc_cel_color = GetShaderLocation(sh, "u_cel_color");
    loc_star_twinkle = GetShaderLocation(sh, "u_star_twinkle");
    TraceLog(LOG_INFO, "WATER: pixel water shader ready (v4.3 + sky reflection)");
}

// ---------------- 岸线遮罩烧录：Chamfer 距离场 -> 连续灰度（POINT）----------------
void WaterRenderer::BakeMask(const World& w) {
    const int N = MAP_W;
    std::vector<float> dl((size_t)N * N, 1e9f), dw((size_t)N * N, 1e9f);
    for (int i = 0; i < N * N; i++) {
        if (w.tiles[(size_t)i] != Tile::Water) dl[(size_t)i] = 0.0f;
        else                                   dw[(size_t)i] = 0.0f;
    }
    auto Chamfer = [&](std::vector<float>& d) {        // 正交 1 / 对角 1.414 两遍扫描
        const float D1 = 1.0f, D2 = 1.4142f;
        for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
            size_t i = (size_t)y * N + x; float v = d[i];
            if (x > 0)              v = fminf(v, d[i - 1] + D1);
            if (y > 0)              v = fminf(v, d[i - N] + D1);
            if (x > 0 && y > 0)     v = fminf(v, d[i - N - 1] + D2);
            if (x < N - 1 && y > 0) v = fminf(v, d[i - N + 1] + D2);
            d[i] = v;
        }
        for (int y = N - 1; y >= 0; y--) for (int x = N - 1; x >= 0; x--) {
            size_t i = (size_t)y * N + x; float v = d[i];
            if (x < N - 1)              v = fminf(v, d[i + 1] + D1);
            if (y < N - 1)              v = fminf(v, d[i + N] + D1);
            if (x < N - 1 && y < N - 1) v = fminf(v, d[i + N + 1] + D2);
            if (x > 0 && y < N - 1)     v = fminf(v, d[i + N - 1] + D2);
            d[i] = v;
        }
    };
    Chamfer(dl); Chamfer(dw);
    std::vector<float> sd((size_t)N * N);              // 有符号距离（tile 单位，陆正水负）
    for (int i = 0; i < N * N; i++)
        sd[(size_t)i] = (w.tiles[(size_t)i] != Tile::Water) ? dw[(size_t)i] : -dl[(size_t)i];

    // 遮罩纹理（每 texel = 2 世界 px；随地图尺寸自适应，uv 采样覆盖全图）
    const int M = (MAP_W * TILE) / 2;
    Image img = GenImageColor(M, M, BLACK);
    unsigned char* px = (unsigned char*)img.data;
    for (int my = 0; my < M; my++) {
        float gy = ((my + 0.5f) * 2.0f) / 16.0f - 0.5f;
        int y0 = (int)floorf(gy); float fy = gy - y0;
        if (y0 < 0)     { y0 = 0;     fy = 0.0f; }
        if (y0 > N - 2) { y0 = N - 2; fy = 1.0f; }
        for (int mx = 0; mx < M; mx++) {
            float gx = ((mx + 0.5f) * 2.0f) / 16.0f - 0.5f;
            int x0 = (int)floorf(gx); float fx = gx - x0;
            if (x0 < 0)     { x0 = 0;     fx = 0.0f; }
            if (x0 > N - 2) { x0 = N - 2; fx = 1.0f; }
            float sdt = sd[(size_t)y0 * N + x0]            * (1 - fx) * (1 - fy)
                       + sd[(size_t)y0 * N + x0 + 1]       * fx       * (1 - fy)
                       + sd[(size_t)(y0 + 1) * N + x0]     * (1 - fx) * fy
                       + sd[(size_t)(y0 + 1) * N + x0 + 1] * fx       * fy;
            float nv = (sdt * 16.0f) / 40.0f;
            if (nv < -1.0f) nv = -1.0f;
            if (nv >  1.0f) nv =  1.0f;
            unsigned char b = (unsigned char)((nv * 0.5f + 0.5f) * 255.0f);
            size_t o = ((size_t)my * M + mx) * 4;
            px[o] = b; px[o + 1] = b; px[o + 2] = b; px[o + 3] = 255;
        }
    }
    if (mask.id != 0) UnloadTexture(mask);
    mask = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(mask, TEXTURE_FILTER_POINT);       // 强制 Nearest
    SetTextureWrap(mask, TEXTURE_WRAP_CLAMP);
    TraceLog(LOG_INFO, "WATER: shoreline mask baked (1280x1280, POINT)");
}

// ---------------- 帧绘制 ----------------
// 像素水面：块状波浪带 + 闪烁高光 + 倒影 + 雨天涟漪/水花
void WaterRenderer::Draw(const RenderTexture2D& scene, const RenderTexture2D& snapshot,
                         int camXp, int camYp, int resW, int resH,
                         float nowT, float warmK, float nightK, float rainAmt,
                         const float* objs, int objCount,
                         const float cel[4], const float celColor[3], float starTwinkle) {
    if (!valid || mask.id == 0) return;

    // 场景快照（倒影采样源，与绘制目标分离避免同纹理读写反馈）
    BeginTextureMode(const_cast<RenderTexture2D&>(snapshot));
    ClearBackground(BLANK);
    DrawTexturePro(scene.texture, { 0, 0, (float)resW, (float)-resH },
                   { 0, 0, (float)resW, (float)resH }, { 0, 0 }, 0, WHITE);
    EndTextureMode();

    // ---- 时段色：白天墨青深水 -> 黄昏灰青 -> 夜晚墨黑深水（连续插值，整体冷调压暗）----
    auto L = [](float a, float b, float k) { return a + (b - a) * k; };
    float dk = warmK, nk = nightK;
    // 深水 #10222c -> 黄昏 #0e1e2c -> 夜 #081220（墨青冷调）
    float dR = L(L(16, 14, dk), 8, nk), dG = L(L(34, 30, dk), 18, nk), dB = L(L(44, 44, dk), 32, nk);
    float dA = L(L(190, 175, dk), 195, nk);
    // 浅水 #1e363e -> 黄昏 #1a3040 -> 夜 #102034（灰青湿岸带）
    float sR = L(L(30, 26, dk), 16, nk), sG = L(L(54, 48, dk), 32, nk), sB = L(L(62, 64, dk), 52, nk);
    float sA = L(L(130, 122, dk), 155, nk);
    // 浪尖高光 #6096b4 -> 黄昏 #508296 -> 夜 #38648c（幽蓝冷光）
    float cR = L(L(96, 80, dk), 56, nk), cG = L(L(150, 130, dk), 100, nk), cB = L(L(180, 150, dk), 140, nk);
    // 水花亮色 #b4dceb -> 夜 #6e96b4（冷白闪点）
    float pR = L(180, 110, nk), pG = L(220, 150, nk), pB = L(235, 180, nk);

    float flow[2]   = { par.flowDirX, par.flowDirY };
    float resv[2]   = { (float)resW, (float)resH };
    float camv[2]   = { (float)camXp, (float)camYp };
    float deepv[4]  = { dR / 255, dG / 255, dB / 255, dA / 255 };
    float shalv[4]  = { sR / 255, sG / 255, sB / 255, sA / 255 };
    float crestv[4] = { cR / 255, cG / 255, cB / 255, 1.0f };
    float speckv[4] = { pR / 255, pG / 255, pB / 255, 1.0f };
    float waveSpd   = par.waveSpeed * (1.0f + rainAmt * 0.6f);
    float worldSize = (float)(MAP_W * TILE);
    int   cnt       = objCount > MAX_OBJECTS ? MAX_OBJECTS : objCount;

    BeginTextureMode(const_cast<RenderTexture2D&>(scene));
    BeginShaderMode(sh);
    SetShaderValue(sh, loc_time, &nowT, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, loc_resolution, resv, SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, loc_cam, camv, SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, loc_flow_dir, flow, SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, loc_wave_speed, &waveSpd, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, loc_wave_amplitude, &par.waveAmplitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, loc_threshold, &par.threshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, loc_max_dist, &par.maxDist, SHADER_UNIFORM_FLOAT);
    SetShaderValueV(sh, loc_objects, objs, SHADER_UNIFORM_VEC4, MAX_OBJECTS);
    SetShaderValue(sh, loc_object_count, &cnt, SHADER_UNIFORM_INT);
    SetShaderValue(sh, loc_deep_color, deepv, SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, loc_shallow_color, shalv, SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, loc_crest_color, crestv, SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, loc_speck_color, speckv, SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, loc_world_size, &worldSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, loc_rain, &rainAmt, SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(sh, loc_shoreline_mask, mask);
    SetShaderValueTexture(sh, loc_scene_texture, const_cast<RenderTexture2D&>(snapshot).texture);
    // 天体倒影（类型 z<=0 时 shader 自动跳过，地牢恒为 0）
    SetShaderValue(sh, loc_celestial, cel, SHADER_UNIFORM_VEC4);
    SetShaderValue(sh, loc_cel_color, celColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(sh, loc_star_twinkle, &starTwinkle, SHADER_UNIFORM_FLOAT);
    DrawRectangle(0, 0, resW, resH, WHITE);            // 全屏连续水面 quad
    EndShaderMode();
    EndTextureMode();
}

void WaterRenderer::Unload() {
    if (mask.id != 0) { UnloadTexture(mask); mask = {}; }
    if (sh.id != 0)   { UnloadShader(sh);   sh = {}; }
    valid = false;
}
