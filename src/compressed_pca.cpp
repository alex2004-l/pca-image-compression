#include <iostream>
#include <fstream>
#include "compressed_pca.hpp"


void save_compressed_pca(const std::string &path, double *projected_matrix, double *eigenvectors,
    double mean_val, uint32_t rows, uint32_t cols, int num_components)
{
    std::ofstream out(path, std::ios::binary);

    if (!out.is_open()) {
        std::cerr << "Could not open " << path << std::endl;
        exit(EXIT_FAILURE);
    }

    out.write(reinterpret_cast<char*>(&rows), sizeof(int));
    out.write(reinterpret_cast<char*>(&cols), sizeof(int));
    out.write(reinterpret_cast<char*>(&num_components), sizeof(int));

    // Write the mean value
    out.write(reinterpret_cast<char*>(&mean_val), sizeof(double));

    // Write the eigenvectors (D × k)
    out.write(reinterpret_cast<char*>(eigenvectors), sizeof(double) * cols * num_components);

    // Write the projected matrix (k)
    out.write(reinterpret_cast<char*>(projected_matrix), sizeof(double) * rows * num_components);
    out.close();
}


void load_compressed_pca(const std::string &path, double **projected_matrix, double **eigenvectors,
    double &mean_val, uint32_t &rows, uint32_t &cols, int &num_components)
{
    std::ifstream in(path, std::ios::binary);

    if (!in.is_open()) {
        std::cerr << "Could not open " << path << std::endl;
        exit(EXIT_FAILURE);
    }
    
    in.read(reinterpret_cast<char*>(&rows), sizeof(int));
    in.read(reinterpret_cast<char*>(&cols), sizeof(int));
    in.read(reinterpret_cast<char*>(&num_components), sizeof(int));

    *eigenvectors     = new double[cols * num_components];
    *projected_matrix = new double[rows * num_components];

     // Read the mean value
    in.read(reinterpret_cast<char*>(&mean_val), sizeof(double));

    // Read the eigenvectors (D × k)
    in.read(reinterpret_cast<char*>(*eigenvectors), sizeof(double) * cols * num_components);

    // Read the projected vector (k)
    in.read(reinterpret_cast<char*>(*projected_matrix), sizeof(double) * rows * num_components);

    in.close();
}
