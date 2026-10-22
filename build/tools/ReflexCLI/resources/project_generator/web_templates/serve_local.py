"""Serve one generated browser app with shared-memory isolation headers."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse
import json
import os
import socket
import subprocess
import sys
import webbrowser


class LocalServer(ThreadingHTTPServer):
    # Windows SO_REUSEADDR can let another server bind the same address/port.
    # Set exclusive access before HTTPServer.server_bind performs the bind.
    allow_reuse_address = os.name != "nt"
    allow_reuse_port = False

    def server_bind(self):
        if os.name == "nt":
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        super().server_bind()


class Handler(SimpleHTTPRequestHandler):
    extensions_map = {**SimpleHTTPRequestHandler.extensions_map, ".wasm": "application/wasm", ".js": "text/javascript"}

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


if __name__ == "__main__":
    here = Path(__file__).resolve().parent
    config = json.loads((here / "build-config.json").read_text(encoding="utf-8"))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default=config["default_configuration"])
    parser.add_argument("--target", help="target name (required if the project has multiple apps)")
    parser.add_argument("--port", type=int, default=0, help="localhost port (default: choose an available port)")
    parser.add_argument("--build", action="store_true", help="build the configuration before serving")
    parser.add_argument("--open", action="store_true", help="open the app in the default browser")
    args = parser.parse_args()
    if not 0 <= args.port <= 65535:
        parser.error("--port must be between 0 and 65535 (0 chooses an available port)")
    apps = [target for target in config["targets"] if not target["static"]
            and target["configuration"] == args.configuration
            and (not args.target or target["name"] == args.target)]
    if len(apps) != 1:
        parser.error("Select exactly one app with --configuration and --target")
    if args.build:
        try:
            subprocess.run([Path(sys.executable).as_posix(), (here / "build.py").as_posix(),
                            "--configuration", args.configuration, "--target", apps[0]["name"]], check=True)
        except subprocess.CalledProcessError as error:
            sys.exit(error.returncode)
    product = Path(apps[0]["product"])
    if not product.is_file() or not (product.parent / "index.html").is_file():
        parser.error("Build the selected app first")
    try:
        server = LocalServer(("127.0.0.1", args.port), partial(Handler, directory=str(product.parent)))
    except OSError as error:
        parser.error(f"Cannot bind local server to 127.0.0.1:{args.port}: {error}; omit --port to choose an available port")
    url = f"http://127.0.0.1:{server.server_port}/"
    print(f"Serving {product.parent} at {url} (Ctrl+C to stop)", flush=True)
    with server:
        if args.open and not webbrowser.open(url):
            print(f"Open {url} in your browser", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass
