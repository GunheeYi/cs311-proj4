#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BYTES_PER_WORD 4
#define MAX_LEN 1024

// * copied from project 2
char** str_split(char *a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
	if (a_delim == *tmp)
	{
	    count++;
	    last_comma = tmp;
	}
	tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
     *        knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
	size_t idx  = 0;
	char* token = strtok(a_str, delim);

	while (token)
	{
	    assert(idx < count);
	    *(result + idx++) = strdup(token);
	    token = strtok(0, delim);
	}
	assert(idx == count - 1);
	*(result + idx) = 0;
    }

    return result;
}


/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */   
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize){

	printf("Cache Configuration:\n");
    	printf("-------------------------------------\n");
	printf("Capacity: %dB\n", capacity);
	printf("Associativity: %dway\n", assoc);
	printf("Block Size: %dB\n", blocksize);
	printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat		                       */   
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
	int reads_hits, int write_hits, int reads_misses, int write_misses) {
	printf("Cache Stat:\n");
    	printf("-------------------------------------\n");
	printf("Total reads: %d\n", total_reads);
	printf("Total writes: %d\n", total_writes);
	printf("Write-backs: %d\n", write_backs);
	printf("Read hits: %d\n", reads_hits);
	printf("Write hits: %d\n", write_hits);
	printf("Read misses: %d\n", reads_misses);
	printf("Write misses: %d\n", write_misses);
	printf("\n");
}


/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */ 
/* 							       */
/* Cache Design						       */
/*  							       */
/* 	    cache[set][assoc][word per block]		       */
/*      						       */
/*      						       */
/*       ----------------------------------------	       */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*      						       */
/*                                                             */
/***************************************************************/
void xdump(int set, int way, uint32_t** cache)
{
	int i,j,k = 0;

	printf("Cache Content:\n");
    	printf("-------------------------------------\n");
	for(i = 0; i < way;i++)
	{
		if(i == 0)
		{
			printf("    ");
		}
		printf("      WAY[%d]",i);
	}
	printf("\n");

	for(i = 0 ; i < set;i++)
	{
		printf("SET[%d]:   ",i);
		for(j = 0; j < way;j++)
		{
			if(k != 0 && j == 0)
			{
				printf("          ");
			}
			printf("0x%08x  ", cache[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

int logTwo(int a) {
	int count = 0;
	while (a!=1) {
		a /= 2;
		count += 1;
	}
	return count;
}

uint32_t** cache;
uint32_t** history;
uint32_t** dirty;
uint32_t* occupied;
int capacity = 256;
int way = 4;
int blocksize = 8;
int set;
int words;
int tagLength;
int indexLength;
int blockOffset;

int main(int argc, char *argv[]) {                              

	int i, j, k;	

	int count = 1;
	int dumpContent = 0;

	while(count != argc-1){
		if(strcmp(argv[count], "-c") == 0){
			char** tokens = str_split(argv[++count],':');

			capacity = (int)atoi(*(tokens));
			way = (int)atoi(*(tokens+1));
			blocksize = (int)atoi(*(tokens+2));
		}
		else if(strcmp(argv[count], "-x") == 0)
			dumpContent = 1;
		else{
			printf("Error: usage: %s -c capacity:associativity:block_size [-x] <input trace file>\n", argv[0]);
			exit(1);
		}
		count++;
    }

	set = capacity/way/blocksize;
	words = blocksize / BYTES_PER_WORD;
	
	indexLength = logTwo(set);
	blockOffset = logTwo(blocksize);
	tagLength = 32 - indexLength - blockOffset;

	int reads = 0, writes = 0, writeBacks = 0, readHits = 0, writeHits = 0, readMisses = 0, writeMisses = 0;
	
	// open input trace file
	FILE* file = fopen(argv[argc-1], "r");
    if (file == NULL) {
		printf("Error: Can't open program file %s\n", argv[argc-1]);
		exit(-1);
    }

	// allocate
	cache = (uint32_t**) malloc (sizeof(uint32_t*) * set);
	history = (uint32_t**) malloc (sizeof(uint32_t*) * set);
	dirty = (uint32_t**) malloc (sizeof(uint32_t*) * set);
	for(i = 0; i < set; i++) {
		cache[i] = (uint32_t*) malloc(sizeof(uint32_t) * way);
		history[i] = (uint32_t*) malloc(sizeof(uint32_t) * way);
		dirty[i] = (uint32_t*) malloc(sizeof(uint32_t) * way);
	}
	for(i = 0; i < set; i++) {
		for(j = 0; j < way; j ++) 
			cache[i][j] = 0;
			history[i][j] = 0;
			dirty[i][j] = 0;
	}

	occupied = (uint32_t*) malloc(sizeof(uint32_t*) * set);
	for ( i = 0; i < set; i++)
	{
		occupied[i] = 0;
	}

	// process read and writes
	int t = 0;
	char buffer[MAX_LEN];
	char** line;
	char RW;
	uint32_t target, entry, tag, index, hit;
    while (fgets(buffer, MAX_LEN - 1, file))
    {
        // Remove trailing newline
        buffer[strcspn(buffer, "\n")] = 0;
		line = str_split(buffer,' ');
		RW = **line;
		target = (int) strtol(*(line+1), NULL, 16);

		entry = target - target % blocksize;
		tag = entry >> (32-tagLength);
		index = entry << tagLength >> (tagLength+blockOffset); 

		RW=='R' ? reads++ : writes++;

		hit = 0;
		for(i = 0; i < way; i++) {
			if(cache[index][i]==entry) {
				RW=='R' ? readHits++ : writeHits++;
				history[index][i] = t;
				if(RW=='W') dirty[index][i] = 1;
				hit = 1;
				break;
			}
		}
		if(!hit) {
			RW=='R' ? readMisses++ : writeMisses++;
			if(occupied[index] < way) { // Cache not filled yet

				cache[index][occupied[index]] = entry;
				history[index][occupied[index]] = t;
				dirty[index][occupied[index]] = RW=='R' ? 0 : 1;
				occupied[index]++;

			} else { // LRU has to be evicted

				uint32_t LRUt = history[index][0], LRU = 0;
				for(i = 1; i < way; i++) {
					if (history[index][i] < LRUt) {
						LRUt = history[index][i];
						LRU = i;
					}
				}

				if(dirty[index][LRU]) writeBacks++;

				cache[index][LRU] = entry;
				history[index][LRU] = t;
				dirty[index][LRU] = RW=='R' ? 0 : 1;

			}
		} 

		t++;
    }

	// test example
	cdump(capacity, way, blocksize);
	sdump(reads, writes, writeBacks, readHits, writeHits, readMisses, writeMisses); 
	if (dumpContent) xdump(set, way, cache);

	return 0;
}
