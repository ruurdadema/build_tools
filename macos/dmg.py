# Copyright (c) 2025 Owllab
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

import os
import platform
import shutil
import subprocess
import tempfile
from pathlib import Path

debug = False


class DMGBuilder:
    """
    A class which can build a DMG containing one or more items.

    Typical usage:

        builder = DMGBuilder("MyApp")
        builder.add_app_bundle(Path("MyApp.app"))
        builder.add_applications_link()
        builder.build(Path("dist/MyApp.dmg"))
    """

    class Item:
        """
        Represents an item inside the DMG volume.
        """

        def __init__(self, source: Path | None, destination: Path, symlink_target: str | None = None) -> None:
            """
            :param source:       Path to the source file/folder on disk, or None for pure symlinks.
            :param destination:  Destination path relative to the root of the DMG volume.
            :param symlink_target: If not None, create a symlink at `destination` pointing to this target.
            """
            self.source = source
            self.destination = destination
            self.symlink_target = symlink_target

    def __init__(self, volume_name: str):
        """
        :param volume_name: Name of the mounted DMG volume users will see.
        """
        self._volume_name = volume_name
        self._items: list[DMGBuilder.Item] = []

    def add_app_bundle(self, app_bundle: Path, destination: Path | None = None):
        """
        Add an .app bundle to the DMG.

        :param app_bundle:   Path to the .app bundle.
        :param destination:  Relative destination inside the DMG
                             (defaults to app_bundle.name at the root).
        """
        app_bundle = app_bundle.resolve()
        if not app_bundle.exists():
            raise FileNotFoundError(f"App bundle not found: {app_bundle}")

        if destination is None:
            destination = Path(app_bundle.name)

        self._items.append(DMGBuilder.Item(app_bundle, destination))

    def add_file(self, source: Path, destination: Path):
        """
        Add a generic file or folder to the DMG.

        :param source:      Path to the file/folder on disk.
        :param destination: Relative destination inside the DMG.
        """
        source = source.resolve()
        if not source.exists():
            raise FileNotFoundError(f"Source not found: {source}")

        self._items.append(DMGBuilder.Item(source, destination))

    def add_symlink(self, target: str, destination: Path):
        """
        Add a symlink to the DMG.

        :param target:      Target of the symlink (e.g. '/Applications').
        :param destination: Relative path where the symlink will be created.
        """
        self._items.append(DMGBuilder.Item(None, destination, symlink_target=target))

    def add_applications_link(self, name: str = "Applications"):
        """
        Convenience helper to add an /Applications symlink to the DMG root.
        """
        self.add_symlink("/Applications", Path(name))

    def build(
            self,
            output_path: Path,
            dmg_format: str = "UDZO",
            overwrite: bool = True
    ):
        """
        Build the DMG based on the current state.

        :param output_path:  Path of the DMG output file.
        :param dmg_format:   hdiutil format (defaults to 'UDZO' compressed read-only).
        :param overwrite:    Whether to overwrite an existing DMG.
        """
        if platform.system() != "Darwin":
            raise RuntimeError("DMGBuilder can only be used on macOS (Darwin).")

        output_path = output_path.resolve()
        output_path.parent.mkdir(parents=True, exist_ok=True)

        if output_path.exists():
            if overwrite:
                output_path.unlink()
            else:
                raise FileExistsError(f"Output DMG already exists: {output_path}")

        with tempfile.TemporaryDirectory() as tmpdir:
            # Staging directory that will become the root of the DMG volume
            volume_root = Path(tmpdir) / self._volume_name
            volume_root.mkdir(parents=True, exist_ok=True)

            # Copy all items into the staging folder
            for item in self._items:
                dest_path = volume_root / item.destination
                dest_path.parent.mkdir(parents=True, exist_ok=True)

                if item.symlink_target is not None:
                    # Create a symlink pointing at item.symlink_target
                    if debug:
                        print(f"Creating symlink: {dest_path} -> {item.symlink_target}")
                    os.symlink(item.symlink_target, dest_path)
                else:
                    if item.source.is_dir():
                        if debug:
                            print(f"Copying directory: {item.source} -> {dest_path}")
                        if dest_path.exists():
                            shutil.rmtree(dest_path)
                        shutil.copytree(item.source, dest_path)
                    else:
                        if debug:
                            print(f"Copying file: {item.source} -> {dest_path}")
                        shutil.copy2(item.source, dest_path)

            # Build DMG using hdiutil
            cmd = [
                "hdiutil", "create",
                "-volname", self._volume_name,
                "-srcfolder", str(volume_root),
                "-format", dmg_format,
            ]
            if overwrite:
                cmd.append("-ov")

            cmd.append(str(output_path))

            if debug:
                print("Running:", " ".join(str(c) for c in cmd))

            subprocess.run(cmd, check=True)

        return output_path
