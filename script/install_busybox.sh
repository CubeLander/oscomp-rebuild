#!/bin/bash
ROOT_DIR="$1"
BUSYBOX_DIR="$2"

# Copy bin directory if it exists
if [ -d "$BUSYBOX_DIR/bin" ]; then
    cp -r "$BUSYBOX_DIR/bin"/* "$ROOT_DIR/bin/" || echo "Failed to copy bin directory"
else
    echo "Busybox bin directory not found"
fi

# Copy sbin directory if it exists
if [ -d "$BUSYBOX_DIR/sbin" ]; then
    cp -r "$BUSYBOX_DIR/sbin"/* "$ROOT_DIR/sbin/" || echo "Failed to copy sbin directory"
else
    echo "Busybox sbin directory not found"
fi

# Copy usr/bin directory if it exists
if [ -d "$BUSYBOX_DIR/usr/bin" ]; then
    cp -r "$BUSYBOX_DIR/usr/bin"/* "$ROOT_DIR/usr/bin/" || echo "Failed to copy usr/bin directory"
else
    echo "usr/bin directory not found, skipping"
fi

# Copy usr/sbin directory if it exists
if [ -d "$BUSYBOX_DIR/usr/sbin" ]; then
    cp -r "$BUSYBOX_DIR/usr/sbin"/* "$ROOT_DIR/usr/sbin/" || echo "Failed to copy usr/sbin directory"
else
    echo "usr/sbin directory not found, skipping"
fi
