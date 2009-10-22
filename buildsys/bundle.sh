#!/bin/bash

BINARY="$1"
TARGET="$2"

APP_BUNDLE="wxDBGp.app"

rm -rf "$TARGET"
mkdir -p "$TARGET"
cp -r "mac/$APP_BUNDLE/Contents" "$TARGET"
find "$TARGET" -name .svn -a -type d -exec rm -rf \{\} \; >& /dev/null
cp "$BINARY" "$TARGET/Contents/MacOS"