#!/usr/bin/env python3
"""Convert Arcanist's factory preset XMLs from the JUCE-era APVTS ValueTree
schema (<Parameters><PARAM id=".." value=".."/></Parameters>) to
AudioPlugins/Common's generic Preset schema
(<AudioPluginsPreset name=".." pluginId=".." version=".."><Parameter id=".." value=".."/></AudioPluginsPreset>).
Data (every id/value pair) is preserved exactly; only the wrapper tags and
attribute names change. Run once, in place, then commit the rewritten files."""
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

PRESETS_DIR = Path(__file__).resolve().parent.parent / "Source" / "Presets" / "Factory"
PLUGIN_ID = "com.spellbound.arcanist"

# Names sourced from Source/_juce_reference/PresetManager.cpp's factoryDefs
# table (name/category pairs, in file-numbering order). Cross-checked
# 2026-09-07 against that file -- matches exactly, including "GM Sci-Fi"
# and "DX Bass 1".
NAME_MAP = {
    "001_CelestialDrift.xml": "Celestial Drift", "002_HeavenlyGlow.xml": "Heavenly Glow",
    "003_CloudNine.xml": "Cloud Nine", "004_Starlight.xml": "Starlight",
    "005_Nebula.xml": "Nebula", "006_InfiniteSpace.xml": "Infinite Space",
    "007_MidnightCathedral.xml": "Midnight Cathedral", "008_OminousSwell.xml": "Ominous Swell",
    "009_CinematicStrings.xml": "Cinematic Strings", "010_DeepMystery.xml": "Deep Mystery",
    "011_ShadowedDream.xml": "Shadowed Dream", "012_RichStrings.xml": "Rich Strings",
    "013_VelvetWarmth.xml": "Velvet Warmth", "014_ChoirShimmer.xml": "Choir Shimmer",
    "015_GoldenHour.xml": "Golden Hour", "016_OrganicBloom.xml": "Organic Bloom",
    "017_CrystalBell.xml": "Crystal Bell", "018_SparklingSunrise.xml": "Sparkling Sunrise",
    "019_LuminousPad.xml": "Luminous Pad", "020_PrismaticGlow.xml": "Prismatic Glow",
    "021_MorphingLandscape.xml": "Morphing Landscape", "022_BreathingPulse.xml": "Breathing Pulse",
    "023_SpiralingCosmos.xml": "Spiraling Cosmos", "024_VintageSynthPad.xml": "Vintage Synth Pad",
    "025_BladeRunnerNight.xml": "Blade Runner Night", "026_AlphaCentauri.xml": "Alpha Centauri",
    "027_CS80Strings.xml": "CS80 Strings", "028_EpicCinema.xml": "Epic Cinema",
    "029_StringMachine.xml": "String Machine", "030_OceanicSweep.xml": "Oceanic Sweep",
    "031_CosmicHorizon.xml": "Cosmic Horizon", "032_ElectricNight.xml": "Electric Night",
    "033_LaserHarp.xml": "Laser Harp", "034_GMFantasia.xml": "GM Fantasia",
    "035_GMCrystal.xml": "GM Crystal", "036_GMAtmosphere.xml": "GM Atmosphere",
    "037_GMBrightness.xml": "GM Brightness", "038_GMEchoes.xml": "GM Echoes",
    "039_GMSciFi.xml": "GM Sci-Fi", "040_GMVoice.xml": "GM Voice",
    "041_GMStrings.xml": "GM Strings", "042_DXBass1.xml": "DX Bass 1",
}


def convert(path: Path) -> None:
    tree = ET.parse(path)
    root = tree.getroot()
    assert root.tag == "Parameters", f"{path}: unexpected root tag {root.tag!r}"

    name = NAME_MAP[path.name]
    new_root = ET.Element("AudioPluginsPreset", {"name": name, "pluginId": PLUGIN_ID, "version": "1"})
    for param in root.findall("PARAM"):
        ET.SubElement(new_root, "Parameter", {"id": param.get("id"), "value": param.get("value")})

    ET.indent(new_root, space="  ")
    new_tree = ET.ElementTree(new_root)
    new_tree.write(path, encoding="UTF-8", xml_declaration=True)


def main() -> int:
    files = sorted(PRESETS_DIR.glob("*.xml"))
    if len(files) != 42:
        print(f"expected 42 factory preset files, found {len(files)}", file=sys.stderr)
        return 1
    missing = [f.name for f in files if f.name not in NAME_MAP]
    if missing:
        print(f"NAME_MAP missing entries for: {missing}", file=sys.stderr)
        return 1
    for f in files:
        convert(f)
        print(f"converted {f.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
