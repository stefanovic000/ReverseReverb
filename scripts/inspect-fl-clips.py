"""Read-only FL 25 clip/channel inspection. Never writes or opens a project in FL.

Event layout references:
https://github.com/dawhubapp/flpdiff/blob/main/docs/fl-format/flp-format-spec.md
Only clip source identity is reported; this is not a complete mixer-route decoder.
"""
import hashlib
import json
import struct
import sys
from pathlib import Path


def inspect(path):
    data = path.read_bytes()
    if data[:4] != b"FLhd" or data[14:18] != b"FLdt":
        raise ValueError("Unsupported FLP envelope")
    if struct.unpack_from("<I", data, 18)[0] != len(data) - 22:
        raise ValueError("Incomplete project data")
    position, events = 22, []
    while position < len(data):
        event_id = data[position]
        position += 1
        if event_id == 0xAC:  # FL 25's three-byte exception to the size ranges.
            size = 3
        elif event_id < 0xC0:
            size = 1 if event_id < 0x40 else 2 if event_id < 0x80 else 4
        else:
            size, shift = 0, 0
            while True:
                if position >= len(data) or shift > 28:
                    raise ValueError("Invalid event length")
                byte = data[position]
                position += 1
                size |= (byte & 127) << shift
                shift += 7
                if byte < 128:
                    break
        if position + size > len(data):
            raise ValueError("Truncated event")
        events.append((event_id, data[position:position + size]))
        position += size

    channels, clips = {}, []
    current_channel = None
    for event_id, payload in events:
        if event_id == 0x40:
            current_channel = struct.unpack("<H", payload)[0]
            channels[current_channel] = {"channel_id": current_channel}
        elif event_id in (0x63, 0x62):
            current_channel = None
        elif current_channel is not None and event_id == 0xCB:
            channels[current_channel]["name"] = payload.decode("utf-16le").rstrip("\0")
        if event_id == 0xE9:
            if len(payload) % 80:
                raise ValueError("Expected FL 25's 80-byte playlist records")
            for offset in range(0, len(payload), 80):
                tick, pattern_base, item, length, lane = struct.unpack_from("<IHHIH", payload, offset)
                clips.append({"position_ticks": tick, "length_ticks": length,
                              "lane_reverse_index": lane,
                              "audio_channel_id": item if item < pattern_base else None,
                              "pattern_id": item - pattern_base if item >= pattern_base else None})
    for clip in clips:
        if clip["audio_channel_id"] in channels:
            clip["audio_channel_name"] = channels[clip["audio_channel_id"]].get("name")
    if hashlib.sha256(path.read_bytes()).digest() != hashlib.sha256(data).digest():
        raise ValueError("Project changed during inspection; save and retry")
    return {"path": str(path), "sha256": hashlib.sha256(data).hexdigest(),
            "channels": list(channels.values()), "playlist_clips": clips,
            "note": "Equal audio_channel_id means clips share the same Audio Clip channel. Mixer destinations are not decoded."}


if __name__ == "__main__":
    print(json.dumps(inspect(Path(sys.argv[1])), indent=2))
