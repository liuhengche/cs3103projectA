#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/shm.h>        // This is necessary for using shared memory constructs
#include <semaphore.h>      // This is necessary for using semaphore
#include <fcntl.h>          // This is necessary for using semaphore
#include <pthread.h>        // This is necessary for Pthread          
#include <string.h>


#include "helpers.h"


// To prevent multiple students to define semaphore with the same name, 
// please always define the name of a semaphore ending with "_GROUP_NAME"
// where GROUP is your group number and NAME is your full name.
// For example, if you have 3 semaphores, you can define them as:
// semaphore_1_GROUP_NAME, semaphore_2_GROUP_NAME, semaphore_3_GROUP_NAME ...
#define PARAM_ACCESS_SEMAPHORE "/param_access_semaphore_GROUP_8"
#define NUM_THREADS 9


long int global_param = 0;


#define DIGIT_ONE "DIGIT_ONE"
#define DIGIT_TWO "DIGIT_TWO"
#define DIGIT_THREE "DIGIT_THREE"
#define DIGIT_FOUR "DIGIT_FOUR"
#define DIGIT_FIVE "DIGIT_FIVE"
#define DIGIT_SIX "DIGIT_SIX"
#define DIGIT_SEVEN "DIGIT_SEVEN"
#define DIGIT_EIGHT "DIGIT_EIGHT"
#define DIGIT_NINE "DIGIT_NINE"






int digits_group_8[NUM_THREADS + 2];
sem_t* semaphores_group_8[NUM_THREADS];




/**
* This function should be implemented by yourself. It must be invoked
* in the child process after the input parameter has been obtained.
* @parms: The input parameter from the terminal.
*/
void multi_threads_run(long int input_param, long int loop_time, int time_to_exit_group_8);


int main(int argc, char **argv)
{
   int shmid, status;
   long int local_param = 0;
   long int *shared_param_p, *shared_param_c;


   if (argc < 2) {
       printf("Please enter a nine-digit decimal number as the input parameter.\nUsage: ./main <input_param>\n");
       exit(-1);
   }


   /*
       Creating semaphores. Mutex semaphore is used to acheive mutual
       exclusion while processes access (and read or modify) the global
       variable, local variable, and the shared memory.
   */ 


   // Checks if the semaphore exists, if it exists we unlink him from the process.
   sem_unlink(PARAM_ACCESS_SEMAPHORE);
   
   // Create the semaphore. sem_init() also creates a semaphore. Learn the difference on your own.
   sem_t *param_access_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);


   // Check for error while opening the semaphore
   if (param_access_semaphore != SEM_FAILED){
       printf("Successfully created new semaphore!\n");
   }   
   else if (errno == EEXIST) {   // Semaphore already exists
       printf("Semaphore appears to exist already!\n");
       param_access_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE, 0);
   }
   else {  // An other error occured
       assert(param_access_semaphore != SEM_FAILED);
       exit(-1);
   }


   /*  
       Creating shared memory. 
       The operating system keeps track of the set of shared memory
       segments. In order to acquire shared memory, we must first
       request the shared memory from the OS using the shmget()
       system call. The second parameter specifies the number of
       bytes of memory requested. shmget() returns a shared memory
       identifier (SHMID) which is an integer. Refer to the online
       man pages for details on the other two parameters of shmget()
   */
   shmid = shmget(IPC_PRIVATE, sizeof(long int), 0666|IPC_CREAT); // We request an array of one long integer


   /* 
       After forking, the parent and child must "attach" the shared
       memory to its local data segment. This is done by the shmat()
       system call. shmat() takes the SHMID of the shared memory
       segment as input parameter and returns the address at which
       the segment has been attached. Thus shmat() returns a char
       pointer.
   */


   if (fork() == 0) { // Child Process
       
       printf("Child Process: Child PID is %jd\n", (intmax_t) getpid());
       
       /*  shmat() returns a long int pointer which is typecast here
           to long int and the address is stored in the long int pointer shared_param_c. */
       shared_param_c = (long int *) shmat(shmid, 0, 0);


       while (1) // Loop to check if the variables have been updated.
       {
           // Get the semaphore
           sem_wait(param_access_semaphore);
           printf("Child Process: Got the variable access semaphore.\n");


           if ( (global_param != 0) || (local_param != 0) || (shared_param_c[0] != 0) )
           {
               printf("Child Process: Read the global variable with value of %ld.\n", global_param);
               printf("Child Process: Read the local variable with value of %ld.\n", local_param);
               printf("Child Process: Read the shared variable with value of %ld.\n", shared_param_c[0]);


               // Release the semaphore
               sem_post(param_access_semaphore);
               printf("Child Process: Released the variable access semaphore.\n");
               
               break;
           }


           // Release the semaphore
           sem_post(param_access_semaphore);
           printf("Child Process: Released the variable access semaphore.\n");
       }


       /**
        * After you have fixed the issue in Problem 1-Q1, 
        * uncomment the following multi_threads_run function 
        * for Problem 1-Q2. Please note that you should also
        * add an input parameter for invoking this function, 
        * which can be obtained from one of the three variables,
        * i.e., global_param, local_param, shared_param_c[0].
        */
       // multi_threads_run(shared_param_c[0]);




       // rewrite this logic; it is not correct
       // long int num_of_operations_group_8 = strtol(argv[2], NULL, 10);
       // int flag = 0;
       // int temp = (int)(num_of_operations_group_8);
       // for (int i = 0; i < temp; i++) {
       //  if (i = temp - 1) {
       //      flag = -1;
       //  }
       //  multi_threads_run(shared_param_c[0], i , flag);
       // }
       /* each process should "detach" itself from the 
          shared memory after it is used */
       //printf("doesnt the child process reach here?\n");
       long int loop_time = strtol(argv[2], NULL, 10);
       int time_to_exit_group_8 = -1;
       //printf("Child Process: The input parameter is %s\n", argv[1]);
       
       multi_threads_run(shared_param_c[0], loop_time, time_to_exit_group_8);


       shmdt(shared_param_c);


       exit(0);
   }
   else { // Parent Process


       printf("Parent Process: Parent PID is %jd\n", (intmax_t) getpid());


       /*  shmat() returns a long int pointer which is typecast here
           to long int and the address is stored in the long int pointer shared_param_p.
           Thus the memory location shared_param_p[0] of the parent
           is the same as the memory locations shared_param_c[0] of
           the child, since the memory is shared.
       */
       shared_param_p = (long int *) shmat(shmid, 0, 0);


       // Get or lock the semaphore first
       sem_wait(param_access_semaphore);
       printf("Parent Process: Got the variable access semaphore.\n");


       global_param = strtol(argv[1], NULL, 10);
       local_param = strtol(argv[1], NULL, 10);
       shared_param_p[0] = strtol(argv[1], NULL, 10);


       // Release or unlock the semaphore
       sem_post(param_access_semaphore);
       printf("Parent Process: Released the variable access semaphore.\n");
       
       wait(&status);


       /* each process should "detach" itself from the 
          shared memory after it is used */


       shmdt(shared_param_p);


       /* Child has exited, so parent process should delete
          the created shared memory. Unlike attach and detach,
          which is to be done for each process separately,
          deleting the shared memory has to be done by only
          one process after making sure that noone else
          will be using it 
        */


       shmctl(shmid, IPC_RMID, 0);


       // Close and delete semaphore. 
       sem_close(param_access_semaphore);
       sem_unlink(PARAM_ACCESS_SEMAPHORE);


       exit(0);
   }


   exit(0);
}


/**
* This function should be implemented by yourself. It must be invoked
* in the child process after the input parameter has been obtained.
* @parms: The input parameter from terminal.
*/




void *thread_function(int *para) {
   int *digit = para;
   int *traverse = digit;
   int pos_to_exit = 0;
   long int loop_time = 0;
   //printf("could we reach here?\n");
   while(*traverse != -1) {
       traverse++;
       //printf("this is the traverse value: %ld\n", *traverse);
       pos_to_exit++;
   }
   //printf("this is the pos to exit: %d\n", pos_to_exit);
   loop_time = (long int)*(traverse + 1);
   int thread_id = NUM_THREADS - pos_to_exit;
   //printf("this is the thread id: %d\n", thread_id);
   int index1 = thread_id;
   int index2 = thread_id + 1;
   if (index2 == NUM_THREADS) {
       index2 = 0;
   }


   while(1) {
       int getSemResult1 = sem_trywait(semaphores_group_8[index1]);
       int getSemResult2 = sem_trywait(semaphores_group_8[index2]);
       if (getSemResult1 == 0 && getSemResult2 == 0) {
           break;
       }
       else {
           if (getSemResult1 == 0) {
               sem_post(semaphores_group_8[index1]);
           }
           if (getSemResult2 == 0) {
               sem_post(semaphores_group_8[index2]);
           }
           usleep(100);
       }
   }


   for (int i = 0; i < loop_time; i++) {
       *digit = (*digit + 1) % 10;
       if (*(digit + 1) == -1) {
           *(digit - 8) = (*(digit-8) + 1) % 10;
       }
       else {
           *(digit + 1) = (*(digit + 1) + 1) % 10;
       }
   }


   sem_post(semaphores_group_8[index1]);
   sem_post(semaphores_group_8[index2]);
   printf("Thread %d finished\n", thread_id);
   return NULL;
   
}






void multi_threads_run(long int input_param, long int loop_time, int time_to_exit_group_8)
{
   // Add your code here


   //first we need to create the semaphores
   sem_unlink(DIGIT_ONE);
   sem_unlink(DIGIT_TWO);
   sem_unlink(DIGIT_THREE);
   sem_unlink(DIGIT_FOUR);
   sem_unlink(DIGIT_FIVE);
   sem_unlink(DIGIT_SIX);
   sem_unlink(DIGIT_SEVEN);
   sem_unlink(DIGIT_EIGHT);
   sem_unlink(DIGIT_NINE);


   semaphores_group_8[0] = sem_open(DIGIT_ONE, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
   semaphores_group_8[1] = sem_open(DIGIT_TWO, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
   semaphores_group_8[2] = sem_open(DIGIT_THREE, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
   semaphores_group_8[3] = sem_open(DIGIT_FOUR, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
   semaphores_group_8[4] = sem_open(DIGIT_FIVE, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
   semaphores_group_8[5] = sem_open(DIGIT_SIX, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
   semaphores_group_8[6] = sem_open(DIGIT_SEVEN, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
   semaphores_group_8[7] = sem_open(DIGIT_EIGHT, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
   semaphores_group_8[8] = sem_open(DIGIT_NINE, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);


   pthread_t threads_group_8[NUM_THREADS];


   // store the nine digits of the input
   for (int i = 0; i < NUM_THREADS; i++) {
       int result = input_param % 10;
       digits_group_8[8-i] = result;
       input_param = input_param / 10;
   }
   digits_group_8[9] = -1;
   digits_group_8[10] = loop_time;


   //printf("we proceeds to before the thread creation\n");


   for (int i = 0; i < NUM_THREADS; i++) {
       //printf("we proceeds to during the process %d creation\n", i);
       pthread_create(&threads_group_8[i], NULL, thread_function, &digits_group_8[i]);
   }


   //printf("we proceeds to after the thread creation\n");


   for (int i = 0; i < NUM_THREADS; i++) {
       pthread_join(threads_group_8[i], NULL);
   }


  // printf("we proceeds to after the thread join\n");




   // pthread_t threads_group_8[NUM_THREADS];
   // int temp = (int)(input_param);
   // int index = 8;
   // if (time == 0 ) {
   //  while(temp) {
   //      int result = temp % 10;
   //      digits_group_8[index] = result;
   //      temp = temp / 10;
   //      index--;
   //  }
   // }
   // for (int i = 0; i < NUM_THREADS; i++) {
   //  sem_init(&semaphores_group_8[i], 0, 1);
   // }
   // for (int i = 0; i < NUM_THREADS; i++) {
   //  pthread_create(&threads_group_8[i], NULL, thread_function, i);
   // }
   // for (int i = 0; i < NUM_THREADS; i++) {
   //  pthread_join(threads_group_8[i], NULL);
   // }
   // for (int i = 0; i < NUM_THREADS; i++) {
   //  sem_close(&semaphores_group_8[i]);
   // }
   // int output = 0;
   // if (flag == -1) {
   //  for (int i = 0; i < NUM_THREADS; i++) {
   //      output += digits_group_8[i];
   //      if (i != digits_group_8 - 1) {
   //          output *= 10;
   //      }
   //  }
   //  saveResult("p1_result.txt", output);
   // }


   //close the semaphores
   sem_close(semaphores_group_8[0]);
   sem_close(semaphores_group_8[1]);
   sem_close(semaphores_group_8[2]);
   sem_close(semaphores_group_8[3]);
   sem_close(semaphores_group_8[4]);
   sem_close(semaphores_group_8[5]);
   sem_close(semaphores_group_8[6]);
   sem_close(semaphores_group_8[7]);
   sem_close(semaphores_group_8[8]);
   //printf("we proceeds to after the sem close\n");


   sem_unlink(DIGIT_ONE);
   sem_unlink(DIGIT_TWO);
   sem_unlink(DIGIT_THREE);
   sem_unlink(DIGIT_FOUR);
   sem_unlink(DIGIT_FIVE);
   sem_unlink(DIGIT_SIX);
   sem_unlink(DIGIT_SEVEN);
   sem_unlink(DIGIT_EIGHT);
   sem_unlink(DIGIT_NINE);


   //printf("we proceeds to after the sem unlink\n");


   int output = 0;
   for (int i = 0; i < NUM_THREADS; i++) {
       output += digits_group_8[i];
       if (i != NUM_THREADS - 1) {
           output *= 10;
       }
   }
   printf("Output: %d\n", output);
   saveResult("p1_result.txt", output);




}