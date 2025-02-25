#!/usr/bin/env python3 -u
import multiprocessing
import platform
from datetime import datetime
from shutil import copytree, copy2

import pygit2
import boto3

from ravennakit.submodules.build_tools.cmake import *
from ravennakit.submodules.build_tools.macos.codesigning import *
from ravennakit.submodules.build_tools.macos.productbuilder import ProductBuilder
from ravennakit.submodules.build_tools.macos.universal import *
from ravennakit.submodules.build_tools.windows.innosetup import InnoSetup
from ravennakit.submodules.build_tools.windows.signtool import signtool_get_sign_command, signtool_verify

app_target_name = 'ravennakit_juce_demo'
app_artefacts_dir = f'{app_target_name}_artefacts'
app_name = 'RAVENNAKIT JUCE Demo'

# Script location matters, cwd does not
script_path = Path(__file__)
script_dir = script_path.parent

# Git version
repo = pygit2.Repository(path='.')
git_version = repo.describe(pattern='v*')
git_branch = repo.head.shorthand

# Installer
installer_publisher_name = 'Sound on Digital'
installer_publisher_url = 'https://soundondigital.com'
installer_app_id = '57dab4e4-3198-4637-94f7-e7eab5d85af8'


def upload_to_spaces(args, file: Path):
    session = boto3.session.Session()

    key = args.spaces_key
    secret = args.spaces_secret

    if not key:
        raise Exception('Need spaces key')

    if not secret:
        raise Exception('Need spaces secret')

    client = session.client('s3',
                            endpoint_url="https://ams3.digitaloceanspaces.com",
                            region_name="ams3",
                            aws_access_key_id=key,
                            aws_secret_access_key=secret)

    folder = 'branches/' + git_branch

    if git_branch == 'HEAD':
        # If we're in head, we're most likely building from a tag in which case we want to archive the artifacts
        folder = 'archive/' + git_version

    bucket = 'ravennakit-juce-demo'
    file_name = folder + '/' + file.name
    client.upload_file(str(file), bucket, file_name)

    print("Uploaded artefacts to {}/{}".format(bucket, file_name))


def pack_and_sign_macos(args, path_to_build: Path, build_config: Config):
    path_to_build_archive = Path(args.path_to_build).joinpath('archive')

    def copy_into_archive(file: Path, sub_folder: Path = Path()):
        dst = path_to_build_archive / sub_folder / file.name
        if dst.exists():
            shutil.rmtree(dst)
        shutil.copytree(file, dst, symlinks=True, dirs_exist_ok=True)

    def process_bundle(artefacts_dir: Path, bundle: Path):
        full_path_to_bundle = path_to_build / artefacts_dir / bundle
        sign_file(full_path_to_bundle, args.macos_developer_id_application,
                  entitlements_file=script_dir / 'plist' / 'entitlements.plist')

        return full_path_to_bundle

    # RAVENNAKIT JUCE Demo
    path_to_app = process_bundle(Path(app_artefacts_dir, str(build_config.value)), Path(f'{app_name}.app'))
    copy_into_archive(path_to_build / app_artefacts_dir)

    # Create installer package
    builder = ProductBuilder(title=app_name)

    builder.add_component(path_to_app,
                          install_location=Path('/Applications'),
                          identifier=args.macos_notarization_bundle_id,
                          title=app_name)

    path_to_installer = path_to_build_archive / 'ravennakit-juce-demo-{}.pkg'.format(git_version)
    builder.build(output_path=path_to_installer, developer_id_installer=args.macos_developer_id_installer)

    # Notarize installer package
    if args.notarize:
        notarize_file(args.macos_notarization_username, args.macos_notarization_password, args.macos_development_team,
                      path_to_installer)

    # Create ZIP from archive
    archive_path = args.path_to_build + '/ravennakit-juce-demo-' + git_version + '-' + args.build_number + '-macos'
    zip_path = Path(archive_path + '.zip')
    zip_path.unlink(missing_ok=True)

    shutil.make_archive(archive_path, 'zip', path_to_build_archive)

    return zip_path


def get_signtool_command(args):
    return ' '.join(signtool_get_sign_command(args.windows_code_sign_identity))


def pack_windows(args, path_to_build_x64: Path, build_config: Config):
    path_to_build_archive = Path(args.path_to_build).joinpath('archive')

    def copy_into_archive(file: Path, sub_folder: Path = Path()):
        if file.is_dir():
            shutil.copytree(file, path_to_build_archive / sub_folder / file.name, symlinks=True, dirs_exist_ok=True)
        else:
            os.makedirs(path_to_build_archive / sub_folder, exist_ok=True)
            shutil.copy(file, path_to_build_archive / sub_folder, follow_symlinks=True)

    # RAVENNAKIT JUCE Demo application
    path_to_desktop_receiver_app = path_to_build_x64 / app_artefacts_dir / str(build_config.value) / f'{app_name}.exe'
    copy_into_archive(path_to_build_x64 / app_artefacts_dir)

    # Create installer package
    innosetup = InnoSetup(appname=app_name, appversion=git_version, app_publisher=installer_publisher_name)
    innosetup.set_appid(installer_app_id)
    innosetup.set_app_publisher_url(installer_publisher_url)
    innosetup.set_allow_change_destination(False)

    if args.sign:
        innosetup.set_signtool_command(get_signtool_command(args))

    # Desktop sender app
    desktop_send = InnoSetup.File(path_to_desktop_receiver_app)
    desktop_send.set_add_to_start_menu(True)
    desktop_send.set_add_to_desktop(True)
    desktop_send.set_run_after_install(True)
    innosetup.add_file(desktop_send)

    installer_file_name = 'ravennakit-juce-demo-{}'.format(git_version).replace(' ', '-')
    innosetup.generate(path_to_build_archive / 'innosetup', installer_file_name)
    innosetup.build(path_to_build_archive)

    if args.sign:
        signtool_verify(path_to_build_archive / (installer_file_name + '.exe'))

    # Create ZIP from archive
    archive_path = args.path_to_build + '/ravennakit-jude-demo-' + git_version + '-' + args.build_number + '-windows'
    zip_path = Path(archive_path + '.zip')
    zip_path.unlink(missing_ok=True)

    shutil.make_archive(archive_path, 'zip', path_to_build_archive)

    return zip_path


def build_macos(args, build_config: Config):
    path_to_build = Path(args.path_to_build) / 'macos-universal'

    if args.skip_build:
        return path_to_build

    path_to_build.mkdir(parents=True, exist_ok=True)

    cmake = CMake()
    cmake.path_to_build(path_to_build)
    cmake.path_to_source(script_dir)
    cmake.build_config(build_config)
    cmake.generator('Xcode')
    cmake.parallel(multiprocessing.cpu_count())

    cmake.option('CMAKE_TOOLCHAIN_FILE', 'ravennakit/submodules/vcpkg/scripts/buildsystems/vcpkg.cmake')
    cmake.option('VCPKG_OVERLAY_TRIPLETS', 'ravennakit/triplets')
    cmake.option('VCPKG_TARGET_TRIPLET', 'macos-universal')
    cmake.option('CMAKE_OSX_ARCHITECTURES', 'x86_64;arm64')
    cmake.option('CMAKE_OSX_DEPLOYMENT_TARGET', args.macos_deployment_target)
    cmake.option('CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY', 'Apple Development')
    cmake.option('CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM', args.macos_development_team)
    cmake.option('BUILD_NUMBER', args.build_number)
    cmake.option('RAV_ENABLE_SPDLOG', 'ON')

    cmake.configure()
    cmake.build()

    return path_to_build


def build_windows(args, arch, build_config: Config):
    path_to_build = Path(args.path_to_build) / ('windows-' + arch)

    if args.skip_build:
        return path_to_build

    path_to_build.mkdir(parents=True, exist_ok=True)

    cmake = CMake()
    cmake.path_to_build(path_to_build)
    cmake.path_to_source(script_dir)
    cmake.build_config(build_config)
    cmake.architecture(arch)
    cmake.parallel(multiprocessing.cpu_count())

    cmake.option('CMAKE_TOOLCHAIN_FILE', 'ravennakit/submodules/vcpkg/scripts/buildsystems/vcpkg.cmake')
    cmake.option('VCPKG_OVERLAY_TRIPLETS', 'ravennakit/triplets')
    cmake.option('VCPKG_TARGET_TRIPLET', 'windows-' + arch)
    cmake.option('BUILD_NUMBER', args.build_number)
    cmake.option('RAV_WINDOWS_VERSION', args.windows_version)
    cmake.option('RAV_ENABLE_SPDLOG', 'ON')

    cmake.configure()
    cmake.build()

    return path_to_build


def build_dist(args):
    path_to_dist = Path(args.path_to_build) / 'dist'
    path_to_dist.mkdir(parents=True, exist_ok=True)

    # Manually choose the files to copy to prevent accidental leaking of files when the repo changes or is not clean.

    copy2('README.md', path_to_dist)
    copy2('.clang-format', path_to_dist)
    copy2('.gitignore', path_to_dist)
    copy2('CMakeLists.txt', path_to_dist)
    copytree('source', path_to_dist / 'source', dirs_exist_ok=True)
    (path_to_dist / 'ravennakit').mkdir(exist_ok=True)
    copy2('templates/RAVENNAKIT-README.md', path_to_dist / 'ravennakit' / 'README.md', )

    ravennakit_repo = pygit2.Repository(path='ravennakit')
    ravennakit_version = ravennakit_repo.describe(pattern='v*')

    version_data = {
        "juce_demo_version": git_version,
        "ravennakit_version": ravennakit_version,
        "build_number": args.build_number,
        "date": str(datetime.now())
    }

    with open(path_to_dist / 'version.json', 'w') as file:
        json.dump(version_data, file, indent=4)

    # Create ZIP from archive
    archive_path = args.path_to_build + '/ravennakit-jude-demo-' + git_version + '-' + args.build_number + '-dist'
    zip_path = Path(archive_path + '.zip')
    zip_path.unlink(missing_ok=True)

    shutil.make_archive(archive_path, 'zip', path_to_dist)

    return zip_path


def build(args):
    build_config = Config.debug if args.debug else Config.release_with_debug_info

    archive = None

    if args.dist:
        archive = build_dist(args)
    elif platform.system() == 'Darwin':
        path_to_build = build_macos(args, build_config)
        archive = pack_and_sign_macos(args, path_to_build, build_config)
    elif platform.system() == 'Windows':
        path_to_build_x64 = build_windows(args, 'x64', build_config)
        archive = pack_windows(args, path_to_build_x64, build_config)

    if archive and args.upload:
        upload_to_spaces(args, archive)


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("--debug",
                        help="Enable debug builds",
                        action='store_true')

    parser.add_argument("--path-to-build",
                        help="The folder to build the project in",
                        default="build")

    parser.add_argument("--dist",
                        help="Prepare the source code for distribution",
                        action='store_true')

    parser.add_argument("--build-number",
                        help="Specifies the build number",
                        default="0")

    parser.add_argument("--skip-build",
                        help="Skip building",
                        action='store_true')

    parser.add_argument("--upload",
                        help="Upload the archive to spaces",
                        action='store_true')

    parser.add_argument("--spaces-key",
                        help="Specify the key for uploading to spaces")

    parser.add_argument("--spaces-secret",
                        help="Specify the secret for uploading to spaces")

    if platform.system() == 'Darwin':
        parser.add_argument("--notarize",
                            help="Notarize packages",
                            action="store_true")

        parser.add_argument("--macos-deployment-target",
                            help="Specify the minimum macOS deployment target (macOS only)",
                            default="10.15")

        parser.add_argument("--macos-developer-id-application",
                            help="Specify the developer id application identity (macOS only)",
                            default="Developer ID Application: Owllab (3Y6DLW6UXM)")

        parser.add_argument("--macos-developer-id-installer",
                            help="Specify the developer id installer identity (macOS only)",
                            default="Developer ID Installer: Owllab (3Y6DLW6UXM)")

        parser.add_argument("--macos-development-team",
                            help="Specify the Apple development team (macOS only)",
                            default="3Y6DLW6UXM")  # Owllab

        parser.add_argument("--macos-notarization-bundle-id",
                            help="Specify the bundle id for notarization (macOS only)",
                            default="com.soundondigital.ravennakit-juce-demo")

        parser.add_argument("--macos-notarization-username",
                            help="Specify the username for notarization (macOS only)",
                            default="ruurd@owllab.nl")

        parser.add_argument("--macos-notarization-password",
                            help="Specify the username for notarization (macOS only)")

    elif platform.system() == 'Windows':
        parser.add_argument("--sign",
                            help="Sign the installer",
                            action="store_true")

        parser.add_argument("--windows-code-sign-identity",
                            help="Specify the code signing identity (Windows only)",
                            default="431e889eeb203c2db5dd01da91d56186b20d1880")  # GlobalSign cert

        parser.add_argument("--windows-version",
                            help="Specify the minimum supported version of Windows",
                            default="0x0A00")  # Windows 10

    build(parser.parse_args())


if __name__ == '__main__':
    print("Invoke {} as script. Script dir: {}".format(script_path, script_dir))
    main()
