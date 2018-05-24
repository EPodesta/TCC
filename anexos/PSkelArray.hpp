#ifndef PSKEL_ARRAY_HPP
#define PSKEL_ARRAY_HPP
#include <cstring>
#include <iostream>

#ifndef MPPA_MASTER
#include <omp.h>
#endif

// Maximum size of the cluster
#define KB 1024
#define MB 1024 * KB
// maximum size of a message
#define MAX_CLUSTER_SIZE 1 * MB + MB / 2

//Change how to read and write in the array for later use for a bunch of clusters
namespace PSkel{

template<typename T>
ArrayBase<T>::ArrayBase(size_t width, size_t height, size_t depth){
	this->width = width;
	this->height = height;
	this->depth = depth;
	this->realWidth = width;
	this->realHeight = height;
	this->realDepth = depth;
	this->widthOffset = 0;
	this->heightOffset = 0;
	this->depthOffset = 0;
	this->hostArray = 0;
	#ifdef PSKEL_CUDA
		this->deviceArray = 0;
	#endif
	#ifdef PSKEL_MPPA
	this->mppaArray = 0;
	this->write_portal = 0;
	this->read_portal = 0;
	this->aux_write_portal = 0;
	this->aux_read_portal = 0;
	this->aux = (int *) malloc(sizeof(size_t*) * 16);
	if(size()>0) this->mppaAlloc();
	#else
		if(size()>0) this->hostAlloc();
	#endif
}

template<typename T>
void ArrayBase<T>::hostAlloc(size_t width, size_t height, size_t depth){
	this->width = width;
	this->height = height;
	this->depth = depth;
	this->realWidth = width;
	this->realHeight = height;
	this->realDepth = depth;
	this->widthOffset = 0;
	this->heightOffset = 0;
	this->depthOffset = 0;
	this->hostArray = NULL;
	#ifdef PSKEL_CUDA
	this->deviceArray = NULL;
	#endif

	this->hostAlloc();
}

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::mppaAlloc(){
	if(this->mppaArray==NULL){
		this->mppaArray = (T*) calloc(this->size(), sizeof(T));
		assert(this->mppaArray!= NULL);
		#if defined (DEBUG) || defined (BUG_TEST)
			#ifdef MPPA_MASTER
				std::cout<<"MASTER: Allocating "<<this->size()*sizeof(T)<<" bytes starting on address "<<&(this->mppaArray)<<std::endl;
			#else
				std::cout<<"SLAVE: Allocating "<<this->size()*sizeof(T)<<" bytes starting on address "<<&(this->mppaArray)<<std::endl;
			#endif
		#endif
	}
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::mppaAlloc(size_t width, size_t height, size_t depth){
	this->width = width;
	this->height = height;
	this->depth = depth;
	this->realWidth = width;
	this->realHeight = height;
	this->realDepth = depth;
	this->widthOffset = 0;
	this->heightOffset = 0;
	this->depthOffset = 0;

	this->mppaArray = NULL;
	this->mppaAlloc();
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::mppaFree(){
	//if(this->mppaArray!=NULL){
	#if defined (DEBUG) || defined (BUG_TEST)
		#ifdef MPPA_MASTER
			std::cout<<"MASTER: Deallocating "<<this->size()*sizeof(T)<<" bytes of address "<<&(this->mppaArray)<<std::endl;
		#else
			std::cout<<"SLAVE: Deallocating "<<this->size()*sizeof(T)<<" bytes of address "<<&(this->mppaArray)<<std::endl;
		#endif
	#endif
	free(this->mppaArray);
	this->mppaArray = NULL;
	//}
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::auxFree(){

	free(this->aux);
	this->aux = NULL;
}
#endif

template<typename T>
void ArrayBase<T>::hostAlloc(){
	if(this->hostArray==NULL){
		this->hostArray = (T*) calloc(size(), sizeof(T));
		assert(this->hostArray != NULL);
		//gpuErrchk( cudaMallocHost((void**)&hostArray, size()*sizeof(T)) );
		//memset(this->hostArray, 0, size()*sizeof(T));
	}
}

template<typename T>
void ArrayBase<T>::hostFree(){

	free(this->hostArray);

	this->hostArray = NULL;

}

template<typename T>
size_t ArrayBase<T>::getWidth() const{
	return width;
}

template<typename T>
size_t ArrayBase<T>::getHeight() const{
	return height;
}

template<typename T>
size_t ArrayBase<T>::getDepth() const{
	return depth;
}

template<typename T>
size_t ArrayBase<T>::memSize() const{
	return size()*sizeof(T);
}

template<typename T>
size_t ArrayBase<T>::size() const{
	return height*width*depth;
}

template<typename T>
size_t ArrayBase<T>::realSize() const{
	return realHeight*realWidth*realDepth;
}

template<typename T>
T & ArrayBase<T>::hostGet(size_t h, size_t w, size_t d) const {
	return this->hostArray[ ((h+heightOffset)*realWidth + (w+widthOffset))*realDepth + (d+depthOffset) ];
}

#ifdef PSKEL_MPPA
template<typename T>
T& ArrayBase<T>::mppaGet(size_t h, size_t w, size_t d) const {
	return this->mppaArray[ ((h+heightOffset)*realWidth + (w+widthOffset))*realDepth + (d+depthOffset) ];
}
#endif

template<typename T> template<typename Arrays>
void ArrayBase<T>::hostSlice(Arrays array, size_t widthOffset, size_t heightOffset, size_t depthOffset, size_t width, size_t height, size_t depth){
	//maintain previous allocated area
	#ifdef PSKEL_CUDA
	if(this->deviceArray!=NULL){
		if(this->size()!=(width*height*depth)){
			this->deviceFree();
			this->deviceArray = NULL;
		}
	}
	#endif

	this->width = width;
	this->height = height;
	this->depth = depth;
	this->widthOffset = array.widthOffset+widthOffset;
	this->heightOffset = array.heightOffset+heightOffset;
	this->depthOffset = array.depthOffset+depthOffset;
	this->realWidth = array.realWidth;
	this->realHeight = array.realHeight;
	this->realDepth = array.realDepth;
    std::cout << "Print barrier for management: " << std::endl;
    // std::cout << "ArrayWidthOffset: " << array.widthOffset << "ArrayHeightOffset: " << array.heightOffset <<std::endl;
	#ifdef MPPA_MASTER

	#endif
	#ifndef PSKEL_MPPA
	this->hostArray = array.hostArray;
	#else
	this->mppaArray = array.mppaArray;
	#endif
}

template<typename T> template<typename Arrays>
void ArrayBase<T>::hostClone(Arrays array){
	//Copy dimensions
	this->width = array.width;
	this->height = array.height;
	this->depth = array.depth;
	this->widthOffset = 0;
	this->heightOffset = 0;
	this->depthOffset = 0;
	this->realWidth = array.width;
	this->realHeight = array.height;
	this->realDepth = array.depth;
	//Alloc clone memory
	this->hostArray = NULL;
	this->hostAlloc();
	//Copy clone memory
	this->hostMemCopy(array);
}
#ifndef MPPA_MASTER
template<typename T> template<typename Arrays>
void ArrayBase<T>::mppaClone(Arrays array){
	//Copy dimensions
	this->width = array.width;
	this->height = array.height;
	this->depth = array.depth;
	this->widthOffset = 0;
	this->heightOffset = 0;
	this->depthOffset = 0;
	this->realWidth = array.width;
	this->realHeight = array.height;
	this->realDepth = array.depth;
	//Alloc clone memory
	this->mppaArray = NULL;
	this->mppaAlloc();
	//Copy clone memory
	//#ifdef MPPA_SLAVE
	this->mppaMemCopy(array);
	//#endif
}
#endif

#ifdef PSKEL_MPPA
template<typename T> template<typename Arrays>
void ArrayBase<T>::mppaMasterClone(Arrays array){
	//Copy dimensions
	this->width = array.width;
	this->height = array.height;
	this->depth = array.depth;
	this->widthOffset = 0;
	this->heightOffset = 0;
	this->depthOffset = 0;
	this->realWidth = array.width;
	this->realHeight = array.height;
	this->realDepth = array.depth;
	//Alloc clone memory
	this->mppaArray = NULL;
	this->mppaAlloc();
	//Copy clone memory
	//#ifdef MPPA_SLAVE
	this->mppaMasterCopy(array);
	//#endif
}
#endif


#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::setTrigger(int trigger){
    mppa_aiocb_set_trigger(&read_portal->aiocb, trigger);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
int* ArrayBase<T>::getAux(){
	return this->aux;
}
#endif

// #ifdef PSKEL_MPPA
// template<typename T>
// void ArrayBase<T>::auxAlloc(){
// 	this->aux = (int *) malloc(sizeof(size_t*) * 13);
// 	assert(this->aux != NULL);
// }
// #endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::setAux(int heightOffset, int widthOffset, int it, int subIterations, size_t coreWidthOffset, size_t coreHeightOffset, size_t coreDepthOffset, size_t coreWidth, size_t coreHeight, size_t coreDepth, int outterIterations, size_t height, size_t width, size_t depth, int baseWidth, int baseHeight){

    this->aux[0] = heightOffset;
	this->aux[1] = it;
	this->aux[2] = subIterations;
	this->aux[3] = coreWidthOffset;
	this->aux[4] = coreHeightOffset;
	this->aux[5] = coreDepthOffset;
	this->aux[6] = coreWidth;
	this->aux[7] = coreHeight;
	this->aux[8] = coreDepth;
	this->aux[9] = outterIterations;
	this->aux[10] = height;
	this->aux[11] = width;
	this->aux[12] = depth;
    this->aux[13] = widthOffset;
    this->aux[14] = baseWidth;
    this->aux[15] = baseHeight;


}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::copyToAux(){
	mppa_async_write_portal(this->aux_write_portal, this->aux, (sizeof(int) * 16), 0);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::copyFromAux(){
	mppa_async_read_wait_portal(this->aux_read_portal);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::copyToo(size_t offsetSlave, size_t offsetMaster, int tam){
    tam = tam*sizeof(T);
	T *mppaSlicePtr = (T*)(this->mppaArray) + size_t(offsetSlave*realDepth);
	mppa_async_write_portal(this->write_portal, mppaSlicePtr, tam, sizeof(T)*offsetMaster);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::copyTo(int coreHeight, int coreWidth, size_t sJump, size_t tJump, size_t sOffset, size_t tOffset){
	// T *mppaSlicePtr = &((this->mppaArray) + size_t(sOffset*realDepth));
    T *mppaSlicePtr = (T*)(this->mppaArray) + size_t(sOffset*realDepth);


	int sstride = sJump*sizeof(T);
	int tstride = tJump*sizeof(T);
	int ecount = coreHeight;
	int size = coreWidth*sizeof(T);

	mppa_async_write_stride_portal(this->write_portal, mppaSlicePtr, size, ecount, sstride, tstride, sizeof(T)*tOffset);

}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::copyTo(){
	T *mppaSlicePtr = (T*)(this->mppaArray);
	mppa_async_write_portal(this->write_portal, mppaSlicePtr, this->memSize(), 0);


}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::copyFrom(){
	mppa_async_read_wait_portal(this->read_portal);

}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::portalReadAlloc(int trigger, int nb_cluster){
	#ifdef MPPA_MASTER
		char pathMaster[256];
	    sprintf(pathMaster, "/mppa/portal/%d:3", 128);
		this->read_portal = mppa_create_read_portal(pathMaster, this->mppaArray, this->memSize(), trigger, NULL);
	#endif
	#ifdef MPPA_SLAVE
	char pathSlave[25];
	char path[25];
	sprintf(pathSlave, "/mppa/portal/%d:%d", nb_cluster, 4 + nb_cluster);

    this->read_portal = mppa_create_read_portal(pathSlave, this->mppaArray, this->memSize(), trigger, NULL);
	#endif
}
#endif


#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::portalAuxWriteAlloc(int nb_cluster){
	char path[256];
	#ifdef MPPA_MASTER
		sprintf(path, "/mppa/portal/%d:%d", nb_cluster, (21 + nb_cluster));
    	this->aux_write_portal = mppa_create_write_portal(path, NULL, 0, nb_cluster);
	#endif
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::portalAuxReadAlloc(int trigger, int nb_cluster){
	char path[25];
	#ifdef MPPA_SLAVE
		sprintf(path, "/mppa/portal/%d:%d", nb_cluster, (21 + nb_cluster));
    	this->aux_read_portal = mppa_create_read_portal(path, this->aux, (sizeof(int) * 16), trigger, NULL);
	#endif
}
#endif


#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::portalWriteAlloc(int nb_cluster){
	char path[256];
	#ifdef MPPA_MASTER
		sprintf(path, "/mppa/portal/%d:%d", nb_cluster, 4 + nb_cluster);
    	this->write_portal = mppa_create_write_portal(path, NULL, 0, nb_cluster);
	#endif
    #ifdef MPPA_SLAVE
		sprintf(path, "/mppa/portal/%d:3", 128);
    	this->write_portal = mppa_create_write_portal(path, NULL, 0, 128);
    #endif
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::waitRead(){
	mppa_async_read_wait_portal(this->read_portal);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::waitWrite(){
	mppa_async_write_wait_portal(this->write_portal);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::waitAuxWrite(){
	mppa_async_write_wait_portal(this->aux_write_portal);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::closeReadPortal(){
	mppa_close_portal(this->read_portal);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::closeAuxReadPortal(){
	mppa_close_portal(this->aux_read_portal);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::closeAuxWritePortal(){
	mppa_close_portal(this->aux_write_portal);
}
#endif

#ifdef PSKEL_MPPA
template<typename T>
void ArrayBase<T>::closeWritePortal(){
	mppa_close_portal(this->write_portal);
}
#endif

#ifndef MPPA_MASTER
template<typename T> template<typename Arrays>
void ArrayBase<T>::hostMemCopy(Arrays array){
	if(array.size()==array.realSize() && this->size()==this->realSize()){
		memcpy(this->hostArray, array.hostArray, size()*sizeof(T));
	}else{
		#pragma omp parallel for
		for(size_t i = 0; i<height; ++i){
		for(size_t j = 0; j<width; ++j){
		for(size_t k = 0; k<depth; ++k){
                        this->hostGet(i,j,k)=array.hostGet(i,j,k);
		}}}
	}
}
#endif

template<typename T> template<typename Arrays>
void ArrayBase<T>::mppaMasterCopy(Arrays array){
	if(array.size()==array.realSize() && this->size()==this->realSize()){
		#ifdef DEBUG
			std::cout<<"MASTER: Copying memory from address "<<&(array.mppaArray)<<" to address "<<&(this->mppaArray)<<std::endl;
		#endif
		memcpy(this->mppaArray, array.mppaArray, size()*sizeof(T));
	}
	else{
		#ifdef DEBUG
			std::cout<<"MASTER: Copying element-by-element from address "<<&(array.mppaArray)<<" to address "<<&(this->mppaArray)<<std::endl;
		#endif
		for(size_t i = 0; i<height; ++i){
			for(size_t j = 0; j<width; ++j){
				for(size_t k = 0; k<depth; ++k){
                    this->mppaGet(i,j,k) = array.mppaGet(i,j,k);
		}}}
	}
}


#ifndef MPPA_MASTER
template<typename T> template<typename Arrays>
void ArrayBase<T>::mppaMemCopy(Arrays array){
	if(array.size()==array.realSize() && this->size()==this->realSize()){
		#ifdef DEBUG
			std::cout<<"SLAVE: Copying memory from address "<<&(array.mppaArray)<<" to address "<<&(this->mppaArray)<<std::endl;
		#endif
		memcpy(this->mppaArray, array.mppaArray, size()*sizeof(T));
	}else{
		#ifdef DEBUG
			std::cout<<"SLAVE: Copying element-by-element from address "<<&(array.mppaArray)<<" to address "<<&(this->mppaArray)<<std::endl;
		#endif
		#pragma omp parallel for
		for(size_t i = 0; i<height; ++i){
			for(size_t j = 0; j<width; ++j){
				for(size_t k = 0; k<depth; ++k){
                    this->mppaGet(i,j,k) = array.mppaGet(i,j,k);
		}}}
	}
}
#endif

template<typename T>
ArrayBase<T>::operator bool() const {
	return(this->hostArray!=NULL);
}

//****************************************************
// Array 3D
//****************************************************

template<typename T>
Array3D<T>::Array3D() : ArrayBase<T>(0,0,0) {}

/*
~Array3D(){
free(hostArray);
cudaFree(deviceArray);
}*/

template<typename T>
Array3D<T>::Array3D(size_t width, size_t height, size_t depth) : ArrayBase<T>(width,height,depth){}

template<typename T>
T & Array3D<T>::operator()(size_t h,size_t w,size_t d) const {
	#ifdef PSKEL_MPPA
		return this->mppaGet(h,w,d);
	#else
		return this->hostGet(h,w,d);
	#endif
}

//****************************************************
// Array 2D
//****************************************************

template<typename T>
Array2D<T>::Array2D() : ArrayBase<T>(0,0,0) {}

//template<typename T>
// Array2D<T>::~Array2D(){
// 	this->mppaFree();
// }

template<typename T>
Array2D<T>::Array2D(size_t width, size_t height) : ArrayBase<T>(width,height,1){}

template<typename T>
T & Array2D<T>::operator()(size_t h, size_t w) const {
	#ifdef PSKEL_MPPA
		return this->mppaGet(h,w,0);
	#else
		return this->hostGet(h,w,0);
	#endif
}

//******************************************************
// Array 1D
//******************************************************

template<typename T>
Array<T>::Array() : ArrayBase<T>(0,0,0){}

template<typename T>
Array<T>::Array(size_t size) : ArrayBase<T>(size,1,1){}

template<typename T>
T & Array<T>::operator()(size_t w) const {
	#ifdef PSKEL_MPPA
		return this->mppaGet(0,w,0);
	#else
		return this->hostGet(0,w,0);
	#endif
}

}//end namespace
#endif
