#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_THRESHOLD 1000000000

int has_initialized = 0;

void *mem_start;
void *mem_end;

typedef struct mem_header{
	int is_available;
	long size; 
} mem_header;

void malloc_init(){
	mem_end = sbrk(0);
	mem_start = mem_end;
	has_initialized = 1;
}


void check_threshold(mem_header *head){
	char *thres;
	int thr_size, dif;
	//if the last chunk's being used we can't free anything
	if(!head->is_available)
		return;
	//has a custom threshold been set?
	thres = getenv("M_TRIM_THRESHOLD");
	if(thres == NULL)
		thr_size = DEFAULT_THRESHOLD;
	else
		thr_size = strtol(thres, NULL, 0);
	//is it beyond the threshold?
	if(head->size>thr_size){
		//free the difference
		dif = thr_size-head->size;	//negative value
		sbrk(dif);
		mem_end+=dif;
		head->size+=dif;
		printf("decreased\n");
	}
	return;
}


//checks for adjustent free chunks and merges them
//also calls threshold check at the last chunk
void check_for_merges(){
	mem_header *cur, *next, *prev = NULL;
	cur = mem_start;
	//we merge cur and prev
	while(cur != mem_end){
		next = (void*)cur+cur->size;
		//if it's the first iteration
		if(prev == NULL){
			prev = cur;
			cur = next;
			continue;
		}
		//if it's the last iteration
		if(next == mem_end){
			if(prev->is_available && cur->is_available){
				prev->size+=cur->size;
				cur=prev;
			}
			check_threshold(cur);
			return;
		}
		//merge case
		if(prev->is_available && cur->is_available){
			prev->size+=cur->size;
			cur=(void*)prev+prev->size;
			continue;
		}
		prev=cur;
		cur=next;
	}
	return;
}

void free(void *addr){
	mem_header *head;
	head = addr-sizeof(mem_header);
	head->is_available = 1;
	check_for_merges();
	return;
}

//split an existing chunk in two, one of which will have "bytes" size 
void split_chunks(mem_header *head, long bytes){
	//if there's not enough extra space for a new chunk we just return
	int extra = head->size - bytes;
	if(extra<=sizeof(mem_header)){
		return;
	}
	//init the new chunk
	mem_header *new_head = head+bytes;
	new_head->is_available=1;
	new_head->size=extra;
	//modify the old chunk's length
	head->size=bytes;
	return;
}

void *malloc(size_t bytes){
	void *cur, *result;
	int worst_size = 0;	//size of the biggest block we've seen
	int cur_size;
	mem_header *cur_head, *worst_head = NULL;
	if(!has_initialized){
		malloc_init();
	}
	bytes+=sizeof(mem_header);

	//start from the beginning
	cur = mem_start;
	while(cur != mem_end){
		cur_head = (mem_header*) cur;
		//check available
		if(cur_head->is_available){
			cur_size = cur_head->size;
			//check big enough
			if(cur_size >= bytes){
				//chunk bigger than any we've seen before
				if(cur_size>worst_size){
					worst_size = cur_size;
					worst_head = cur_head;
				}
			}
		}
		cur+=cur_head->size;
	}
	//if we haven't found an available chunk we allocate a new one
	if(worst_head == NULL){
		sbrk(bytes);
		result = mem_end;
		
		mem_end+=bytes;

		worst_head = (mem_header*)result;
		worst_head->is_available = 0;
		worst_head->size = bytes;
	}
	else{
		result = worst_head;
		worst_head->is_available = 0;
		split_chunks(worst_head, bytes);
		check_for_merges();
	}
	result+=sizeof(mem_header);
	return result;
}

void meminfo(){
	int i = 0;
	mem_header *head;
	head = mem_start;
	while(head != mem_end){
		i++;
		printf("Chunk #%d\n", i);
		printf("Address: %p\n", head);
		printf("Size:    %lu\n", head->size);
		printf("Status: ");
		if(head->is_available)
			printf("Available\n");
		else
			printf("In Use\n");
		printf("\n");

		head=(void*)head+head->size;
	}
}
