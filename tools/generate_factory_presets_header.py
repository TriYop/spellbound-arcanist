#!/usr/bin/env python3
"""Generate Source/FactoryPresets.h from the converted Source/Presets/Factory/*.xml
files (Task 5 Step 2's output). Run after any factory preset XML changes."""
import xml.etree.ElementTree as ET
from pathlib import Path

PRESETS_DIR = Path(__file__).resolve().parent.parent / "Source" / "Presets" / "Factory"
OUT_PATH = Path(__file__).resolve().parent.parent / "Source" / "FactoryPresets.h"

HEADER = '''#pragma once
// GENERATED FILE -- do not hand-edit. Regenerate with
// tools/generate_factory_presets_header.py after changing
// Source/Presets/Factory/*.xml.
#include "audioplugins/common/presets/Preset.h"
#include <vector>

inline std::vector<audioplugins::common::presets::Preset> arcanistFactoryPresets()
{
    using audioplugins::common::presets::Preset;
    using audioplugins::common::presets::ParameterValue;
    std::vector<Preset> presets;
'''
FOOTER = '''    return presets;
}
'''


def main() -> None:
    files = sorted(PRESETS_DIR.glob("*.xml"))
    parts = [HEADER]
    for f in files:
        root = ET.parse(f).getroot()
        assert root.tag == "AudioPluginsPreset", f"{f}: unexpected root tag {root.tag!r}"
        name = root.get("name")
        plugin_id = root.get("pluginId")
        parts.append("    {\n        Preset p;\n")
        parts.append(f'        p.name = "{name}";\n')
        parts.append(f'        p.pluginId = "{plugin_id}";\n')
        parts.append("        p.schemaVersion = 1;\n")
        parts.append("        p.parameters = {\n")
        for param in root.findall("Parameter"):
            # A bare integer digit-sequence + "f" (e.g. "2f") is not a valid
            # C++ floating literal -- it needs a decimal point (e.g. "2.0f").
            # float(...) round-trips every value in these XMLs exactly
            # (they're all small decimal literals to begin with), and repr()
            # always includes a ".".
            value = repr(float(param.get("value")))
            parts.append(f'            {{"{param.get("id")}", {value}f}},\n')
        parts.append("        };\n        presets.push_back(std::move(p));\n    }\n")
    parts.append(FOOTER)
    OUT_PATH.write_text("".join(parts))
    print(f"wrote {OUT_PATH} ({len(files)} presets)")


if __name__ == "__main__":
    main()
