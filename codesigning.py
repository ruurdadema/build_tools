#!/usr/bin/env python3 -u
import argparse
import json
import subprocess
import re
import sys
import time
import requests


def call(cmd: []):
    print('\n' + ' '.join(cmd))
    subprocess.run(cmd, check=True)


def sign_file(file, codesign_identity=None, development_team=None, organization=None):
    if codesign_identity is None:
        result = get_codesigning_developer_id_application_identities(team=development_team,
                                                                     organization=organization)[0]
        if result:
            print('Found codesign identity: {}'.format(result))

        codesign_identity = result[0]

    if codesign_identity is None:
        raise Exception('Failed to find a code signing identity')

    print('Signing file \"{}\" with identity \"{}\"'.format(file, codesign_identity))

    subprocess.run(['codesign', '--force', '--timestamp', '--options=runtime', '-s', codesign_identity, file],
                   check=True)
    subprocess.run(['codesign', '-dvvv', file], check=True)


def get_notarization_log_json(url):
    print('Download log from: {}'.format(url))
    r = requests.get(url)
    return r.json()


def get_notarization_status(request_uuid, username, password, timeout_seconds=30):
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
            time.sleep(timeout_seconds)
        else:
            print('Notarization status Message: {}'.format(result['Status Message']))
            print(json.dumps(get_notarization_log_json(result['LogFileURL']), indent=4))
            if result['Status'] == 'invalid':
                raise Exception('Notarization failed')
            elif result['Status'] == 'success':
                break


def notarize_file(primary_bundle_id, username, password, team_id, file):
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

    call(['spctl', '-a', '-vvv', '-t', 'install', file])


def get_codesigning_developer_id_application_identities(organization=None, team=None):
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
    subprocess.run(['security', 'find-identity', '-p', 'codesigning', '-v'], check=True)
