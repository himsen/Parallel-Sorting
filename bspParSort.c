#include "bspedupack.h"
//#include "include/mcbsp.h"


/*
  This program sort an array of integers. 
  Invariants: P is a power of two and n is divisible by p.
  Notes: 
    - Data are distributed by the block distribution (processor s contains n/p numbers) 
    - Not many boundary checks are performed. 
    - Merging is performed in a unidirected form (to the left). 
    - The text file storing the sequence must have the following format:
      [number of elements] [number 1] [number 2] etc
      And the must must be named 'sort' 
*/


//Global fields
int P; /* number of processors requested */

void bspParSort(){

  int Log2(int x);
  void mergeSort(int x, int *temp1);
  void merge2(int *arr1, int *arr2, int size);

  int *localArr; /* local array in each processor */
  int i,j,k; /* index variables */
  int n_divide_p; /* Avoid multiple computation */
  int n; /* Number of elements to be sorted */
  int szLocalArray; /* Size of local array */
  double time0, time1; /* Time */
  FILE *ifp = 0; /* Reader to read sequence of numbers to be sorted */

  bsp_begin(P);
  int p= bsp_nprocs(); /* Number of processors obtained */ 
  int s= bsp_pid();    /* Processor number */ 

  //Get number of elements to be sorted
  if(s==0){
    ifp = fopen("sort","r");
    if(ifp == NULL){
      fprintf(stderr, "Can't open input file!\n");
      exit(1);
    }
    fscanf(ifp, "%i", &n);
  }

  // Make sure every processor knows everything
  bsp_push_reg(&n,sizeof(int));
  bsp_sync();
  bsp_get(0,&n,0,&n,sizeof(int));
  bsp_sync();
  bsp_pop_reg(&n);

  //Setup distribution 
  n_divide_p = n/p;
  szLocalArray = n/pow(2,ceil(Log2(s+1)));
  localArr = vecalloci(szLocalArray);
  bsp_push_reg(localArr,sizeof(int)*szLocalArray);

  if(s==0){ 
    printf("Distribution start\n"); fflush(stdout); 
  }

  bsp_sync();
  int value;
  if(s==0){
    //allocate to array on proc 0
    for(i=0; i< n_divide_p; i++){
      fscanf(ifp, "%i", &value);
      localArr[i]=value;      
    }
    //Send to arrays on other processors
    for(i=1; i< p; i++){
      for(j=0;j<n_divide_p;j++){
        fscanf(ifp, "%i", &value);
        bsp_put(i,&value,localArr,j*sizeof(int),sizeof(int));
      }
    }
    fclose(ifp);
  }
  bsp_sync();
  if(s==0){ 
    printf("Distribution done\n"); fflush(stdout); 
  }

  //Distribution done and we can start time measurement 
  if(s==0){
    printf("Time start\n"); fflush(stdout);
  }
  time0 = bsp_time();

  //Locally sort each array
  if(s==0){
    printf("Local sort\n"); fflush(stdout);
  }
  mergeSort(n_divide_p, localArr);
  bsp_sync();

  //Merging 
  int *temp = malloc(sizeof(int)*pow(2,Log2(p))*n_divide_p);
  for(j=1;j<Log2(p)+1;j++){
    if(s<p/pow(2,j)){
      for(k=0;k<pow(2,j-1)*n_divide_p;k++){
        bsp_get(s+(p/pow(2,j)),localArr,k*sizeof(int),&(temp[k]),sizeof(int));
      }
    }
    bsp_sync();

    if(s<p/pow(2,j)){
      merge2(localArr, temp, n_divide_p*pow(2,j-1));
    }

    bsp_sync();
    if(s==0){ 
      printf("Round %i out of %i rounds of merging done (on proc 0)\n",j,Log2(p)); fflush(stdout); 
    }
  }
  if(s==0){
    printf("Sorting done\n"); fflush(stdout);
  }
  bsp_sync();
 
  //Print sorted array - expensive if sample is big
  /*
  if(s==0){
    printf("Sorted sequence is:\n");
    for(i=0; i<szLocalArray; i++){
      printf("%i ",localArr[i]); fflush(stdout);
    }
    printf("\n"); fflush(stdout);
  }
  */

  //Parallel algorithm ends
  time1 = bsp_time();
  if(s==0){
    printf("Time stop\n"); fflush(stdout);
  }

  //Report time to user
  if(s==0){
    printf("Sorting took %.6lf seconds.\n", time1-time0); fflush(stdout);
  }
  
  //Clean up
  free(temp);
  bsp_pop_reg(localArr); free(localArr);

  bsp_end();
} /* End bspParSort */


/*
* Calculates the base 2 logarithm. 
*/
int Log2(int x){
 return log(x)/log(2);
} /* End Log2 */


/*
* Merge algorithm used in bspParSort. 
* Takes two arrays as arguments and length
* (current length in current merging round). 
* Merge the second array into the first array (first argument). 
* Notice that arr1 is by construction in method bspParSort 
* big enough to hold all elements. 
*/
void merge2(int *arr1, int *arr2, int size){
  int *temp = malloc(sizeof(int)*size);
  int i,j,k;
  memcpy(temp, arr1, sizeof(int)*size);
  for(i=j=k=0; i<size && j<size; ){
    if(temp[i]<arr2[j]){
      arr1[k]=temp[i];
      i++;
      k++;
     
    }
    else{
      arr1[k]=arr2[j];
      j++;
      k++;
    }
  }
  while(i<size){
    arr1[k]=temp[i];
    i++;
    k++;
  }
  while(j<size){
    arr1[k]=arr2[j];
    j++;
    k++;   
  }
  free(temp);
} /* End merge2 */


/*
* Next three methods constitute the merge sort algorithm used to sort locally.
* This is a well known algorithm!  
*/
void merge(int *l, int left, int *r, int right, int *tempArr){
  int i,j, k;
  for(i=j=k=0; i<left && j<right;){
    if(l[i]<r[j]){
      tempArr[k]=l[i]; 
      i++;
      k++;
    }
    else{
      tempArr[k]=r[j]; 
      j++;
      k++;
    }
  }
  while(i<left){
    tempArr[k]=l[i];
    k++;
    i++;
  }
  while(j<right){
    tempArr[k]=r[j];
    k++;
    j++;
  }
} /* End merge */


void recursion(int *temp2, int *temp1, int x){
  void merge(int *l, int left, int *r, int right, int *tempArr);
  int l=x/2;
  if(x < 2) 
    return;

  recursion(temp1, temp2, l);
  recursion(temp1+l, temp2+l, x-l);
  merge(temp1, l, temp1+l, x-l, temp2);
} /* End recursion */


void mergeSort(int size, int *temp1){
  void recursion(int *temp2, int *temp1, int x);
  int *temp2 = malloc(sizeof(int)*size);
  memcpy(temp2, temp1, sizeof(int)*size);
  recursion(temp1, temp2, size);
  free(temp2);
} /* End MergeSort */
/* 
* End of the merge sort algorithm 
*/


int main(int argc, char **argv){

  bsp_init(bspParSort, argc, argv);
  
  /* sequential part */
  printf("How many processors do you want to use (must be a power of 2)?\n"); fflush(stdout);
  //No check for power of 2 invariant!
  scanf("%d",&P);
  if (P > bsp_nprocs()){
      printf("Sorry, not enough processors available.\n"); fflush(stdout);
      exit(1);
  }
  printf("\n"); fflush(stdout);
  printf("----------We start with peace----------\n"); fflush(stdout);

  /* SPMD part */
  bspParSort();

  /* sequential part */
  printf("----------And we end with peace----------\n"); fflush(stdout);
  printf("\n"); fflush(stdout);
  exit(0);

} /* end main */
