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
import shutil


def copy_into(file: Path, into: Path = Path(), exists_ok: bool = True):
    """
    Copies given file into given directory.
    :param file: The file or directory to copy.
    :param into: The folder to copy into.
    :param exists_ok: Whether to overwrite existing files.
    :return: The path to the copied file.
    """
    dst = into / file.name

    if into.exists():
        if not into.is_dir():
            raise Exception('Destination "' + str(into) + '" is not a directory')

    if dst.exists():
        if exists_ok:
            shutil.rmtree(dst)
        else:
            raise Exception('Destination "' + str(dst) + '" already exists')

    shutil.copytree(file, dst, symlinks=True)

    return dst
