#ifndef PSKEL_STENCIL_HPP
#define PSKEL_STENCIL_HPP
//#define BARRIER_SYNC_MASTER "/mppa/sync/128:1"
//#define BARRIER_SYNC_SLAVE "/mppa/sync/[0..15]:2"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "hr_time.h"

#include <iostream>
#include <unistd.h>

using namespace std;
#ifdef PSKEL_CUDA
#include <ga/ga.h>
#include <ga/std_stream.h>
#endif

#define ARGC_SLAVE 11

namespace PSkel{

    //***********************************************
    // Stencil Base
    //***********************************************
#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::runSequential(){
            this->runSeq(this->input, this->output);
        }
#endif

    //***********************************************
    // MPPA
    //***********************************************

#ifdef PSKEL_MPPA
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::spawn_slaves(const char slave_bin_name[], size_t tilingHeight, size_t tilingWidth, int nb_clusters, int nb_threads, int iterations, int innerIterations){
            // Prepare arguments to send to slaves
            int i;
            int cluster_id;
            int pid;

            size_t wTiling = ceil(float(this->input.getWidth())/float(tilingWidth));
            size_t hTiling = ceil(float(this->input.getHeight())/float(tilingHeight));
            size_t totalSize = float(hTiling*wTiling);

            int tiles = totalSize/nb_clusters;

            int itMod = totalSize % nb_clusters;

            int tilesSlave;
            int r;
            int outterIterations = ceil(float(iterations)/innerIterations);


#ifdef DEBUG
            cout<<"MASTER: width="<<this->input.getWidth()<<" height="<<this->input.getWidth();
            cout<<" tilingHeight="<<tilingHeight <<" iterations="<<iterations;
            cout<<" inner_iterations="<<innerIterations<<" nbclusters="<<nb_clusters<<" nbthreads="<<nb_threads<<endl;
            cout<<"MASTER: tiles="<<tiles<<" itMod="<<itMod<<" outterIterations="<<outterIterations<<endl;
#endif

            char **argv_slave = (char**) malloc(sizeof (char*) * ARGC_SLAVE);
            for (i = 0; i < ARGC_SLAVE - 1; i++)
                argv_slave[i] = (char*) malloc (sizeof (char) * 11);

            sprintf(argv_slave[1], "%d", tilingWidth);
            sprintf(argv_slave[2], "%d", tilingHeight);
            sprintf(argv_slave[4], "%d", nb_threads);
            sprintf(argv_slave[5], "%d", iterations);
            sprintf(argv_slave[6], "%d", outterIterations);
            sprintf(argv_slave[7], "%d", itMod);
            sprintf(argv_slave[8], "%d", this->input.getHeight());
            sprintf(argv_slave[9], "%d", this->input.getHeight());

            argv_slave[10] = NULL;


            // Spawn slave processes
            for (cluster_id = 0; cluster_id < nb_clusters && cluster_id < totalSize; cluster_id++) {
                r = (cluster_id < itMod)?1:0;
                tilesSlave = tiles + r;

                sprintf(argv_slave[0], "%d", tilesSlave);
                sprintf(argv_slave[3], "%d", cluster_id);
                pid = mppa_spawn(cluster_id, NULL, slave_bin_name, (const char **)argv_slave, NULL);
                assert(pid >= 0);
            }
            for (i = 0; i < ARGC_SLAVE; i++)
                free(argv_slave[i]);
            free(argv_slave);
        }
#endif


#ifdef PSKEL_MPPA
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::mppaSlice(size_t tilingHeight, size_t tilingWidth, int nb_clusters, int iterations, int innerIterations) {

            size_t wTiling = ceil(float(this->input.getWidth())/float(tilingWidth));
            size_t hTiling = ceil(float(this->input.getHeight())/float(tilingHeight));
            size_t totalSize = float(hTiling*wTiling);
            int tiles = totalSize/nb_clusters;
            int itMod = totalSize % nb_clusters;



            size_t outterIterations;
            size_t heightOffset;
            size_t widthOffset;

            StencilTiling<Array, Mask>tiling(this->input, this->output, this->mask);

            barrier_t *barrierNbClusters;
            outterIterations = ceil(float(iterations)/innerIterations);
            Array inputCopy(this->input.getWidth(),this->input.getHeight());
            this->output.portalReadAlloc(nb_clusters, 0);



            for(size_t it = 0; it<outterIterations; it++){
                size_t subIterations = innerIterations;
                if(((it+1)*innerIterations)>iterations){
                    subIterations = iterations-(it*innerIterations);
                }

                if(tiles == 0) {
                    Array slice[totalSize];
                    Array outputNumberHt[totalSize];
                    Array tmp;
                    ////////////////////////////////Number of clusters are higher//////////////
                    barrierNbClusters = mppa_create_master_barrier(BARRIER_SYNC_MASTER, BARRIER_SYNC_SLAVE, totalSize);

                    mppa_barrier_wait(barrierNbClusters);

                    for (int i = 0; i < totalSize; i++) {
                        outputNumberHt[i].portalAuxWriteAlloc(i);
                    }

                    mppa_barrier_wait(barrierNbClusters);

                    int tS = 0;
                    for (int ht = 0; ht < hTiling; ht++) {
                        for (int wt = 0; wt < wTiling; wt++) {
                            heightOffset = ht*tilingHeight;
                            widthOffset = wt*tilingWidth;
                            tiling.tile(subIterations, widthOffset, heightOffset, 0, tilingWidth, tilingHeight, 1);
                            outputNumberHt[tS].hostSlice(tiling.input, tiling.widthOffset, tiling.heightOffset, tiling.depthOffset, tiling.width, tiling.height, tiling.depth);
                            outputNumberHt[tS].setAux(heightOffset, widthOffset, it, subIterations, tiling.coreWidthOffset, tiling.coreHeightOffset, tiling.coreDepthOffset, tiling.coreWidth, tiling.coreHeight, tiling.coreDepth, outterIterations, tiling.height, tiling.width, tiling.depth, this->input.getWidth(), this->input.getHeight());
                            // cout << "Tiling Height Offset: " << tiling.heightOffset << "Tiling Width Offset: " << tiling.widthOffset << "Tiling Core Height Offset: " << tiling.coreHeightOffset << "Tiling Core Width Offset: " << tiling.coreHeightOffset << endl;
                            tS++;
                        }
                    }

                    for (int i = 0; i < totalSize; i++) {
                        outputNumberHt[i].copyToAux();
                    }
                    for (int i = 0; i < totalSize; i++) {
                        outputNumberHt[i].waitAuxWrite();
                    }

                    for (int i = 0; i < totalSize; i++) {
                        outputNumberHt[i].closeAuxWritePortal();
                    }

                    for (int i = 0; i < totalSize; i++) {
                        slice[i].portalWriteAlloc(i);
                    }
                    mppa_barrier_wait(barrierNbClusters);
                    tS = 0;


                    for(int ht = 0; ht < hTiling; ht++) {
                        for(int wt = 0; wt < wTiling; wt++){
                            heightOffset = ht*tilingHeight;
                            widthOffset = wt*tilingWidth;
                            tiling.tile(subIterations, widthOffset, heightOffset, 0, tilingWidth, tilingHeight, 1);
                            slice[tS].hostSlice(tiling.input, tiling.widthOffset, tiling.heightOffset, tiling.depthOffset, tiling.width, tiling.height, tiling.depth);
                            slice[tS].copyTo(tiling.height, tiling.width, this->input.getWidth(), tiling.width, (tiling.heightOffset*this->input.getWidth())+tiling.widthOffset, 0);
                            for(size_t h=0;h<slice[tS].getHeight();h++) {
                                for(size_t w=0;w<slice[tS].getWidth();w++) {
                                    printf("ClusterSlice(%d,%d):%d\n",h,w, slice[tS](h,w));
                                }
                            }

                            tS++;
                        }
                    }
                    for (int i = 0; i < totalSize; i++) {
                        slice[i].waitWrite();
                    }

                    this->output.setTrigger(totalSize);
                    this->output.copyFrom();
                    printf("Output: ");
                    for(size_t h=0;h<output.getHeight();h++) {
                        printf("\n");
                        for(size_t w=0;w<output.getWidth();w++) {
                            printf(" %f ", output(h,w));
                        }
                        printf("\n");
                    }

                    for (int i = 0; i < totalSize; i++) {
                        slice[i].closeWritePortal();
                    }
                    mppa_close_barrier(barrierNbClusters);
                } else {
                    ///////////////////////////hTiling is higher///////////////////////////////
                    int counter = 0;

                    Array cluster[nb_clusters];
                    Array tmp;
                    barrierNbClusters = mppa_create_master_barrier(BARRIER_SYNC_MASTER, BARRIER_SYNC_SLAVE, nb_clusters);

                    Array outputNumberNb[nb_clusters];
                    Array outputNumberMod[itMod];

                    int baseItHt = 0;
                    int baseItWt = 0;
                    int auxHt = 0;
                    int auxWt = 0;
                    int ht = 0;
                    int wt = 0;
                    for(int i = 0; i < tiles; i++) {

                        mppa_barrier_wait(barrierNbClusters);

                        for (int i = 0; i < nb_clusters; i++) {
                            outputNumberNb[i].portalAuxWriteAlloc(i);
                        }

                        mppa_barrier_wait(barrierNbClusters);
                        int tS = 0;

                        auxHt = baseItHt;
                        auxWt = baseItWt;
                        if (auxWt == wTiling) {
                            auxHt++;
                            auxWt = 0;
                        }

                        for (auxHt; auxHt < hTiling; auxHt++) {
                            for (auxWt; auxWt < wTiling; auxWt++) {
                                heightOffset = auxHt*tilingHeight;
                                widthOffset = auxWt*tilingWidth;
                                tiling.tile(subIterations, widthOffset, heightOffset, 0, tilingWidth, tilingHeight, 1);
                                outputNumberNb[tS].hostSlice(tiling.input, tiling.widthOffset, tiling.heightOffset, tiling.depthOffset, tiling.width, tiling.height, tiling.depth);
                                outputNumberNb[tS].setAux(heightOffset, widthOffset, it, subIterations, tiling.coreWidthOffset, tiling.coreHeightOffset, tiling.coreDepthOffset, tiling.coreWidth, tiling.coreHeight, tiling.coreDepth, outterIterations, tiling.height, tiling.width, tiling.depth, this->input.getWidth(), this->input.getHeight());
                                tS++;
                                if (tS >= nb_clusters) {
                                    auxHt = hTiling;
                                    auxWt = wTiling;
                                }
                            }
                            if (auxWt == wTiling) {
                                auxWt = 0;
                            }

                        }

                        for (int i = 0; i < nb_clusters; i++) {
                            outputNumberNb[i].copyToAux();
                        }
                        for (int i = 0; i < nb_clusters; i++) {
                            outputNumberNb[i].waitAuxWrite();
                        }
                        for (int i = 0; i < nb_clusters; i++) {
                            outputNumberNb[i].closeAuxWritePortal();
                        }


                        for (int i = 0; i < nb_clusters; i++) {
                            cluster[i].portalWriteAlloc(i);
                        }
                        mppa_barrier_wait(barrierNbClusters);
                        tS = 0;
                        ht = baseItHt;
                        wt = baseItWt;
                        if (wt == wTiling) {
                            ht++;
                            wt = 0;
                        }

                        for (ht; ht < hTiling; ht++) {
                            for (wt; wt < wTiling; wt++) {
                                // masterCopyItStart = mppa_master_get_time();
                                heightOffset = ht*tilingHeight;
                                widthOffset = wt*tilingWidth;
                                tiling.tile(subIterations, widthOffset, heightOffset, 0, tilingWidth, tilingHeight, 1);
                                cluster[tS].hostSlice(tiling.input, tiling.widthOffset, tiling.heightOffset, tiling.depthOffset, tiling.width, tiling.height, tiling.depth);
                                cluster[tS].copyTo(tiling.height, tiling.width, this->input.getWidth(), tiling.width, (tiling.heightOffset*this->input.getWidth())+tiling.widthOffset, 0);
                                // for (int i = 0; i < nb_clusters; i++) {
                                    // for(size_t h=0;h<cluster[tS].getHeight();h++) {
                                    //     for(size_t w=0;w<cluster[tS].getWidth();w++) {
                                    //         printf("ClusterSlice(%d,%d):%d\n",h,w, cluster[tS](h,w));
                                    //     }
                                    // }
                                // }
                                // cout<<"Tile size(" << ht <<","<< wt << ") is: H(" << tmp.getHeight() << ") ~ W(" << tmp.getWidth() << ")" << endl;
                                // sleep(1);
                                // cluster[tS].mppaMasterClone(tmp);
                                // masterCopyItEnd = mppa_master_get_time();
                                // cluster[tS].copyTo();
                                tS++;
                                if (tS >= nb_clusters) {
                                    baseItHt = ht;
                                    ht = hTiling;
                                    baseItWt = wt + 1;
                                    wt = wTiling;
                                }
                                // masterSendItEnd = mppa_master_get_time();

                            }
                            if (wt == wTiling) {
                                wt = 0;
                            }
                        }


                        for (int i = 0; i < nb_clusters; i++) {
                            cluster[i].waitWrite();
                        }



                        this->output.setTrigger(nb_clusters);

                        this->output.copyFrom();

                        printf("Output: ");
                        for(size_t h=0;h<output.getHeight();h++) {
                            printf("\n");
                            for(size_t w=0;w<output.getWidth();w++) {
                                printf(" %f ", output(h,w));
                            }
                        }

                        for (int i = 0; i < nb_clusters; i++) {
                            cluster[i].closeWritePortal();
                        }


                    }

                    int hTilingMod = hTiling;
                    int wTilingMod = wTiling;
                    if (itMod == 0) {
                        hTilingMod = itMod;
                        wTilingMod = itMod;
                    }
                    mppa_barrier_wait(barrierNbClusters);

                    for(int j = 0; j < itMod; j++) {
                        outputNumberMod[j].portalAuxWriteAlloc(j);
                    }

                    mppa_barrier_wait(barrierNbClusters);
                    int tS = 0;
                    auxHt = baseItHt;
                    auxWt = baseItWt;
                    if (auxWt == wTilingMod) {
                        auxHt++;
                        auxWt = 0;
                    }
                    for (auxHt; auxHt < hTilingMod; auxHt++) {
                        for (auxWt; auxWt < wTilingMod; auxWt++) {
                            heightOffset = auxHt*tilingHeight;
                            widthOffset = auxWt*tilingWidth;
                            tiling.tile(subIterations, widthOffset, heightOffset, 0, tilingWidth, tilingHeight, 1);
                            outputNumberMod[tS].hostSlice(tiling.input, tiling.widthOffset, tiling.heightOffset, tiling.depthOffset, tiling.width, tiling.height, tiling.depth);
                            outputNumberMod[tS].setAux(heightOffset, widthOffset, it, subIterations, tiling.coreWidthOffset, tiling.coreHeightOffset, tiling.coreDepthOffset, tiling.coreWidth, tiling.coreHeight, tiling.coreDepth, outterIterations, tiling.height, tiling.width, tiling.depth, this->input.getWidth(), this->input.getHeight());
                            tS++;
                        }
                        auxWt = 0;
                    }

                    for (int i = 0; i < itMod; i++) {
                        outputNumberMod[i].copyToAux();
                    }
                    for (int i = 0; i < itMod; i++) {
                        outputNumberMod[i].waitAuxWrite();
                    }
                    for(int i = 0; i < itMod; i++) {
                        outputNumberMod[i].closeAuxWritePortal();
                    }


                    for(int j = 0; j < itMod; j++) {
                        cluster[j].portalWriteAlloc(j);
                    }
                    mppa_barrier_wait(barrierNbClusters);


                    tS = 0;
                    ht = baseItHt;
                    wt = baseItWt;
                    if (wt == wTiling) {
                        ht++;
                        wt = 0;
                    }

                    for (ht; ht < hTilingMod; ht++) {
                        for (wt; wt < wTilingMod; wt++) {
                            heightOffset = ht*tilingHeight;
                            widthOffset = wt*tilingWidth;
                            tiling.tile(subIterations, widthOffset, heightOffset, 0, tilingWidth, tilingHeight, 1);
                            cluster[tS].hostSlice(tiling.input, tiling.widthOffset, tiling.heightOffset, tiling.depthOffset, tiling.width, tiling.height, tiling.depth);
                            cluster[tS].copyTo(tiling.height, tiling.width, this->input.getWidth(), tiling.width, (tiling.heightOffset*this->input.getWidth())+tiling.widthOffset, 0);
                            tS++;
                        }
                        wt = 0;
                    }

                    for (int i = 0; i < itMod; i++) {
                        cluster[i].waitWrite();
                    }
                    this->output.setTrigger(itMod);


                    this->output.copyFrom();


                    for (int i = 0; i < itMod; i++) {
                        cluster[i].closeWritePortal();
                    }
                    for(int i = 0; i < itMod; i++) {
                        outputNumberMod[i].auxFree();
                    }
                    mppa_close_barrier(barrierNbClusters);

                }

                inputCopy.mppaMasterCopy(this->input);
                this->input.mppaMasterCopy(this->output);
                this->output.mppaMasterCopy(inputCopy);

            }



            inputCopy.mppaFree();
            this->output.closeReadPortal();
            this->output.mppaMasterCopy(this->input);


        }
#endif




#ifdef PSKEL_MPPA
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::waitSlaves(int nb_clusters, int tilingHeight, int tilingWidth) {
            size_t hTiling = ceil(float(this->input.getHeight())/float(tilingHeight));
            size_t wTiling = ceil(float(this->input.getHeight())/float(tilingWidth));
            size_t totalSize = float(hTiling*wTiling);
            int pid;
            int wait;
            if(totalSize < nb_clusters)
                nb_clusters = totalSize;
            for (pid = 0; pid < nb_clusters; pid++) {
                wait = mppa_waitpid(pid, NULL, 0);
                assert(wait != -1);
            }
        }
#endif


#ifdef PSKEL_MPPA
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::scheduleMPPA(const char slave_bin_name[], int nb_clusters, int nb_threads, size_t tilingHeight, size_t tilingWidth, int iterations, int innerIterations){

            this->spawn_slaves(slave_bin_name, tilingHeight, tilingWidth, nb_clusters, nb_threads, iterations, innerIterations);

            this->mppaSlice(tilingHeight, tilingWidth, nb_clusters, iterations, innerIterations);

            this->waitSlaves(nb_clusters, tilingHeight, tilingWidth);

        }
#endif


#ifdef PSKEL_MPPA
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::runMPPA(int cluster_id, int nb_threads, int nb_tiles, int outterIterations, int itMod){
            Array finalArr;
            Array coreTmp;
            Array tmp;
            Array inputTmp;
            Array outputTmp;
            Array input;
            Array auxPortal;
            int *aux;
            for(int j = 0; j < outterIterations; j++) {
                barrier_t *global_barrier = mppa_create_slave_barrier(BARRIER_SYNC_MASTER, BARRIER_SYNC_SLAVE);
                for(int i = 0; i < nb_tiles; i++) {
                    mppa_barrier_wait(global_barrier);

                    if(i == 0) {
                        auxPortal.portalAuxReadAlloc(1, cluster_id);
                        finalArr.portalWriteAlloc(0);
                    }

                    mppa_barrier_wait(global_barrier);
                    auxPortal.copyFromAux();

                    aux = auxPortal.getAux();

                    int heightOffset = aux[0];
                    int it = aux[1];
                    int subIterations = aux[2];
                    int coreWidthOffset = aux[3];
                    int coreHeightOffset = aux[4];
                    int coreDepthOffset = aux[5];
                    int coreWidth = aux[6];
                    int coreHeight = aux[7];
                    int coreDepth = aux[8];
                    int h = aux[10];
                    int w = aux[11];
                    int d = aux[12];
                    int widthOffset = aux[13];
                    int baseWidth = aux[14];

                    finalArr.mppaAlloc(w,h,d);
                    inputTmp.mppaAlloc(w,h,d);
                    outputTmp.mppaAlloc(w,h,d);
                    inputTmp.portalReadAlloc(1, cluster_id);
                    mppa_barrier_wait(global_barrier);

                    inputTmp.copyFrom();

                    for(size_t h=0;h<inputTmp.getHeight();h++) {
                    	for(size_t w=0;w<inputTmp.getWidth();w++) {
                      	  printf("Arrived(%d,%d):%d\n",h,w, inputTmp(h,w));
                      }
                    }

                    this->runIterativeMPPA(inputTmp, outputTmp, subIterations, nb_threads);
                    for(size_t h=0;h<outputTmp.getHeight();h++) {
                    	for(size_t w=0;w<outputTmp.getWidth();w++) {
                      	  printf("Computated(%d,%d):%d\n",h,w, outputTmp(h,w));
                      }
                    }

                    if (subIterations%2==0) {
                        finalArr.mppaMemCopy(inputTmp);
                    } else {
                        finalArr.mppaMemCopy(outputTmp);
                    }


                    coreTmp.hostSlice(finalArr, coreWidthOffset, coreHeightOffset, coreDepthOffset, coreWidth, coreHeight, coreDepth);
                    for(size_t h=0;h<coreTmp.getHeight();h++) {
                        for(size_t w=0;w<coreTmp.getWidth();w++) {
                            printf("finalArr(%d,%d):%d, cluster[%d]\n",h,w, coreTmp(h,w), cluster_id);
                        }
                    }

                    int masterBaseOffset = ((heightOffset*baseWidth) + widthOffset);
                    finalArr.copyTo(coreHeight, coreWidth, w, baseWidth, (inputTmp.getWidth()*coreHeightOffset)+coreWidthOffset, masterBaseOffset);
                    finalArr.waitWrite();
                    finalArr.mppaFree();
                    finalArr.auxFree();
                    inputTmp.mppaFree();
                    inputTmp.auxFree();


                    outputTmp.mppaFree();
                    outputTmp.auxFree();

                    inputTmp.closeReadPortal();

                    if (i == (nb_tiles-1)) {
                        auxPortal.closeAuxReadPortal();
                        finalArr.closeWritePortal();
                    }
                }
                if(cluster_id >= itMod) {
                    mppa_barrier_wait(global_barrier);
                    mppa_barrier_wait(global_barrier);
                    mppa_barrier_wait(global_barrier);
                }
                mppa_close_barrier(global_barrier);
            }

        }
#endif
#ifdef PSKEL_MPPA
#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::runIterativeMPPA(Array in, Array out, int iterations, size_t numThreads){
            size_t width = this->input.getWidth();
            size_t height = this->input.getHeight();
            size_t depth = this->input.getDepth();
            size_t maskRange = this->mask.getRange();
            for(int i = 0; i < iterations; i++) {
                if(i%2==0) {
                    this->runOpenMP(in, out, width, height, depth, maskRange, numThreads);
                } else {
                    this->runOpenMP(out, in, width, height, depth, maskRange, numThreads);

                }
            }

        }
#endif
#endif
    //***********************************************
    //***********************************************
#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::runCPU(size_t numThreads){
            numThreads = (numThreads==0)?omp_get_num_procs():numThreads;
            this->runOpenMP(this->input, this->output, this->input.getWidth(), this->input.getHeight(), this->input.getDepth(), this->mask.getRange(), numThreads);
        }
#endif

#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::runIterativeSequential(size_t iterations){
            Array inputCopy;
            inputCopy.hostClone(input);
            for(size_t it = 0; it<iterations; it++){
                if(it%2==0) this->runSeq(inputCopy, this->output);
                else this->runSeq(this->output, inputCopy);
            }
            if((iterations%2)==0) output.hostMemCopy(inputCopy);
            inputCopy.hostFree();
        }
#endif

#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void StencilBase<Array, Mask,Args>::runIterativeCPU(size_t iterations, size_t numThreads){
            numThreads = (numThreads==0)?omp_get_num_procs():numThreads;
            size_t width = this->input.getWidth();
            size_t height = this->input.getHeight();
            size_t depth = this->input.getDepth();
            size_t maskRange = this->mask.getRange();
            //cout << "numThreads: " << numThreads << endl;
            Array inputCopy;
            inputCopy.hostClone(input);
            for(size_t it = 0; it<iterations; it++){
                if(it%2==0){
                    // this->runOpenMP(inputCopy, this->output, numThreads);
                    this->runOpenMP(input, this->output, width, height, depth, maskRange, numThreads);
                }else {
                    // this->runOpenMP(this->output, inputCopy, numThreads);
                    this->runOpenMP(this->output, input, width, height, depth, maskRange, numThreads);
                }
            }
            if((iterations%2)==0) output.hostMemCopy(inputCopy);
            inputCopy.hostFree();
        }
#endif

//***********************************************
// Stencil 3D
//***********************************************


template<class Array, class Mask, class Args>
    Stencil3D<Array,Mask,Args>::Stencil3D(){}

template<class Array, class Mask, class Args>
    Stencil3D<Array,Mask,Args>::Stencil3D(Array _input, Array _output, Mask _mask, Args _args){
        this->input = _input;
        this->output = _output;
        this->args = _args;
        this->mask = _mask;
    }

#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void Stencil3D<Array,Mask,Args>::runSeq(Array in, Array out){
            for (int h = 0; h < in.getHeight(); h++){
                for (int w = 0; w < in.getWidth(); w++){
                    for (int d = 0; d < in.getDepth(); d++){
                        stencilKernel(in,out,this->mask,this->args,h,w,d);
                    }}}
        }
#endif

#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void Stencil3D<Array,Mask,Args>::runOpenMP(Array in, Array out, size_t numThreads){
            omp_set_num_threads(numThreads);
#pragma omp parallel for
            for (int h = 0; h < in.getHeight(); h++){
                for (int w = 0; w < in.getWidth(); w++){
                    for (int d = 0; d < in.getDepth(); d++){
                        stencilKernel(in,out,this->mask,this->args,h,w,d);
                    }}}
        }
#endif

//***************************************************
// Stencil 2D
//***************************************************

template<class Array, class Mask, class Args>
    Stencil2D<Array,Mask,Args>::Stencil2D(){}

template<class Array, class Mask, class Args>
    Stencil2D<Array,Mask,Args>::Stencil2D(Array _input, Array _output, Mask _mask, Args _args){
        this->input = _input;
        this->output = _output;
        this->args = _args;
        this->mask = _mask;
    }

template<class Array, class Mask, class Args>
    Stencil2D<Array,Mask,Args>::Stencil2D(Array _input, Array _output, Mask _mask){
        this->input = _input;
        this->output = _output;
        this->mask = _mask;
    }


template<class Array, class Mask, class Args>
    Stencil2D<Array,Mask,Args>::~Stencil2D(){
#ifdef PSKEL_MPPA
            this->input.mppaFree();
            this->output.mppaFree();
#endif
#ifdef PSKEL_CPU
            this->input.hostFree();
            this->output.hostFree();
#endif
        }

#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void Stencil2D<Array,Mask,Args>::runSeq(Array in, Array out){
            for (int h = 0; h < in.getHeight(); h++){
                for (int w = 0; w < in.getWidth(); w++){
                    stencilKernel(in,out,this->mask, this->args,h,w);
                }}
        }
#endif

#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        inline __attribute__((always_inline)) void Stencil2D<Array,Mask,Args>::runOpenMP(Array in, Array out, size_t width, size_t height, size_t depth, size_t maskRange, size_t numThreads){
//             size_t hrange = height-maskRange;
//             size_t wrange = width-maskRange;
//             int count = 0;
// #pragma omp parallel num_threads(numThreads)
//             {
// #pragma omp for
//                 for (size_t h = maskRange; h < hrange; h++){
//                     for (size_t w = maskRange; w < wrange; w++){
//                         stencilKernel(in,out,this->mask,this->args,h,w);
//                     }}
//             }
            omp_set_num_threads(numThreads);
#pragma omp parallel for
            for (int h = 0; h < in.getHeight(); h++){
                for (int w = 0; w < in.getWidth(); w++){
                    stencilKernel(in,out,this->mask,this->args,h,w);
                    }}
        }
#endif

//***************************************************
// Stencil 1D
//***************************************************


template<class Array, class Mask, class Args>
    Stencil<Array,Mask,Args>::Stencil(){}

template<class Array, class Mask, class Args>
    Stencil<Array,Mask,Args>::Stencil(Array _input, Array _output, Mask _mask, Args _args){
        this->input = _input;
        this->output = _output;
        this->args = _args;
        this->mask = _mask;
    }

#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void Stencil<Array,Mask,Args>::runSeq(Array in, Array out){
            for (int i = 0; i < in.getWidth(); i++){
                stencilKernel(in,out,this->mask, this->args,i);
            }
        }
#endif

#ifndef MPPA_MASTER
    template<class Array, class Mask, class Args>
        void Stencil<Array,Mask,Args>::runOpenMP(Array in, Array out, size_t numThreads){
            omp_set_num_threads(numThreads);
#pragma omp parallel for
            for (int i = 0; i < in.getWidth(); i++){
                stencilKernel(in,out,this->mask, this->args,i);
            }
        }
#endif

}//end namespace


#endif
