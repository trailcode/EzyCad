#!/usr/bin/env python3
"""Run a simple HTTP server that logs the requested path and status for every request.
   Usage: python serve_with_logging.py [port]
   Default port: 8000. Serves the current directory.
   When a 404 occurs, the log line will show the path that was not found."""
import sys
from http.server import HTTPServer, SimpleHTTPRequestHandler


class LoggingHandler(SimpleHTTPRequestHandler):
    def log_request(self, code="-", size="-"):
        # Log METHOD path -> code so 404s show which path was not found
        sys.stderr.write("%s %s -> %s\n" % (self.command, self.path, str(code)))
        sys.stderr.flush()


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    server = HTTPServer(("", port), LoggingHandler)
    print("Serving current directory at http://localhost:%s/" % port, file=sys.stderr)
    print("404s will show the requested path in the log.", file=sys.stderr)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.", file=sys.stderr)
        server.shutdown()


if __name__ == "__main__":
    main()
