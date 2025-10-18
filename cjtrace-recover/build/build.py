#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
# This source file is part of the Cangjie project, licensed under Apache-2.0
# with Runtime Library Exception.
#
# See https://cangjie-lang.cn/pages/LICENSE for license information.

# The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

from enum import Enum
import logging
import os
from pathlib import Path
import shutil
import subprocess
import platform
import argparse
import sys

BASE_DIR = Path(os.path.dirname(os.path.realpath(__file__))).parent
BUILD_DIR_NAME = 'cmake-build'

IS_WINDOWS = platform.system() == "Windows"
IS_MACOS = platform.system() == "Darwin"
IS_LINUX = platform.system() == "Linux"

# a map from target to toolchain file
CROSS_COMPILE_MAP = {
    "windows-x86_64": "mingw_x86_64_toolchain.cmake"
}

def run_cmd(cmd, cwd=BASE_DIR):
    try:
        logging.info(f"running cmd: {' '.join(cmd)}")
        output = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE)
        while True:
            line = output.stdout.readline()
            if not line:
                output.communicate()
                returncode = output.returncode
                if returncode != 0:
                    logging.error("build error: %d!\n", returncode)
                    sys.exit(1)
                break
            logging.info(line.decode("utf-8", "ignore").rstrip())
    except Exception as e:
        logging.error(f"Execute cmd failed: {' '.join(cmd)}")
        logging.exception(e)
        sys.exit(1)


def generate_cmake_param(args):
    param = [
        f"-DCMAKE_BUILD_TYPE={args.build_type.value}"
    ]

    if args.target:
        # cross compile
        if not args.target_sysroot:
            logging.fatal(f'you must specify windows toolchain when cross compiling windows version')
            sys.exit(1)
        
        toolchain_file = CROSS_COMPILE_MAP.get(args.target)
        if toolchain_file is None:
            logging.fatal(f"{args.target} is not supported")
            sys.exit(1)

        param.append(f'-DCMAKE_SYSROOT={args.target_sysroot}')
        param.append(f"-DCMAKE_TOOLCHAIN_FILE={str(BASE_DIR)}/cmake/{toolchain_file}")

    if args.cangjie_sdk:
        param.append(f"-DCANGJIE_HOME={str(args.cangjie_sdk)}")

    if args.prefix:
        param.append(f"-DCMAKE_INSTALL_PREFIX={args.prefix}")

    if args.set_rpath:
        param.append(f"-DRUNTIME_RPATH={args.set_rpath}")

    return param


def build(args):
    logging.info("start build cjtrace-recover")
    generator = "Ninja"
    if IS_WINDOWS:
        try:
            output = subprocess.Popen(["ninja", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            output.communicate()
            if output.returncode == 0:
                generator = "MinGW Makefiles"
        except:
            pass            

    # configure cmake
    run_cmd(['cmake', str(BASE_DIR), '-G', generator, '-B', BUILD_DIR_NAME] + generate_cmake_param(args))

    # start build
    run_cmd(['cmake', '--build', str(BASE_DIR/BUILD_DIR_NAME)])
    logging.info("end build cjtrace-recover")


def install(args):
    build_dir = BASE_DIR/BUILD_DIR_NAME
    logging.info("start install cjtrace-recover")
    if not build_dir.exists():
        logging.error('Please build cjtrace-recover first')
    cmd = ['cmake', '--install', str(build_dir)]
    if args.prefix:
        cmd.append('--prefix')
        cmd.append(args.prefix)

    run_cmd(cmd)
    logging.info("end install cjtrace-recover")


def clean(args):
    build_dir = BASE_DIR/BUILD_DIR_NAME
    logging.info("start clean cjtrace-recover")
    if build_dir.is_dir():
        logging.info(f"removing {build_dir}")
        shutil.rmtree(build_dir)
    logging.info("end clean cjtrace-recover")


class DirectoryPathVerifier(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        p = Path(values)
        if not p.exists():
            raise argparse.ArgumentTypeError(f"Directory '{values}' does not exist.")
        if not p.is_dir():
            raise argparse.ArgumentTypeError(f"'{values}' is not a directory.")
        setattr(namespace, self.dest, p)


class BuildType(Enum):
    """CMAKE_BUILD_TYPE options"""

    debug = "Debug"
    release = "Release"
    relwithdebinfo = "RelWithDebInfo"
    minsizerel = "MinSizeRel"
    minsizerelwithdebinfo = "MinSizeRelWithDebInfo"

    def __str__(self):
        return self.name

    def __repr__(self):
        return str(self)

    @staticmethod
    def argparse(s):
        try:
            return BuildType[s]
        except KeyError:
            return s.build_type


def main():
    logging.basicConfig(
        level=logging.INFO, format='[%(asctime)s] [%(module)s] [%(levelname)s] %(message)s', stream=sys.stdout)
    parser = argparse.ArgumentParser(description='Build system')
    subparsers = parser.add_subparsers(dest='command', help='Available commands')

    # Build command
    build_parser = subparsers.add_parser('build', help='Build cjtrace-recover')
    build_parser.add_argument(
        "-t",
        "--build-type",
        type=BuildType.argparse,
        dest="build_type",
        default="debug",
        choices=list(BuildType),
        help="select target build type",
    )
    build_parser.add_argument('--target', type=str, dest='target', help='Specify build target platform')
    build_parser.add_argument('--set-rpath', type=str, help='Set rpath value')
    build_parser.add_argument('--target-sysroot', action=DirectoryPathVerifier,
                              help='specify target sysroot when cross compiling')
    build_parser.add_argument('--prefix', help='Specify installation prefix')
    build_parser.add_argument('--cangjie-sdk', action=DirectoryPathVerifier, help='Specify cangjie sdk path')
    build_parser.set_defaults(func=build)

    # Install command
    install_parser = subparsers.add_parser('install', help='Install cjtrace-recover')
    install_parser.add_argument('--prefix', help='Specify installation prefix')
    install_parser.set_defaults(func=install)

    # Clean command
    clean_parser = subparsers.add_parser('clean', help='Clean build files')
    clean_parser.set_defaults(func=clean)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
