/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This code is an adaptation of the Lee algorithm's implementation originally included in the STAMP Benchmark
 * by Stanford University.
 *
 * The original copyright notice is included below.
 *
  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * Unless otherwise noted, the following license applies to STAMP files:
 *
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 *
 * CircuitRouter-SeqSolver.c
 *
 * =============================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "lib/list.h"
#include "maze.h"
#include "router.h"
#include "lib/timer.h"
#include "lib/types.h"
#include <pthread.h>

enum param_types {
    PARAM_TASKSCOUNT = (unsigned char)'t',
    PARAM_BENDCOST   = (unsigned char)'b',
    PARAM_XCOST      = (unsigned char)'x',
    PARAM_YCOST      = (unsigned char)'y',
    PARAM_ZCOST      = (unsigned char)'z',
};

enum param_defaults {
    PARAM_DEFAULT_TASKSCOUNT = 4,
    PARAM_DEFAULT_BENDCOST   = 1,
    PARAM_DEFAULT_XCOST      = 1,
    PARAM_DEFAULT_YCOST      = 1,
    PARAM_DEFAULT_ZCOST      = 2,
};

bool_t global_doPrint = TRUE;
char* global_inputFile = NULL;
long global_params[256]; /* 256 = ascii limit */
pthread_t *global_threads;
pthread_mutex_t *global_mutexes;
int NUM_TAREFAS;


/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void displayUsage (const char* appName){
    printf("Usage: %s [options] input_filename\n", appName);
    puts("\nOptions:                            (defaults)\n");
    printf("    t <INT>    [t]asks count        (%i)\n", PARAM_DEFAULT_TASKSCOUNT);
    printf("    b <INT>    [b]end cost          (%i)\n", PARAM_DEFAULT_BENDCOST);
    printf("    x <UINT>   [x] movement cost    (%i)\n", PARAM_DEFAULT_XCOST);
    printf("    y <UINT>   [y] movement cost    (%i)\n", PARAM_DEFAULT_YCOST);
    printf("    z <UINT>   [z] movement cost    (%i)\n", PARAM_DEFAULT_ZCOST);
    printf("    h          [h]elp message       (false)\n");
    exit(1);
}

/* =============================================================================
 * setDefaultParams
 * =============================================================================
 */
static void setDefaultParams (){
    global_params[PARAM_TASKSCOUNT] = PARAM_DEFAULT_TASKSCOUNT;
    global_params[PARAM_BENDCOST]   = PARAM_DEFAULT_BENDCOST;
    global_params[PARAM_XCOST]      = PARAM_DEFAULT_XCOST;
    global_params[PARAM_YCOST]      = PARAM_DEFAULT_YCOST;
    global_params[PARAM_ZCOST]      = PARAM_DEFAULT_ZCOST;
}

/* =============================================================================
 * parseArgs
 * =============================================================================
 */
static void parseArgs (long argc, char* const argv[]){
    long opt;

    opterr = 0;

    setDefaultParams();

    while ((opt = getopt(argc, argv, "hb:x:y:z:t:")) != -1) {
        switch (opt) {
            //Set PARAM_TASKSCOUNT as user entered
            case 't':
                global_params[(unsigned char)opt] = atol(optarg);
                NUM_TAREFAS = atoi(optarg);
                break;
            case 'b':
            case 'x':
            case 'y':
            case 'z':
                global_params[(unsigned char)opt] = atol(optarg);
                break;
            case '?':
            case 'h':
                displayUsage(argv[0]);
            default:
                fprintf(stderr, "Missing -t and the argument\n");
                displayUsage(argv[0]);
                break;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Missing input file\n");
        displayUsage(argv[0]);
    }

    global_inputFile = argv[optind];
}
/* =============================================================================
 * novaThread
 * =============================================================================
 */
 void *novaThread(){
   printf("Hello, I'm a thread\n");

   pthread_exit(NULL);
 }
/* =============================================================================
 * router_parSolve
 * =============================================================================
 */
 void router_parSolve(router_solve_arg_t arg){
   global_threads = malloc(sizeof(pthread_t)*(NUM_TAREFAS + 1));
   global_mutexes = malloc(sizeof(pthread_mutex_t)*3);
   argAndMutexes_t args = {arg, global_mutexes};
   for(int i = 0; i < 3 ; i++){
     //init every available mutex
     if (pthread_mutex_init(&global_mutexes[i], NULL) != 0){
       perror("Failed mutex initialization!\n");
       exit(EXIT_FAILURE);
     }
   }
   for(int i = 0 ; i < NUM_TAREFAS + 1; i++){
     if(i == NUM_TAREFAS){
       if((pthread_create(&global_threads[i], NULL, novaThread, NULL)) != 0){
         //Failed to create novaThread!
         perror("Failed to create novaThread!\n");
         exit(EXIT_FAILURE);
       }
     }
     if((pthread_create(&global_threads[i], NULL, router_solve, (void*) &args)) != 0){
       //Failed to create thread!
       perror("Failed to create thread!\n");
       exit(EXIT_FAILURE);
     }
   }
   for(int i = 0 ; i < NUM_TAREFAS + 1; i++){
     //Wait for thread to finish execution
     if(pthread_join(global_threads[i], NULL) != 0){
       perror("Failed pthread_join!\n");
       exit(EXIT_FAILURE);
     }
   }
   for(int i = 0; i < 3 ; i++){
     //init every available mutex
     if (pthread_mutex_destroy(&global_mutexes[i]) != 0){
       perror("Failed mutex destruction!\n");
     }
   }
 }

/* =============================================================================
 * outputFile
 * =============================================================================
 */
FILE* outputFile() {
    FILE *fp;

    char result_outputFile[strlen(global_inputFile) + strlen(".res") + 1];
    sprintf(result_outputFile, "%s.res", global_inputFile);

    if (access(result_outputFile, F_OK) == 0) {
        char old_outputFile[strlen(global_inputFile) + strlen(".res.old") + 1];
        sprintf(old_outputFile, "%s.res.old", global_inputFile);
        if (rename(result_outputFile, old_outputFile) == -1) {
            perror("Error renaming output file");
            exit(EXIT_FAILURE);
        }
    }
    fp = fopen(result_outputFile, "wt");
    if (fp == NULL) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }
    return fp;
}

/* =============================================================================
 * main
 * =============================================================================
 */
int main(int argc, char** argv){
    /*
     * Initialization
     */

    parseArgs(argc, argv);
    FILE* resultFp = outputFile();
    maze_t* mazePtr = maze_alloc();
    assert(mazePtr);
    long numPathToRoute = maze_read(mazePtr, global_inputFile, resultFp);
    router_t* routerPtr = router_alloc(global_params[PARAM_XCOST],
                                       global_params[PARAM_YCOST],
                                       global_params[PARAM_ZCOST],
                                       global_params[PARAM_BENDCOST]);
    assert(routerPtr);
    list_t* pathVectorListPtr = list_alloc(NULL);
    assert(pathVectorListPtr);

    router_solve_arg_t routerArg = {routerPtr, mazePtr, pathVectorListPtr};
    TIMER_T startTime;
    TIMER_READ(startTime);

    router_parSolve(routerArg);
    //router_solve((void *)&routerArg);

    TIMER_T stopTime;
    TIMER_READ(stopTime);

    long numPathRouted = 0;
    list_iter_t it;
    list_iter_reset(&it, pathVectorListPtr);
    while (list_iter_hasNext(&it, pathVectorListPtr)) {
        vector_t* pathVectorPtr = (vector_t*)list_iter_next(&it, pathVectorListPtr);
        numPathRouted += vector_getSize(pathVectorPtr);
    }
    fprintf(resultFp, "Paths routed    = %li\n", numPathRouted);
    fprintf(resultFp, "Elapsed time    = %f seconds\n", TIMER_DIFF_SECONDS(startTime, stopTime));


    /*
     * Check solution and clean up
     */
    assert(numPathRouted <= numPathToRoute);
    bool_t status = maze_checkPaths(mazePtr, pathVectorListPtr, resultFp, global_doPrint);
    assert(status == TRUE);
    fputs("Verification passed.\n",resultFp);

    maze_free(mazePtr);
    router_free(routerPtr);

    list_iter_reset(&it, pathVectorListPtr);
    while (list_iter_hasNext(&it, pathVectorListPtr)) {
        vector_t* pathVectorPtr = (vector_t*)list_iter_next(&it, pathVectorListPtr);
        vector_t* v;
        while((v = vector_popBack(pathVectorPtr))) {
            // v stores pointers to longs stored elsewhere; no need to free them here
            vector_free(v);
        }
        vector_free(pathVectorPtr);
    }
    list_free(pathVectorListPtr);


    free(global_threads);
    free(global_mutexes);
    fclose(resultFp);
    exit(0);
}

/* =============================================================================
 *
 * End of CircuitRouter-SeqSolver.c
 *
 * =============================================================================
 */