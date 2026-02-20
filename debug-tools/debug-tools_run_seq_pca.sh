#!/bin/bash

apptainer exec debug-tools.sif valgrind --version
apptainer exec debug-tools.sif perf --version
apptainer exec debug-tools.sif valgrind --tool=callgrind ../seq_pca ../input/test00.ppm 100