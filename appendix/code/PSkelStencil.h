#ifndef PSKEL_STENCIL_H
#define PSKEL_STENCIL_H

#include "PSkelDefs.h"
#include "PSkelArray.h"
#include "PSkelMask.h"
//#ifdef PSKEL_CUDA
  #include "PSkelStencilTiling.h"
//#endif

namespace PSkel{

//***************************************************
// Stencil Kernels that must be implemented by the users.
//***************************************************

/**
 * Function signature of the stencil kernel for processing 1-dimensional arrays.
 * This function must be implemented by the user.
 * \param[in] input 1-dimensional Array with the input data.
 * \param[in] output 1-dimensional Array that will hold the output data.
 * \param[in] mask 1-dimensional Mask used to scan through the input data,
 * accessing the neighbourhood for each element.
 * \param[in] args extra arguments that may be used by the stencil computations.
 * \param[in] i index for the current element to be processed.
 **/
template<typename T1, typename T2, class Args>
__parallel__ void stencilKernel(Array<T1> input, Array<T1> output, Mask<T2> mask, Args args, size_t i);

/**
 * Function signature of the stencil kernel for processing 2-dimensional arrays.
 * This function must be implemented by the user.
 * \param[in] input 1-dimensional Array with the input data.
 * \param[in] output 1-dimensional Array that will hold the output data.
 * \param[in] mask 1-dimensional Mask used to scan through the input data,
 * accessing the neighbourhood for each element.
 * \param[in] args extra arguments that may be used by the stencil computations.
 * \param[in] h height index for the current element to be processed.
 * \param[in] w width index for the current element to be processed.
 **/
template<typename T1, typename T2, class Args>
__parallel__ void stencilKernel(Array2D<T1> input, Array2D<T1> output, Mask2D<T2> mask, Args args, size_t h, size_t w);

/**
 * Function signature of the stencil kernel for processing 3-dimensional arrays.
 * This function must be implemented by the user.
 * \param[in] input 1-dimensional Array with the input data.
 * \param[in] output 1-dimensional Array that will hold the output data.
 * \param[in] mask 1-dimensional Mask used to scan through the input data,
 * accessing the neighbourhood for each element.
 * \param[in] args extra arguments that may be used by the stencil computations.
 * \param[in] h height index for the current element to be processed.
 * \param[in] w width index for the current element to be processed.
 * \param[in] d depth index for the current element to be processed.
 **/
template<typename T1, typename T2, class Args>
__parallel__ void stencilKernel(Array3D<T1> input, Array3D<T1> output, Mask3D<T2> mask, Args args, size_t h, size_t w, size_t d);

//***************************************************
// Stencil Base
//***************************************************

/**
 * Class that implements the basic functionalities supported by the stencil skeletons.
 **/
template<class Array, class Mask, class Args=int>
class StencilBase{
private:
protected:
	Array input;
	Array output;
	Args args;
	Mask mask;

	#ifndef MPPA_MASTER
	virtual void runSeq(Array in, Array out) = 0;
	#endif

	#ifndef MPPA_MASTER
	// virtual void runOpenMP(Array in, Array out, size_t numThreads) = 0;
  virtual inline __attribute__((always_inline)) void runOpenMP(Array in, Array out, size_t width, size_t height, size_t depth, size_t maskRange, size_t numThreads) = 0;
	#endif

public:
	/**
	 * Executes sequentially in CPU a single iteration of the stencil computation.
	 **/
	#ifndef MPPA_MASTER
	void runSequential();
	#endif

	/**
	 * Executes in CPU, using multithreads, a single iteration of the stencil computation.
	 * \param[in] numThreads the number of threads used for processing the stencil kernel.
	 * if numThreads is 0, the number of threads is automatically chosen.
	 **/
	#ifndef MPPA_MASTER
	void runCPU(size_t numThreads=0);
	#endif

	/**
	 * Executes sequentially in CPU multiple iterations of the stencil computation.
	 * At each given iteration, except the first, the previous output is used as input.
	 * \param[in] iterations the number of iterations to be computed.
	 **/
	#ifndef MPPA_MASTER
	void runIterativeSequential(size_t iterations);
	#endif
	/**
	 * Executes in CPU, using multithreads, multiple iterations of the stencil computation.
	 * At each given iteration, except the first, the previous output is used as input.
	 * \param[in] iterations the number of iterations to be computed.
	 * \param[in] numThreads the number of threads used for processing the stencil kernel.
	 * if numThreads is 0, the number of threads is automatically chosen.
	 **/
	#ifndef MPPA_MASTER
	void runIterativeCPU(size_t iterations, size_t numThreads=0);
	#endif

	#ifdef PSKEL_MPPA
	/**
	 * Spawn the slaves in MPPA.
	 * \param[in] slave_bin_name the name of the slave bynary code.
	 * \param[in] tilingHeight the height for each tile.
	 * \param[in] nb_clusters the number of clusters to be spawn.
	 * \param[in] nb_threads the number of threads per cluster.
	 **/
	void spawn_slaves(const char slave_bin_name[], size_t tilingHeight, size_t tilingWidth, int nb_clusters, int nb_threads, int iterations, int innerIterations);
	#endif

	#ifdef PSKEL_MPPA
	/**
	* Create the slices for MPPA.
	* \param[in] tilingHeight the height for each tile.
	* \param[in] nb_clusters the number of clusters to divide the tiles.
	**/
	void mppaSlice(size_t tilingHeight, size_t tilingWidth, int nb_clusters, int iterations, int innerIterations);
	#endif

	#ifdef PSKEL_MPPA
	/**
	* wait for the slaves to complete.
	* \param[in] nb_clusters the number of clusters to wait.
	**/
	void waitSlaves(int nb_clusters, int tilingHeight, int tilingWidth);
	#endif

	#ifdef PSKEL_MPPA
	/**
	* Configure the slave execution and wait for them to finish.
	* \param[in] slave_bin_name the name of the slave bynary code.
	* \param[in] nb_clusters the number of clusters to be spawn.
	* \param[in] nb_threads the number of threads per cluster.
	* \param[in] tilingHeight the height for each tile.
	* \param[in] iterations the number of iterations for the execution.
	**/
	void scheduleMPPA(const char slave_bin_name[], int nb_clusters, int nb_threads, size_t tilingHeight, size_t tilingWidth, int iterations, int innerIterations);
	#endif

	#ifdef PSKEL_MPPA
	/**
	* Configure the portals for the slave and execute the kernel.
	* \param[in] cluster_id the id of the executing cluster.
	* \param[in] nb_threads the number of threads for the kernel execution.
	* \param[in] nb_tiles the number of tiles for the cluster to execute.
	**/
	void runMPPA(int cluster_id, int nb_threads, int nb_tiles, int outterIterations, int itMod);
	#endif

	#ifdef PSKEL_MPPA
	/**
	*
	* \param[in] cluster_id the id of the executing cluster.
	* \param[in] nb_threads the number of threads for the kernel execution.
	* \param[in] nb_tiles the number of tiles for the cluster to execute.
	* \param[in] iterations the number of iterations for the execution.
	**/
	void runIterativeMPPA(Array in, Array out, int iterations, size_t numThreads);
	#endif
};

//***************************************************
// Stencil 3D
//***************************************************

template<class Array, class Mask, class Args>
class Stencil3D : public StencilBase<Array, Mask, Args>{
protected:
	#ifndef MPPA_MASTER
	void runSeq(Array in, Array out);
	#endif

	#ifndef MPPA_MASTER
	void runOpenMP(Array in, Array out, size_t numThreads);
	#endif

public:
	Stencil3D();
	Stencil3D(Array _input, Array _output, Mask _mask, Args _args);
};

//***************************************************
// Stencil 2D
//***************************************************

template<class Array, class Mask, class Args>
class Stencil2D : public StencilBase<Array, Mask, Args>{
protected:

	#ifndef MPPA_MASTER
	void runSeq(Array in, Array out);
	#endif

	#ifndef MPPA_MASTER
	// void runOpenMP(Array in, Array out, size_t numThreads);
    inline __attribute__((always_inline)) void runOpenMP(Array in, Array out, size_t width, size_t height, size_t depth, size_t maskRange, size_t numThreads);
	#endif

public:
	Stencil2D();
	Stencil2D(Array _input, Array _output, Mask _mask, Args _args);
	Stencil2D(Array _input, Array _output, Mask _mask);
	~Stencil2D();
};

//***************************************************
// Stencil 1D
//***************************************************


template<class Array, class Mask, class Args>
class Stencil: public StencilBase<Array, Mask, Args>{
protected:

	#ifndef MPPA_MASTER
	void runSeq(Array in, Array out);
	#endif

	#ifndef MPPA_MASTER
	void runOpenMP(Array in, Array out, size_t numThreads);
	#endif

public:
	Stencil();
	Stencil(Array _input, Array _output, Mask _mask, Args _args);
};

}

#include "PSkelStencil.hpp"
//#include "PSkelStencilMPPA.hpp"

#endif
