#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>

#include "helpers.h"
#define MEM_SIZE 1024*1024

/**
 * @brief This function recursively traverse the source directory.
 * 
 * @param dir_name : The source directory name.
 */

// mutex lock
sem_t *read_lock;
sem_t *write_lock;


// shared memory

char* shared_memory_ptr_parent_group_8;
char* shared_memory_ptr_child_group_8;
int* stop_ptr_parent_group_8;
int* stop_ptr_child_group_8;
int shmid_group_8;
int shmid_group_8_file_end;



// 4.3 tried implementing stack to achieve bfs traversal

typedef struct FileNode_group_8 {
	char *file_name;
	struct FileNode_group_8 *next;
} filenode;

typedef struct FileStack_group_8 {
	filenode *top;
} filestack;

filestack stack;

filenode *createNode(char *file_name) {
	filenode *new_node = (filenode *)malloc(sizeof(filenode));
	new_node->file_name = file_name;
	new_node->next = NULL;
	return new_node;
}

void initStack() {
	stack.top = NULL;
}

void push(filenode *node) {
	node->next = stack.top;
	stack.top = node;
}

char *pop() {
	if (isEmpty()) {
		return "";
	}
	filenode *temp = stack.top;
	stack.top = stack.top->next;
	char *file_name = temp->file_name;
	free(temp);
	return file_name;

}

int isEmpty() {
	return stack.top == NULL;
}

// traverseDir function needs to be implemented
void traverseDir(char *dir_name, char files[][100], int* count);

int main(int argc, char **argv) {
	int process_id; // Process identifier 
	
    // The source directory. 
    // It can contain the absolute path or relative path to the directory.
	char *dir_name = argv[1];

	if (argc < 2) {
		printf("Main process: Please enter a source directory name.\nUsage: ./main <dir_name>\n");
		exit(-1);
	}

	// 4.11 tried saving path and file name by the traverse function
	char filelist_group_8[100][100];
	//number of files
	int filecount_group_8 = 0;
	int *filecount_ptr_group_8 = &filecount_group_8;


	// create the filestack to do bfs traversal
	initStack();


	traverseDir(dir_name, filelist_group_8, filecount_ptr_group_8);

    /////////////////////////////////////////////////
    // You can add some code here to prepare before fork.
    /////////////////////////////////////////////////

	// shared memory
	shmid_group_8 = shmget(IPC_PRIVATE, MEM_SIZE, IPC_CREAT | 0666);
	shmid_group_8_file_end = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);

	// semaphores
	sem_unlink("read_lock");
	sem_unlink("write_lock");
	read_lock = sem_open("read_lock", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
	write_lock = sem_open("write_lock", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);

	// check for error
	if (write_lock != SEM_FAILED && read_lock != SEM_FAILED) {
		printf("Successfully created new semaphores!\n");
	} else {
		// error occured
		exit(-1);
	} 


	switch (process_id = fork()) {

	default:
		/*
			Parent Process
		*/
		printf("Parent process: My ID is %jd\n", (intmax_t) getpid());
        
        /////////////////////////////////////////////////
        // Implement your code for parent process here.
        /////////////////////////////////////////////////

		shared_memory_ptr_parent_group_8 = (char *)shmat(shmid_group_8, 0, 0);
		stop_ptr_parent_group_8 = (int *)shmat(shmid_group_8_file_end, 0, 0);
		stop_ptr_parent_group_8[0] = 0;

		for (int i = 0; i < filecount_group_8; i++) {
			sem_wait(write_lock);
			FILE *file = fopen(filelist_group_8[i], "r");
			if (file == NULL) {
				printf("Error opening file %s\n", filelist_group_8[i]);
				exit(1);
			}
			else {
				long long int file_size = fileLength(file);
				while(file_size > MEM_SIZE - 1) { // read until the end of the file if the file is larger than the shared memory
					fread(shared_memory_ptr_parent_group_8, 1, MEM_SIZE - 1, file);
					// null terminate the string
					shared_memory_ptr_parent_group_8[MEM_SIZE - 1] = '\0';
					// signal the child process
					file_size -= MEM_SIZE - 1;
					sem_post(read_lock);
					sem_wait(write_lock);

				}
				// exits the loop when the file is smaller than the shared memory
				fread(shared_memory_ptr_parent_group_8, 1, file_size, file);
				shared_memory_ptr_parent_group_8[file_size] = '\0';
				// signal the child process
				fclose(file);
				sem_post(read_lock);
				
			}
		}

		// signal the child process that there are no more files to read
		stop_ptr_parent_group_8[0] = 1;
		waitpid(process_id, NULL, 0);
		// clean up
		shmdt(shared_memory_ptr_parent_group_8);
		shmdt(stop_ptr_parent_group_8);
		

		printf("Parent process: Finished.\n");
		break;

	case 0:
		/*
			Child Process
		*/

		printf("Child process: My ID is %jd\n", (intmax_t) getpid());

        /////////////////////////////////////////////////
        // Implement your code for child process here.
        /////////////////////////////////////////////////

		int tot_count_group_8 = 0;
		shared_memory_ptr_child_group_8 = (char *)shmat(shmid_group_8, 0, 0);
		stop_ptr_child_group_8 = (int *)shmat(shmid_group_8_file_end, 0, 0);

		// below is incorrect because it waits for the read lock even if it needs to exit
		/* while (1) {
			sem_wait(read_lock);
			if (stop_ptr_child_group_8[0] == 1) {
				break;
			}
			// count the number of words in the shared memory
			tot_count_group_8 += wordCount(shared_memory_ptr_child_group_8) - 1;
			sem_post(write_lock);
		} */

		/* sem_wait(read_lock);
		while (1) {
			if (stop_ptr_child_group_8[0] == 1) {
				break;
			}
			sem_wait(read_lock);
			// count the number of words in the shared memory
			tot_count_group_8 += wordCount(shared_memory_ptr_child_group_8) - 1;
			sem_post(write_lock);
			
		} */
		sem_wait(read_lock);
		while(stop_ptr_child_group_8[0] == 0) {
			sem_wait(read_lock);
			tot_count_group_8 += wordCount(shared_memory_ptr_child_group_8) - 1;
			sem_post(write_lock);
		}
		shmdt(shared_memory_ptr_child_group_8);
		shmdt(stop_ptr_child_group_8);

		printf("file coutn: %d\n", filecount_group_8);
		printf("Child process: Total number of words in all files: %d\n", tot_count_group_8);
		printf("Child process: Total number of words in all files: %d\n", tot_count_group_8 + filecount_group_8);
		saveResult("p2_result.txt", tot_count_group_8 + filecount_group_8);


		printf("Child process: Finished.\n");
		exit(0);

	case -1:
		/*
		Error occurred.
		*/
		printf("Fork failed!\n");
		exit(-1);
	}
	
	/////////////////////////////////////////////////
	// You can add some code here to do some post-processing after fork.
	/////////////////////////////////////////////////


	// releasing the shared memory
	shmctl(shmid_group_8, IPC_RMID, 0);
	shmctl(shmid_group_8_file_end, IPC_RMID, 0);
	sem_close(read_lock);
	sem_close(write_lock);
	sem_unlink("read_lock");
	sem_unlink("write_lock");


	exit(0);
}

/**
 * @brief This function recursively traverse the source directory.
 * 
 * @param dir_name : The source directory name.
 */
void traverseDir(char *dir_name, char filelist[][100], int* count){
   
    // Implement your code here to find out
    // all textfiles in the source directory.

	DIR *dir;
	struct dirent *entry;
	
	
	if (!(dir = opendir(dir_name))) {
	 	return;
	}

	while((entry = readdir(dir))!=NULL) {
		char path[1024];
		struct stat statbuf;
		char current_file_group_8[1024];
		snprintf(current_file_group_8, sizeof(current_file_group_8), "%s/%s", dir_name, entry->d_name);
		if (stat(current_file_group_8, &statbuf) == -1) {
			printf("Error getting file information\n");
			exit(-1);
		}
	 	if (S_ISDIR(statbuf.st_mode)) { // if it's directory
	 		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
				continue;
	 		}
	 		snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);
	 		traverseDir(path, filelist, count);
	 	}
	 	else {
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
				continue;
	 		}
	 		if (validateTextFile(entry->d_name) == 1){
	 			snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);
	 			strcpy(filelist[*count], path);
	 			(*count)++;
	 		}
	 	}

	 }
	 closedir(dir);






}