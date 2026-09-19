# 还魂人（Revenant · Huanhun Ren）

C++17 + raylib 5.5 开发的 2D 俯视像素**生存**游戏，包一层中式微恐「收鬼」外壳。英文版发布名为 **Revenant**。

**零外部素材。** 仓库里没有一张图、一段音频、甚至一个字体文件。所有精灵由「调色板 + 像素字符串」在启动时程序化生成，所有音效由 PCM 波形合成，中文字形按需从系统字体栅格化。你 clone 下来的就是游戏运行所需要的一切。

[English README](README.md) · [MIT 许可](LICENSE)

---

## 亮点

- **鬼靠规则杀人，不靠数值。** 每只鬼生成时从 11 条规则里抽 1 条（35% 概率抽到第 2 条）。规则触发前它是中立的、完全无害；一旦触发就永久敌对——*它记得你*。两只长得一模一样的鬼，规则可能完全不同，所以你得读它头顶浮着的两个字，而不是背表。
- **凡兵伤不了鬼。** 你是活人，你的剑对鬼打 0 伤害。要么按规则绕开，要么**用鬼打鬼**——用摄魂幡收服残魂，再驱使鬼仆上前。
- **双语，游戏内可切。** 默认中文，可切英文，选择写入 `lang.cfg`。所有面向玩家的文案都走同一层翻译，见 [本地化](#本地化)。
- **全部程序化生成。** 地形是值噪声岛屿，一次渲染进 2560×2560 贴图；昼夜 2 分钟白天对 1 分钟夜晚；水面着色器反射天空里真实存在的天体。
- **像素对齐是构造出来的。** 640×360 内部分辨率整数倍缩放，所有贴图 `NEAREST`，相机坐标取整。

---

## 截图

以下八张全部由游戏自带的**无头截图管线**导出，是真实帧，不是效果图。

<table>
  <tr>
    <td><img src="screenshots/title-en.png" width="400" alt="标题画面（英文）"></td>
    <td><img src="screenshots/hud.png" width="400" alt="游戏内 HUD"></td>
  </tr>
  <tr>
    <td align="center">标题画面（英文模式）</td>
    <td align="center">HUD：状态、Buff、伙伴、每日悬赏</td>
  </tr>
  <tr>
    <td><img src="screenshots/crafting.png" width="400" alt="合成面板"></td>
    <td><img src="screenshots/night-camp.png" width="400" alt="夜里扎营"></td>
  </tr>
  <tr>
    <td align="center">合成面板：14 个配方分两页</td>
    <td align="center">夜里扎下带符的营地</td>
  </tr>
  <tr>
    <td><img src="screenshots/book-bestiary.png" width="400" alt="图鉴页"></td>
    <td><img src="screenshots/pause-settings-en.png" width="400" alt="暂停与设置"></td>
  </tr>
  <tr>
    <td align="center">手册·图鉴页（用到才知道）</td>
    <td align="center">暂停面板（最后一行是语言开关）</td>
  </tr>
  <tr>
    <td><img src="screenshots/night-lightmap.png" width="400" alt="夜间光照"></td>
    <td><img src="screenshots/territory.png" width="400" alt="已立领地"></td>
  </tr>
  <tr>
    <td align="center">夜间光照：灯笼与篝火的衰减</td>
    <td align="center">一块刚立起来的领地</td>
  </tr>
</table>

---

## 构建

### Windows 一键

双击 **`build.bat`**，产出 `game.exe`（并把 `raylib.dll` 拷到旁边）。

脚本默认假设：

| 依赖 | 默认位置 | 兜底 |
|---|---|---|
| MinGW-w64 `g++` | `D:\codetool\mingw64\bin\g++.exe` | `PATH` 里的 `g++` |
| raylib 5.5 (MinGW-w64) | `D:\codetool\raylib-5.5_win64_mingw-w64` | 自动搜索 `D:\codetool\raylib*`、`D:\raylib*`、`D:\*\raylib*` |

工具链装在别处的话，改 `build.bat` 顶部那几行 `set` 即可。

### CMake + Ninja

```bat
cmake -B build -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER=<mingw>/bin/gcc.exe ^
  -DCMAKE_CXX_COMPILER=<mingw>/bin/g++.exe ^
  -DCMAKE_MAKE_PROGRAM=<ninja>/ninja.exe
cmake --build build
```

### 手写命令行

```
g++ main.cpp world.cpp creature.cpp assets.cpp audio.cpp water.cpp ^
    progress.cpp net.cpp l10n.cpp ^
    -o game.exe -std=c++17 -O2 -Wall ^
    -I<raylib>/include -L<raylib>/lib ^
    -lraylibdll -lopengl32 -lgdi32 -lwinmm -lws2_32
```

> 九个编译单元一个都不能少：漏掉 `progress.cpp` 或 `net.cpp` 会在链接期报错，漏 `l10n.cpp` 会报 `L10N` 未定义。动态链接版需要把 `raylib.dll` 放在 exe 旁边。

---

## 操作

| 按键 | 作用 |
|---|---|
| `W A S D` | 移动（八向，120 px/s） |
| `Shift` | 疾跑（170 px/s） |
| `J` / 鼠标左键 | 攻击（扇形判定，0.35 s 冷却）；环形栏打开时用于确认槽位 |
| `F` | 开关镇鬼幡栏；栏内左键放出鬼仆、右键召回 |
| 滚轮 | 打开环形道具栏并切换槽位（闲置 2.2 s 自动收起） |
| `1`–`9`、`0` | 选槽（合成时 `1`–`7` 直达对应配方） |
| `Q` | 标记/取消标记最近的鬼——被标记的鬼会持续显示它的规则 |
| `B` | 摄魂幡面板：收鬼全套参数 |
| `N` | 就地立界碑、开领地（木 ×30 + 石 ×15） |
| `K` | 盘问最近的同伴（每天 2 次，每级领地 +1）。4 s 内再按一次可驱逐 |
| `E` | 情境交互：铁匠 / 可交互的鬼 / 幸存者 / **界碑（立、升级）** / 床（睡觉） |
| `X` | 符箓环——滚轮切换，左键使用 |
| `Tab` | 开关锻炉面板；在 14 个配方间循环（跨页） |
| `CapsLock` / `Enter` | 合成当前选中的配方 |
| `↑` / `↓` | 配方上下移动 |
| `C` | 合成并放下灯笼（木 ×5） |
| `G` | 在灯笼旁把生肉烤成熟肉 |
| `T` | 用生肉（×1）驯服附近的兔子或鹿 |
| `P` | 重新唤出新手按键指导 |
| `[` / `]` | BGM 音量减 / 加（10% 一档） |
| `Esc` | 暂停 / 继续——设置面板（含语言开关）在这里 |
| `R` | 死亡后以新生成的世界重开 |

---

## 系统

### 循环

采集 → 活着 → 打 → 给营地立符 → 下探。饥饿每 3 s 掉 1，归零后每 2 s 掉 5 血，80 以上缓慢回复。死亡结算显示**活了几天**和**击杀数**，按 `R` 重开。

### 是收，不是杀

攻击鬼造成 **0 伤害**——一串红字和绿色鬼火告诉你原因。鬼是靠收的，不是靠杀的：拿好摄魂幡，在游荡残魂旁按 `V`，它就成为能伤到鬼的鬼仆。跟在身边的铁匠鬼打不掉也驱不散——对它出手只会得到一句「杀不死」。

### 符与法器

玩家唯一能直接伤到鬼的手段。八张符、八件法器，各自独立冷却。

| 符 | 效果 | 冷却 |
|---|---|---|
| 缚 | 定住最近的鬼 3 s | 8 s |
| 驱 | 击退并强制它**重新潜行**（清掉已触发状态） | 10 s |
| 封 | 贴到最近的木墙上，该墙可挡一次攻击 | 12 s |
| 雷 | 对鬼 **50 点直接伤害** | 6 s |
| 隐 | 8 s 内**任何规则都无法触发** | 22 s |

掘坟有 45% 概率出随机符。

### 无面者

一只潜行中的无面鬼，只要累计在你 96 px 内待够 6 s，就会以「同伴」身份混进队伍——前提是你身边本来就有别人同行。它永不敌对，也永不自我暴露。每 24 s，它替换掉 140 px 内最近的一个真人同伴，而替换上来的又是无面鬼。队伍就是这样，一个接一个被悄悄换掉。

同伴数超过 5、且无面鬼占比达到 50%（领地 2 级后降到 30%）时，你就死了。领地 1 级时无面鬼的脸还是空白的破绽，2 级之后连这点破绽都没了。

唯一的识破手段是**盘问**（`K`）：问最近的「人」今天几号。真同伴答得对，无面鬼会差 2–3 天。4 s 内再按一次 `K` 驱逐——猜对了无面鬼消失，猜错了真同伴永久离开。

### 被盯上，以及声音

入夜后待在室内 25 s，你会听到三下敲门声。从那之后你被**盯上**：鬼不再理会自己的规则，每 9 s 直冲你一次。你的选择是符、鬼仆，或者熬到天亮。

15% 的鬼是声系。它们每 7 s 发一次声；在 220 px 内听到，就会无视规则锁定你，屏幕边缘会给出红色的方向箭头和距离，持续 2.2 s。

### 鬼的规则

规则触发前鬼是中立的。学会规则，就能活着走过去。

| 鬼 | 规则 | 触发条件 | 怎么活 |
|---|---|---|---|
| 游魂 | 只在夜里 | 夜晚 | 白天赶路 |
| 疾尸 | 追快不追慢 | 你在疾跑 | 松开 `Shift` |
| 尸王 | 闻到血味 | 夜晚且（血量 < 50% 或血月） | 保持健康 |
| 纸人 | 近身扑击 | 距离 < 44 | 拉开距离 |
| 蛛鬼 | 奔网而来 | 距离 < 72 | 绕开它 |
| 守冢鬼 | 擅入者死 | 距离 < 64 | 别进墓地 |
| 夜枭 | 动则击 | 夜晚且你在移动 | 站在暗处别动 |
| 酸涎鬼 | 谁打它就反谁 | 它被打过 | 别打它 |
| 石卫 | 永不回头 | 你背对它 | 面朝它 |
| 溺鬼 | 近水而现 | 你靠近水边 | 离岸远点 |
| 织魂鬼 | 看你用东西 | 在它附近使用道具 | 在它旁边什么也别用 |
| 血月王 | 血月升起 | 血月夜 | 那晚待在屋里 |

> 攻击鬼本身就是一种触发——0 伤害，但你刚刚按下了它的开关。

### 领地

立界碑（`N`）开出领地：一圈留了东南缺口的木墙、一张草床、一盏灯笼。站到自己界碑 64 px 内按 `E` 升级，同伴数与材料都要达标。

| 等级 | 同伴 | 材料 | 效果 |
|---|---|---|---|
| 1 → 2 | ≥ 2 | 木 ×40 + 石 ×25 | 容量 4；每天 3 次盘问；**无面鬼致死阈值降到 30%**，且混入者失去所有破绽 |
| 2 → 3 | ≥ 4 | 木 ×80 + 石 ×40 + 铁 ×10 | 容量 6；每天 4 次盘问 |

### 合成

地表页 7 个配方，工作台再 7 个。`Tab` 在全部 14 个之间循环，`CapsLock` 确认，`1`–`7` 直达。动画分三段：材料飞出到工作台周围的环形位置，逐个向中心相撞，最后闪光融合。

---

## 本地化

游戏内置双语，**默认中文**。运行时在暂停/设置面板最后一行切换。选择写入 exe 旁边的 `lang.cfg`，在画出任何东西之前生效。

实现上，所有面向玩家的字符串都经过 `L10N()`（`l10n.h` / `l10n.cpp`），它挂在两个绘制入口——`ZhTextStyled()` 与 `ZhWidth()`——因此调用点全部无需改动。

```
L10N(const char* s)   // 中文原串进，英文出。中文模式或未命中时原样返回 s
```

- 中文模式下 `L10N()` 第一行就返回，没有任何逐帧开销。
- 运行时拼接的串（`TextFormat`）保留原始格式串，保证查表键仍然匹配。
- 表是一张**按键排序的静态数组，用 `strcmp` 二分查找**——不碰堆、没有静态初始化顺序问题，共 811 条。

改文案的流程：

```bash
node tools/extract_zh.js      # 报告哪些中文串还没翻译、哪些键已经失效
node tools/gen_l10n.js        # 由 tools/translations.json 重新生成 l10n.cpp / l10n.h
```

`tools/translations.json` 是英文文案的唯一来源（`"中文原串": "English"`）。键必须与 C++ 字面量逐字节一致——包括 `%d` / `%s` 占位符和 `\n`——否则查表会静默落空，运行时回退成中文。

---

## 目录结构

```
main.cpp          玩家、战斗、命中反馈、粒子、UI、主循环、渲染管线
world.hpp/cpp     值噪声地形、物件网格、墓地生成、掉落、圆形碰撞滑动
creature.hpp/cpp  生物状态机 AI（7 种墓地生物 + 血月王）
assets.hpp/cpp    调色板像素画与程序化贴图（玩家 / 生物 / 地块 / 物件 / UI / 特效）
audio.hpp/cpp     PCM 合成的音效（方波 / 锯齿 / 噪声 + 包络）
water.hpp/cpp     水面着色器（波浪、反射、雨点涟漪、天体倒影）
progress.hpp/cpp  成长系统：命途、成就、图鉴、残卷、存档读写
net.hpp/cpp       局域网联机
l10n.hpp/cpp      翻译表（生成物）+ 查表
tools/
  translations.json  中英对照表（811 条）——改文案改这里
  gen_l10n.js        重新生成 l10n.cpp / l10n.h
  extract_zh.js      翻译覆盖率报告
build.bat         一键构建
CMakeLists.txt    CMake + Ninja 构建
gen_zhtext.py     重新生成 main.cpp 里的中文字库字符池
check_zh.py       校验字形覆盖
```

---

## 开发工具

### 无头 UI 出图

本机窗口截图不可靠（独占全屏程序会抢焦点），所以游戏可以把 GPU 渲染目标直接导出成 PNG。它会自动跑一套约 1750 帧的脚本化流程，把每个界面各导一张。

```
g++ main.cpp world.cpp creature.cpp assets.cpp audio.cpp water.cpp ^
    progress.cpp net.cpp l10n.cpp -o game_dbg.exe ^
    -std=c++17 -O2 -Wall -DDEBUG_HEADLESS -DDEBUG_AUTO_SHOT ^
    -I<raylib>/include -L<raylib>/lib ^
    -lraylibdll -lopengl32 -lgdi32 -lwinmm -lws2_32
game_dbg.exe > dbg_run.log 2>&1
```

导出的 PNG 方向正常，且就是游戏内部的 640×360 分辨率，所以两次运行之间可以直接对比。`screenshots/` 下那八张就是这么来的。

### 行为回归验证（DEBUG_VERIFY）

`main.cpp` 内置一组断言，覆盖历史上踩过的坑——地牢重开、采集下溢、合成动画推进、死亡环形栏、`NewGame` 状态重置、伴生铁匠鬼、鬼域进出、存档往返、横扫多目标、铁匠鬼不死、无面鬼混入与同化、死亡阈值、领地双重门槛。

```
g++ ... -DDEBUG_HEADLESS -DDEBUG_VERIFY -o pw_verify.exe ...
pw_verify.exe      # 期望 VERIFY1..VERIFY21 全部 PASS
```

### 中文字形维护

游戏里用到的每一个汉字都必须在 `main.cpp` 的 `ZHTEXT` 常量里登记，漏一个就会显示成空白。新增或修改任何中文文案之后：

```
python gen_zhtext.py     # 重扫全部源码，重建 ZHTEXT
python check_zh.py       # 期望 「no missing glyphs」
```

---

## 许可

[MIT](LICENSE)。随便用、随便改、随便发——保留版权声明即可。
