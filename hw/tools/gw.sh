#!/bin/bash

# Detect if we're in a Cursor environment
if [[ "$PWD" == *".cursor"* ]] || [[ "$TERM_PROGRAM" == *"cursor"* ]] || [[ -z "$DISPLAY" ]]; then
    echo "Detected headless environment - using offscreen Qt"
    export QT_QPA_PLATFORM=minimal
    export DISPLAY=:0
    export QT_LOGGING_RULES="qt.qpa.xcb.warning=false"
else
    echo "GUI environment available - Qt GUI enabled"
fi

/opt/gowin/IDE/bin/gw_sh "$@"