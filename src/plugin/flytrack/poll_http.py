#!/usr/bin/env python3
"""Repeatedly poll the BIAS FlyTrack HTTP server for the fly ellipse + wing angles,
and report dropped/skipped frames, request latency, and hangs.

Use this to reproduce/measure the high-rate polling slowdown.

Usage:
  python poll_http.py [--host 127.0.0.1] [--port 5010] [--cmd pop-back-track]
                      [--rate 0] [--duration 10] [--timeout 2] [--quiet]

  --rate 0   poll as fast as possible; otherwise a target Hz.
  --cmd      pop-back-track : newest entry, popped (what the colleague polls). With this,
                              frame gaps mix "polled too slow" and tracker drops.
             pop-front-track: oldest entry, popped -> drains the queue IN ORDER, so
                              frame gaps = genuine tracker-dropped input frames. Best for
                              measuring whether wing tracking causes drops.
             get-last-clear-track: newest entry, clears the queue.
  --timeout  per-request timeout (s); requests slower than this count as hangs.

Notes:
  * The server port is the one shown in the BIAS window's status / config (default 5010).
    If you don't get data, try a neighboring port (e.g. 5020) -- BIAS offsets the port by
    camera number.
  * Stdlib only (urllib/json) -- no numpy/requests needed.
"""
import argparse
import json
import time
import urllib.parse
import urllib.request
import urllib.error


def poll_once(host, port, plugin, cmd, timeout):
    q = urllib.parse.quote(json.dumps({"plugin": plugin, "cmd": cmd}))
    url = "http://%s:%d/?plugin-cmd=%s" % (host, port, q)
    t0 = time.perf_counter()
    with urllib.request.urlopen(url, timeout=timeout) as r:
        body = r.read().decode("utf-8", "replace")
    dt = time.perf_counter() - t0
    # response is a JSON array: [{success, message, value, command}], value = ellipse JSON str
    data = json.loads(body)
    item = data[0] if isinstance(data, list) and data else data
    ell = None
    val = item.get("value", "") if isinstance(item, dict) else ""
    if isinstance(item, dict) and item.get("success") and val:
        try:
            ell = json.loads(val) if isinstance(val, str) else val
        except (json.JSONDecodeError, TypeError):
            ell = None
    return dt, ell


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=5010)
    ap.add_argument("--plugin", default="FlyTrack")
    ap.add_argument("--cmd", default="pop-back-track",
                    choices=["pop-back-track", "pop-front-track", "get-last-clear-track"])
    ap.add_argument("--rate", type=float, default=0.0, help="target poll Hz (0 = as fast as possible)")
    ap.add_argument("--duration", type=float, default=10.0, help="seconds to poll")
    ap.add_argument("--timeout", type=float, default=2.0, help="per-request timeout (s) = hang threshold")
    ap.add_argument("--quiet", action="store_true", help="suppress the per-poll lines (summary only)")
    args = ap.parse_args()

    period = 1.0 / args.rate if args.rate > 0 else 0.0
    latencies = []
    frames = []                 # frame numbers of non-empty responses, in poll order
    wing_of = {}                # frame -> (nwings, |anglel|+|angler|)
    n_poll = n_empty = n_hang = n_err = 0
    prev_fr = None

    print("polling http://%s:%d  cmd=%s  rate=%s  for %.0fs ..."
          % (args.host, args.port, args.cmd, "max" if args.rate == 0 else args.rate, args.duration))
    t_end = time.perf_counter() + args.duration
    next_t = time.perf_counter()
    while time.perf_counter() < t_end:
        if period:
            now = time.perf_counter()
            if now < next_t:
                time.sleep(next_t - now)
            next_t += period
        n_poll += 1
        try:
            dt, ell = poll_once(args.host, args.port, args.plugin, args.cmd, args.timeout)
            latencies.append(dt)
            if ell is None:
                n_empty += 1
                if not args.quiet:
                    print("[%5d] %6.1fms  <empty>" % (n_poll, 1e3 * dt))
            else:
                fr = int(ell.get("frame", -1))
                gap = (fr - prev_fr - 1) if (prev_fr is not None and fr > prev_fr) else 0
                prev_fr = fr
                frames.append(fr)
                wl = abs(float(ell.get("wing_anglel", 0.0)))
                wr = abs(float(ell.get("wing_angler", 0.0)))
                wing_of[fr] = (int(ell.get("nwings", 0)), wl + wr)
                if not args.quiet:
                    print("[%5d] %6.1fms  frame=%-7d gap=%-4d  x=%7.1f y=%7.1f th=%+.3f  "
                          "nw=%d wl=%+.3f wr=%+.3f"
                          % (n_poll, 1e3 * dt, fr, gap,
                             float(ell.get("x", 0)), float(ell.get("y", 0)), float(ell.get("theta", 0)),
                             int(ell.get("nwings", 0)),
                             float(ell.get("wing_anglel", 0)), float(ell.get("wing_angler", 0))))
        except (urllib.error.URLError, TimeoutError, ConnectionError) as e:
            reason = getattr(e, "reason", e)
            is_to = isinstance(reason, TimeoutError) or "timed out" in str(reason).lower()
            n_hang += int(is_to)
            n_err += int(not is_to)
            if not args.quiet:
                print("[%5d] %s: %s" % (n_poll, "HANG" if is_to else "ERR", reason))
        except Exception as e:
            n_err += 1
            if not args.quiet:
                print("[%5d] ERR: %s" % (n_poll, e))

    # ---- summary ----
    print("\npolls=%d  with-data=%d  empty=%d  hangs(>%.1fs)=%d  errors=%d  achieved=%.0f Hz"
          % (n_poll, len(frames), n_empty, args.timeout, n_hang, n_err, n_poll / args.duration))
    if latencies:
        latencies.sort()
        pc = lambda p: latencies[min(len(latencies) - 1, int(p * len(latencies)))]
        print("latency ms: min=%.1f  p50=%.1f  p95=%.1f  p99=%.1f  max=%.1f"
              % (1e3 * latencies[0], 1e3 * pc(.5), 1e3 * pc(.95), 1e3 * pc(.99), 1e3 * latencies[-1]))
    if len(frames) >= 2:
        gaps = [(a, b, b - a - 1) for a, b in zip(frames, frames[1:]) if b - a > 1]
        dropped = sum(g[2] for g in gaps)
        span = max(frames) - min(frames) + 1     # frames the tracker *could* have produced
        tracked = len(set(frames))
        nz = sum(1 for nw, w in wing_of.values() if nw > 0)
        print("frame range: %d..%d  (span %d)  tracked=%d  frames-with-wings=%d/%d"
              % (min(frames), max(frames), span, tracked, nz, len(wing_of)))
        drop_pct = 100.0 * dropped / span if span else 0.0
        maxrun = max((g[2] for g in gaps), default=0)
        if args.cmd == "pop-front-track":
            print(">>> DROPPED frames (tracker skipped): %d of %d  (%.1f%%)   max run dropped: %d"
                  % (dropped, span, drop_pct, maxrun))
            print("    (drain-in-order, so this is the true tracker drop count -- as long as you")
            print("     polled fast enough that the queue never overflowed)")
        elif args.cmd == "get-last-clear-track":
            print(">>> frames DROPPED by grab-newest-and-clear: %d of %d  (%.1f%%)   max run dropped: %d"
                  % (dropped, span, drop_pct, maxrun))
            print("    (each poll keeps the newest and clears the queue; this is every produced")
            print("     frame the consumer never saw = queue-cleared + tracker-dropped combined.")
            print("     pass -o on BIAS and diff against the trajectory file to split the two.)")
        else:
            print(">>> frames skipped between polls: %d  (newest-only; mixes drops + poll rate;"
                  % dropped)
            print("    use --cmd pop-front-track or get-last-clear-track for a drop count)")
        if gaps:
            gaps.sort(key=lambda g: -g[2])
            print("largest gaps (from -> to, skipped, nwings/|wing| at 'from'):")
            for a, b, sk in gaps[:8]:
                nw, w = wing_of.get(a, (0, 0.0))
                print("  %7d -> %-7d  skipped %-5d  nwings=%d  |wing|=%.3f" % (a, b, sk, nw, w))


if __name__ == "__main__":
    main()
