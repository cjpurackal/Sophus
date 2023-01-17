#!/bin/bash
set -e

cd super_project
rm clang-tidy-build -rf
mkdir clang-tidy-build
cd clang-tidy-build
CC=clang CXX=clang++ cmake -DSUPER_PROJ_FARM_NG_PROTOS=$BUILD_PROTOS ..
make -j2
cd ../..

# run clang tidy
run-clang-tidy-12 -quiet -p super_project/clang-tidy-build/Sophus-build cpp/sophus/.*\.cpp -fix -style none