from pathlib import Path

Import("env")
root = Path(env["PROJECT_DIR"])
html = (root / "index.html").read_text(encoding="utf-8")
assert ')PUSHBOT_UI"' not in html
header = '#pragma once\n#include <Arduino.h>\nstatic const char DRIVER_STATION[] PROGMEM = R"PUSHBOT_UI(' + html + ')PUSHBOT_UI";\n'
(root / "driver_station.h").write_text(header, encoding="utf-8")

