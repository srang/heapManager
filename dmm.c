#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"

/* You can improve the below metadata structure using the concepts from Bryant
 * and OHallaron book (chapter 9).
 */

typedef struct metadata {
       /* size_t is the return type of the sizeof operator. Since the size of
 	* an object depends on the architecture and its implementation, size_t 
	* is used to represent the maximum size of any object in the particular
 	* implementation. 
	* size contains the size of the data object or the amount of free
 	* bytes 
	*
	* Because each block is aligned to the size_t the bottom three bits will
	* always be zero. Knowing this, the bottom bit can be used like a boolean
	* without increasing the size of metadata_t. The bottom most bit is used 
	* to store whether or not a block is in use.  
	*/
	size_t size;//stores the size of the free or allocated memory in the block 
			//AND whether its allocated
	struct metadata* next;//used for coalescing and traversing
	struct metadata* prev;//used for coalescing  
} metadata_t;

/* freelist points to the head of the metadata_t list and contains both free and used blocks */

static metadata_t* freelist = NULL;

void* dmalloc(size_t numbytes) {
	if(freelist == NULL) { 			/*Initialize through sbrk call first time*/
		if(!dmalloc_init())
			return NULL;
	}
	assert(numbytes > 0);
	numbytes = ALIGN(numbytes); /* clears bottom n bits */
	metadata_t* check = freelist;
	/* looks for a chunk that can fit numbytes that is unused */
	while(numbytes > check->size || check->size & 1) {
        check = check->next;
        /* Reached the end of the heap */
		if(check == NULL) {
	            return NULL;
        	}
    
        }
    void* ret;
    if(numbytes + METADATA_T_ALIGNED > check->size) {
        check->size += 1;
        ret = (void*) check;
        ret += METADATA_T_ALIGNED;//returns the first non-header address
    }
    else { // split
        metadata_t* used = check;//block to be returned
    	ret = (void*) check;
    	ret = ret + numbytes + METADATA_T_ALIGNED;//moves by bytes to where the next block will start
    	check = (metadata_t *) ret;
        check->size = used->size - (numbytes + METADATA_T_ALIGNED);
    	check->next = used->next;
    	check->prev = used;
    	used->next = check;
    	used->size = numbytes;
        /* designates it as allocated */
        used->size += 1;
        ret = (char*)used;
        ret += METADATA_T_ALIGNED;//returns the first non-header address
    }
	return ret;
}

void dfree(void* ptr) {
    assert(ptr > 0);
    ptr -= METADATA_T_ALIGNED;
    metadata_t* check = (metadata_t*) ptr;
    /* marks the block as free */
    check->size -= 1;
    /* coalesce */
    if(check->next != NULL && !(check->next->size & 1)) {
        check->size = check->size + check->next->size + METADATA_T_ALIGNED;
        check->next = check->next->next;
    }
    if(check->prev != NULL && !check->prev->size & 1) {
        check->prev->size += check->size + METADATA_T_ALIGNED;
        check->prev->next = check->next;
    }
}

bool dmalloc_init() {

	/* Two choices: 
 	* 1. Append prologue and epilogue blocks to the start and the end of the freelist
 	* 2. Initialize freelist pointers to NULL
 	*
 	* Note: We provide the code for 2. Using 1 will help you to tackle the
 	* corner cases succinctly.
 	*/
	/* Q: Why casting is used? i.e., why (void*)-1? */
	size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
	freelist = (metadata_t*) sbrk(max_bytes);
	freelist->size = max_bytes - METADATA_T_ALIGNED;
	if (freelist == (void *)-1)
		return false;
	freelist->next = NULL;
	freelist->prev = NULL;
	return true;
}

/*Only for debugging purposes; can be turned off through -NDEBUG flag*/
void print_freelist() {
	metadata_t *freelist_head = freelist;
	while(freelist_head != NULL) {
		DEBUG("\tFreelist Size:%Zd, Head:%p, Prev:%p, Next:%p\t",freelist_head->size,freelist_head,freelist_head->prev,freelist_head->next);
		freelist_head = freelist_head->next;
	}
	DEBUG("\n");
}
