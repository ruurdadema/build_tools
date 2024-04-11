import glob
import subprocess
from pathlib import Path


def signtool_get_path():
    """
    :return: Returns the path to signtool, if found, otherwise throws an exception.
    """
    results = glob.glob('C:\\Program Files (x86)\\Windows Kits\\10\\bin\\*\\x64\\signtool.exe')

    if not results:
        raise Exception('Failed to find signtool.exe')

    return Path(results[0])


def signtool_get_sign_command(cert_thumbprint: str):
    """
    Returns a path to signtool without a file specified. Caller is responsible for adding a file as last parameter before invoking the command.
    :param cert_thumbprint: The thumbprint of the certificate to use
    :return: The constructed path
    """
    return [str(signtool_get_path()), 'sign',
            '/tr', 'http://timestamp.globalsign.com/tsa/r6advanced1',
            '/td', 'SHA256',
            '/fd', 'SHA256',
            '/sha1', cert_thumbprint,
            '/Debug']


def signtool_sign(cert_thumbprint: str, file: Path):
    """
    Signs given file
    :param cert_thumbprint: The thumbprint of the certificate to use
    :param file: The file to sign
    """
    cmd = signtool_get_sign_command(cert_thumbprint)
    cmd += [str(file)]
    subprocess.run(cmd, check=True)


def signtool_verify(file: Path):
    """
    Verifies the signing of given file.
    :param file:
    :return:
    """
    subprocess.run([signtool_get_path(), 'verify', '/v', '/pa', str(file)], check=True)

