#!/usr/bin/env python3
"""Local static preview with negotiated gzip, including over SSH forwarding."""
import argparse
import functools
import gzip
import hashlib
import http.server
import pathlib
import shutil
import tempfile
import threading


def accepts_gzip(header):
    qualities = {}
    for item in header.lower().split(','):
        name, *params = item.strip().split(';')
        quality = 1.0
        for param in params:
            key, _, value = param.strip().partition('=')
            if key == 'q':
                try:
                    quality = float(value)
                except ValueError:
                    quality = 0.0
        qualities[name] = quality
    return qualities.get('gzip', qualities.get('*', 0)) > 0


class GzipCache:
    """Compress each file revision once; scratch never enters the served tree."""
    def __init__(self, directory):
        self.directory = pathlib.Path(directory)
        self.entries = {}
        self.lock = threading.Lock()

    def get(self, source, stat):
        identity = f'{source.resolve()}:{stat.st_mtime_ns}:{stat.st_size}'
        key = hashlib.sha256(identity.encode()).hexdigest()
        with self.lock:
            if key not in self.entries:
                target = self.directory / (key + '.gz')
                if stat.st_size >= 1024:
                    with source.open('rb') as inp, target.open('wb') as raw:
                        with gzip.GzipFile(filename='', mode='wb', fileobj=raw, compresslevel=6, mtime=0) as out:
                            shutil.copyfileobj(inp, out)
                    if target.stat().st_size >= stat.st_size:
                        target.unlink()
                self.entries[key] = target if target.exists() else None
            return self.entries[key]


class Handler(http.server.SimpleHTTPRequestHandler):
    def send_head(self):
        source = pathlib.Path(self.translate_path(self.path))
        if source.is_dir():
            # Preserve SimpleHTTPRequestHandler's trailing-slash redirect.
            if not self.path.split('?', 1)[0].endswith('/'):
                return super().send_head()
            for name in ('index.html', 'index.htm'):
                if (source / name).is_file():
                    source = source / name
                    break
        if not source.is_file():
            return super().send_head()
        try:
            stat = source.stat()
            compressed = self.server.gzip_cache.get(source, stat) if accepts_gzip(self.headers.get('Accept-Encoding', '')) else None
            selected = compressed or source
            stream = selected.open('rb')
        except OSError:
            self.send_error(404, 'File not found')
            return None
        identity = f'{source.resolve()}:{stat.st_mtime_ns}:{stat.st_size}:{bool(compressed)}'
        etag = '"' + hashlib.sha256(identity.encode()).hexdigest() + '"'
        validators = [v.strip().removeprefix('W/') for v in self.headers.get('If-None-Match', '').split(',')]
        not_modified = etag in validators or '*' in validators
        self.send_response(304 if not_modified else 200)
        self.send_header('Vary', 'Accept-Encoding')
        self.send_header('ETag', etag)
        self.send_header('Cache-Control', 'no-cache')
        self.send_header('Last-Modified', self.date_time_string(stat.st_mtime))
        if compressed:
            self.send_header('Content-Encoding', 'gzip')
        if not not_modified:
            self.send_header('Content-Type', self.guess_type(str(source)))
            self.send_header('Content-Length', str(selected.stat().st_size))
        self.end_headers()
        if not_modified:
            stream.close()
            return None
        return stream


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=pathlib.Path, required=True)
    parser.add_argument('--bind', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=18766)
    args = parser.parse_args()
    # Do not resolve symlinks: the stable preview path can switch releases live.
    directory = args.directory.absolute()
    if not directory.is_dir():
        parser.error('directory does not exist')
    with tempfile.TemporaryDirectory(prefix='ballance-http-gzip-') as scratch:
        handler = functools.partial(Handler, directory=str(directory))
        with http.server.ThreadingHTTPServer((args.bind, args.port), handler) as server:
            server.gzip_cache = GzipCache(scratch)
            print(f'Serving {directory} on http://{args.bind}:{args.port}/ (gzip enabled)', flush=True)
            try:
                server.serve_forever()
            except KeyboardInterrupt:
                pass


if __name__ == '__main__':
    main()
