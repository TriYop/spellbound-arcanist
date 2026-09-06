#!/usr/bin/env bash
# Install Arcanist plugins and standalone app (VST3/CLAP/LV2 + JACK standalone).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYSTEM=0

for arg in "$@"; do
    case "$arg" in
        --system) SYSTEM=1 ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

if [[ $SYSTEM -eq 1 ]]; then
    VST3_DIR="/usr/lib/vst3"; CLAP_DIR="/usr/lib/clap"; LV2_DIR="/usr/lib/lv2"; BIN_DIR="/usr/local/bin"
else
    VST3_DIR="${HOME}/.vst3"; CLAP_DIR="${HOME}/.clap"; LV2_DIR="${HOME}/.lv2"; BIN_DIR="${HOME}/.local/bin"
fi

echo "Installing Arcanist..."
mkdir -p "${VST3_DIR}"; rm -rf "${VST3_DIR}/Arcanist.vst3"
cp -r "${SCRIPT_DIR}/VST3/Arcanist.vst3" "${VST3_DIR}/"
echo "  VST3  -> ${VST3_DIR}/Arcanist.vst3"

mkdir -p "${CLAP_DIR}"
cp "${SCRIPT_DIR}/CLAP/Arcanist.clap" "${CLAP_DIR}/"
chmod 755 "${CLAP_DIR}/Arcanist.clap"
echo "  CLAP  -> ${CLAP_DIR}/Arcanist.clap"

mkdir -p "${LV2_DIR}"; rm -rf "${LV2_DIR}/Arcanist.lv2"
cp -r "${SCRIPT_DIR}/LV2/Arcanist.lv2" "${LV2_DIR}/"
echo "  LV2   -> ${LV2_DIR}/Arcanist.lv2"

mkdir -p "${BIN_DIR}"
cp "${SCRIPT_DIR}/bin/Arcanist" "${BIN_DIR}/"
chmod 755 "${BIN_DIR}/Arcanist"
echo "  App   -> ${BIN_DIR}/Arcanist"
echo "Done."
