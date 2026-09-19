#include "audio.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>

// ============================================================
// 音效合成实现：16bit 单声道 PCM，22050Hz
// 生成后一次性 LoadSoundFromWave，主循环零分配
// ============================================================

namespace {

constexpr int SR = 22050;   // 采样率

// 通用合成器：fn(t, dur) 返回 [-1,1] 波形，加 5ms 起音防爆音
Sound Synth(float dur, const std::function<float(float, float)>& fn) {
    int n = (int)(SR * dur);
    short* data = new short[n];
    for (int i = 0; i < n; i++) {
        float t = i / (float)SR;
        float v = fn(t, dur);
        float atk = t < 0.005f ? t / 0.005f : 1.0f;   // 起音包络
        v *= atk;
        if (v > 1) v = 1;
        if (v < -1) v = -1;
        data[i] = (short)(v * 30000);
    }
    Wave w;
    w.frameCount = (unsigned)n;
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = data;
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);   // LoadSoundFromWave 内部已复制数据，释放原始缓冲
    return s;
}

// ============================================================
// 背景音乐：程序化合成一段可无缝循环的 WAV，用 Music 流循环播放
// 中式恐怖氛围：低音持续 drone + 五声音阶（宫商角徵羽）稀疏音符 + 每隔数秒的钟声
// ============================================================
unsigned char* GenBgmWav(int& outSize) {
    constexpr float DUR = 24.0f;                 // 循环长度（秒）
    int n = (int)(SR * DUR);
    short* pcm = new short[n];
    // 五声音阶（C 宫）低八度基频
    const float PENTA[5] = { 130.81f, 146.83f, 164.81f, 196.00f, 220.00f };
    for (int i = 0; i < n; i++) {
        float t = i / (float)SR;
        float v = 0.0f;
        // 低音 drone：两个失谐低频，缓慢起伏（阴气弥漫感）
        v += sinf(6.283185f * 65.41f * t) * 0.095f * (0.75f + 0.25f * sinf(t * 0.55f));
        v += sinf(6.283185f * 97.99f * t) * 0.045f * (0.75f + 0.25f * sinf(t * 0.31f));
        // 稀疏旋律：每 3 秒一音，指数衰减包络（似古筝/编钟）
        const float BEAT = 3.0f;
        int ni = (int)(t / BEAT);
        float lt = t - ni * BEAT;
        float nf = PENTA[(ni * 3) % 5] * 2.0f;
        float env = expf(-lt * 1.6f);
        v += sinf(6.283185f * nf * t) * 0.115f * env;
        v += sinf(6.283185f * nf * 2.0f * t) * 0.035f * env;    // 泛音
        // 钟声：每 12 秒一记（低频 + 五度泛音）
        float bt = fmodf(t, 12.0f);
        if (bt < 2.6f) {
            float be = expf(-bt * 1.25f);
            v += sinf(6.283185f * 98.00f * t) * 0.095f * be;
            v += sinf(6.283185f * 146.83f * t) * 0.045f * be;
        }
        // 循环缝合：首尾各 0.4s 交叉淡化，消除接缝爆音
        float fade = 1.0f;
        if (t < 0.4f) fade = t / 0.4f;
        else if (t > DUR - 0.4f) fade = (DUR - t) / 0.4f;
        v *= fade;
        if (v > 1) v = 1;
        if (v < -1) v = -1;
        pcm[i] = (short)(v * 21000);
    }
    // 打包为 WAV（RIFF / PCM 16bit 单声道）
    int dataSize = n * 2;
    int total = 44 + dataSize;
    unsigned char* buf = new unsigned char[total]();
    auto put32 = [&](int off, unsigned v) {
        buf[off] = (unsigned char)(v & 0xFF);
        buf[off + 1] = (unsigned char)((v >> 8) & 0xFF);
        buf[off + 2] = (unsigned char)((v >> 16) & 0xFF);
        buf[off + 3] = (unsigned char)((v >> 24) & 0xFF);
    };
    auto put16 = [&](int off, unsigned v) {
        buf[off] = (unsigned char)(v & 0xFF);
        buf[off + 1] = (unsigned char)((v >> 8) & 0xFF);
    };
    memcpy(buf + 0, "RIFF", 4);
    put32(4, total - 8);
    memcpy(buf + 8, "WAVE", 4);
    memcpy(buf + 12, "fmt ", 4);
    put32(16, 16);            // fmt chunk size
    put16(20, 1);             // PCM
    put16(22, 1);             // mono
    put32(24, (unsigned)SR);
    put32(28, (unsigned)(SR * 2));   // byteRate
    put16(32, 2);             // blockAlign
    put16(34, 16);            // bitsPerSample
    memcpy(buf + 36, "data", 4);
    put32(40, dataSize);
    memcpy(buf + 44, pcm, dataSize);
    delete[] pcm;
    outSize = total;
    return buf;
}

// 基础波形
float Sq(float f, float t)  { return sinf(6.283185f * f * t) >= 0.0f ? 0.8f : -0.8f; }   // 方波
float SqSoft(float f, float t) {                                                          // 软方波（减刺耳）
    float s = sinf(6.283185f * f * t);
    return (s >= 0 ? 0.7f : -0.7f) + s * 0.25f;
}
float Saw(float f, float t) {
    float p = fmodf(t * f, 1.0f);
    return (p * 2.0f - 1.0f) * 0.8f;
}
float Noise() { return ((float)rand() / RAND_MAX) * 2.0f - 1.0f; }
float Env(float t, float k) { float e = expf(-t * k); return e; }   // 指数衰减包络

} // namespace

void AudioBank::Init() {
    SetMasterVolume(0.6f);

    // 挥砍破空：带通感白噪声快速衰减
    swing = Synth(0.13f, [](float t, float) {
        return Noise() * Env(t, 30.0f) * 0.5f;
    });

    // 命中敌人：清脆方波高音 + 少量噪声
    hit = Synth(0.10f, [](float t, float) {
        float f = 880.0f - t * 1800.0f;
        return (SqSoft(f, t) * Env(t, 35.0f) + Noise() * Env(t, 60.0f) * 0.25f) * 0.9f;
    });

    // 砍树：低方波闷响 + 木屑噪声
    chop = Synth(0.13f, [](float t, float) {
        return Sq(170.0f, t) * Env(t, 28.0f) * 0.8f + Noise() * Env(t, 45.0f) * 0.35f;
    });

    // 采石：清脆噪声脉冲 + 石质高频
    mine = Synth(0.11f, [](float t, float) {
        return Noise() * Env(t, 38.0f) * 0.8f + Sq(340.0f, t) * Env(t, 55.0f) * 0.3f;
    });

    // 拾取：两段上行方波
    pickup = Synth(0.13f, [](float t, float) {
        float f = t < 0.06f ? 660.0f : 990.0f;
        return SqSoft(f, t) * Env(fmodf(t, 0.06f) * 14.0f + 8.0f, 1.0f) * 0.6f;
    });

    // 进食：两声咀嚼噪声脉冲
    eat = Synth(0.18f, [](float t, float) {
        float g = 0;
        if (t < 0.07f) g = Env(t, 34.0f);
        else if (t > 0.1f) g = Env(t - 0.1f, 34.0f);
        return Noise() * g * 0.55f;
    });

    // 玩家受击：下行锯齿
    hurt = Synth(0.26f, [](float t, float) {
        float f = 300.0f - t * 700.0f; if (f < 80) f = 80;
        return Saw(f, t) * Env(t, 13.0f) * 0.8f;
    });

    // 僵尸受击：闷方波 + 噪声
    zombieHit = Synth(0.13f, [](float t, float) {
        return Sq(150.0f, t) * Env(t, 26.0f) * 0.7f + Noise() * Env(t, 40.0f) * 0.4f;
    });

    // 僵尸死亡：低频呻吟下行
    zombieDie = Synth(0.45f, [](float t, float) {
        float f = 110.0f - t * 130.0f; if (f < 40) f = 40;
        return SqSoft(f, t) * Env(t, 7.0f) * 0.7f + Noise() * Env(t, 10.0f) * 0.3f;
    });

    // 树倒下：低沉轰响
    treeFall = Synth(0.4f, [](float t, float) {
        return Noise() * Env(t, 8.0f) * 0.7f + Sq(70.0f, t) * Env(t, 10.0f) * 0.5f;
    });

    // 合成成功：C-E-G 琶音
    craft = Synth(0.34f, [](float t, float) {
        float f = 523.25f;
        if (t > 0.11f) f = 659.25f;
        if (t > 0.22f) f = 783.99f;
        float lt = fmodf(t, 0.11f);
        return SqSoft(f, t) * Env(lt * 40.0f, 1.0f) * 0.55f;
    });

    // 放置：闷木声
    place = Synth(0.12f, [](float t, float) {
        return Sq(220.0f, t) * Env(t, 32.0f) * 0.7f;
    });

    // 玩家死亡：缓慢下行挽歌
    playerDie = Synth(0.7f, [](float t, float) {
        float f = 220.0f - t * 200.0f; if (f < 50) f = 50;
        return Saw(f, t) * Env(t, 4.5f) * 0.7f;
    });

    // Boss 咆哮：低频锯齿下滑 + 颤音调制（压迫感）
    roar = Synth(0.9f, [](float t, float) {
        float f = 130.0f - t * 70.0f; if (f < 45) f = 45;
        float vib = 1.0f + 0.12f * sinf(6.283185f * 9.0f * t);
        return Saw(f * vib, t) * Env(t, 3.2f) * 0.75f + Noise() * Env(t, 6.0f) * 0.2f;
    });

    // 震地冲击：超低频方波轰鸣 + 密集噪声
    shock = Synth(0.5f, [](float t, float) {
        return Sq(55.0f, t) * Env(t, 9.0f) * 0.85f + Noise() * Env(t, 14.0f) * 0.5f;
    });

    // 收鬼铃音：双音铜铃（E6 起振，A6 追随）+ 铃舌轻击噪声
    capture = Synth(0.55f, [](float t, float) {
        float v = sinf(6.283185f * 1318.5f * t) * Env(t, 7.0f) * 0.45f;
        if (t > 0.12f)
            v += sinf(6.283185f * 1760.0f * t) * Env(t - 0.12f, 6.0f) * 0.4f;
        v += Noise() * Env(t, 30.0f) * 0.06f;
        return v;
    });

    // 摄魂牵引啸音：收鬼施法（符纸破空 + 魂火被拽出的上滑啸声）
    capturePull = Synth(0.38f, [](float t, float) {
        float f = 300.0f + t * 1500.0f;                        // 频率上滑：符力牵引
        return sinf(6.283185f * f * t) * Env(t, 6.0f) * 0.32f
             + Noise() * Env(t, 16.0f) * 0.30f;                // 破空噪声起手
    });

    // 记忆恢复钟音：身份归位（E5→A5 清澈双音正弦钟，释然感）
    memoryRestore = Synth(0.5f, [](float t, float) {
        float f = (t < 0.18f) ? 659.25f : 880.0f;
        float lt = (t < 0.18f) ? t : t - 0.18f;
        return sinf(6.283185f * f * t) * Env(lt * 9.0f, 1.0f) * 0.42f;
    });

    // 记忆错乱咕哝：两只失谐方波拍频（鬼在耳边絮语，越听越糊涂）
    confuseWarble = Synth(0.55f, [](float t, float) {
        float w = 1.0f + 0.06f * sinf(6.283185f * 7.0f * t);   // 颤音摆动
        return (SqSoft(196.0f * w, t) + SqSoft(207.0f * w, t)) * Env(t, 4.0f) * 0.26f;
    });

    SetSoundVolume(swing, 0.6f);
    SetSoundVolume(hit, 0.8f);
    SetSoundVolume(chop, 0.8f);
    SetSoundVolume(mine, 0.8f);
    SetSoundVolume(pickup, 0.6f);
    SetSoundVolume(eat, 0.7f);
    SetSoundVolume(hurt, 0.85f);
    SetSoundVolume(zombieHit, 0.7f);
    SetSoundVolume(zombieDie, 0.8f);
    SetSoundVolume(treeFall, 0.8f);
    SetSoundVolume(craft, 0.7f);
    SetSoundVolume(place, 0.7f);
    SetSoundVolume(playerDie, 0.9f);
    SetSoundVolume(roar, 1.0f);
    SetSoundVolume(shock, 0.95f);
    SetSoundVolume(capture, 0.8f);
    SetSoundVolume(capturePull, 0.7f);
    SetSoundVolume(memoryRestore, 0.75f);
    SetSoundVolume(confuseWarble, 0.6f);
    // BGM：合成 WAV 字节流 -> Music 流（循环由 main 每帧 Update 驱动）
    {
        int sz = 0;
        unsigned char* wav = GenBgmWav(sz);
        bgm = LoadMusicStreamFromMemory(".wav", wav, sz);
        delete[] wav;                       // raylib 内部已复制
        SetMusicVolume(bgm, bgmVol);
    }
}

void AudioBank::Unload() {
    UnloadSound(swing); UnloadSound(hit); UnloadSound(chop); UnloadSound(mine);
    UnloadSound(pickup); UnloadSound(eat); UnloadSound(hurt);
    UnloadSound(zombieHit); UnloadSound(zombieDie); UnloadSound(treeFall);
    UnloadSound(craft); UnloadSound(place); UnloadSound(playerDie);
    UnloadSound(roar); UnloadSound(shock); UnloadSound(capture);
    UnloadSound(capturePull); UnloadSound(memoryRestore); UnloadSound(confuseWarble);
    if (bgm.ctxData != nullptr) { StopMusicStream(bgm); UnloadMusicStream(bgm); bgm = {}; }
}
