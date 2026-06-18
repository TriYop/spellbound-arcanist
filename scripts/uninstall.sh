#!/usr/bin/env bash
# Remove PM0 from all known install locations.
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

echo "Uninstalling PM0..."

remove "${HOME}/.vst3/PM0.vst3"
remove "${HOME}/.clap/PM0.clap"
remove "${HOME}/.local/bin/PM0"

if [[ $EUID -eq 0 ]]; then
    remove "/usr/lib/vst3/PM0.vst3"
    remove "/usr/lib/clap/PM0.clap"
    remove "/usr/local/bin/PM0"
else
    for path in "/usr/lib/vst3/PM0.vst3" \
                "/usr/lib/clap/PM0.clap" \
                "/usr/local/bin/PM0"; do
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
