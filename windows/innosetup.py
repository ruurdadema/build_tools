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

    class File:
        def __init__(self, source: Path):
            """
            Initialise this File instance
            :param source: The source file to represent.
            """
            self.source = source
            self.excludes = ['*.pdb', '*.ilk']
            self.type = type
            self.destination = None

        def set_custom_destination(self, destination: Path):
            """
            Set custom destination for this file.
            :param destination: The destination to place this file into.
            """
            self.destination = destination

        def append_excludes(self, excludes: [str]):
            """
            Append exclude (glob) patterns to not include in the installer.
            :param excludes: The excludes patterns to append (as list of strings).
            """
            self.excludes.extend(excludes)

        def set_excludes(self, excludes: [str]):
            """
            Sets the exclude patterns (overwriting existing ones). To append see append_excludes.
            :param excludes: Excludes patterns to set.
            :return:
            """
            self.excludes = excludes

    def __init__(self, appname: str, app_publisher: str, appversion: str):
        """
        Initialises this InnoSetup instance
        :param appname: The name of the app.
        :param app_publisher: The publisher of the app.
        :param appversion: The version of the app.
        """
        self._set_allowed_to_change_destination = True
        self._app_updates_url = None
        self._app_support_url = None
        self._app_publisher_url = None
        self._license_file = None
        self._build_path = Path()
        self._installer_file_name = None
        self._appid = None
        self._appname = appname
        self._appversion = appversion
        self._app_publisher = app_publisher
        self._generated_iss_file = None
        self._run = []
        self._files = []
        self._install_delete = []
        self._signtool_command = None

    def set_appid(self, appid: str):
        """
        Sets the app id of this installer. Used to relate to previous installations of previous versions.
        :param appid: The app id.
        """
        self._appid = appid

    def set_app_publisher_url(self, url: str):
        """
        Sets the url of the app publisher.
        :param url: The url to set.
        """
        self._app_publisher_url = url

    def set_app_support_url(self, url: str):
        """
        Sets the support url.
        :param url: The url to set.
        """
        self._app_support_url = url

    def set_app_updates_url(self, url: str):
        """
        Sets the updates url.
        :param url: The url to set.
        """
        self._app_updates_url = url

    def set_license_file(self, license_file: Path):
        """
        Sets an optional license file.
        :param license_file: The license file to use.
        """
        self._license_file = license_file

    def set_allow_change_destination(self, should_be_enabled: bool):
        """
        When true users are allowed to change the destination of the software.
        :param should_be_enabled: True to allow, of false to not allow.
        """
        self._set_allowed_to_change_destination = should_be_enabled

    def set_signtool_command(self, command: str):
        """
        Sets the signtool command to use to sign the installer and uninstaller
        :param command: The command to use for signing.
        """
        self._signtool_command = command

    def add_file(self, file: File):
        """
        Add given file to the installer.
        :param file: The file to add.
        """
        self._files.append(file)

    def generate(self, generate_path: Path, installer_file_name: str):
        """
        Generates a .iss Inno Setup file with current configuration.
        :param generate_path: The path in where to generate the file.
        :param installer_file_name: The name of the installer to generate.
        """
        self._build_path = generate_path
        self._build_path.mkdir(parents=True, exist_ok=True)
        self._installer_file_name = installer_file_name

        script = ''
        script += '[Setup]\n'

        if self._appid:
            script += '; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.\n'
            script += '; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)\n'
            script += 'AppId={{{{{}}}\n'.format(self._appid)

        # Signtool
        if self._signtool_command:
            script += 'SignTool=signtool $f\n'
            script += 'SignedUninstaller=yes\n'

        # AppName
        script += 'AppName={}\n'.format(self._appname)

        # AppVersion
        script += 'AppVersion={}\n'.format(self._appversion)

        # AppPublisher
        script += 'AppPublisher={}\n'.format(self._app_publisher)

        # URLs
        if self._app_publisher_url:
            script += 'AppPublisherURL={}\n'.format(self._app_publisher_url)

        if self._app_support_url:
            script += 'AppSupportURL={}\n'.format(self._app_support_url)

        if self._app_updates_url:
            script += 'AppUpdatesURL={}\n'.format(self._app_updates_url)

        if not self._has_at_least_one_file_with_default_destination():
            script += 'CreateAppDir=no\n'

        # DefaultDirName
        script += 'DefaultDirName={{autopf64}}\\{}\\{}\n'.format(self._app_publisher, self._appname)

        # DefaultGroupName
        script += 'DefaultGroupName={}\n'.format(self._appname)

        # OutputBaseFilename
        script += 'OutputBaseFilename={}\n'.format(installer_file_name)

        script += 'DisableDirPage={}\n'.format('auto' if self._set_allowed_to_change_destination else 'yes')

        # Compression
        script += 'Compression=lzma\n'
        script += 'SolidCompression=yes\n'

        # Style
        script += 'WizardStyle=modern\n'

        if self._license_file:
            script += 'LicenseFile={}\n'.format(self._license_file)

        script += '\n'

        # Languages
        script += '[Languages]\n'
        script += 'Name: "english"; MessagesFile: "compiler:Default.isl"\n'

        script += '\n'

        # Files
        script += '[Files]\n'

        for file in self._files:
            script += _generate_file_entry(file) + '\n'
            if file.destination:
                self._install_delete.append(file.destination / file.source.name)

        if self._install_delete:
            script += '\n'
            script += '[InstallDelete]\n'

            # InstallDelete
            for e in self._install_delete:
                script += 'Type: filesandordirs; Name: "{}"\n'.format(e)

        # Generate the iss file
        generated_iss_file = self._build_path / Path(self._appname).with_suffix('.iss')

        with open(generated_iss_file, 'w') as f:
            f.write(script)

        self._generated_iss_file = generated_iss_file

        return self

    def build(self, build_path: Path):
        """
        Build the installer based on the generated .iss Inno Setup file.
        :param build_path: The path of the installer output.
        """

        build_path.mkdir(parents=True, exist_ok=True)

        inno_setup_path = 'C:\\Program Files (x86)\\Inno Setup 6\\ISCC.exe'

        cmd = [inno_setup_path, self._generated_iss_file, '/O' + str(build_path)]
        cmd += ['/Ssigntool={} $f'.format(self._signtool_command)]

        # Create installer package
        subprocess.run(cmd, check=True)

        return build_path / (self._installer_file_name + '.exe')

    def _has_at_least_one_file_with_default_destination(self):
        """
        :return: True if there is at least one file with a default (None) destination.
        """
        for file in self._files:
            if not file.destination:
                return True
        return False


def _generate_file_entry(file: InnoSetup.File):
    """
    :param file: The file to generate the entry from.
    :return: A file entry as string which goes into the .iss file under the [Files] section.
    """
    if not file.source.exists():
        raise Exception('Given file does not exist')

    destination = file.destination if file.destination else "{app}"

    if file.source.is_file():
        return 'Source: "{}"; Excludes: "{}"; DestDir: "{}"; Flags: ignoreversion'.format(
            file.source, ','.join(file.excludes), destination)
    elif file.source.is_dir():
        return 'Source: "{}\*"; Excludes: "{}"; DestDir: "{}\\{}"; Flags: ignoreversion recursesubdirs createallsubdirs'.format(
            file.source, ','.join(file.excludes), destination, file.source.name)
