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


import subprocess
from pathlib import Path


class InnoSetup:
    """
    A class which can build an installer using Inno Setup
    """

    def __init__(self, appname: str, app_publisher: str, appversion: str):
        self._license_file = None
        self._build_path = Path()
        self._appid = None
        self._appname = appname
        self._appversion = appversion
        self._app_publisher = app_publisher

    def generate(self, build_path: Path):
        self._build_path = build_path
        self._build_path.mkdir(parents=True, exist_ok=True)

        script = ''
        script += '[Setup]\n'

        if self._appid:
            script += '; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.\n'
            script += '; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)\n'
            script += 'AppId={{{{{}}}\n'.format(self._appid)

        # AppName
        script += 'AppName={}\n'.format(self._appname)

        # AppVersion
        script += 'AppVersion={}\n'.format(self._appversion)

        # AppPublisher=My Company, Inc.
        # AppPublisherURL=https://www.example.com/
        # AppSupportURL=https://www.example.com/
        # AppUpdatesURL=https://www.example.com/
        
        # DefaultDirName
        script += 'DefaultDirName={{autopf}}\\{}\\{}\n'.format(self._app_publisher, self._appname)

        # DefaultGroupName
        script += 'DefaultGroupName={}\n'.format(self._appname)

        # OutputBaseFilename
        script += 'OutputBaseFilename={}\n'.format(self._appname)

        # Compression
        script += 'Compression=lzma\n'
        script += 'SolidCompression=yes\n'

        # Style
        script += 'WizardStyle=modern\n'

        if self._license_file:
            script += 'LicenseFile={}'.format(self._license_file)

        with open(self._build_path / 'generated.iss', 'w') as f:
            f.write(script)

    def set_appid(self, appid: str):
        self._appid = appid

    def set_license_file(self, license_file: Path):
        self._license_file = license_file

    def build(self, output_path: Path):
        """
        Build the installer based on the current state.
        :param output_path: The path of the installer output.
        :return:
        """

        output_path.mkdir(parents=True, exist_ok=True)

        inno_setup_path = 'C:\\Program Files (x86)\\Inno Setup 6\\ISCC.exe'

        # Create installer package
        subprocess.run([inno_setup_path, self._build_path / 'generated.iss', '/O' + str(output_path)], check=True)
