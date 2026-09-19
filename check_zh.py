#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
中文字形覆盖检查：
1) 从全部源码的字符串字面量中提取所有非 ASCII 字符
2) 从 main.cpp 的 ZHTEXT 常量中提取已注册码点
3) 报告未覆盖字符及其出现位置（缺字 -> 显示为空白/豆腐块/问号）
"""
import re, sys, glob, io

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

SRC = ['main.cpp', 'world.cpp', 'world.hpp', 'creature.cpp', 'creature.hpp',
       'assets.cpp', 'assets.hpp', 'audio.cpp', 'audio.hpp', 'water.cpp',
       'water.hpp']

# --- 1. 读取 ZHTEXT ---
src_main = open('main.cpp', encoding='utf-8', errors='replace').read()
m = re.search(r'static const char\* ZHTEXT\s*=\s*((?:\s*"[^"]*")+)\s*;', src_main)
if not m:
    print('[FATAL] 未找到 ZHTEXT 定义')
    sys.exit(1)
zhtext = ''.join(re.findall(r'"([^"]*)"', m.group(1)))
registered = set(zhtext)

# --- 2. 扫描源码中的字符串字面量 ---
# 匹配 "...." 忽略转义引号；同时跳过 // 行注释（粗略：用行级判断）
missing = {}          # ch -> [file:line:snippet]
used = set()

for path in SRC:
    try:
        lines = open(path, encoding='utf-8', errors='replace').read().split('\n')
    except FileNotFoundError:
        continue
    for ln, line in enumerate(lines, 1):
        code = line
        # 去掉行注释
        ci = code.find('//')
        if ci >= 0:
            code = code[:ci]
        for lit in re.findall(r'"((?:[^"\\]|\\.)*)"', code):
            # 仅解反斜杠转义（不能用 unicode_escape，会破坏 UTF-8 多字节序列）
            lit = re.sub(r'\\(.)', lambda m: {'n': '\n', 't': '\t',
                                              'r': '\r', '0': '\0'}.get(m.group(1), m.group(1)), lit)
            for ch in lit:
                if ord(ch) < 128:
                    continue
                used.add(ch)
                if ch not in registered:
                    missing.setdefault(ch, []).append(f'{path}:{ln}: {line.strip()[:80]}')

print(f'ZHTEXT 已注册码点: {len(registered)}')
print(f'源码中实际使用非 ASCII 字符: {len(used)}')
print()

if not missing:
    print('[OK] 全部中文字符已在 ZHTEXT 中注册，无缺字风险。')
else:
    print(f'[WARN] 发现 {len(missing)} 个未注册字符（这些字在游戏中将无法显示）：')
    print('缺字集合:', ''.join(sorted(missing.keys())))
    print()
    for ch in sorted(missing.keys()):
        locs = missing[ch][:3]
        print(f'  U+{ord(ch):04X} "{ch}"  x{len(missing[ch])}')
        for l in locs:
            print(f'      {l}')

# --- 3. 反向：已注册但未使用（仅提示）---
unused = registered - used
if unused:
    print(f'\n[INFO] ZHTEXT 中已注册但源码未使用的字符 {len(unused)} 个（无害，仅占图集）:')
    print(''.join(sorted(unused)))
