#!/bin/bash

# Intel VTune MPI Profiling Script for PCA
# This script profiles the MPI PCA implementation using Intel VTune

# Load Intel VTune environment
source /opt/intel/oneapi/vtune/latest/vtune-vars.sh


# Configuration
SRC_DIR="../"
INPUT_FILE="../input/test00_2048x2048.ppm"
INPUT_COMP="50"
OUTPUT_DIR="./vtune_seq_results"
BINARY_NAME="seq_pca"

# Create output directory
mkdir -p $OUTPUT_DIR

echo "=== Intel VTune MPI PCA Profiling ==="
echo "Date: $(date)"
echo "MPI Directory: $SRC_DIR"
echo "Input File: $INPUT_FILE"
echo "Output Directory: $OUTPUT_DIR"
echo "=========================================="

# Build the MPI binary with debugging symbols
echo "Building MPI PCA with profiling flags..."
cd $SRC_DIR

# Add VTune-friendly compilation flags (compatible with GCC)
make clean
g++ -std=c++17 -O2 -g -fno-omit-frame-pointer -fno-inline-functions-called-once \
       -Wall -Wextra -Wno-cast-function-type \
       -I include/eigen-lib -I include/ -I include/spectra/include \
       main.cpp src/sequential.cpp src/compressed_pca.cpp src/ppm_image.cpp \
       -o ${BINARY_NAME}_vtune -DVARIANT_NAME='"seq_pca"'

if [ ! -f "${BINARY_NAME}_vtune" ]; then
    echo "Error: Failed to build MPI PCA for profiling"
    exit 1
fi

cd - > /dev/null

echo "MPI PCA built successfully for VTune profiling"

# Function to run VTune profiling with different analysis types
run_vtune_analysis() {
    local analysis_type=$1
    local description=$2
    local output_name="${OUTPUT_DIR}/vtune_seq_${analysis_type}"
    
    echo ""
    echo "=== Running $description (${np} processes) ==="
    
    # Remove existing results
    rm -rf "$output_name"
    
    # Run VTune analysis
    vtune -collect $analysis_type \
          -result-dir "$output_name" \
          -data-limit=1000 \
          -- $SRC_DIR/${BINARY_NAME}_vtune $INPUT_FILE $INPUT_COMP
    
    if [ $? -eq 0 ]; then
        echo "✓ $description completed successfully"
        echo "  Results saved to: $output_name"
        
        # Generate summary report
        echo "  Generating summary report..."
        vtune -report summary -result-dir "${output_name}."* > "${output_name}_summary.txt"
        
        # Generate hotspot report for performance analysis
        if [ "$analysis_type" == "hotspots" ]; then
            vtune -report hotspots -result-dir "$output_name" -group-by=function \
                  > "${output_name}_hotspots.txt"
        fi
        
    else
        echo "✗ $description failed"
    fi
}

# 1. Hotspots Analysis - Identifies CPU-intensive functions
echo ""
echo "Starting VTune Hotspots Analysis..."
run_vtune_analysis "hotspots" "Hotspots Analysis"

# 2. Memory Access Analysis - Memory bandwidth and cache analysis
echo ""
echo "Starting VTune Memory Access Analysis..."
run_vtune_analysis "memory-access" "Memory Access Analysis"

# Generate comprehensive analysis report
echo ""
echo "=== Generating Comprehensive Analysis Report ==="

REPORT_FILE="${OUTPUT_DIR}/mpi_profiling_report.txt"

cat > $REPORT_FILE << EOF
Intel VTune MPI PCA Profiling Report
==========================================
Generated: $(date)
Binary: $SRC_DIR/${BINARY_NAME}_vtune
Input: $INPUT_FILE

Analysis Types Performed:
1. Hotspots Analysis - CPU performance bottlenecks
2. HPC Performance - MPI communication analysis  
3. Threading Analysis - Concurrency and synchronization
4. Memory Access - Cache and memory bandwidth
5. Scaling Analysis - Performance across process counts

Key Files Generated:
EOF

# List all generated files
for file in ${OUTPUT_DIR}/*; do
    if [ -f "$file" ]; then
        echo "- $(basename $file)" >> $REPORT_FILE
    fi
done

cat >> $REPORT_FILE << EOF

How to View Results:
1. Summary reports: Check *_summary.txt files
2. Detailed analysis: Use VTune GUI
   vtune-gui ${OUTPUT_DIR}/vtune_seq_hotspots_4p
3. Hotspots: Check *_hotspots.txt for function-level analysis
4. MPI Analysis: Check *_mpi_analysis.txt for communication patterns
5. Scaling: Check scaling_summary.txt for performance trends

Recommended Analysis Workflow:
1. Start with hotspots analysis to identify computational bottlenecks
2. Review HPC performance for MPI communication efficiency
3. Check scaling analysis to find optimal process count
4. Use memory access analysis for cache optimization opportunities

EOF

echo "Comprehensive analysis report saved to: $REPORT_FILE"

# Display summary of results
echo ""
echo "=== VTune MPI Profiling Complete ==="
echo "Results directory: $OUTPUT_DIR"
echo "Analysis report: $REPORT_FILE"
echo ""
echo "Quick Results Summary:"
echo "======================"

# Show file sizes as an indicator of successful profiling
ls -la $OUTPUT_DIR/ | head -20

echo ""
echo "To view detailed results:"
echo "1. Text reports: cat ${OUTPUT_DIR}/*_summary.txt"
echo "2. GUI analysis: vtune-gui ${OUTPUT_DIR}/vtune_seq_hotspots_4p"
echo "3. Scaling data: cat ${OUTPUT_DIR}/scaling_summary.txt"

# Cleanup build artifacts
echo ""
echo "Cleaning up build artifacts..."
rm -f $SRC_DIR/${BINARY_NAME}_vtune

echo "VTune MPI profiling completed successfully!" 