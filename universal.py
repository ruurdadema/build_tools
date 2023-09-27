#!/usr/bin/env python3 -u

# MIT License
#
# Copyright (c) 2022 Owllab
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from pathlib import Path
import string
import re
import subprocess
import glob
import shutil


def verify_architecture(path: Path, expected_architectures: [string]):
    """
    Verifies given file to contain all expected architectures. Throws an exception is expectation is not met.
    :param path: The path of the file to check.
    :param expected_architectures: The expected architectures.
    """
    output = subprocess.run(['file', path], stdout=subprocess.PIPE, check=True)
    stdout = output.stdout.decode('utf-8').strip()

    for arch in expected_architectures:
        if not re.search('Mach-O 64-bit.+ ' + arch, stdout):
            print(stdout)
            raise Exception('File "' + str(path) + '" does not contain code for architecture ' + arch)

    print('Verified "' + str(path) + '" to contain code for arch(s): [' + ', '.join(expected_architectures) + ']')


def verify_architecture_recursive(base_path: Path, pattern, expected_architectures: [string]):
    """
    Verifies all matching files if they contain code for given architectures.
    :param base_path: The base path to recursively search in.
    :param pattern: The pattern to search for.
    :param expected_architectures: The expected architectures.
    """
    for path in base_path.rglob(pattern):
        verify_architecture(path, expected_architectures)


def lipo(input_base_path_x86_64: Path, input_base_path_arm64: Path, output_base_path: Path, filename: Path):
    """
    Lipos binaries from two different places together.
    :param input_base_path_x86_64: The path where to find the x86_64 binary.
    :param input_base_path_arm64: The path where to find the arm64 binary.
    :param output_base_path: The path where to put the universal binary.
    :param filename: The filename of the binaries to lipo, relative to the base paths.
    :return:
    """

    x86_64_file_path = input_base_path_x86_64 / filename
    arm64_file_path = input_base_path_arm64 / filename
    output_file_path = output_base_path / filename

    subprocess.run(['lipo', '-create', x86_64_file_path, arm64_file_path, '-output', output_file_path], check=True)

    verify_architecture(output_file_path, ['x86_64', 'arm64'])


def lipo_app_bundle(input_base_path_x86_64: Path, input_base_path_arm64: Path, output_base_path: Path, app_name: str,
                    app_bundle_extension: str = '.app'):
    """
    Copies and lipos and app bundle. Assumes standard macOS folder structure (i.e. MyApp.app/Contents/MacOS/MyApp)
    :param input_base_path_x86_64: Path to x86_64 bundle.
    :param input_base_path_arm64: Path to arm64 bundle.
    :param output_base_path: Path to output directory.
    :param app_name: The name of the app (without .app).
    :param app_bundle_extension: If the bundle is not a default app (AU, VST3 for example) then use this argument to
    specify the extension type.
    """
    app_bundle = Path(app_name + app_bundle_extension)
    shutil.copytree(input_base_path_x86_64 / app_bundle, output_base_path / app_bundle, symlinks=True,
                    dirs_exist_ok=True)

    path_to_binary = app_bundle / 'Contents' / 'MacOS' / app_name

    if app_bundle_extension.endswith('.dSYM'):
        path_to_binary = app_bundle / 'Contents' / 'Resources' / 'DWARF' / app_name

    lipo(input_base_path_x86_64, input_base_path_arm64, output_base_path, path_to_binary)

    return output_base_path / app_bundle


def lipo_glob(input_base_path_x86_64: Path, input_base_path_arm64: Path, output_base_path: Path, glob_pattern):
    """
    Lipos all glob matched binaries together. This requires equal matches for both base directories, otherwise this
    function will raise an exception.
    :param input_base_path_x86_64: The path where to find the x86_64 binary.
    :param input_base_path_arm64: The path where to find the arm64 binary.
    :param output_base_path: The path where to put the universal binary.
    :param glob_pattern: The pattern to match against.
    """

    matches_x86_64 = glob.glob(glob_pattern, root_dir=input_base_path_x86_64)
    matches_arm64 = glob.glob(glob_pattern, root_dir=input_base_path_arm64)

    if matches_x86_64 != matches_arm64:
        raise Exception('Matched files not equal for both directories')

    # Since we've just proven that both lists are equal at this point we only have to iterate one.

    for path_to_binary in matches_x86_64:
        lipo(input_base_path_x86_64, input_base_path_arm64, output_base_path, path_to_binary)
