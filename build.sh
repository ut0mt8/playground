#!/bin/sh
set -eu

mkdir -p .build
cmake -S . -B .build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build .build
printf "\nBuild complete. Install? [y/N] "
read -r -n1 ans
case "$ans" in [yY]|[yY][eE][sS]) sh install.sh;; esac
