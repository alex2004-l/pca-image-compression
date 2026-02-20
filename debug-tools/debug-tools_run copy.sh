#!/bin/bash

apptainer exec debug-tools.sif valgrind --version
apptainer exec debug-tools.sif perf --version
apptainer exec debug-tools.sif valgrind --tool=callgrind ../seq_pca ../input/test00.ppm ../output/test00_1 ../output/test00_2.ppm 0 100