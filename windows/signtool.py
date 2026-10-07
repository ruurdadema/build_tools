import glob
import shutil
import subprocess
import tempfile
from pathlib import Path
from uuid import uuid4


def signtool_get_path():
    """
    :return: Returns the path to the newest x64 signtool, if found, otherwise throws an exception.
    """
    results = glob.glob('C:\\Program Files (x86)\\Windows Kits\\10\\bin\\*\\x64\\signtool.exe')

    if not results:
        raise Exception('Failed to find signtool.exe')

    def version_key(path):
        version = Path(path).parent.parent.name  # e.g. 10.0.22621.0
        return tuple(int(p) if p.isdigit() else 0 for p in version.split('.'))

    return Path(max(results, key=version_key))


def signtool_get_artifact_signing_options(dlib: Path, metadata: Path, q='"'):
    """
    Returns the signtool options (without a file) for signing with Azure Artifact Signing.
    See artifact_signing.artifact_signing_setup() for obtaining the dlib and metadata.
    :param dlib: Path to Azure.CodeSigning.Dlib.dll (x64)
    :param metadata: Path to metadata.json
    :param q: The quote character to use around paths
    """
    return (
        f'sign'
        f' /v'
        f' /fd SHA256'
        f' /tr http://timestamp.acs.microsoft.com'
        f' /td SHA256'
        f' /dlib {q}{dlib}{q}'
        f' /dmdf {q}{metadata}{q}'
    )


def signtool_get_globalsign_token_options(cert_file: Path, container_name: str, password: str, cert_thumbprint: str,
                                          q='"'):
    """
    Returns the signtool options (without a file) for signing with a GlobalSign EV certificate on a SafeNet eToken,
    unlocking the token non-interactively.
    :param q: The quote character to use around values
    """
    return (
        f'sign'
        f' /f {q}{cert_file}{q}'
        f' /csp {q}eToken Base Cryptographic Provider{q}'
        f' /k {q}[{{{{{password}}}}}]={container_name}{q}'
        f' /tr http://timestamp.globalsign.com/tsa/r6advanced1'
        f' /td SHA256 /fd SHA256'
        f' /sha1 {cert_thumbprint}'
        f' /Debug'
    )


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
    print('Sign command: ' + ' '.join(cmd))
    subprocess.run(cmd, check=True)


def signtool_verify(file: Path):
    """
    Verifies the signing of given file.
    :param file:
    :return:
    """
    cmd = [str(signtool_get_path()), 'verify', '/v', '/pa', str(file)]
    print('Verify command: ' + ' '.join(cmd))
    subprocess.run(cmd, check=True)


def signtool_test(cert_thumbprint: str):
    """
    Tests if signing a dummy executable is possible with given cert thumbprint.
    """
    rundll32 = shutil.which('rundll32')  # Using rundll32 as dummy exe

    print('Using rundll32 as dummy exe: {}'.format(rundll32))

    tmp_dir = tempfile.gettempdir()
    tmp_file = Path(tmp_dir) / str(uuid4())

    print('Temp file: {}'.format(tmp_file))

    shutil.copy2(rundll32, tmp_file)

    try:
        signtool_sign(cert_thumbprint, tmp_file)
        signtool_verify(tmp_file)
    finally:
        print('Removing: {}'.format(tmp_file))
        tmp_file.unlink()

    print('Signing test succeeded')
