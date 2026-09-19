#!/usr/bin/env node
// 还魂人 / Huanhun Ren —— 本地化表生成器
//
// 读取 tools/translations.json（{ "中文原串": "English", ... }），生成
//   l10n.cpp  —— 按 UTF-8 字节序升序排列的静态数组 + 二分查找
//   l10n.h
//
// 用法：node tools/gen_l10n.js
//
// 为什么要排序：L10N() 用 strcmp 二分查找，表必须按键升序，否则查不到。
// 为什么不用 unordered_map：静态数组零堆分配、零初始化顺序问题，
// 而且中文模式下 L10N() 第一行就 return，没有任何运行时开销。

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const SRC = path.join(__dirname, 'translations.json');

const merged = JSON.parse(fs.readFileSync(SRC, 'utf8'));

// JS 串 -> C++ 字面量内容：反斜杠加倍，换行还原成转义序列
// （运行时拼接的格式串必须原样保留 %d/%s 与 \n，否则查表键对不上）
function cstr(s) {
  return s.replace(/\\/g, '\\\\').replace(/\n/g, '\\n').replace(/"/g, '\\"');
}

const keys = Object.keys(merged);
const keysAsc = keys
  .slice()
  .sort((a, b) => Buffer.compare(Buffer.from(a, 'utf8'), Buffer.from(b, 'utf8')));
const rows = keysAsc.map(k => `    { "${cstr(k)}", "${cstr(merged[k])}" },`).join('\n');

const cpp = `// ============================================================
//  还魂人 / Huanhun Ren - 本地化层（L10N）
//  中文原串 -> 英文。本文件由 tools/gen_l10n.js 生成，请勿手改；
//  要改文案请改 tools/translations.json 后重新生成。
//  运行时：gL10nEn=false 时 L10N() 直接返回原串（零开销，中文体验不变）。
//  实现：静态有序数组 + 二分查找。零堆分配、零生命周期问题。
// ============================================================
#include "l10n.h"
#include <cstring>
#include <cstddef>

bool gL10nEn = false;                 // 语言开关：false=中文（默认） true=英文

struct L10nEnt { const char* zh; const char* en; };

// 必须按 zh 的 strcmp 升序（由生成脚本保证），否则二分查找失效
static const L10nEnt L10N_TAB[] = {
${rows}
};

static const size_t L10N_N = sizeof(L10N_TAB) / sizeof(L10N_TAB[0]);

void L10n_Init() {
    // 表是编译期静态数据，无需运行时构建；此函数仅保留给未来（如表序校验）
}

// 查表：命中返回英文，未命中返回原串（新文案没翻译也不会显示成空白）
const char* L10N(const char* s) {
    if (!gL10nEn || s == nullptr || *s == 0) return s;
    size_t lo = 0, hi = L10N_N;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int c = strcmp(s, L10N_TAB[mid].zh);
        if (c == 0) return L10N_TAB[mid].en;
        if (c < 0) hi = mid; else lo = mid + 1;
    }
    return s;
}
`;

const h = `#pragma once
// 本地化层：中文原串 -> 英文（表见 l10n.cpp，由 tools/gen_l10n.js 生成）
extern bool gL10nEn;                   // false=中文（默认） true=英文
void L10n_Init();                      // 兼容性入口：静态表无需构建
const char* L10N(const char* s);       // 查表翻译；未命中返回原串
`;

fs.writeFileSync(path.join(ROOT, 'l10n.cpp'), cpp, 'utf8');
fs.writeFileSync(path.join(ROOT, 'l10n.h'), h, 'utf8');

console.log('l10n.cpp / l10n.h 已生成，条目数:', keys.length);

const stillChinese = keysAsc.filter(k => /[\u4e00-\u9fff]/.test(merged[k]));
if (stillChinese.length) {
  console.log('WARN 译文里仍有中文:', stillChinese.length, stillChinese.slice(0, 5));
}
