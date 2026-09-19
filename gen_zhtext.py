#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
重新生成 main.cpp 的 ZHTEXT 常量（中文字形码点表）。

扫描全部源文件的字符串字面量，提取非 ASCII 字符，去重排序后
按每行 30 字写回 ZHTEXT。缺字在游戏里会显示为空白/豆腐块，
因此新增任何中文文案后都应重跑本脚本。

比 gen_zhtext.ps1 更可靠：覆盖全部源文件（不止 main.cpp）。
"""
import re, sys, io, os

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

SRCS = ['main.cpp', 'world.cpp', 'world.hpp', 'creature.cpp', 'creature.hpp',
        'assets.cpp', 'assets.hpp', 'audio.cpp', 'audio.hpp',
        'water.cpp', 'water.hpp',
        'progress.cpp', 'progress.hpp']

chars = set()
for path in SRCS:
    if not os.path.exists(path):
        continue
    lines = open(path, encoding='utf-8', errors='replace').read().split('\n')
    for line in lines:
        code = line
        ci = code.find('//')
        if ci >= 0:
            code = code[:ci]                      # 去行注释
        for lit in re.findall(r'"((?:[^"\\]|\\.)*)"', code):
            lit = re.sub(r'\\(.)', lambda m: {'n': '\n', 't': '\t',
                                              'r': '\r', '0': '\0'}.get(m.group(1), m.group(1)), lit)
            for ch in lit:
                if ord(ch) >= 128:
                    chars.add(ch)

sorted_chars = sorted(chars)
print(f'扫描到非 ASCII 字符 {len(sorted_chars)} 个')

# 每 30 字一行（与原格式一致）
LINES = []
for i in range(0, len(sorted_chars), 30):
    LINES.append('    "' + ''.join(sorted_chars[i:i + 30]) + '"')
block = '\n'.join(LINES)

src = open('main.cpp', encoding='utf-8').read()
m = re.search(r'(static const char\* ZHTEXT\s*=\s*)((?:\s*"[^"]*")+)(\s*;)', src)
if not m:
    print('[FATAL] 未找到 ZHTEXT 定义')
    sys.exit(1)
old = m.group(2)
src = src[:m.start(2)] + '\n' + block + src[m.end(2):]
open('main.cpp', 'w', encoding='utf-8', newline='\n').write(src)
print('[OK] 已更新 main.cpp 的 ZHTEXT')
print(block[:200] + ('...' if len(block) > 200 else ''))
