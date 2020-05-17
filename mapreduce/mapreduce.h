#ifndef MAPREDUCE_H_
#define MAPREDUCE_H_

/******************************************************************************
 * Definition of the MapReduce framework API.
 *
 * IMPORTANT!  The ONLY change you may make to this file is to add your data
 * members to the map_reduce struct definition.  Making any other changes alters
 * the API, which breaks compatibility with all of the other programs that are
 * using your framework!
 ******************************************************************************/

/* Header includes */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <fcntl.h>
#include <vector>
#include <string>

#define MR_BUFFER_THRESHOLD 0.85

/* Forward-declaration, the definition is farther down */
struct map_reduce;
struct kvpair;

/*
 * Type aliases for callback function pointers.  These are functions which will
 * be provided by the application.  All of them will return 0 to indicate success and
 * nonzero to indicate failure.
 */

/**
 * Function signature for application-provided Map functions.
 * A Map function will read input provided by the framework through kv, process it, and call mr_produce
 * for each key-value pair it outputs.
 * Returns 0 if the operation was completed successfuly and -1 if there was
 * an error.
 *
 */
int32_t map(struct map_reduce *mr, struct kvpair *kv);

/**
 * Function signature for application-provided Reduce functions.  A Reduce function
 * will receive key-value pairs from the mr_consume function through kvset , combine them, and write the result to a buffer.
 * The num parameter informs the Reduce function how many valid key-value pairs are
 * passed through the kvset buffer.
 *
 * num 	Indicates the number of kvpairs present in the buffer passed to the function.
 *		If the num is given as 0, then the reduce function calls mr_output to produce the output file.
 *
 * Returns 0 if the operation was completed successfuly and -1 if there was
 * an error.
 *
 */
int32_t reduce(struct map_reduce *mr, struct kvpair *kvset, size_t num);


/*
 * Structure for storing any needed persistent data - do not use global
 * variables when writing a system!  You may put whatever data is needed by your
 * functions into this struct definition.
 *
 * The contents of this structure are the ONLY part of the mapreduce.h file that
 * you may change!
 *
 * The application will utilize one paramter from this struct. The member variable named
 * 'genOutput', when set to non-zero value, will indicate to the application
 *  that the output has to be generated now and the application will call mr_output.
 *
 */
struct map_reduce {
	char* application;
	int n_threads; 
	std::vector<std::string> mapper_addresses;
	std::string reducer_address;
	char *inpath;
	char *outpath; 
	int32_t (*map_fnc)(struct map_reduce*, struct kvpair*);
	int32_t (*reduce_fnc)(struct map_reduce*, struct kvpair*, size_t);
	int32_t	*mapfn_status;
	int32_t	reducefn_status;
	int32_t receiver_finished;
	pthread_mutex_t lock;
	pthread_cond_t not_full, not_empty;
	pthread_cond_t map_finished;				
	pthread_t *map_threads;
	pthread_t reduce_thread;
	char *map_buffer;
	int32_t	used_size;	
	char *reduce_buffer;
	char *args;
	int barrier;
	char *key_assigned_address;
	int32_t genOutput;
};

/**
 * Structure which represents an arbitrary key-value pair.  This structure will
 * be used for communicating between Map and Reduce threads.  In this framework,
 * you do not need to parse the information in the key or value, only pass it on
 * to the next stage.
 */
struct kvpair {
	/* Pointers to the key and value data */
	void *key;
	void *value;

	/* Size of the key and value data in bytes */
	uint32_t keysz;
	uint32_t valuesz;
};


/*
 * MapReduce function API
 *
 * These are the seven functions you will be implementing in mapreduce.c.
 */

/**
 * Allocates and initializes an instance of the MapReduce framework.  This
 * function should allocate a map_reduce structure and any memory or resources
 * that may be needed by later functions. Also setups the input file on the HDFS.
 *
 * application 	Name of the application run by the framework {wordc, grep}
 * threads    	Number of worker threads to use
 * inpath     	Path to the file from which input is read.
 *      	    The framework should load that file onto the HDFS.
 * outpath    	Path to the file to which output is written.
 * helper_args 	Used by the framework to generate tokenized input for the map_fn.
 *				For wordc application, used to provide delimiters and for grep,
 *				used to provide the search string.
 *
 * Returns a pointer to the newly allocated map_reduce structure on success, or
 * NULL to indicate failure.
 */
map_reduce *mr_init(const char *application, int32_t threads, const char *inpath, const char *outpath, const char *helper_args);

/**
 * Destroys and cleans up an existing instance of the MapReduce framework.  Any
 * resources which were acquired or created in mr_create should be released or
 * destroyed here.
 *
 * mr  Pointer to the instance to destroy and clean up
 */
void mr_destroy(struct map_reduce *mr);

/**
 * Begins a multithreaded MapReduce operation.  This operation will process data
 * from the given input file and write the result to the given output file.
 *
 * mr              Pointer to the instance to start
 * barrierEnable   Configuration parameter to intimate the framework about
 * 								 barrier implementation at the reducer. If the value is 1,
 *								 then the barrier at the reducer should be enabled.
 *
 * Returns 0 if the operation was started successfuly and nonzero if there was
 * an error.
 */
int32_t mr_start(struct map_reduce *mr, const int32_t barrierEnable);

/**
 * Blocks until the entire MapReduce operation is complete.  When this function
 * returns, you are guaranteeing to the application that all Map and Reduce threads
 * have completed.
 *
 * mr  Pointer to the instance to wait for
 *
 * Returns 0 if every Map and Reduce function returned 0 (success), and nonzero
 * if any of the Map or Reduce functions failed.
 */
int32_t mr_finish(struct map_reduce *mr);

/**
 * Called by a Map thread each time it produces a key-value pair, to be stored
 * in the in-memory buffer.	 If the framework cannot currently store another
 * key-value pair, this function should block until it can.
 *
 * mr  Pointer to the MapReduce instance
 * kv  Pointer to the key-value pair that was produced by Map.  This pointer
 *     belongs to the caller, so you must copy the key and value data if you
 *     wish to store them somewhere.
 *
 * Returns 1 if one key-value pair is successfully stored (success), -1 on
 * failure.  (This convention mirrors that of the standard "write" function.)
 */
int32_t mr_produce(struct map_reduce *mr, const struct kvpair *kv);

/**
 * Called by the reducer thread to consume key-value pairs from the in-memory buffer.
 * If there is no key-value pair available, this function should block
 * until one is produced (in which case it will return 1) or all Mapper
 * threads return (in which case it will return 0).
 *
 * mr             Pointer to the MapReduce instance
 * kv             Pointer to the key-value pairs that were consumed by this function from the buffer.
 *                The pointer memory will be populated by mr_consume. Allocation of the memory needs to be
 *		            carefully managed to avoid invalid memory operations/memory leaks.
 * num            Will be populated by mr_consume to indicate the number of pairs read.
 * barrierEnable  The value provided in mr_start is propogated here
 *
 * Returns 1 if one pair is successfully consumed, 0 if the Map thread returns
 * without producing any more pairs, or -1 on error.  (This convention mirrors
 * that of the standard "read" function.)
 */
int32_t mr_consume(struct map_reduce *mr, struct kvpair *kvset, size_t *num, const int32_t barrierEnable);

/**
 * Called by the reducer thread, to store the final outputs produced into a file on the HDFS.
 * The output will be formatted by the Reduce function into a data buffer and provided to the function.
 *
 * mr             Pointer to the MapReduce instance
 * writeBuffer    Output produced by the Reduce funtion to be stored on file.
 * bufferLength   Size (in bytes) of the valid data in the buffer.
 *
 * Returns 0 if the key-value pairs were written to the file (success), -1 on
 * failure.
 */
int32_t mr_output(struct map_reduce *mr, char *writeBuffer, size_t bufferLength);

#endif
