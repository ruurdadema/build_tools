import glob
import json
import os
import urllib.request
import zipfile
from pathlib import Path

# NuGet package containing Azure.CodeSigning.Dlib.dll (formerly Microsoft.Trusted.Signing.Client)
ARTIFACT_SIGNING_CLIENT_PACKAGE = 'microsoft.artifactsigning.client'
ARTIFACT_SIGNING_CLIENT_VERSION = '1.0.128'


def artifact_signing_setup(path: Path, endpoint: str, account_name: str, certificate_profile: str,
                           dlib: Path = None):
    """
    Prepares the dlib and metadata.json needed by signtool for Azure Artifact Signing (formerly Azure Trusted Signing).
    Credentials are read by the dlib from the AZURE_TENANT_ID, AZURE_CLIENT_ID and AZURE_CLIENT_SECRET environment
    variables. Use signtool.signtool_get_artifact_signing_options() to get the signtool options.
    :param path: Folder to download the dlib into and to write metadata.json to.
    :param endpoint: Artifact Signing account endpoint, e.g. https://weu.codesigning.azure.net/
    :param account_name: Artifact Signing account name.
    :param certificate_profile: Certificate profile name.
    :param dlib: Optional path to Azure.CodeSigning.Dlib.dll (x64). Downloaded from NuGet when not given.
    :return: Tuple of (path to Azure.CodeSigning.Dlib.dll, path to metadata.json)
    """
    for name in ['AZURE_TENANT_ID', 'AZURE_CLIENT_ID', 'AZURE_CLIENT_SECRET']:
        if not os.getenv(name):
            raise Exception(f'Environment variable {name} is required for Azure Artifact Signing')

    for name, value in [('endpoint', endpoint),
                        ('account_name', account_name),
                        ('certificate_profile', certificate_profile)]:
        if not value:
            raise Exception(f'{name} is required for Azure Artifact Signing')

    # The dlib needs the .NET 8 (or newer) x64 runtime. Without it signtool exits silently with code 255.
    runtimes = glob.glob('C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\*')
    major_versions = [int(v.split('.')[0]) for v in (Path(r).name for r in runtimes) if v.split('.')[0].isdigit()]
    if not any(v >= 8 for v in major_versions):
        raise Exception('Azure Artifact Signing requires the .NET 8 (or newer) x64 runtime, found: '
                        + (', '.join(Path(r).name for r in runtimes) or 'none')
                        + '. Install it from https://dotnet.microsoft.com/download/dotnet/8.0')

    path = path.resolve()
    path.mkdir(parents=True, exist_ok=True)

    if dlib:
        dlib = Path(dlib)
    else:
        package = ARTIFACT_SIGNING_CLIENT_PACKAGE
        version = ARTIFACT_SIGNING_CLIENT_VERSION
        path_to_package = path / f'{package}.{version}'
        dlib = path_to_package / 'bin' / 'x64' / 'Azure.CodeSigning.Dlib.dll'

        if not dlib.exists():
            url = f'https://api.nuget.org/v3-flatcontainer/{package}/{version}/{package}.{version}.nupkg'
            nupkg = path / f'{package}.{version}.nupkg'
            print(f'Downloading {url}')
            urllib.request.urlretrieve(url, nupkg)
            with zipfile.ZipFile(nupkg) as z:
                z.extractall(path_to_package)

    if not dlib.exists():
        raise Exception(f'Azure dlib not found: {dlib}')

    metadata = path / 'metadata.json'
    with open(metadata, 'w') as f:
        json.dump({
            'Endpoint': endpoint,
            'CodeSigningAccountName': account_name,
            'CertificateProfileName': certificate_profile,
        }, f, indent=2)

    print(f'Azure Artifact Signing: dlib={dlib} metadata={metadata}')

    return dlib, metadata
