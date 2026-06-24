#!/usr/bin/env python3
"""Interactive BIAS FlyTrack query.

Press Enter to print the current fly info (body pose + wing angles) from the
FlyTrack HTTP server. 'q' then Enter, or EOF (Ctrl-Z then Enter on Windows,
Ctrl-D elsewhere), quits.

Usage:
  python query_fly.py [host] [port] [--cmd get-last-clear-track]

Needs BIAS running with the FlyTrack plugin enabled and capturing (so the track
queue has data). Stdlib only -- no extra packages.
"""
import argparse
import json
import math
import urllib.parse
import urllib.request

PLUGIN_NAME = "FlyTrack"
DEG = 180.0 / math.pi


def query(host, port, cmd):
    """Return (ellipse_dict_or_None, message)."""
    inner = json.dumps({"plugin": PLUGIN_NAME, "cmd": cmd})
    url = "http://%s:%d/?plugin-cmd=%s" % (host, port, urllib.parse.quote(inner))
    with urllib.request.urlopen(url, timeout=5) as r:
        raw = r.read().decode("utf-8", "replace")
    outer = json.loads(raw)
    if isinstance(outer, list):          # BIAS returns an array of results
        outer = outer[0] if outer else {}
    if not outer.get("success", False):
        return None, outer.get("message", "")
    val = outer.get("value", "")
    ell = json.loads(val) if isinstance(val, str) and val else val
    return ell, ""


def fmt(ell):
    g = ell.get
    return (
        "frame {fr}  pos=({x:.1f}, {y:.1f})  theta={th:6.1f} deg  "
        "a={a:.1f} b={b:.1f}  nwings={nw}  "
        "wingL={wl:6.1f}  wingR={wr:6.1f} deg (rel. rear axis)  "
        "areaL={al:.0f} areaR={ar:.0f}".format(
            fr=int(g("frame", -1)),
            x=float(g("x", 0)), y=float(g("y", 0)),
            th=float(g("theta", 0)) * DEG,
            a=float(g("a", 0)), b=float(g("b", 0)),
            nw=int(g("nwings", 0)),
            wl=float(g("wing_anglel", 0)) * DEG,
            wr=float(g("wing_angler", 0)) * DEG,
            al=float(g("wing_areal", 0)), ar=float(g("wing_arear", 0)),
        )
    )


def main():
    ap = argparse.ArgumentParser(description="Press Enter to print the current fly info.")
    ap.add_argument("host", nargs="?", default="127.0.0.1")
    ap.add_argument("port", nargs="?", type=int, default=5010)
    ap.add_argument("--cmd", default="get-last-clear-track",
                    help="query command: get-last-clear-track (default), "
                         "pop-back-track, or pop-front-track")
    args = ap.parse_args()

    print("BIAS FlyTrack @ %s:%d  (cmd=%s)" % (args.host, args.port, args.cmd))
    print("Press Enter for current fly info; 'q' then Enter (or Ctrl-Z/Ctrl-D) to quit.")
    while True:
        try:
            line = input("> ")
        except EOFError:
            print()
            break
        if line.strip().lower() in ("q", "quit", "exit"):
            break
        try:
            ell, msg = query(args.host, args.port, args.cmd)
        except Exception as e:
            print("  error:", e)
            continue
        if ell is None:
            print("  no data:", msg or "(empty track queue -- is it capturing?)")
        else:
            print("  " + fmt(ell))


if __name__ == "__main__":
    main()
