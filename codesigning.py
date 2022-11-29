#!/usr/bin/env python3 -u

# MIT License
#
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

import argparse
import json
import subprocess
import re
import sys
import time
import requests
from pathlib import Path


def sign_file(file: Path, codesign_identity=None, development_team=None, organization=None, entitlements_file=None):
    """
    Signs given file.
    :param file: The file to sign.
    :param codesign_identity: (Optional) the codesign identity to use. If none give, one will be selected automatically.
    :param development_team: (Optional) the development team to use when searching a codesign identity.
    :param organization: (Optional) the organization to specify when searching a codesign identity.
    :param entitlements_file: An optional entitlements file used for signing
    """
    if codesign_identity is None:
        result = get_codesigning_developer_id_application_identities(team=development_team,
                                                                     organization=organization)[0]
        if result:
            print('Found codesign identity: {}'.format(result))

        codesign_identity = result[0]

    if codesign_identity is None:
        raise Exception('Failed to find a code signing identity')

    print('Signing file \"{}\" with identity \"{}\"'.format(file, codesign_identity))

    cmd = ['codesign']

    if entitlements_file:
        cmd += ['--entitlements', entitlements_file]

    cmd += ['--force', '--timestamp', '--options=runtime', '--verbose', '-s', codesign_identity, file]

    subprocess.run(cmd, check=True)


def verify_signature(file: Path, check_notarization=False):
    """
    Verifies signature and optionally notarization
    :param file: The file to check
    :param check_notarization: True to check notarization status
    :return: True if ok, or false if not ok.
    """

    if file.suffix == '.pkg':
        # Note: codesign does not work with .pkg files...
        subprocess.run(['pkgutil', '--check-signature', file], check=True)
    else:
        subprocess.run(['codesign', '-vv', '--deep', '--entitlements', '--strict', file], check=True)

    if check_notarization:
        subprocess.run(['spctl', '--assess', '-vvv', '-t', 'install', file], check=True)


def get_notarization_log_json(url):
    """
    Downloads the notarization log form given url.
    :param url: THe url to download from.
    :return: The log encoded as json.
    """
    print('Download log from: {}'.format(url))
    r = requests.get(url)
    return r.json()


def get_notarization_status(request_uuid, username, password, wait_time_seconds=30):
    """
    Gets the notarization status, and keeps trying until the package was approved or denied, or when an error occurred.
    :param request_uuid: The uuid for the request (Apple provided).
    :param username: The username for the request.
    :param password: The password for the request.
    :param wait_time_seconds: Amount of seconds to wait between each call.
    """
    while True:
        output = subprocess.check_output(['xcrun',
                                          'altool',
                                          '--notarization-info', request_uuid,
                                          '--username', username,
                                          '--password', password])

        print(output.decode())

        result = {}

        for line in output.decode().splitlines():
            split = line.split(': ')
            if len(split) > 1:
                result[split[0].strip()] = split[1].strip()
            elif len(split) > 0:
                result[split[0].strip()] = 'no_value'

        # Remove empty keys
        result = {k: v for k, v in result.items() if k}

        if not result['No errors getting notarization info.'] == 'no_value':
            raise Exception('Failed to get notarization status')

        if result['Status'] == 'in progress':
            time.sleep(wait_time_seconds)
        else:
            print('Notarization status Message: {}'.format(result['Status Message']))
            print(json.dumps(get_notarization_log_json(result['LogFileURL']), indent=4))
            if result['Status'] == 'invalid':
                raise Exception('Notarization failed')
            elif result['Status'] == 'success':
                break


def notarize_file(primary_bundle_id, username, password, team_id, file):
    """
    Notarizes given file (which ust be a container, pkg, dmg, zip).
    :param primary_bundle_id: The bundle ID to sign with.
    :param username: The username for signing.
    :param password: The password for signing.
    :param team_id: The team ID.
    :param file: The file to notarize.
    """
    print('\nSend notarization request for file: {}'.format(file))

    output = subprocess.check_output(['xcrun',
                                      'altool',
                                      '--notarize-app',
                                      '--primary-bundle-id', primary_bundle_id,
                                      '--username', username,
                                      '--password', password,
                                      '--asc-provider', team_id,
                                      '--file', file])

    m = re.match("(?P<status>.+) '(?P<file>.+)'.*\nRequestUUID = (?P<request_uuid>.+)\n", output.decode())

    if not m.group('status') == 'No errors uploading':
        raise Exception('Failed to upload notarization request')

    get_notarization_status(m.group('request_uuid'), username, password)

    print('Staple:')
    subprocess.run(['xcrun', 'stapler', 'staple', file], check=True)

    verify_signature(file, check_notarization=True)


def get_codesigning_developer_id_application_identities(organization=None, team=None):
    """
    Finds most optimal codesign identity available on the current system, based on optionally given parameters.
    :param organization: Specifies the organization of the codesign id.
    :param team: Specifies the team of the codesign id.
    :return: The codesign id, or None if no (matching) codesign id was found.
    """
    lines = subprocess.check_output(['security', 'find-identity', '-p', 'codesigning', '-v']).splitlines()
    ids = []

    for line in lines:
        match = re.search(r'.+\d+\) (\w+) "Developer ID Application: (.+) \((.+)\)"', line.decode())

        if match:
            # If an organization was specified, only add this identity if it matches this organization.
            if organization and organization != match.groups()[1]:
                continue

            # If team was specified, only add this identity if it matches this team.
            if team and team != match.groups()[2]:
                continue

            ids.append(match.groups())

    return ids


def print_codesigning_identities():
    """
    Prints the available code sign identities.
    """
    subprocess.run(['security', 'find-identity', '-p', 'codesigning', '-v'], check=True)
