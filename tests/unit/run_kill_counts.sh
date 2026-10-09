#!/usr/bin/env bash
set -eu
project_root=$(cd "$(dirname "$0")/../.." && pwd)
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
cd "$project_root"
"${CXX:-g++}" -std=c++17 -ffunction-sections -fdata-sections -Isrc -c src/critter.cc -o "$test_dir/critter.o"
"${CXX:-g++}" -std=c++17 -Wl,--gc-sections -Isrc tests/unit/kill_counts.cc "$test_dir/critter.o" -o "$test_dir/kill_counts"
"$test_dir/kill_counts"
printf 'Kill counter tests passed.\n'
