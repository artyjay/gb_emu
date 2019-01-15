#!/bin/bash
export EMSCRIPTEN_ROOT=/emsdk_portable/sdk
apt-get update
apt-get install ninja-build
python cmake/scripts/compile_web.py