#!/usr/bin/env python3 -u
import argparse
import subprocess
import re


def find_codesigning_developer_id_application_identities():
    lines = subprocess.check_output(['security', 'find-identity', '-p', 'codesigning', '-v']).splitlines()
    ids = []

    for line in lines:
        match = re.search(r'.+\d+\) (\w+) "Developer ID Application: (.+) \((.+)\)"', line.decode())
        if match:
            ids.append(match.groups())

    return ids


def print_codesigning_identities():
    subprocess.call(['security', 'find-identity', '-p', 'codesigning', '-v'])
