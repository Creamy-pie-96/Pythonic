#!/usr/bin/env python3
"""
Extract function signatures defined in C/C++ header files.

Usage:
  ./extract_functions.py <header-or-dir>... -o output.md

Writes numbered list of function signatures found in the provided header files.
Skips functions found inside `private:` sections of `class` definitions.
"""
import argparse
import os
import re
from pathlib import Path


def read_file(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def remove_comments(src: str) -> str:
    # Remove /* */ and // comments
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.S)
    src = re.sub(r'//.*?$', '', src, flags=re.M)
    return src


def find_class_blocks(src: str):
    # Find class/struct blocks and return list of (start_idx, end_idx, kind)
    blocks = []
    for m in re.finditer(r'\b(class|struct)\s+\w+[^{]*\{', src):
        kind = m.group(1)
        start = m.start()
        # find matching brace
        i = m.end() - 1
        depth = 1
        N = len(src)
        while i < N - 1 and depth > 0:
            i += 1
            if src[i] == '{':
                depth += 1
            elif src[i] == '}':
                depth -= 1
        end = i + 1
        blocks.append((start, end, kind))
    return blocks


def private_ranges_in_block(block_src: str, block_offset: int):
    # Find private: ranges inside a class/struct block
    ranges = []
    # default access: class -> private, struct -> public. We only care about explicit private:
    # find all access specifiers
    spec_iter = list(re.finditer(r'\b(public|private|protected)\s*:', block_src))
    for idx, m in enumerate(spec_iter):
        name = m.group(1)
        start = m.end()
        end = len(block_src)
        if idx + 1 < len(spec_iter):
            end = spec_iter[idx + 1].start()
        if name == 'private':
            ranges.append((block_offset + start, block_offset + end))
    return ranges


def in_any_range(pos: int, ranges) -> bool:
    for a, b in ranges:
        if a <= pos < b:
            return True
    return False


def extract_function_signatures(src: str, filepath: Path):
    cleaned = remove_comments(src)
    blocks = find_class_blocks(cleaned)
    # build list of private ranges across all class blocks
    private_ranges = []
    for start, end, kind in blocks:
        block_src = cleaned[start:end]
        private_ranges.extend(private_ranges_in_block(block_src, start))

    # Regex: capture signature lines that are followed by '{' (function with body)
    # Use multiline to capture multi-line signatures as one by allowing newlines before '('
    pattern = re.compile(r'(^[^;{}\n][^{;]*?\([^;{}]*?\)\s*(?:[^\{;]*?)\{)', re.M)

    results = []
    for m in pattern.finditer(cleaned):
        sig = m.group(1).strip()
        pos = m.start(1)
        # Skip functions inside private ranges
        if in_any_range(pos, private_ranges):
            continue

        # remove opening brace
        sig = re.sub(r'\s*\{\s*$', '', sig).strip()

        # skip control statements (if/for/while/switch/else etc.)
        if re.match(r'^(?:if|else|for|while|switch|case|return|goto|break|continue|do|catch|sizeof)\b', sig):
            continue

        # remove leading closing braces or stray tokens
        sig = re.sub(r'^[}\s]+', '', sig)

        # remove initializer lists that appear between ')' and '{' (e.g., ": member(a)" )
        last_paren = sig.rfind(')')
        if last_paren != -1:
            after = sig[last_paren+1:]
            colon_idx = after.find(':')
            if colon_idx != -1:
                # keep only up to the ')' (and any allowed qualifiers before colon)
                sig = sig[: last_paren + 1 ]

        # strip common qualifiers that might follow the parameter list
        sig = re.sub(r'\)\s*(?:const|noexcept|constexpr|volatile|mutable|\bthrow\([^)]*\))', ')', sig)

        # collapse whitespace
        sig = re.sub(r'\s+', ' ', sig).strip()

        # ensure it looks like a function: find the token immediately before '('
        mname = re.search(r'([A-Za-z_]\w*(?:::\w+)*)\s*\(', sig)
        if not mname:
            continue
        name = mname.group(1)

        # skip common C/C++ keywords and control tokens used in code (not function names)
        cpp_keywords = {
            'if', 'else', 'for', 'while', 'switch', 'case', 'return', 'goto', 'break', 'continue',
            'do', 'catch', 'sizeof', 'constexpr', 'template', 'throw', 'public', 'private', 'protected'
        }
        if name in cpp_keywords:
            continue

        # skip lines that include access labels, initializer lists, or stray colons
        if ':' in sig and not sig.strip().startswith('template'):
            # likely an initializer list or access label (e.g., 'VarData() : ll(0)' or 'public:')
            continue

        # balanced parentheses check
        if sig.count('(') != sig.count(')'):
            continue

        results.append((filepath.as_posix(), sig))
    return results


def collect_paths(inputs):
    paths = []
    for p in inputs:
        p = Path(p)
        if p.is_dir():
            for ext in ('*.h', '*.hpp', '*.hh', '*.hxx', '*.inl', '*.ipp', '*.inc'):
                for f in p.rglob(ext):
                    paths.append(f)
        elif p.is_file():
            paths.append(p)
    return paths


def main():
    ap = argparse.ArgumentParser(description='Extract function signatures from C/C++ header files')
    ap.add_argument('paths', nargs='+', help='Header file or directory to scan')
    ap.add_argument('-o', '--output', required=True, help='Output markdown or txt file')
    args = ap.parse_args()

    paths = collect_paths(args.paths)
    all_sigs = []
    for p in paths:
        try:
            src = read_file(p)
        except Exception as e:
            print(f'warning: could not read {p}: {e}')
            continue
        sigs = extract_function_signatures(src, p)
        all_sigs.extend(sigs)

    # write output
    out = Path(args.output)
    lines = []
    for i, (file, sig) in enumerate(all_sigs, start=1):
        lines.append(f'{i}. {sig}  // {file}')

    out.write_text('\n'.join(lines), encoding='utf-8')
    print(f'Wrote {len(lines)} signatures to {out}')


if __name__ == '__main__':
    main()
