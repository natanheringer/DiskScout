#!/bin/bash
echo "Clearing DiskScout cache..."
if [ -d "$HOME/.cache/diskscout" ]; then
    rm -rf "$HOME/.cache/diskscout"
    echo "Cache cleared successfully!"
else
    echo "No cache found."
fi
echo "Done."
