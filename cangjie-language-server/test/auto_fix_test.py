# Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
# This source file is part of the Cangjie project, licensed under Apache-2.0
# with Runtime Library Exception.
#
# See https://cangjie-lang.cn/pages/LICENSE for license information.
import os.path
import re


def read_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        return file.readlines()

def write_file(file_path, content):
    with open(file_path, 'w', encoding='utf-8') as file:
        return file.write(content)

def get_relative_path(full_path):
    match = re.search(r'test/(.+)', full_path)
    if match:
        return './' + match.group(1).strip()
    return None

def parse_build_log_and_fix_tests(build_log):
    if not os.path.exists(build_log):
        print(f"not find file: {build_log}")
        return
    lines = read_file(build_log)

    base_file_path = None
    base_content = None
    result_content = None

    i = 0
    fix_count = 0
    while i < len(lines):
        line = lines[i]

        if line.strip().endswith('curBaseFile:'):
            if i + 1 < len(lines):
                next_line = lines[i + 1];
                match = re.search(r'file:///(.+)', next_line)
                if match:
                    full_path = match.group(1).strip()
                    base_file_path = get_relative_path(full_path)
            i += 1

        if 'inBase={' in line:
            match = re.search(r'inBase=\{(.+?)\}', line)
            if match:
                base_content = match.group().strip()

        if 'result={' in line and base_content:
            match = re.search(r'result=\{(.*)\}', line)
            if match:
                result_content = '{' + match.group(1).strip() + '}'

        if base_file_path and result_content and base_content:
            if os.path.exists(base_file_path):
                write_file(base_file_path, result_content)
                print(f"Updated file: {base_file_path}")
                fix_count += 1
            else:
                print(f"File not found: {base_file_path}")

            base_file_path = None
            base_content = None
            result_content = None

        i += 1
    print(f"修改 {fix_count} 个测试用例")

if __name__ == '__main__':
    parse_build_log_and_fix_tests('build_log.txt')