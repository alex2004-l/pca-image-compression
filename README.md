# Image compression using Principal Components Analysis

---

## Project Overview

The working principle of using [PCA](https://en.wikipedia.org/wiki/Principal_component_analysis) for image compression is interpreting the image as an n-dimensional dataset and translating it into some k-dimensional space where k < n by detecting linear dependencies between the pixel values and eliminating them. 

To do this we initially convert an image to grayscale so we can work with a singular domain. We can note this image in internal memory as the real number matrix 'A'. We can normalize matrix 'A' to then generate its corresponding [Covariance Matrix](https://en.wikipedia.org/wiki/Covariance_matrix) (noted further down as 'Cov'), which visualizes the relation between every pixel and is going to help us determine what values are linearly independent and which are dependent. Specifically we can achieve this by determining the first 'p' most prevalent principal components with the help of an eigenvalue decomposition. For our implementation we have opted to use the [Lanczos Algorithm](https://en.wikipedia.org/wiki/Lanczos_algorithm), which finds the 'm' most useful eigenvalues and vectors for a given [Hermitian matrix](https://en.wikipedia.org/wiki/Hermitian_matrix).

***Proof of applicability***: Any covariance matrix is implicitly square and symmetric, thus 'Cov' is equal 'Cov'ᵗ. The domain of 'A' and 'Cov' is ℝ, thus the [conjugate transpose](https://en.wikipedia.org/wiki/Conjugate_transpose) (represented as 'X'ᴴ for some matrix X) of 'Cov', 'Cov'ᴴ is equal to 'Cov'ᵗ. Therefore, 'Cov' is equal to 'Cov'ᴴ and thus 'Cov' is Hermitian.

***Time complexity***: The expected time complexity is $O(k * n^3)$, where `k` is the number of components used for compression, and `n` is the matrix dimension.

## Implementations

Implementing PCA involves a sequence of standard linear algebra operations, performed one after the other:
1. *Normalizing the matrix data* : this step ensures that all pixel values contribute equally to the PCA by substracting the mean from all the elements of matrix A
2. *Computing the covariance matrix* : the covariance matrix, used to quantify the different relationships between the different dimensions in the normalized data, is computed as <br>
$Cov=\dfrac{1}{(M−1)} * ​Anorm^T * ​Anorm$
3. *Computing the top k eigenvectors of the covariance matrix* : ​this steps involves computing the top k PCs(principals components)
4. *Projecting the top k eigenvectors on the original data*: final step to obtain the compressed format

| Uncompressed |  15 components | 25 components   |
| -------------| -------------  | --------------- |
| <img src="https://i.ibb.co/vv3yrKhp/img01.jpg" width="300"> | <img src="https://i.ibb.co/cXFKrffW/img2.png" width="300">|<img src="https://i.ibb.co/qZcsc1R/img3.png" width="300"> |

|   35 components | 50 components  | 75 components|
| --------------- | ------------- | -------------  |
| <img src="https://i.ibb.co/YTQ2sR5W/img4.png" width="300"> | <img src="https://i.ibb.co/NdNkWyN6/img5.png" width="300"> | <img src="https://i.ibb.co/FbvCK09M/img6.png" width="300"> |

### I.Sequential implementation

The most substantial challenge encountered during the serial implementation was accurately and efficiently computing the eigenvalues (λ) and eigenvectors (v) of the large N×N covariance matrix 'Cov'.

Unlike high-level technical computing environments such as Python or MATLAB, which provide optimized, built-in functions for this purpose, core C and C++ do not natively support robust linear algebra capabilities like the Eigenvalue Decomposition.

As such, before reaching a final eigenvalue decomposition implementation, multiple approaches were used.

#### I.I. Sequential Eigen implementation

* The first implementation relied solely on the `Eigen library` to perform a full decomposition.
Eigen is a comprehensive, header-only C++ template library for linear algebra, providing a wide array of numerical solvers and matrix operations.
* While this approach was the most straightforward to implement, it possessed a critical limitation: the library's standard functions compute all eigenvalues and eigenvectors of the covariance matrix.
* This resulted in a high memory footprint and excessive computation time, often causing the application to crash or hang when processing images *larger than 375×375 pixels*.

#### I.II. Sequential Spectra implementation

* The second implementation optimized the eigenvalue decomposition by *replacing the dense solver with a selective one*.
* By utilizing the `Spectra` header-only library—a C++ template library built on top of Eigen—the process focuses exclusively on the *top k eigenvalues and eigenvectors*. This prevents the computational waste of calculating the remaining components that are eventually discarded.
* However, profiling revealed a **significant bottleneck**: *the vast majority of execution time was consumed by internal library routines*. As a result, any attempt to parallelize the surrounding code would have a negligible impact on the program performance.

#### I.III. Sequential Lanczos implementation

* The third approach involved a custom implementation of the `Lanczos Algorithm`.
* Unlike the library-based variants, this implementation provides direct control over the iterative process of constructing a tridiagonal matrix T that approximates the extremal eigenvalues of the large N×N covariance matrix.
* This version avoids the shortcomings of the previous versions: it uses less memory than the basic Eigen version and avoids the "black box" problems of the Spectra library.
* The Lanczos implementation is faster than the one using Spectra, even though the images obtained using this algorithm might look worse when the number of principal components is small.

<img src="https://i.ibb.co/Hf0BTYqX/lanczos-vs-spectra.png" alt="lanczos-vs-spectra" width=100%>

### II.OpenMP implementation

* The main focus for the OpenMP implementation was finding which loops to parallelize and how to distribute the work efficiently.
* Initially, we tried to parallelize every `for` loop, but found it counterproductive. The outer k iterations of the `Lanczos algorithm` could not be parallelized due to sequential data dependencies, and parallelizing very small loops introduced more overhead than speedup.
* Ultimately, the best approach was to parallelize loops dependent on the matrix size (N), such as matrix multiplications and dot products.
* After experimenting with different types of scheduling strategies (dynamic, guided, and static), the best and most consistent result was obtained using static scheduling, since the load was uniform.

<img src="https://i.ibb.co/CKzLtqbm/openmp-components-comparison-2048.png" width=100%>
<br>

* ***OpenMP speedup and efficiency***

| Implementation   |   Threads |   Time (s) |   Speedup |   Efficiency (%) |
|:-----------------|----------:|-----------:|----------:|-----------------:|
| OpenMP           |         2 |     31.3   |      1.03 |             51.3 |
| OpenMP           |         4 |     16.458 |      1.95 |             48.8 |
| OpenMP           |         8 |      8.957 |      3.59 |             44.9 |
| OpenMP           |        16 |      5.004 |      6.42 |             40.1 |
| OpenMP           |        32 |      2.733 |     11.76 |             36.8 |



### III. Pthreads implementation

* The Pthreads implementaion was based on the OMP implemenation, using what we learned there to prioritise the correct loops to paralelise.
* To avoid the overhead of repeated thread creation and distruction we implemented a **thread pool class** that allowed us to start several threads at the begining of compression and feed them "tasks" representatives of processing continuous chunks of the matrices or vectors.
* The 'main' thread await the completion of all tasks it starts, which we've achieved using a semaphore that the main thread repeatedly waits on while the tasks release it when they finish processing.

<img src="https://i.ibb.co/84gp4YRK/pthreads-components-comparison-2048.png" width=100%>
<br>

* ***Pthreads speedup and efficiency***

| Implementation   |   Threads |   Time (s) |   Speedup |   Efficiency (%) |
|:-----------------|----------:|-----------:|----------:|-----------------:|
| Pthreads         |         2 |     32.179 |      1    |             49.9 |
| Pthreads         |         4 |     17.093 |      1.88 |             47   |
| Pthreads         |         8 |      9.443 |      3.4  |             42.5 |
| Pthreads         |        16 |      5.433 |      5.92 |             37   |
| Pthreads         |        32 |      3.446 |      9.33 |             29.1 |

* ***Comparison between OpenMP implementation and Pthreads implementation***

<img src="https://i.ibb.co/q35Lhf60/parallel-metrics-1024-100.png" width=100%>
<img src="https://i.ibb.co/ShYZ6s9/parallel-metrics-2048-100.png" width=100%>

### MPI implementation

* For the multi-process part, our team opted to use MPI `to split` a given compression
problem into a number of smaller problems that can be handled in parallel by different
processes.
* We propose splitting a given image into a grid of sub-blocks, such as a `2x2, 3x3` or `4x4`.
* To achieve this, we split the world communicator into several `sub-communicators`:
the coordinator communicator, which allows the processes corresponding to the
first column of the grid to communicate and a a series of coordinated communicators,
one for connecting every process that 'belongs' to a given row of the grid.
* For a grid size `NxN`, (Rank / N, Rank % N) determines the block managed by the given Rank process.
* The `Root node` is responsible for the `I/O` of reading the image, and will always be Rank 0.
* The Root scatters to all processes that manage blocks on its own column (Rank % N == 0)
an approximately equal number of lines of bytes, dividing the image into N rectangle sub-images.
* Each of these coordinator processors then scatters the rectangular image across its own row,
further dividing each rectangle into another N rectangles. After this distribution,
every process performs the sequential implementation of Lanczos on its own image fragment.
* For image reconstitution, `each process decompresses` a block of the image, passes it back
to its coordinator through a gather. Every coordinator recomposes the rectangular block it
initially should've had and passes it back to the root through another gather.
* The Root is then responsible for the I/O of writing the decompressed image.

For the table down below, each process compressed their part of the original image with the number of components mentioned.

| Uncompressed |  1 process, 20 components | 4 processes, 20 components  |
| -------------| -------------  | --------------- |
| <img src="https://i.ibb.co/vv3yrKhp/img01.jpg" width="300"> | <img src="https://i.ibb.co/zhGx3Sxb/mpi-1.jpg" width="300">|<img src="https://i.ibb.co/27Z90qJQ/mpi-2.jpg" width="300"> |

|   9 processes, 20 components | 16 processes, 15 components  | 25 processes, 15 components|
| --------------- | ------------- | -------------  |
| <img src="https://i.ibb.co/wZ0YZ9fc/mpi-3.jpg" width="300"> | <img src="https://i.ibb.co/5h5gTcY7/mpi-4.jpg" width="300"> | <img src="https://i.ibb.co/TM3VXXk7/mpi-5.jpg" width="300"> |

### __Speedup and Efficiency__
| Implementation   |   Processes |   Time (s) |   Speedup |   Efficiency (%) |
|:-----------------|------------:|-----------:|----------:|-----------------:|
| MPI              |           4 |   8.60939  |   3.72117 |          93.0292 |
| MPI              |           9 |   2.94754  |  10.8691  |         120.767  |
| MPI              |          16 |   1.42261  |  22.5199  |         140.749  |
| MPI              |          25 |   0.771419 |  41.53    |         166.12   |

#### __Time evolution comparison based on the number of components__

<img src="https://i.ibb.co/23ys53JC/mpi-timp-executie.png" width=100%>

#### __Scalability based on the image dimension__

<img src="https://i.ibb.co/dJ7bKky9/mpi-multidim-scaling-k25.png" width=100%>

### Hybrid OpenMP-MPI and Pthreads-MPI implementation

* For the hybrid implementations, we leveraged the construction of our solutions.
* The MPI implementation relied on a different main configuration, but could use
any implementation for the effective image compression of the determined sub-images.
* The 2 compression implementations are perfectly workable to use even on smaller images, in fact,
it works exponentially faster due to the nature of the complex matrix multiplications
involved in eigenvalue determination.
* Therefore, `achieving a hybrid implementation` is as
easy as `linking the MPI Implementation Main with either the OMP Implementation Library or the Pthreads Implementation Library` 
instead of the sequential version.

<img src="https://i.ibb.co/v6pmJZ0L/plot-lines-hybrid-openmp.png" width=100%>
<img src="https://i.ibb.co/7tHLK7dV/plot-lines-hybrid-pthreads.png" width=100%>

## Profiling Notes:

For profiling, the data being depicted will correspond to a 2048x2048 resolution image. Initially our team tried using 1024x1024, but the smaller output failed to represent the speed-up gained in a significant manner while also determining a high percentage of runtime (close to 10%) being dedicated to MPI's Init overhead.

### Serial Profiling

For the part of Serial Profiling, we are looking at a single-process, single-thread solution, which represent a baseline of comparison. In this instance, our top consuming hotspots are as follows:

<img src="https://i.ibb.co/v48yHXVy/serial-1.png" width=80%>

The histogram representation of CPU utilization is the following:

<img src="https://i.ibb.co/rKcv1xGY/serial-2.png" width=80%>

### OMP Profiling

For the part of OMP profiling, we are looking at a single-process, multi-thread solution. We will be presenting the data yielded by running it with 4, 8, 16 and 32 threads.
- 4 threads <br>

<img src="https://i.ibb.co/jvL5jvvG/omp1.png" width=80%>
<img src="https://i.ibb.co/G4tRH1zX/omp2.png" width=80%>

- 8 threads <br>
<img src="https://i.ibb.co/JFH36kZp/omp3.png" width=80%>
<img src="https://i.ibb.co/bjgMTKpb/omp4.png" width=80%>

- 16 threads <br>
<img src="https://i.ibb.co/GvNC0TYV/omp5.png" width=80%>
<img src="https://i.ibb.co/2YyDHTGm/omp6.png" width=80%>

- 32 threads <br>
<img src="https://i.ibb.co/r2Y71n8R/omp7.png" width=80%>
<img src="https://i.ibb.co/4RkCPX65/omp8.png" width=80%>

Another way to represent this data would be:
| metric |    4   |    8   |   16   |   32   |
| ------ | ------ | ------ | ------ | ------ |
|CPU Time|    119.070    |    135.090    |   144.010     |   198.040     |
|Real Time|    30.818    |     17.597   |     9.757   |    7.543    |


In all variants, multiply_matrices is the overwhelming bottleneck, accounting for 99.4% to 98.8% of the total CPU time. This confirms that the matrix-vector products within the Lanczos iterations are the primary targets for optimization.
Even though the "CPU Time" number in VTune looks higher with more threads, the Elapsed Time (the actual time I waited for the program to finish) dropped significantly. It went from about 30 seconds with 4 threads down to under 8 seconds with 32 threads.


### MPI Profiling

For the part of MPI profiling, we are looking at a multi-process, single-thread solution. We will be presenting the data yielded by running it with 4, 9 and 16 processes, as perfect square number are preferred for the grid distribution of the MPI implementation.

- 4 procs <br>

<img src="https://i.ibb.co/1Jv8pC8t/mpi1.png" width=80%>
<img src="https://i.ibb.co/JWzQ1DzW/mpi2.png" width=80%>

- 9 procs <br>

<img src="https://i.ibb.co/YsfmZ6s/mpi3.png" width=80%>
<img src="https://i.ibb.co/KcYNdCh6/mpi4.png" width=80%>

- 16 procs <br>

<img src="https://i.ibb.co/mZSPPKS/mpi5.png" width=80%>
<img src="https://i.ibb.co/KcPzDm1v/mpi6.png" width=80%>


|Metric | 4| 9| 16|
| ------ | ------ | ------ | ------ |
|   CPU Time     |   10.170     |   8.150     |   11.990     |
|  Real Time      |    3.622    |  2.333      |    2.505    |

We can observe that as the number of processes increases, the elapsed time does admittedly go down, though the gains aren't extremely significant compared the initial 4 processor split. Further, as we increase the number of processors used, the communication overhead of using the MPI functions increases dramatically, to the point of representing over 30% of the runtime. The CPU utilization histograms also suggest that the greatest level of concurrence is achieved when using 4 processes as opposed to more. The elapsed, real time values suggest that an increase in processes is not strictly optimal, as 16 processes show a worse performance than 9 processes. 

### MPI-OMP Hybrid Profiling

For the part of MPI-OMP Hybrid profiling, we are looking at a multi-process, multi-thread solution. Considering the above observations, as well as the cluster's limit of CPUs, we will be presenting the data yielded by running 4 processes and 8 threads respectively 16 processes and 2 threads

<img src="https://i.ibb.co/9944fX7k/hyb1.png" width=80%>
<img src="https://i.ibb.co/1G6MvBRY/hyb2.png" width=100%>

Although the CPU utilization histogram looks somewhat not preferable, it makes complete sense considering the structure of the program. Until the image has been read, split into blocks and distributed, 28 of the CPU are not doing anything, hence a large bar of the Idle section. The single-CPU bar likely corresponds to the Root processor's I/O. The two-CPU bar likely corresponds to the distribution of the initial image into coordinator partitions. The three-CPU and four-CPU bars likely correspond to the distribution of the coordinator partitions into coordinated partitions. After that we have minimal activity that isn't represented in the full 32-CPU activation, which shows that the program is running smoothly and as intended.

<img src="https://i.ibb.co/5P6d2f9/hyb3.png" width=80%>
<img src="https://i.ibb.co/wrPQK61H/hyb4.png" width=100%>

The 16/2 results are much less aligned with our intentions, suffering from a greater MPI overhead and never achieving full 32-CPU activation. Where the pure 16 process implementation of just MPI did beat out the 4 process version of just MPI in time even with lesser concurrence, but when threads are involved, 4/8 achieved a time 27% lower than 16/2, strongly suggesting it is the better of the 2 options.
## Results

### Time heatmap
<img src="https://i.ibb.co/5gpVNfYr/time-heatmap-2048.png" width=100%>

### Speedup heatmap
<img src="https://i.ibb.co/278L782q/speedup-heatmap-2048.png" width=100%>

## Timeline

Week 17 Nov - 23 Nov

Sequential implementation using Eigen.
This implementation work for images up to 350x350.

Week 24 Nov - 30 Nov

Start OpenMP and MPI implementations.
Use Spectra on top of Eigen for speedup. Now, the program supports images up to 1024x1024, even bigger, depending on the number of components.

Week 1 Dec - 7 Dec

Make hybrid Lanczos and Eigen implementation (to have something worth parallelizing).
OpenMP implementation almost finished.

Week 8 Dec - 14 Dec

Finish OpenMP and MPI implementations.
Hybrid implementation done.

Week 15 Dec - 19 Dec

Add profiling with VTune and basics plots.
Improve matrix multiplication by switching the order of loops.
Add pthreads implementation using threadpool.

Week 12 Jan - 16 Jan

Improve plots
Make presentation
