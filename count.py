import os
import re

def count_lines_in_file(filepath):
    total = 0
    code = 0
    comments = 0
    blanks = 0
    in_block_comment = False

    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            total += 1
            stripped = line.strip()
            if not stripped:
                blanks += 1
                continue
            if in_block_comment:
                comments += 1
                if '*/' in stripped:
                    in_block_comment = False
                continue
            if stripped.startswith('//'):
                comments += 1
                continue
            if '/*' in stripped:
                comments += 1
                if not '*/' in stripped:
                    in_block_comment = True
                continue
            # Remove inline // comments
            code_line = re.sub(r'//.*', '', stripped)
            # Remove inline /* ... */ comments
            code_line = re.sub(r'/\\*.*?\\*/', '', code_line)
            if code_line.strip():
                code += 1

    return total, code, comments, blanks

def scan_dir(root):
    total = code = comments = blanks = 0
    file_stats = []
    for dirpath, _, filenames in os.walk(root):
        for fname in filenames:
            if fname.endswith('.hpp') or fname.endswith('.cpp'):
                fpath = os.path.join(dirpath, fname)
                t, c, com, b = count_lines_in_file(fpath)
                file_stats.append((fpath, t, com, b))
                total += t
                code += c
                comments += com
                blanks += b
    return total, code, comments, blanks, file_stats

if __name__ == '__main__':
    total1, code1, comments1, blanks1, files1 = scan_dir('include/pythonic')
    total2, code2, comments2, blanks2, files2 = scan_dir('src')
    total = total1 + total2
    code = code1 + code2
    comments = comments1 + comments2
    blanks = blanks1 + blanks2
    all_files = files1 + files2


    # Prepare and sort by code lines descending
    file_code_lines = [(fpath, t - com - b) for fpath, t, com, b in all_files]
    file_code_lines.sort(key=lambda x: x[1], reverse=True)

    for fpath, code_lines in file_code_lines:
        print(f'{fpath}: {code_lines}')

    print('\nsummery:')
    print(f'Total Lines: {total}')
    print(f'Total comments: {comments}')
    print(f'Total Code: {total - comments - blanks}')