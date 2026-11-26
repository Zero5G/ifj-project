#!/bin/bash

make || exit 1

tmp_file=$(mktemp) || exit 1
trap 'rm -f "$tmp_file"' EXIT

if ! ./compiler < "$1" > "$tmp_file"; then
    err=$?
    echo "Compiler failed with exit code: $err" >&2
    exit "$err"
fi

./ic26int-linux-x86_64 "$tmp_file"
