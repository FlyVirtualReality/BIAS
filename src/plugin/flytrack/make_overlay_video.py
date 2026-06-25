#!/usr/bin/env python3
"""Render a BIAS FlyTrack trajectory-overlay video.

Reads a source video and a FlyTrack JSON trajectory log (the file written when
Logging is enabled) and writes a new video with the body ellipse and wing lines
drawn on each frame.

Usage:
  python make_overlay_video.py INPUT_VIDEO TRACK_JSON OUTPUT_VIDEO
                               [--carry] [--frame-offset N] [--fps F]
                               [--crop-radius R] [--scale S] [--trim]

Notes:
  * Colors: body ellipse = C0 (blue), wings = C1 (orange). Lines are antialiased.
  * --crop-radius R zooms in: each output frame is cropped to a 2R x 2R box that
    follows the fly (useful when the wings are too small to see full-frame).
  * --scale S upscales the (cropped) output S-fold for higher resolution; the
    overlay is drawn at the upscaled resolution, so the ellipse/wings stay sharp.
  * Track entries are matched to video frames by the JSON "frame" field, plus
    --frame-offset (default -2, because BIAS skips 2 startup frames before it
    starts numbering, so track frame f corresponds to video frame f+2). If the
    overlay looks shifted in time, nudge --frame-offset by +/-1. Use --carry to
    hold the last pose on frames the tracker skipped.
  * Requires: opencv-python (and optionally tqdm for a progress bar).
"""
import argparse
import json
import math

import cv2

try:
    from tqdm import tqdm
except ImportError:  # progress bar is optional
    tqdm = None


def load_tracks(path):
    with open(path, "r") as f:
        data = json.load(f)
    track = data["track"] if isinstance(data, dict) and "track" in data else data
    by_frame = {}
    for e in track:
        by_frame[int(e["frame"])] = e
    return by_frame


# matplotlib default colors, as BGR for OpenCV
COLOR_BODY = (180, 119, 31)   # C0  (#1f77b4)
COLOR_WING = (14, 127, 255)   # C1  (#ff7f0e)


def draw_overlay(frame, e, ox=0.0, oy=0.0, scale=1.0):
    """Draw the overlay onto `frame`. Image coords are mapped to frame coords as
    (coord - offset) * scale, so the overlay can be drawn at the cropped/upscaled
    resolution and stay crisp. Lines are antialiased."""
    def fx(px):
        return int(round((px - ox) * scale))

    def fy(py):
        return int(round((py - oy) * scale))

    x, y = float(e["x"]), float(e["y"])
    a, b, theta = float(e["a"]), float(e["b"]), float(e["theta"])
    th = max(1, int(round(scale)))  # line thickness scales with zoom

    # body ellipse (a, b are semi-axes, as the plugin draws them)
    cv2.ellipse(frame, (fx(x), fy(y)),
                (int(round(a * scale)), int(round(b * scale))),
                theta * 180.0 / math.pi, 0, 360, COLOR_BODY, th, cv2.LINE_AA)

    # wings (lines from the centroid toward each wing tip, behind the body)
    nwings = int(e.get("nwings", 0))
    if nwings > 0:
        wing_len = 2.0 * a
        rear = theta + math.pi
        for key in ("wing_anglel", "wing_angler"):
            if key not in e or e[key] is None:
                continue
            ang = float(e[key])
            # skip the padded phantom (zero-angle) wing when only one is detected
            if nwings < 2 and abs(ang) < 1e-6:
                continue
            wa = rear + ang
            tipx = x + wing_len * math.cos(wa)
            tipy = y + wing_len * math.sin(wa)
            cv2.line(frame, (fx(x), fy(y)), (fx(tipx), fy(tipy)), COLOR_WING, th, cv2.LINE_AA)


def main():
    ap = argparse.ArgumentParser(description="Render a FlyTrack trajectory-overlay video.")
    ap.add_argument("input_video")
    ap.add_argument("track_json")
    ap.add_argument("output_video")
    ap.add_argument("--carry", action="store_true",
                    help="hold the last pose on frames with no track entry")
    ap.add_argument("--frame-offset", type=int, default=-2,
                    help="add to the video frame index to match track 'frame' numbers. "
                         "Default -2: BIAS skips 2 startup frames, so track frame f = video "
                         "frame f+2.")
    ap.add_argument("--fps", type=float, default=0.0,
                    help="output fps (default: source fps)")
    ap.add_argument("--crop-radius", type=int, default=0,
                    help="zoom in: crop each output frame to a 2R x 2R box centered on the "
                         "fly (R pixels). 0 = no crop (full frame).")
    ap.add_argument("--scale", type=float, default=1.0,
                    help="upscale the output by this factor (e.g. 4) for higher resolution; "
                         "the overlay is drawn at the upscaled resolution so it stays crisp.")
    ap.add_argument("--trim", action="store_true",
                    help="only render the video frames covered by the trajectory (seek to the "
                         "first tracked frame, stop after the last) instead of the whole video. "
                         "Useful for segment tracking (-s).")
    args = ap.parse_args()

    by_frame = load_tracks(args.track_json)
    if by_frame:
        print(f"loaded {len(by_frame)} track entries, "
              f"frame range {min(by_frame)}..{max(by_frame)}")
    else:
        print("warning: no track entries found")

    cap = cv2.VideoCapture(args.input_video)
    if not cap.isOpened():
        raise SystemExit(f"cannot open input video: {args.input_video}")
    w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = args.fps if args.fps > 0 else (cap.get(cv2.CAP_PROP_FPS) or 30.0)

    # optional zoom: crop each output frame to a fixed box around the fly, then upscale
    crop_radius = max(0, args.crop_radius)
    scale = max(1.0, args.scale)
    if crop_radius > 0:
        box_w, box_h = min(2 * crop_radius, w), min(2 * crop_radius, h)
    else:
        box_w, box_h = w, h
    out_w, out_h = int(round(box_w * scale)), int(round(box_h * scale))
    crop_cx, crop_cy = w / 2.0, h / 2.0  # follows the fly; image center until first track

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    out = cv2.VideoWriter(args.output_video, fourcc, fps, (out_w, out_h))
    if not out.isOpened():
        raise SystemExit(f"cannot open output video for writing: {args.output_video}")

    total = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    if total <= 0:
        total = None  # unknown/unreliable -> tqdm shows a count instead of a bar

    # optionally restrict output to the frames covered by the trajectory
    start_i, stop_i = 0, None
    if args.trim and by_frame:
        start_i = max(0, min(by_frame) - args.frame_offset)
        stop_i = max(by_frame) - args.frame_offset
        cap.set(cv2.CAP_PROP_POS_FRAMES, start_i)
        start_i = int(round(cap.get(cv2.CAP_PROP_POS_FRAMES)))  # actual (codec may snap)
        total = max(0, stop_i - start_i + 1)
        print(f"trim: rendering video frames {start_i}..{stop_i}")

    pbar = tqdm(total=total, unit="frame", desc="overlay") if tqdm is not None else None

    i = start_i
    last = None
    n_drawn = 0
    while True:
        ok, frame = cap.read()
        if not ok:
            break
        if stop_i is not None and i > stop_i:
            break
        if frame.ndim == 2 or frame.shape[2] == 1:
            frame = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
        e = by_frame.get(i + args.frame_offset)
        if e is None and args.carry:
            e = last
        if e is not None:
            last = e
            n_drawn += 1
            crop_cx, crop_cy = float(e["x"]), float(e["y"])  # crop follows the fly

        # crop a fixed box of raw pixels around the fly (full frame if no crop)
        if crop_radius > 0:
            x0 = max(0, min(int(round(crop_cx)) - crop_radius, w - box_w))
            y0 = max(0, min(int(round(crop_cy)) - crop_radius, h - box_h))
        else:
            x0, y0 = 0, 0
        patch = frame[y0:y0 + box_h, x0:x0 + box_w]

        # upscale for resolution (or copy so the sliced view is writable)
        if scale != 1.0:
            patch = cv2.resize(patch, (out_w, out_h), interpolation=cv2.INTER_LINEAR)
        elif crop_radius > 0:
            patch = patch.copy()

        # draw the overlay at the (cropped, upscaled) resolution so it stays crisp
        if e is not None:
            draw_overlay(patch, e, ox=x0, oy=y0, scale=scale)

        # stamp the track/JSON frame number in the top-left corner (white w/ black outline)
        label = "frame %d" % (i + args.frame_offset)
        fscale = max(0.5, out_h / 500.0)
        fth = max(1, int(round(fscale * 1.5)))
        org = (8, int(round(28 * fscale)))
        cv2.putText(patch, label, org, cv2.FONT_HERSHEY_SIMPLEX, fscale, (0, 0, 0), fth + 2, cv2.LINE_AA)
        cv2.putText(patch, label, org, cv2.FONT_HERSHEY_SIMPLEX, fscale, (255, 255, 255), fth, cv2.LINE_AA)

        out.write(patch)
        i += 1
        if pbar is not None:
            pbar.update(1)

    if pbar is not None:
        pbar.close()
    cap.release()
    out.release()
    print(f"wrote {args.output_video}: {i} frames, {n_drawn} with overlay, {fps:.1f} fps")


if __name__ == "__main__":
    main()
