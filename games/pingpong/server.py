#!/usr/bin/env python3
"""Loopback-only static server for the Ping Pong Scorekeeper."""

from __future__ import annotations

import argparse
import json
import os
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


APP_DIR = Path(__file__).resolve().parent


class ScorekeeperHandler(SimpleHTTPRequestHandler):
    server_version = "PingPongScorekeeper/1.0"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(APP_DIR), **kwargs)

    def do_GET(self):
        if self.path.rstrip("/") == "/health":
            body = json.dumps(
                {"status": "ok", "service": "pingpong-scorekeeper"},
                separators=(",", ":"),
            ).encode()
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(body)
            return
        super().do_GET()

    def end_headers(self):
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Permissions-Policy", "camera=(self), microphone=(self)")
        self.send_header(
            "Content-Security-Policy",
            "default-src 'self'; script-src 'self'; style-src 'self'; "
            "img-src 'self' data: blob:; media-src 'self' blob:; "
            "connect-src 'self'; object-src 'none'; base-uri 'none'; "
            "frame-ancestors 'none'",
        )
        super().end_headers()

    def log_message(self, format_string, *args):
        print(
            f'{self.log_date_time_string()} {self.client_address[0]} '
            f'{format_string % args}',
            flush=True,
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4173)
    args = parser.parse_args()

    os.chdir(APP_DIR)
    server = ThreadingHTTPServer((args.bind, args.port), ScorekeeperHandler)
    print(f"Ping Pong Scorekeeper listening on http://{args.bind}:{args.port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
