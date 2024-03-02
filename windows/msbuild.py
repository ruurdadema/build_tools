#!/usr/bin/env python3 -u

import os
import subprocess
import sys

vs_versions = ['2022', '2019', '2017']
vs_editions = ['Enterprise', 'Professional', 'Community']


class MSBuild:
    """
    This is a little wrapper around msbuild which makes it easy and convenient to call msbuild from python.
    """

    def __init__(self, path, version, edition, vs_path):
        self.path = path
        self.version = version
        self.edition = edition
        self.vs_path = vs_path

    def __repr__(self):
        return "<MSBuild %s %s>" % (self.edition, self.version)

    def __lt__(self, other):
        if self.version == other.version:
            return vs_editions.index(self.edition) < vs_editions.index(other.edition)
        else:
            return self.version < other.version

    def call(self, args, with_dev_env=True):
        """
        Calls a specific MSBuild referenced by this instance.

        Args:
            args: The arguments to pass wot msbuild
            with_dev_env: If True, a VS devenv setup script will be called before calling MSBuild
        """
        msbuild_command = []
        if with_dev_env:
            msbuild_command += [os.path.join(self.vs_path, 'Common7', 'Tools', 'VsDevCmd.bat'), '&']
        msbuild_command += [self.path] + args
        print('Execute MSBuild: ' + str(msbuild_command))
        if subprocess.call(msbuild_command) != 0:
            sys.exit('Error executing MSbuild')


def find_most_suitable_msbuild():
    """
    Tries to find the most suitable MSBuild by iterating different directories to see what versions are installed.
    :return: An MSBuild class referencing the most ideal MSBuild or None if no MSBuild was found.
    """

    vs_studio_base_dir = os.path.join(os.environ["ProgramFiles"], 'Microsoft Visual Studio')

    for version in vs_versions:
        for edition in vs_editions:
            vs_studio_path = os.path.join(vs_studio_base_dir, version, edition)
            msbuild_path = os.path.join(vs_studio_path, 'MSBuild', 'Current', 'Bin', 'MSBuild.exe')
            if os.path.exists(vs_studio_path):
                if os.path.isfile(msbuild_path):
                    return MSBuild(msbuild_path, version, edition, vs_studio_path)

    return None


