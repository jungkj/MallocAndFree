#define _XOPEN_SOURCE 500 // needed for sbrk() on cslab

#include <unistd.h>
typedef struct freenode
{
    size_t size;
    struct freenode *next;
} freenode;

#define HEAP_CHUNK_SIZE 4096

// head node for the freelist
freenode *freelist = NULL;

size_t available_bytes = 0;
/* allocate size bytes from the heap */
void *malloc(size_t size)
{
    // can't have less than 1 byte requested
    if (size < 1)
    {
        return NULL;
    }

    // add 8 bytes for bookkeeping
    size += 8;

    // 32 bytes is the minimum allocation
    if (size < 32)
    {
        size = 32;
    }

    // round up to the nearest 16-byte multiple
    else if (size%16 != 0)
    {
        size = ((size/16)+1)*16;
    }

    // if we have no memory, grab one chunk to start
    if (freelist == NULL){
    	void *tmp = sbrk(size);
	if (tmp == (void *) -1)
		return NULL;
	tmp += 8;
	freelist = (freenode *)tmp;
	freelist->size = size - 8;
    }
    
	    
    // look for a freenode that's large enough for this request
    // have to track the previous node also for list manipulation later
    freenode *prev = freelist;
    freenode *cur = freelist;

    for (freenode *tmp = freelist; tmp != NULL; tmp = tmp->next){
    	if (tmp ->size >=size){
		cur = tmp;
		break;
	}
	prev = tmp;
    }
    // if there is no freenode that's large enough, allocate more memory
    freenode *newnode = freelist;
    if (cur == NULL){
    	void *tmp = sbrk(((size/HEAP_CHUNK_SIZE)+1)*HEAP_CHUNK_SIZE);
	if (tmp == (void *)-1)
		return NULL;
	tmp += 8;
	newnode = (freenode *)tmp;
	newnode->next = freelist;
	newnode->size = size-8;
    }
    // here, should have a freenode with enough space for the request
    // - if there would be less than 32 bytes left, then return the entire chunk
    // - if there are remaining bytes, then break up the chunk into two pieces
    //     return the front of this memory chunk to the user
    //     and put the rest into the freelist 

    size_t nodesize = cur->size;
    size_t remainingbytes = nodesize - size;
    if (remainingbytes < 32){
    	size = nodesize;
	if(cur == freelist)
		freelist = cur->next;
	else
		prev->next = cur->next;
    }
    else{
    	freenode *temp_node = freelist;
	cur = (void *) cur;
	cur->size = size;
	temp_node = (freenode *)cur;
	temp_node->size = remainingbytes;
	temp_node->next = cur->next;
	if (temp_node == freelist)
		freelist = temp_node->next;
	else
		prev->next = newnode->next;
    }
    // here, get the address for the chunk being returned to the user and return it
    cur->size = size;
    void *temp;
    temp = (void *)cur + 8;
    return temp;
}

/* return a previously allocated memory chunk to the allocator */
void free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    // make a new freenode starting 8 bytes before the provided address
    freenode *new_node = (freenode *)(ptr-8);

    // the size is already in memory at the right location (ptr-8)

    // add this memory chunk back to the beginning of the freelist
    new_node->next = freelist;
    freelist = new_node;

    return;
}

