#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fetch checksummed dependencies into this project's private deps directory."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tarfile
import tempfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def fetch(name, spec, cached):
    parent = ROOT / 'deps'
    parent.mkdir(exist_ok=True)
    destination = parent / spec['destination']
    if destination.exists():
        marker = destination / '.source-lock.json'
        if marker.exists() and json.loads(marker.read_text()).get('sha256') == spec['sha256']:
            print(f'{name}: already prepared')
            return
        raise RuntimeError(f'{destination} exists without the expected lock; refusing to overwrite source')
    with tempfile.TemporaryDirectory(prefix=f'.fetch-{name}-', dir=parent) as temporary:
        temporary = Path(temporary)
        archive = Path(cached).resolve() if cached else temporary / 'download'
        if not cached:
            request = urllib.request.Request(spec['url'], headers={'User-Agent': 'PS3ME-source-setup'})
            with urllib.request.urlopen(request, timeout=60) as source, archive.open('wb') as target:
                shutil.copyfileobj(source, target, 1024 * 1024)
        if archive.stat().st_size != spec['bytes'] or digest(archive) != spec['sha256']:
            raise RuntimeError(f'{name}: checksum/size mismatch; nothing extracted')
        unpack = temporary / 'unpacked'
        unpack.mkdir()
        if spec['archive'] == 'zip':
            with zipfile.ZipFile(archive) as source:
                for info in source.infolist():
                    target = (unpack / info.filename).resolve()
                    if not target.is_relative_to(unpack.resolve()):
                        raise RuntimeError('Unsafe archive member')
                source.extractall(unpack)
        else:
            if sys.version_info < (3, 12):
                raise RuntimeError('Python 3.12+ is required for safe SDK tar extraction')
            with tarfile.open(archive, 'r:gz') as source:
                source.extractall(unpack, filter='data')
        folders = list(unpack.iterdir())
        if len(folders) != 1 or not folders[0].is_dir():
            raise RuntimeError('Unexpected dependency archive layout')
        (folders[0] / '.source-lock.json').write_text(json.dumps(spec, indent=2) + '\n')
        folders[0].rename(destination)
    print(f'{name}: prepared {destination}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--component', choices=['all', 'phoneme', 'sdk'], default='all')
    parser.add_argument('--cached-archive', help='Use a previously downloaded archive for a single component')
    args = parser.parse_args()
    if args.cached_archive and args.component == 'all':
        parser.error('--cached-archive requires one explicit component')
    lock = json.loads((ROOT / 'dependencies.lock.json').read_text())
    try:
        for name in (lock if args.component == 'all' else [args.component]):
            fetch(name, lock[name], args.cached_archive)
    except (OSError, RuntimeError, ValueError, tarfile.TarError, zipfile.BadZipFile) as error:
        print(f'Setup failed: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
