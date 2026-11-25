#!/bin/bash

make || exit 1

tmp_file=$(mktemp) || exit 1
trap 'rm -f "$tmp_file"' EXIT

./compiler < "$1" > "$tmp_file"
./ic26int-linux-x86_64 "$tmp_file"
