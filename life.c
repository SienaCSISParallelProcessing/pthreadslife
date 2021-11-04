/* 

   Conway's Game of Life - implementation using pthreads for parallelization

   Jim Teresco
   Williams College
   Mount Holyoke College
   Siena College

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* using globals for variables that need to be shared and accessible
   in all threads */
pthread_barrier_t barrier;
pthread_mutex_t mutex;
int gridsize;
double init_pct;
int **grid[2];
int live_count, birth_count, death_count;
int num_iters;
int num_threads;


/* a struct to pass to each of our threads that has the info about the
   simulation and its own variables it can use to */
typedef struct lifethread {
  int thread_num;
  pthread_t thread_id;
} lifethread;



// function to do a thread's computation loop
void *thread_compute(void *arg) {

  lifethread *me = (lifethread *)arg;
  int curr, prev; // local copies to avoid extra mutex locks
  int i, j, neigh_count;
  
  // figure out what this thread is computing
  int start_row = me->thread_num * gridsize / num_threads + 1;
  int end_row = start_row + gridsize / num_threads - 1;

  // local counters so we can just sum up at the end in one mutex
  int local_live_count, local_birth_count, local_death_count;
  
  /* start current grid as 0 */
  curr=0; prev=1;

  /* we can now start iterating */
  for (int iter=1; iter<=num_iters; iter++) {
    /* swap the grids */
    curr=1-curr; prev=1-prev;

    if (me->thread_num == 0) printf("Iteration %d...\n",iter);

    local_live_count=0; local_birth_count=0; local_death_count=0;

    /* visit each grid cell -- only in our row range */
    for (i=start_row;i<=end_row;i++)
      for (j=1;j<=gridsize;j++) {
	neigh_count=
	  (grid[prev][i-1][j-1]+grid[prev][i-1][j]+grid[prev][i-1][j+1]+
	   grid[prev][i][j-1]+grid[prev][i][j+1]+
	   grid[prev][i+1][j-1]+grid[prev][i+1][j]+grid[prev][i+1][j+1]);
	switch (neigh_count) {
	case 2:
	  /* no change */
	  grid[curr][i][j]=grid[prev][i][j];
	  break;
	case 3:
	  /* birth */
	  if (!grid[prev][i][j]) local_birth_count++;
	  grid[curr][i][j]=1;
	  break;
	default:
	  /* death of loneliness or overcrowding */
	  if (grid[prev][i][j]) local_death_count++;
	  grid[curr][i][j]=0;
	  break;
	}
	local_live_count+=grid[curr][i][j];
      }

    /* need to combine our counts to print those for this iteration
       in the entire simulation */
    live_count = 0; death_count = 0; birth_count = 0;
    // barrier here so we don't start accumulating results until everyone's
    // gotten here and we know the globals are zeroed out
    pthread_barrier_wait(&barrier);

    // make sure only one at a time is contributing its local values,
    // then add ours in
    pthread_mutex_lock(&mutex);
    live_count += local_live_count;
    death_count += local_death_count;
    birth_count += local_birth_count;
    pthread_mutex_unlock(&mutex);
    
    // and again to make sure everyone's contributed their local values
    pthread_barrier_wait(&barrier);

    /* print the stats but only from one thread */
    if (me->thread_num == 0) {
      printf("  Counters- living: %d, died: %d, born: %d\n",live_count,
	     death_count, birth_count);
    }
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  lifethread *thread_info;
  int i, j, rc;
  
  if (argc != 5) {
    fprintf(stderr,"Usage: %s num_threads gridsize init_pct num_iters\n",argv[0]);
    exit(1);
  }

  /* seed the random number generator */
  srand48(0);

  /* read parameters from the command line */
  num_threads=atoi(argv[1]);
  gridsize=atoi(argv[2]);
  init_pct=atof(argv[3]);
  num_iters=atoi(argv[4]);

  /* allocate the grids */
  grid[0]=(int **)malloc((gridsize+2)*sizeof(int *));
  for (i=0;i<=gridsize+1;i++)
    grid[0][i]=(int *)malloc((gridsize+2)*sizeof(int));
  grid[1]=(int **)malloc((gridsize+2)*sizeof(int *));
  for (i=0;i<=gridsize+1;i++)
    grid[1][i]=(int *)malloc((gridsize+2)*sizeof(int));

  /* initialize the grids (incl boundary buffer all 0's) */
  for (i=0;i<=gridsize+1;i++)
    for (j=0;j<=gridsize+1;j++) {
      grid[0][i][j]=0;
      grid[1][i][j]=0;
    }

  /* initialize the first (0) grid based on the desired percentage of
     living cells specified on the command line */
  live_count=0;
  for (i=1;i<=gridsize;i++)
    for (j=1;j<=gridsize;j++) {
      if (drand48()<init_pct) {
	grid[0][i][j]=1;
	live_count++;
      }
      else grid[0][i][j]=0;
    }

  printf("Initial grid has %d live cells out of %d\n",live_count,
	 gridsize*gridsize);

  // initialize our barrier
  pthread_barrier_init(&barrier, NULL, num_threads);

  // initialize our mutex
  pthread_mutex_init(&mutex, NULL);

  // allocate the simulation info for threads and start them up
  thread_info = (lifethread *)malloc(num_threads*sizeof(lifethread));
  for (int thread = 0; thread < num_threads; thread++) {
    thread_info[thread].thread_num = thread;
    rc = pthread_create(&(thread_info[thread].thread_id), NULL, thread_compute,
			&(thread_info[thread]));
    if (rc != 0) {
      fprintf(stderr, "Could not create thread %d\n", thread);
      exit(1);
    }
  }

  // threads compute....separately in the thread function instances

  // close down threads
  for (int thread = 0; thread < num_threads; thread++) {
    pthread_join(thread_info[thread].thread_id, NULL);
  }

  // clean up our thread barrier and mutex
  pthread_barrier_destroy(&barrier);
  pthread_mutex_destroy(&mutex);

  /* free the grids */
  for (i=0;i<=gridsize+1;i++)
    free(grid[0][i]);
  free(grid[0]);
  for (i=0;i<=gridsize+1;i++)
    free(grid[1][i]);
  free(grid[1]);

}
