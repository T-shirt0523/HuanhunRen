#pragma once
// 本地化层：中文原串 -> 英文（表见 l10n.cpp，由 tools/gen_l10n.js 生成）
extern bool gL10nEn;                   // false=中文（默认） true=英文
void L10n_Init();                      // 兼容性入口：静态表无需构建
const char* L10N(const char* s);       // 查表翻译；未命中返回原串
