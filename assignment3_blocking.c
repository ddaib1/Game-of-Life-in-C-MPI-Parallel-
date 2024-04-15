/*
 Name: Daibik DasGupta
 RowanID: 916479074
 Homework #: 3
 To Compile: mpicc -o output assignment3.c
 To Run: mpiexec -n <num_processes> ./output <rows> <columns> <max_generations>
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

double gettime(void)
{
  struct timeval tval;
  gettimeofday(&tval, NULL);
	return((double)tval.tv_sec + (double)tval.tv_usec/1000000.0);
}

int **array_alloc(int **a, int rm, int cm)
{
	//storing 2D array as an array of pointers
	a = (int**)malloc((rm+2) * sizeof(int*));
  for(int i=0; i<rm+2; i++)
  	a[i] = (int*)malloc((cm+2) * sizeof(int));
  return a;
}

void array_print(int **a, int rm, int cm)
{
  for(int i=1; i<rm+1; i++)
  {
    printf("\n|");
    for(int j=1; j<cm+1; j++)
    {
      if(a[i][j] == 0)
      	printf("   |");
      else
        printf(" * |");
    }
  }
}

int **array_ghostcells(int **a, int rm, int cm)
{
	//one-dimensional data distribution occurs in this method
  MPI_Status status;

  //send and receive upper and lower rows
  MPI_Sendrecv(&a[1][1], cm, MPI_INT, (rank-1+size)%size, 0,
               &a[rm+1][1], cm, MPI_INT, (rank+1)%size, 0,
               MPI_COMM_WORLD, &status);

  MPI_Sendrecv(&a[rm][1], cm, MPI_INT, (rank+1)%size, 0,
               &a[0][1], cm, MPI_INT, (rank-1+size)%size, 0,
               MPI_COMM_WORLD, &status);

    //send and receive left and right columns
  MPI_Sendrecv(&a[1][1], 1, MPI_INT, (rank-1+size)%size, 1,
               &a[1][cm+1], 1, MPI_INT, (rank+1)%size, 1,
               MPI_COMM_WORLD, &status);

  MPI_Sendrecv(&a[1][cm], 1, MPI_INT, (rank+1)%size, 1,
               &a[1][0], 1, MPI_INT, (rank-1+size)%size, 1,
               MPI_COMM_WORLD, &status);
}

void array_copy(int **a, int **b, int rm, int cm)
{
  for(int i=1; i<rm+1; i++)
  {
    for(int j=1; j<cm+1; j++)
      a[i][j] = b[i][j];
  }
}

int array_aresame(int **a, int **b, int rm, int cm)
{
  int flag = 1;
  //if even a single value does not match, the flag will be lowered
  for(int i=1; i<rm+1; i++)
  {
    for(int j=1; j<cm+1; j++)
    	if(a[i][j] != b[i][j])
    		flag = 0;
  }
  return flag;
}

int count_living_cells(int **a, int r, int c)
{
  int count = 0;
  //traverse from top-left to bottom-right element to check neighbors
  for(int i=r-1; i<=r+1; i++)
  {
    for(int j=c-1; j<=c+1; j++)
    {
      if((i==r) && (j==c))
        continue;
      if(a[i][j] == 1)
        count++;
    }
  }
  return count;
}

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    printf("Usage: %s <rows> <columns> <max_generations>\n", argv[0]);
    return 1;
  }

  int row = atoi(argv[1]);
  int col = atoi(argv[2]);
  int generations = atoi(argv[3]);
	int cellcount, gencount = 1;
  int **aptr = NULL, **bptr = NULL;
	double starttime, endtime;

  MPI_Init(&argc, &argv);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int local_row = row/size;
  int local_col = col;

  aptr = array_alloc(aptr, local_row, local_col);
  bptr = array_alloc(bptr, local_row, local_col);

  //generate initial state
  for(int i=1; i<local_row+1; i++)
    for (int j=1; j<local_col+1; j++)
      aptr[i][j] = rand()%2;

  array_ghostcells(aptr, local_row, local_col, rank, size);

  starttime = gettime();

  while (gencount <= generations)
  {
    //calculate the next generation
    for (int i = 1; i < local_row + 1; i++)
    {
      for (int j = 1; j < local_col + 1; j++)
      {
        cellcount = count_living_cells(aptr, i, j);

        if (aptr[i][j] == 1)
        {
          if (cellcount < 2 || cellcount > 3)
            bptr[i][j] = 0;
          else
            bptr[i][j] = 1;
        }
        else
        {
          if (cellcount == 3)
            bptr[i][j] = 1;
          else
            bptr[i][j] = 0;
        }
      }
    }

  	//exchange boundary data for the next generation
  	array_ghostcells(bptr, local_row, local_col, rank, size);

  	//copy the new generation to the current generation
  	array_copy(aptr, bptr, local_row, local_col);

  	gencount++;
  }

  endtime = gettime();
  printf("Execution time for process %d: %lf seconds\n", rank, endtime - starttime);

	//finalize and cleanup on MPI
  MPI_Finalize();

  return 0;
}
