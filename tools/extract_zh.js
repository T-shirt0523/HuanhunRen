#!/usr/bin/env node
// 还魂人 / Huanhun Ren —— 中文字符串提取 / 覆盖率检查
//
// 扫描所有 .cpp 里的字符串字面量（先剔除注释），找出面向玩家的中文串，并对照
// tools/translations.json 报告：
//   - 尚未翻译的串（新增文案后最容易漏的一步）
//   - 翻译表里已失效的键（源码里已经没有这个串了）
//
// 用法：
//   node tools/extract_zh.js            # 覆盖率报告
//   node tools/extract_zh.js --dump     # 额外输出 tools/_zh_strings.json（供逐条翻译）

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const FILES = ['main.cpp', 'world.cpp', 'creature.cpp', 'assets.cpp',
               'audio.cpp', 'water.cpp', 'progress.cpp', 'net.cpp'];

// 逐字符扫描：把注释抹成空白，避免注释里的中文被当成文案
function stripComments(src) {
  let out = '', i = 0;
  let state = 'code';
  while (i < src.length) {
    const c = src[i], d = src[i + 1];
    if (state === 'code') {
      if (c === '/' && d === '/') { state = 'line'; out += '  '; i += 2; continue; }
      if (c === '/' && d === '*') { state = 'block'; out += '  '; i += 2; continue; }
      if (c === '"') { state = 'str'; out += c; i++; continue; }
      if (c === "'") { state = 'chr'; out += c; i++; continue; }
      out += c; i++; continue;
    }
    if (state === 'line') { if (c === '\n') { state = 'code'; out += c; } else out += ' '; i++; continue; }
    if (state === 'block') {
      if (c === '*' && d === '/') { state = 'code'; out += '  '; i += 2; continue; }
      out += (c === '\n') ? '\n' : ' '; i++; continue;
    }
    if (state === 'str') {
      if (c === '\\') { out += c + d; i += 2; continue; }
      if (c === '"') state = 'code';
      out += c; i++; continue;
    }
    if (state === 'chr') {                       // 字符字面量：'\n' 之类
      if (c === '\\') { out += c + d; i += 2; continue; }
      if (c === "'") state = 'code';
      out += c; i++; continue;
    }
  }
  return out;
}

// 中日韩统一表意文字 + 全角标点。注意 ZHTEXT 字库串也是中文，
// 它必须原样保留、不参与翻译（见下方 FONT_POOL 过滤）。
const ZH = /[\u4e00-\u9fff\u3000-\u303f\uff00-\uffef]/;
const map = new Map();

for (const f of FILES) {
  const p = path.join(ROOT, f);
  if (!fs.existsSync(p)) continue;
  const code = stripComments(fs.readFileSync(p, 'utf8'));
  code.split('\n').forEach((ln, li) => {
    const re = /"((?:[^"\\]|\\.)*)"/g;
    let m;
    while ((m = re.exec(ln)) !== null) {
      const s = m[1];
      if (!ZH.test(s)) continue;
      if (!map.has(s)) map.set(s, { str: s, count: 0, locs: [] });
      const e = map.get(s);
      e.count++;
      if (e.locs.length < 3) e.locs.push(f + ':' + (li + 1));
    }
  });
}

// 字库字符池：ZHTEXT 是一长串"所有会用到的汉字"，不是文案，必须排除。
// 注意只排除那一整条长串本身——单字文案（"开"/"关"）的字也在池里，
// 但它们是真的 UI 文案，要照常计入翻译覆盖。
const mainSrc = fs.readFileSync(path.join(ROOT, 'main.cpp'), 'utf8');
const poolMatch = /static const char\* ZHTEXT\s*=\s*((?:\s*"[^"]*")+)\s*;/.exec(mainSrc);
const poolChars = new Set(
  poolMatch ? poolMatch[1].replace(/"/g, '') : ''
);

const all = [...map.values()];
const fontPool = all.filter(e => e.str.length >= 8 && [...e.str].every(ch => poolChars.has(ch)));
const fontPoolSet = new Set(fontPool.map(e => e.str));
const strings = all.filter(e => !fontPoolSet.has(e.str));

// 有意不翻译的串：语言切换行要显示"目标语言的原生名"，切到中文就该显示"中文"
const ALLOWLIST = new Set(['中文']);

const trPath = path.join(__dirname, 'translations.json');
const tr = fs.existsSync(trPath) ? JSON.parse(fs.readFileSync(trPath, 'utf8')) : {};
const missing = strings.filter(e => !(e.str in tr) && !ALLOWLIST.has(e.str));
const unused = Object.keys(tr).filter(k => !map.has(k));

console.log('字面量中的中文串 :', strings.length, '（已排除字库字符池 ' + fontPool.length + ' 条）');
console.log('已翻译           :', strings.length - missing.length);
console.log('未翻译           :', missing.length);
console.log('翻译表条目       :', Object.keys(tr).length, '（其中源码里已不存在 ' + unused.length + ' 条）');

if (missing.length) {
  console.log('\n未翻译的串：');
  for (const e of missing) console.log('  ' + JSON.stringify(e.str) + '   ' + e.locs.join(', '));
}
if (unused.length) {
  console.log('\n翻译表里的失效键（可删）：');
  for (const k of unused.slice(0, 40)) console.log('  ' + JSON.stringify(k));
  if (unused.length > 40) console.log('  ... 共 ' + unused.length + ' 条');
}

if (process.argv.includes('--dump')) {
  const out = path.join(__dirname, '_zh_strings.json');
  fs.writeFileSync(out, JSON.stringify(strings.map(e => e.str), null, 1), 'utf8');
  console.log('\n已写出 ' + out);
}
