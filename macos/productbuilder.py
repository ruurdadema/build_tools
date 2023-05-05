import subprocess
import tempfile
from pathlib import Path

import xml.etree.ElementTree as ElementTree


class ProductBuilder:
    """
    A class which can build an installer
    """

    class Component:
        """
        Represents a component in an installer
        """

        def __init__(self, path: Path, install_location: Path, identifier: str, title: str, must_close: [str]):
            self.path = path
            self.install_location = install_location
            self.identifier = identifier
            self.title = title
            self.must_close = must_close
            self.tmp_file = tempfile.NamedTemporaryFile()

    def __init__(self, title=None):
        self._title = title
        self._components = []

    def add_component(self, path: Path, install_location: Path, identifier: str, title: str, must_close: [str] = []):
        """
        Adds a component to the installer
        :param path: The path to the app bundle.
        :param install_location: The local to install the bundle to.
        :param identifier: The identifier of the component.
        :param title: The title of the component.
        :param must_close: A list of application bundle ids that must be closed before installing this component.
        :return:
        """
        self._components.append(ProductBuilder.Component(path, install_location, identifier, title, must_close))
        print(self._components[-1])

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

        installer_gui_script = ElementTree.Element('installer-gui-script', {'minSpecVersion': '1'})

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
            choice.set('selected', 'true')

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

        with open(distribution_tmp_file.name, 'r') as f:
            print(f.read())

        cmd = ['productbuild']

        for component in self._components:
            cmd += ['--package-path', Path(component.tmp_file.name).parent]

        cmd += ['--distribution', distribution_tmp_file.name]
        cmd += ['--sign', developer_id_installer]
        cmd += ['--timestamp']
        cmd += [str(output_path)]

        subprocess.run(cmd, check=True)
