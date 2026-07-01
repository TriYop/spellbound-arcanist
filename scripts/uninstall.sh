#!/usr/bin/env bash
# Remove Arcanist from all known install locations.
set -euo pipefail

removed=0

remove() {
    local path="$1"
    if [[ -e "$path" ]]; then
        rm -rf "$path"
        echo "  Removed: $path"
        removed=1
    fi
}

echo "Uninstalling Arcanist..."

remove "${HOME}/.vst3/Arcanist.vst3"
remove "${HOME}/.clap/Arcanist.clap"
remove "${HOME}/.local/bin/Arcanist"

if [[ $EUID -eq 0 ]]; then
    remove "/usr/lib/vst3/Arcanist.vst3"
    remove "/usr/lib/clap/Arcanist.clap"
    remove "/usr/local/bin/Arcanist"
else
    for path in "/usr/lib/vst3/Arcanist.vst3" \
                "/usr/lib/clap/Arcanist.clap" \
                "/usr/local/bin/Arcanist"; do
        if [[ -e "$path" ]]; then
            echo "  Skipping $path (re-run with sudo to remove)"
        fi
    done
fi

if [[ $removed -eq 0 ]]; then
    echo "  Nothing to remove."
else
    echo "Done."
fi
