#!/bin/bash

# Script used for converting pictures to PPM P6 format

# To install convert on Ubuntu
sudo apt install imagemagick

CNT=0
DIM=(256 512 1024 2048)

INPUT_DIR="images"
OUTPUT_DIR="input"

mkdir -p $OUTPUT_DIR

for img in $INPUT_DIR/*.jpg; do
    for d in "${DIM[@]}"; do
        output="${OUTPUT_DIR}/test$(printf %02d $CNT)_$(printf %02d $d)x$(printf %02d $d).ppm"

        echo "Converting $(basename $img) to PPM $output."
        convert "$img" -resize $dx$d "$output"
    done
    CNT=$((CNT + 1))
done
