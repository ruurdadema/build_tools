#!/usr/bin/env python3 -u
import argparse
import subprocess
import re


def sign_file(file, codesign_identity=None, development_team=None, organization=None):
    if codesign_identity is None:
        result = get_codesigning_developer_id_application_identities(team=development_team,
                                                                     organization=organization)[0]
        if result:
            print('Found codesign identity: {}'.format(result))

        codesign_identity = result[0]

    if codesign_identity is None:
        raise 'Failed to find a code signing identity'

    print('Signing file \"{}\" with identity \"{}\"'.format(file, codesign_identity))

    subprocess.call(['codesign',
                     '--force',
                     '--timestamp',
                     '--options=runtime',
                     '-s', codesign_identity,
                     file])

    subprocess.call(['codesign', '-vv', '-d', file])


def get_codesigning_developer_id_application_identities(organization=None, team=None):
    lines = subprocess.check_output(['security', 'find-identity', '-p', 'codesigning', '-v']).splitlines()
    ids = []

    for line in lines:
        match = re.search(r'.+\d+\) (\w+) "Developer ID Application: (.+) \((.+)\)"', line.decode())

        if match:
            # If an organization was specified, we only add this identity if it matches this organization.
            if organization and organization != match.groups()[1]:
                continue

            # If team was specified, we only add this identity if it matches this team.
            if team and team != match.groups()[2]:
                continue

            ids.append(match.groups())

    return ids


def print_codesigning_identities():
    subprocess.call(['security', 'find-identity', '-p', 'codesigning', '-v'])
