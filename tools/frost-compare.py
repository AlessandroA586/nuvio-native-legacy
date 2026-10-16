#!/usr/bin/env python3
"""Compare offline frost captures with the owner's approved reference renders."""
import json
from pathlib import Path
import subprocess
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else "/Volumes/ExternalSSD/tmp/foscovelho")
out = root / "cmp2"


def magick(*args):
    return subprocess.check_output(["magick", *map(str, args)])


# Home crop includes the avatar and bar; the live clock is outside it.
# Modal crop includes the entire consent panel. Menu crop includes its left edge.
cases = [
    ("home-bar", "c9/webos/home-1920.png", "c9/webos/home-1920.png", "1680x180+0+0"),
    ("modal", "c9/webos/modal-1920.png", "c9/webos/modal-1920.png", "1300x560+310+210"),
    ("menu", "menu/m-menu.bmp", "menu/m-menu.bmp", "600x1080+0+0"),
]
results = {}
for name, before, after, crop in cases:
    a, b = root / "antes" / before, out / after
    png = out / f"{name}-after.png"
    magick(b, png)
    magick(a, b, "-crop", crop, "+repage", "+append", out / f"{name}-before-after.png")
    diff = out / f"{name}-diff.png"
    magick(a, b, "-crop", crop, "+repage", "-compose", "difference", "-composite", diff)
    pixels = magick(diff, "-alpha", "off", "-depth", "8", "RGB:-")
    maximum = max(pixels)
    full_max = int(magick(a, b, "-compose", "difference", "-composite", "-alpha", "off",
                          "-format", "%[fx:round(maxima*255)]", "info:"))
    results[name] = {
        "crop": crop,
        "max_channel_diff": maximum,
        "mean_channel_diff": round(sum(pixels) / len(pixels), 4),
        "full_frame_max_including_clock": full_max,
    }
    print(f"{name}: max RGB diff={maximum}/255; mean={results[name]['mean_channel_diff']}; "
          f"crop={crop}; full frame including clock={full_max}/255")
(out / "pixel-diff.json").write_text(json.dumps(results, indent=2) + "\n")
