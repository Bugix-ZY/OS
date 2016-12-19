//hw4_paging

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define REF_SIZE 	6000
#define PRESENT 	2
#define IN_USE 		1
#define PFN			0
#define true		1
#define false		0


/*Queue - linked list implementation*/
/* reference: https://gist.github.com/mycodeschool/7510222 */

struct Node {
	int data;
	struct Node* next;
};

// Two glboal variables to store address of front and rear nodes. 
struct Node* front = NULL;
struct Node* rear = NULL;

// To Enqueue an integer
void Enqueue(int x) {
	struct Node* temp = 
		(struct Node*)malloc(sizeof(struct Node));
	temp->data =x; 
	temp->next = NULL;
	if(front == NULL && rear == NULL){
		front = rear = temp;
		return;
	}
	rear->next = temp;
	rear = temp;
}

// To Dequeue an integer.
void Dequeue() {
	struct Node* temp = front;
	if(front == NULL) {
		printf("Queue is Empty\n");
		return;
	}
	if(front == rear) {
		front = rear = NULL;
	}
	else {
		front = front->next;
	}
	free(temp);
}

int Front() {
	if(front == NULL) {
		printf("Queue is empty\n");
		return 0;
	}
	return front->data;
}

void Print() {
	struct Node* temp = front;
	while(temp != NULL) {
		printf("%d ",temp->data);
		temp = temp->next;
	}
	printf("\n");
}
/*-------------------------------------*/



/* Doubly Linked List implementation */
/* reference: https://www.tutorialspoint.com/data_structures_algorithms/doubly_linked_list_program_in_c.htm */
struct node {
   int key; //reference
   struct node *next;
   struct node *prev;
};

//this link always point to first Link
struct node *head = NULL;
//this link always point to last Link 
struct node *last = NULL;
struct node *current = NULL;

//is list empty
int isEmpty() {
   return head == NULL;
}

int length() {
   int length = 0;
   struct node *current;
   for(current = head; current != NULL; current = current->next){
      length++;
   }
   return length;
}

//display the list in from first to last
void displayForward() {
   //start from the beginning
   struct node *ptr = head;
   //navigate till the end of the list
   printf("[");
   while(ptr != NULL) {        
      printf("%d ",ptr->key);
      ptr = ptr->next;
   }
   printf("]\n");
}
//insert link at the first location
void insertFirst(int key) {
   //create a link
   struct node *link = (struct node*) malloc(sizeof(struct node));
   link->key = key;
	
   if(isEmpty()) {
      //make it the last link
      last = link;
   } else {
      //update first prev link
      head->prev = link;
   }
   //point it to old first link
   link->next = head;
   //point first to new first link
   head = link;
}

//insert link at the last location
void insertLast(int key) {
   //create a link
   struct node *link = (struct node*) malloc(sizeof(struct node));
   link->key = key;

   if(isEmpty()) {
      //make it the last link
      last = link;
   } else {
      //make link a new last link
      last->next = link;     
      //mark old last node as prev of new link
      link->prev = last;
   }
   //point last to new last node
   last = link;
}

//delete first item
struct node* deleteFirst() {
   //save reference to first link
   struct node *tempLink = head;
	
   //if only one link
   if(head->next == NULL){
      last = NULL;
   } else {
      head->next->prev = NULL;
   }
   head = head->next;
   //return the deleted link
   return tempLink;
}

//delete link at the last location
struct node* deleteLast() {
   //save reference to last link
   struct node *tempLink = last;
	
   //if only one link
   if(head->next == NULL) {
      head = NULL;
   } else {
      last->prev->next = NULL;
   }
	
   last = last->prev;
	
   //return the deleted link
   return tempLink;
}
//delete a link with given key
struct node* delete(int key) {

   //start from the first link
   struct node* current = head;
   struct node* previous = NULL;
	
   //if list is empty
   if(head == NULL) {
      return NULL;
   }
   //navigate through list
   while(current->key != key) {
      //if it is last node
		
      if(current->next == NULL) {
         return NULL;
      } else {
         //store reference to current link
         previous = current;
			
         //move to next link
         current = current->next;             
      }
   }

   //found a match, update the link
   if(current == head) {
      //change first to point to next link
      head = head->next;
   } else {
      //bypass the current link
      current->prev->next = current->next;
   }    

   if(current == last) {
      //change last to point to prev link
      last = current->prev;
   } else {
      current->next->prev = current->prev;
   }
	
   return current;
}

int insertAfter(int key, int newKey) {
   //start from the first link
   struct node *current = head; 
	
   //if list is empty
   if(head == NULL) {
      return false;
   }

   //navigate through list
   while(current->key != key) {
	
      //if it is last node
      if(current->next == NULL) {
         return false;
      } else {           
         //move to next link
         current = current->next;
      }
   }
	
   //create a link
   struct node *newLink = (struct node*) malloc(sizeof(struct node));
   newLink->key = newKey;

   if(current == last) {
      newLink->next = NULL; 
      last = newLink; 
   } else {
      newLink->next = current->next;         
      current->next->prev = newLink;
   }
	
   newLink->prev = current; 
   current->next = newLink; 
   return true; 
}
/*---------------------------------------------*/


int get_free_disk_number(int disk[], int vp){
for(int i = 0; i < vp; i++){
		if (disk[i] == -1)
			return i;
	}
	return vp;
}

int is_memory_full(int memory[], int pf){
	for(int i = 0; i < pf; i++){
		if (memory[i] == -1)
			return i;// not full, return the smallest block number
	}
	return -1;//full
}

int random_select(int (*page_table)[3], int vp){
	int max = length(); 
	//randomize();
	int number = rand() % max + 1;
	int count = 0;
	int i ;
	for( i = 0 ;i < vp; i++ ){
		if(page_table[i][PRESENT] == 1)
			count++;
		if(count == number)
			break;
	}
	printf("EVIICT = %d\n", i);
	return i;
}

/*---------------------------------------------*/
void fifo(FILE *fp, FILE *out, int vp, int pf){


	printf("====== FIFO ======\n");
	printf("virtual page   = %d\n", vp);
	printf("physical frame = %d\n", pf);
	

	float rate = 0.0;
	int hit = 0;
	int miss = 0;

	int memory[pf];
	int page_table[vp][3];
	int disk[vp]; // make sure have enough space for envited page
	int reference[REF_SIZE];

	//initialization
	memset(memory, -1, pf * sizeof(int));
	memset(page_table, 0, sizeof(page_table));
	memset(reference, -1, REF_SIZE * sizeof(int));
	memset(disk, -1, vp * sizeof(int));
	for(int i = 0; i < vp; i++) 
		page_table[i][0] = -1; // pfn=1

	char line[50];
	int page;
	char *entry;

	if(fgets(line, 50, fp) != NULL){
		//trace..
	}

	int i = 0;
	while(fgets(line, 50, fp) != NULL){
		entry = strtok(line, " ");
		entry = strtok(NULL, " ");
		page = atoi(entry);
		reference[i] = page;
		i += 1;
	}

	//main loop 
	char buffer[1024];
	for(i = 0; i < REF_SIZE && reference[i] != -1; i++){
		memset(buffer, 0, 1024 * sizeof(char));
		// in memory
		if (page_table[reference[i]][PRESENT] == 1){
			sprintf(buffer, "Hit, %d=>%d\n", reference[i], page_table[reference[i]][PFN]);
			hit += 1;
		}
		
		// page fault
		else if (page_table[reference[i]][PRESENT] == 0){
			miss += 1;
			int mm = 0; // record the smallest frame number of memory
			if((mm = is_memory_full(memory, pf)) != -1) { // not full
				// once memory is full, it will stay 'full', so page in(-1 to mm) with smallest mm number
				// enqueue that and change the page_table
				// queue
				Enqueue(reference[i]);
				//memory
				memory[mm] = reference[i];
				//printf("mm = %d\n", mm);

				int old_pfn = page_table[reference[i]][PFN];
				//page table
				page_table[reference[i]][PRESENT] = 1;
				page_table[reference[i]][IN_USE] = 1;
				page_table[reference[i]][PFN] = mm;

				sprintf(buffer, "Miss, %d, -1>>-1, %d<<%d\n", page_table[reference[i]][PFN], reference[i], old_pfn);


			} else { 

				/*page out*/
				// choose the first element in the queue to be the evicted virtual page
				int evict = Front(); 
				Dequeue();
				// page out: disk
				int dk = get_free_disk_number(disk, vp);
				disk[dk] = evict;
				// page out: memory
				int old_pfn = page_table[evict][PFN];
				memory[old_pfn] = -1;
				// page out: page table
				// the evicted virtual page will not in the memory any more
				// inuse bit still = 1, because need to use the entry to store the disk number
				// PFN - store the disk number
				page_table[evict][PRESENT] = 0; 
				page_table[evict][PFN] = dk;


				/*page in*/
				// page in: enqueue
				Enqueue(reference[i]);
				// page in: memory
				memory[old_pfn] = reference[i];
				int resource = page_table[reference[i]][PFN];
				// page in: disk
				disk[resource] = -1;
				// page in: page table
				page_table[reference[i]][PFN] = old_pfn;
				page_table[reference[i]][IN_USE] = 1;
				page_table[reference[i]][PRESENT] = 1;

				sprintf(buffer, "Miss, %d, %d>>%d, %d<<%d\n", page_table[reference[i]][PFN], evict, dk, reference[i], resource);
			}
				
		}
		fputs(buffer, out);
	}

	rate = (float)miss / (float)(hit + miss);
	sprintf(buffer, "Page fault Rate: %.3f\n", rate);
	fputs(buffer, out);


}

void lru(FILE *fp, FILE *out, int vp, int pf){
	printf("====== LRU ======\n");
	printf("virtual page   = %d\n", vp);
	printf("physical frame = %d\n", pf);

	float rate = 0.0;
	int hit = 0;
	int miss = 0;

	int memory[pf];
	int page_table[vp][3];
	int disk[vp]; // make sure have enough space for envited page
	int reference[REF_SIZE];

	//initialization
	memset(memory, -1, pf * sizeof(int));
	memset(page_table, 0, sizeof(page_table));
	memset(reference, -1, REF_SIZE * sizeof(int));
	memset(disk, -1, vp * sizeof(int));
	for(int i = 0; i < vp; i++) 
		page_table[i][0] = -1; // pfn=1

	char line[50];
	int page;
	char *entry;

	if(fgets(line, 50, fp) != NULL){
		//trace..
	}

	int i = 0;
	while(fgets(line, 50, fp) != NULL){
		entry = strtok(line, " ");
		entry = strtok(NULL, " ");
		page = atoi(entry);
		reference[i] = page;
		i += 1;
	}

	//main loop 
	char buffer[1024];
	for(i = 0; i < REF_SIZE && reference[i] != -1; i++){
		memset(buffer, 0, 1024 * sizeof(char));
		// in memory
		if (page_table[reference[i]][PRESENT] == 1){
			//change the stack
			delete(reference[i]);
			if(head == NULL)
				insertFirst(reference[i]);
			else
				insertLast(reference[i]);
			sprintf(buffer , "Hit, %d=>%d\n", reference[i], page_table[reference[i]][PFN]);
			hit += 1;
		}
		
		// page fault
		else if (page_table[reference[i]][PRESENT] == 0){
			miss += 1;
			int mm = 0; // record the smallest frame number of memory
			if((mm = is_memory_full(memory, pf)) != -1) { // not full
				// once memory is full, it will stay 'full', so page in(-1 to mm) with smallest mm number
				if(head == NULL)
					insertFirst(reference[i]);
				else
					insertLast(reference[i]);
				// memory
				memory[mm] = reference[i];
				// printf("mm = %d\n", mm);

				int old_pfn = page_table[reference[i]][PFN];
				//page table
				page_table[reference[i]][PRESENT] = 1;
				page_table[reference[i]][IN_USE] = 1;
				page_table[reference[i]][PFN] = mm;
				sprintf(buffer, "Miss, %d, -1>>-1, %d<<%d\n", page_table[reference[i]][PFN], reference[i], old_pfn);

			} else { 

				/*page out*/
				int evict = head->key;
				deleteFirst();
				
				// page out: disk
				int dk = get_free_disk_number(disk, vp);
				disk[dk] = evict;
				// page out: memory
				int old_pfn = page_table[evict][PFN];
				memory[old_pfn] = -1;
				// page out: page table
				// the evicted virtual page will not in the memory any more
				// inuse bit still = 1, because need to use the entry to store the disk number
				// PFN - store the disk number
				page_table[evict][PRESENT] = 0; 
				page_table[evict][PFN] = dk;


				/*page in*/
				// page in: push
				if(head == NULL)
					insertFirst(reference[i]);
				else
					insertLast(reference[i]);
				// page in: memory
				memory[old_pfn] = reference[i];
				int resource = page_table[reference[i]][PFN];
				// page in: disk
				disk[resource] = -1;
				// page in: page table
				page_table[reference[i]][PFN] = old_pfn;
				page_table[reference[i]][IN_USE] = 1;
				page_table[reference[i]][PRESENT] = 1;

				sprintf(buffer, "Miss, %d, %d>>%d, %d<<%d\n", page_table[reference[i]][PFN], evict, dk, reference[i], resource);
			}

		}
		fputs(buffer, out);
	}

	rate = (float)miss / (float)(hit + miss);
	sprintf(buffer, "Page fault Rate: %.3f\n", rate);
	fputs(buffer, out);

}

void random_paging(FILE *fp, FILE *out, int vp, int pf){
	printf("====== Random ======\n");
	printf("virtual page   = %d\n", vp);
	printf("physical frame = %d\n", pf);

	float rate = 0.0;
	int hit = 0;
	int miss = 0;

	int memory[pf];
	int page_table[vp][3];
	int disk[vp]; // make sure have enough space for envited page
	int reference[REF_SIZE];

	//initialization
	memset(memory, -1, pf * sizeof(int));
	memset(page_table, 0, sizeof(page_table));
	memset(reference, -1, REF_SIZE * sizeof(int));
	memset(disk, -1, vp * sizeof(int));
	for(int i = 0; i < vp; i++) 
		page_table[i][0] = -1; // pfn= -1

	char line[50];
	int page;
	char *entry;

	if(fgets(line, 50, fp) != NULL){
		//trace..
	}

	int i = 0;
	while(fgets(line, 50, fp) != NULL){
		entry = strtok(line, " ");
		entry = strtok(NULL, " ");
		page = atoi(entry);
		reference[i] = page;
		i += 1;
	}

	//main loop 
	char buffer[1024];
	for(i = 0; i < REF_SIZE && reference[i] != -1; i++){
		memset(buffer, 0, 1024 * sizeof(char));
		// in memory
		if (page_table[reference[i]][PRESENT] == 1){
			//change the stack
			delete(reference[i]);
			if(head == NULL)
				insertFirst(reference[i]);
			else
				insertLast(reference[i]);
			sprintf(buffer, "Hit, %d=>%d\n", reference[i], page_table[reference[i]][PFN]);
			hit += 1;
		}
		
		// page fault
		else if (page_table[reference[i]][PRESENT] == 0){
			miss += 1;
			int mm = 0; // record the smallest frame number of memory
			if((mm = is_memory_full(memory, pf)) != -1) { // not full
				// once memory is full, it will stay 'full', so page in(-1 to mm) with smallest mm number
				if(head == NULL)
					insertFirst(reference[i]);
				else
					insertLast(reference[i]);
				// memory
				memory[mm] = reference[i];
				// printf("mm = %d\n", mm);

				int old_pfn = page_table[reference[i]][PFN];
				//page table
				page_table[reference[i]][PRESENT] = 1;
				page_table[reference[i]][IN_USE] = 1;
				page_table[reference[i]][PFN] = mm;
				sprintf(buffer, "Miss, %d, -1>>-1, %d<<%d\n", page_table[reference[i]][PFN], reference[i], old_pfn);

			} else { 

				/*page out*/
				// choose a victim randomly
				int evict = random_select(page_table, vp);
				delete(evict);

				// page out: disk
				int dk = get_free_disk_number(disk, vp);
				disk[dk] = evict;
				// page out: memory
				int old_pfn = page_table[evict][PFN];
				memory[old_pfn] = -1;
				// page out: page table
				// the evicted virtual page will not in the memory any more
				// inuse bit still = 1, because need to use the entry to store the disk number
				// PFN - store the disk number
				page_table[evict][PRESENT] = 0; 
				page_table[evict][PFN] = dk;


				/*page in*/
				// page in: push
				if(head == NULL)
					insertFirst(reference[i]);
				else
					insertLast(reference[i]);
				// page in: memory
				memory[old_pfn] = reference[i];
				int resource = page_table[reference[i]][PFN];
				// page in: disk
				disk[resource] = -1;
				// page in: page table
				page_table[reference[i]][PFN] = old_pfn;
				page_table[reference[i]][IN_USE] = 1;
				page_table[reference[i]][PRESENT] = 1;

				sprintf(buffer, "Miss, %d, %d>>%d, %d<<%d\n", page_table[reference[i]][PFN], evict, dk, reference[i], resource);
			}

		}
		fputs(buffer, out);
	}

	rate = (float)miss / (float)(hit + miss);
	sprintf(buffer, "Page fault Rate: %.3f\n", rate);
	fputs(buffer, out);

}

int main(){
	// read the input file
	// get N, M
	// e.g. fifo_20_10_500

	FILE *fp, *out;
	char line[50];  // line for policy	
	char num[50];   // line for numbers
	char *entry;
	char *policy;
	int vp = 0;
	int pf = 0;

	fp = fopen("./os2016_hw4_trace/random/fifo_20_10_5000.txt", "rb");
	out = fopen("fifo_20_10_5000_out.txt", "wb");

	if (fp == NULL)
		printf("error opening file\n");
	else {
		char *delim = ":";

		// read policy
		if(fgets(line, 50, fp) != NULL){
			entry = strtok(line, delim);
			entry = strtok(NULL, delim);
			entry = strtok(entry, " ");
			policy = strtok(entry, "\n");
		}

		// read vp and pf
		if(fgets(num, 50, fp) != NULL){
			entry = strtok(num, delim);
			entry = strtok(NULL, delim);
			entry = strtok(entry, " ");
			vp = atoi(entry);
		}

		if(fgets(num, 50, fp) != NULL){
			entry = strtok(num, delim);
			entry = strtok(NULL, delim);
			entry = strtok(entry, " ");
			pf = atoi(entry);
		}
		
		// policy, vp, pf
		if(strcmp(policy, "FIFO") == 0){
			fifo(fp, out, vp, pf);
		} else if (strcmp(policy, "LRU") == 0){
			lru(fp, out, vp,pf);
		} else if (strcmp(policy, "Random") == 0){
			random_paging(fp, out, vp, pf);
		}
	}


	return 0;
}
