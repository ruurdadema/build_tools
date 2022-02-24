#!/usr/bin/env python3 -u

import subprocess
import os
from collections import OrderedDict
from enum import Enum


class Config(Enum):
    release = 'Release'
    debug = 'Debug'


class CMake:
    def __init__(self):
        self._architecture = None
        self._num_parallel = None
        self._build_config = Config(Config.release)
        self._path_to_source = None
        self._path_to_build = None
        self._generator = None
        self._options = OrderedDict()

    def path_to_build(self, path_to_build):
        self._path_to_build = path_to_build
        return self

    def path_to_source(self, path_to_source):
        self._path_to_source = path_to_source

    def generator(self, generator):
        self._generator = generator
        return self

    def option(self, key, value):
        self._options[key] = value
        return self

    def build_config(self, build_config: Config):
        self._build_config = build_config
        return self

    def parallel(self, num_parallel):
        self._num_parallel = str(num_parallel)
        return self

    def architecture(self, architecture):
        self._architecture = architecture
        return self

    def build(self):
        cmd = ['cmake']

        if self._generator:
            cmd.append('-G')
            cmd.append(self._generator)

        if self._architecture:
            cmd.append('-A')
            cmd.append(self._architecture)

        if self._path_to_build:
            cmd.append('-B')
            cmd.append(self._path_to_build)

        if self._build_config:
            self.option('CMAKE_BUILD_TYPE', self._build_config.value)

        for key, value in self._options.items():
            cmd.append('-D' + key + '=' + value)

        if self._path_to_source:
            cmd.append('-S')
            cmd.append(self._path_to_source)

        print('Invoke CMake configure command: \"{}\"'.format(' '.join(cmd)))

        # Configure
        subprocess.run(cmd, check=True)

        cmd = ['cmake']

        if self._path_to_build:
            cmd.append('--build')
            cmd.append(self._path_to_build)

        if self._build_config:
            cmd.append('--config')
            cmd.append(self._build_config.value)

        if self._num_parallel:
            cmd.append('--parallel')
            cmd.append(self._num_parallel)

        print('Invoke CMake build command: \"{}\"'.format(' '.join(cmd)))

        # Configure
        subprocess.run(cmd, check=True)
