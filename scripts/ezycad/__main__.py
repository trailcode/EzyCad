"""CLI: python -m ezycad  (or scripts/ezycad_remote.py)."""

from __future__ import annotations

import argparse
import sys
from typing import Optional

from ezycad import connect


def _print_result(output: Optional[str], result: Optional[str], error: Optional[str], ok: bool) -> None:
    if output:
        print(output)
    if result:
        print(result)
    if error:
        print(error, file=sys.stderr)
    if not ok and not error:
        print("request failed", file=sys.stderr)


def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="EzyCad remote Python client (import ezycad)")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("-c", "--code", help="Execute one snippet and exit")
    args = parser.parse_args(argv)

    try:
        app = connect(args.host, args.port)
    except OSError as e:
        print(f"connect failed: {e}", file=sys.stderr)
        return 1

    with app:
        if args.code is not None:
            r = app.execute(args.code)
            _print_result(r.output, r.result, r.error, r.ok)
            return 0 if r.ok else 1

        print(f"Connected to {args.host}:{args.port}. Enter Python; Ctrl-D/Ctrl-Z to quit.")
        print("Tip: import ezycad; app = ezycad.connect()  # typed API with tab completion")
        while True:
            try:
                line = input(">>> ")
            except EOFError:
                print()
                break
            except KeyboardInterrupt:
                print()
                break
            if not line.strip():
                continue
            r = app.execute(line)
            _print_result(r.output, r.result, r.error, r.ok)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
