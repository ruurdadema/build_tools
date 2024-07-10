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
import tempfile
from pathlib import Path

import xml.etree.ElementTree as ElementTree

debug = False


class ProductBuilder:
    """
    A class which can build an installer
    """

    class Component:
        """
        Represents a component in an installer
        """

        def __init__(self, path: Path, install_location: Path, identifier: str, title: str,
                     must_close: [str], scripts: Path):

            if path and not install_location:
                raise Exception('Need install_location when specifying path')

            self.path = path
            self.install_location = install_location
            self.identifier = identifier
            self.title = title
            self.must_close = must_close
            self.tmp_file = tempfile.NamedTemporaryFile()
            self.scripts = scripts.resolve() if scripts else None

    def __init__(self, title):
        self._title = title
        self._license = None
        self._components = []

    def add_component(self, path_to_bundle: Path, install_location: Path, identifier: str, title: str,
                      must_close=None, scripts: Path = None):
        """
        Adds a component to the installer
        :param path_to_bundle: The path to the app bundle. Can be None in which case the --nopayload flag will be added. This requires scripts to be a valid path.
        :param install_location: The local to install the bundle to.
        :param identifier: The identifier of the component.
        :param title: The title of the component.
        :param must_close: A list of application bundle ids that must be closed before installing this component.
        :param scripts: Path to a directory containing scripts which should be added to the installer.
        :return:
        """
        if must_close is None:
            must_close = []
        self._components.append(
            ProductBuilder.Component(path_to_bundle, install_location, identifier, title, must_close, scripts))

    def add_license(self, path: Path):
        """
        Adds a license to the installer
        :param path: The path to the license file.
        :return:
        """
        self._license = path.resolve()

    def build(self, output_path: Path, developer_id_installer: str):
        """
        Build the installer based on the current state.
        :param output_path: The path of the installer output.
        :param developer_id_installer: The developer id to use for signing the installer
        :return:
        """

        # Documentation: https://developer.apple.com/library/archive/documentation/DeveloperTools/Reference/DistributionDefinitionRef/Chapters/Introduction.html#//apple_ref/doc/uid/TP40005370-CH1-DontLinkElementID_16

        # To debug installation: sudo installer -pkg Installer.pkg -target / -dumplog -verbose
        # Or tail /var/log/install.log

        # Note: BundleIsRelocatable is set to true by default which might result in the installer updating the same
        # package in another location than specified by install_location.
        # See https://apple.stackexchange.com/a/219144/324251 for more details.

        # Build the components
        for component in self._components:
            cmd = ['pkgbuild', '--identifier', component.identifier]

            if component.path:
                cmd += ['--component', component.path]
            else:
                cmd += ['--nopayload']

            if component.install_location:
                cmd += ['--install-location', component.install_location]

            if component.scripts:
                cmd += ['--scripts', component.scripts]

            cmd += [component.tmp_file.name]

            subprocess.run(cmd, check=True)

        installer_gui_script = ElementTree.Element('installer-gui-script', {'minSpecVersion': '1'})

        # Title
        title = ElementTree.Element('title')
        title.text = self._title
        installer_gui_script.append(title)

        # License
        if self._license is not None:
            license_file = ElementTree.Element('license', {'file': str(self._license)})
            installer_gui_script.append(license_file)

        installer_gui_script_options = ElementTree.Element('options')
        installer_gui_script_options.set('customize', 'always')
        installer_gui_script_options.set('require-scripts', 'false')
        installer_gui_script_options.set('hostArchitectures', 'x86_64,arm64')

        installer_gui_script.append(installer_gui_script_options)

        # Choice outline
        installer_gui_script_choices_outline = ElementTree.Element('choices-outline')

        for component in self._components:
            installer_gui_script_choices_outline_choice = ElementTree.Element('line')
            installer_gui_script_choices_outline_choice.set('choice', component.identifier)
            installer_gui_script_choices_outline.append(installer_gui_script_choices_outline_choice)

        installer_gui_script.append(installer_gui_script_choices_outline)

        # Choices
        for component in self._components:
            choice = ElementTree.Element('choice', {'id': component.identifier})
            choice.set('title', component.title)
            choice.set('visible', 'true')

            pkg_ref = ElementTree.Element('pkg-ref', {'id': component.identifier})
            choice.append(pkg_ref)

            installer_gui_script.append(choice)

        # pkg refs
        for component in self._components:
            pkg_ref = ElementTree.Element('pkg-ref', {'id': component.identifier})
            pkg_ref.set('version', '0')
            pkg_ref.set('onConclusion', 'none')
            pkg_ref.text = component.tmp_file.name

            installer_gui_script.append(pkg_ref)

        # Must close
        for component in self._components:
            if component.must_close:
                pkg_ref = ElementTree.Element('pkg-ref', {'id': component.identifier})
                must_close = ElementTree.Element('must-close')
                for identifier in component.must_close:
                    must_close.append(ElementTree.Element('app', {'id': identifier}))
                pkg_ref.append(must_close)

                installer_gui_script.append(pkg_ref)

        # Make the XML pretty
        ElementTree.indent(installer_gui_script)

        distribution_tmp_file = tempfile.NamedTemporaryFile()
        tree = ElementTree.ElementTree(installer_gui_script)
        tree.write(distribution_tmp_file.name, encoding="UTF-8", xml_declaration=True)

        if debug:
            with open(distribution_tmp_file.name, 'r') as f:
                print(f.read())

        cmd = ['productbuild']

        for component in self._components:
            cmd += ['--package-path', Path(component.tmp_file.name).parent]

        cmd += ['--distribution', distribution_tmp_file.name]
        cmd += ['--sign', developer_id_installer]
        cmd += ['--timestamp']
        cmd += [str(output_path)]

        output_path.parent.mkdir(parents=True, exist_ok=True)

        subprocess.run(cmd, check=True)
