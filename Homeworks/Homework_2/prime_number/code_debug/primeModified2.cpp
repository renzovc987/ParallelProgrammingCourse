#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h> 
//#include <math.h>
#include <stdlib.h>
#include <pthread.h>


#define NANO           1000000000
#define Max_Thread_Num 4
#define MAXIMUM        0x7fffffffffffffff
#define BLOCK_SIZE     10


long int n = 200;


/*
#define NANO           1000000000
#define Max_Thread_Num 4
#define MAXIMUM        0x7fffffffffffffff
#define BLOCK_SIZE     65536

long int n = 30000000;
*/

long int *vecPrime, nPrime, maximum; 


//
struct WorkerStatus {
       pthread_t  id;
       long int   *vecPrime;
       int        nPrime;
       int        maximum;
}  threads[Max_Thread_Num];


//
int totalThread;
pthread_cond_t   cMaster, cWorker;
pthread_mutex_t  mtx;
long int lbound, ubound, task_size;




// sort algorithm for the buckets
//------------------------------------------------------------------------------------------------------------
long int *sortBucket(long int *bucket,long int *nElementInBucket,long int nBucket)
{  
   long int i, j;
   long int bucketHead;
   long int temp_new, temp_old;
   long int index_new, index_old;
   long int tempShift;
   long int *listHead;
   long int  *listHeadIndex;
      

// bucket list head element record 
   listHead = (long int *)malloc(nBucket*sizeof(long int));
   listHeadIndex = (long int *)malloc(nBucket*sizeof(long int));


   tempShift = 0;
   for (i = 0; i < nBucket; i++)
   {  

      bucketHead = *(bucket + tempShift);               // find the first element in every bucket.
      tempShift += *(nElementInBucket + i);

      *(listHeadIndex + i) = i;
      *(listHead + i) = bucketHead;

   }


   printf("\n\n");
   printf("List Head\n");
   for (i = 0; i< nBucket;i++)
  {
     printf("Element value %ld and id is %ld\n",*(listHead+i), *(listHeadIndex+i));
  }




//bubble sort algorithm O(n^2)
 
  for (i = 0; i < nBucket; i++)
  {

     temp_old = *(listHead+i);
     index_old = *(listHeadIndex+i); 
    
     for(j = i + 1; j < nBucket;j++)
     {
        temp_new = *(listHead+j);
        index_new = *(listHeadIndex+j); 
        if(temp_new < temp_old)     // swap operator
        {
           *(listHead+j) = temp_old;            // value old to new
           *(listHeadIndex+j) = index_old;      // index old to new
           *(listHead+i) = temp_new;            // value new to old
           *(listHeadIndex+i) = index_new;      // index new to old
        }
        temp_old = *(listHead+i);
        index_old = *(listHeadIndex+i);  
     }
  }
  
   printf("\n\n");
   for (i = 0; i< nBucket;i++)
  {
     printf("Element value %ld and id is %ld\n",*(listHead+i), *(listHeadIndex+i));
  }


  free(listHead);

  return listHeadIndex;
  
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------







// sort all elements in the bucket
// function sortAllElements
//------------------------------------------------------------------------------------------------------------

long int *sortAllElements(long int *bucket,long int *nElementInBucket,long int nBucket
,long int *indexBucket,long int nPrime)
{  
   long int i,j;
   long int *temp, tempShift, bucketShift;
   long int index;

   long int *temp_nElementInBucket;
   long int temp_n; 
   
   temp = bucket;
   bucket = (long int*)malloc(nPrime * sizeof(long int));
   
   temp_nElementInBucket = (long int*)malloc(nBucket * sizeof(long int));
   memcpy(temp_nElementInBucket,nElementInBucket,nBucket * sizeof(long int));

   bucketShift = 0;
   for (i = 0; i < nBucket; i++)
   {
       index = *(indexBucket + i);
       tempShift = 0;        
       for (j = 0; j < index; j++)
         {
            tempShift += *(temp_nElementInBucket + j);
         }
       
       memcpy(bucket + bucketShift, (temp + tempShift), (*(temp_nElementInBucket + index)) * sizeof(long int));
       
       temp_n = *(nElementInBucket + index);
       *(nElementInBucket + index) = *(nElementInBucket + i);
       *(nElementInBucket + i) = temp_n;
       
       bucketShift += *(temp_nElementInBucket + index);
   }
  
  free(temp);

  return bucket;
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------


















// serial prime algorithm
//------------------------------------------------------------------------------------------------------------
void serial_prime(long int arg) {
    long int  i, j, k, *temp;
    
    maximum = BLOCK_SIZE;
    vecPrime = (long int*)malloc(maximum*sizeof(long int));
    vecPrime[0] = 2;
    vecPrime[1] = 3;
    j = 2;
    nPrime = 1;
    lbound = 5;
    while (lbound<arg) {
         
         ubound = vecPrime[nPrime]*vecPrime[nPrime];
         if ( ubound<0 || ubound>arg ) ubound = arg;
         
         for ( i=lbound; i<ubound; i+=2 ) {
            while ( i>vecPrime[j-1]*vecPrime[j-1] ) j++;
            for ( k=1; k<j; k++ ) if ( i%vecPrime[k] == 0 ) break;
            if ( k<j ) continue;
            nPrime++;
            if ( nPrime==maximum ) {
               maximum += BLOCK_SIZE;
               temp = vecPrime;
               vecPrime = (long int*) malloc(maximum*sizeof(long int));
               memcpy(vecPrime, temp, (maximum - BLOCK_SIZE) * sizeof(long int));
               free(temp);
            }
            vecPrime[nPrime]=i;
         }
         
         lbound = ubound + 2;
    }
    nPrime++;
    return;
} 
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------




//------------------------------------------------------------------------------------------------------------
int cmpLongInt(const void *p1, const void *p2) {
    long int val1 = *((long int *)p1), val2 = *((long int *)p2);
    if ( val1<val2 ) return -1;
    if ( val1==val2 ) return 0;
    return 1;
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------





// parallel prime algorithm
// mutex worker threads
//------------------------------------------------------------------------------------------------------------   
void *mtx_worker(void *arg) {
     long int i, j, k, *temp;
     long int loc_lbound, loc_ubound;
     WorkerStatus  *pMyStatus=threads;
     
     while ( pMyStatus->id!=pthread_self() ) pMyStatus++; 
     pMyStatus->maximum = BLOCK_SIZE;
     pMyStatus->vecPrime = (long int*) malloc(pMyStatus->maximum*sizeof(long int));
     
     pthread_mutex_lock(&mtx);     
     totalThread++;          
     pthread_cond_broadcast(&cWorker); 
     j = 2;    
     while(lbound<MAXIMUM) {
          pthread_cond_wait(&cMaster, &mtx);
          if ( lbound==MAXIMUM ) continue;
          pthread_mutex_unlock(&mtx);
          
          pMyStatus->nPrime = 0;
          while ( true ) {
          	pthread_mutex_lock(&mtx);
                loc_lbound = lbound;
          	lbound += task_size;
//          	loc_lbound = lbound;                 // There is a problem in original sorce code
          	pthread_mutex_unlock(&mtx);
          	if ( loc_lbound >= ubound ) break;
          	loc_ubound = loc_lbound + task_size;
          	if ( loc_ubound > ubound ) loc_ubound = ubound;
          	
          	for ( i=loc_lbound; i<loc_ubound; i+=2 ) {
          	    while ( i>vecPrime[j-1]*vecPrime[j-1] ) j++;
          	    for ( k=1; k<j; k++ ) if ( i%vecPrime[k] == 0 ) break;
          	    if ( k<j ) continue;
          	    pMyStatus->vecPrime[pMyStatus->nPrime]=i;
          	    pMyStatus->nPrime++;
          	    if ( pMyStatus->nPrime == pMyStatus->maximum ) {
          	       temp = pMyStatus->vecPrime; 
          	       pMyStatus->maximum += BLOCK_SIZE;
          	       pMyStatus->vecPrime = (long int*) malloc(pMyStatus->maximum*sizeof(long int));
          	       memcpy(pMyStatus->vecPrime, temp, pMyStatus->nPrime*sizeof(long int));
          	       free(temp);
          	    }          	    
          	}
          }         
          
          pthread_mutex_lock(&mtx);
          totalThread++;
          pthread_cond_broadcast(&cWorker);
     }
     pthread_mutex_unlock(&mtx);         
     free(pMyStatus->vecPrime);
    return (void*)0;
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------






// mutex master threads
//------------------------------------------------------------------------------------------------------------
void pthread_prime_mtx(long int arg) {
     int thread_num=sysconf(_SC_NPROCESSORS_ONLN);
     long int  i, j, k, *temp;     

   // Code modified
   //---------------------
   long int nBucket;
   long int *nElementInBucket;                                           // This pointer is to show how many 
//   long int *bucket;                                                     // This pointer point to  all elements in the bucket
   
   long int *indexBucket;                                                // this pointer point to buckets' index
   //---------------------


     maximum = BLOCK_SIZE;
     vecPrime = (long int*)malloc(maximum*sizeof(long int));
     vecPrime[0] = 2;
     vecPrime[1] = 3;
     j = 2;
     nPrime = 2;
     lbound = 5;

     // Code modified
     //---------------------
     nBucket = 1;
   
     nElementInBucket = (long int*)malloc(sizeof(long int));
     *nElementInBucket = 2;
        
     //---------------------

     totalThread = 0;
     pthread_mutex_init(&mtx, NULL);
     pthread_cond_init(&cWorker, NULL);
     pthread_cond_init(&cMaster, NULL);
     if ( thread_num>Max_Thread_Num ) thread_num = Max_Thread_Num;
     for(i=0; i<thread_num; i++) pthread_create(&(threads[i].id), NULL, mtx_worker, NULL);
     pthread_mutex_lock(&mtx);
     while ( totalThread!=thread_num ) pthread_cond_wait(&cWorker, &mtx);    
     
     while (lbound<arg) {
         ubound = vecPrime[nPrime-1]*vecPrime[nPrime-1];
         if ( ubound<0 || ubound>arg ) ubound = arg;
         task_size = (ubound-lbound)/(10*thread_num);
         if ( task_size<10 ) task_size = (ubound-lbound+thread_num-1)/thread_num;
         if ( task_size%2==1 ) task_size++;
         
         totalThread = 0;
         pthread_cond_broadcast(&cMaster);         
         while ( totalThread!=thread_num ) pthread_cond_wait(&cWorker, &mtx);


                  
         for (k=0; k<thread_num; k++) 
         {
             i = indexBucket[k];
             if ( threads[i].nPrime==0 ) continue;
             if ( nPrime+threads[i].nPrime>maximum ) {
             	temp = vecPrime;
             	while( nPrime+threads[i].nPrime>maximum ) maximum += BLOCK_SIZE;
             	vecPrime = (long int*) malloc(maximum*sizeof(long int));
             	memcpy(vecPrime, temp, nPrime*sizeof(long int));                                   // copy to vecPrime[0]
             	free(temp);                                                                         
             }
                                                                                     
             memcpy(vecPrime+nPrime, threads[i].vecPrime, threads[i].nPrime*sizeof(long int));     // copy to vecPrime[0 + nPrime]
             nPrime += threads[i].nPrime;       



             // Code modified create bucket data sturcture
             //---------------------
               nBucket ++; 
               
               temp = nElementInBucket;  
               nElementInBucket = (long int*)malloc(nBucket*sizeof(long int));
               memcpy(nElementInBucket, temp, (nBucket-1)*sizeof(long int));
               *(nElementInBucket +  (nBucket-1)) = threads[i].nPrime;
               free(temp);

               printf("\nnBucket: %ld\n",nBucket);
               for (j = 0; j < nBucket; j++)
               {
                   printf("\n threads[i].nPrime: %d\n",threads[i].nPrime);
                   printf("\nNo. %ld Bucket, elements number: %ld \n",j,*(nElementInBucket +  j));       
               }

               
             //---------------------

         }
        
// bucket sort instead of quick sort
//----------------------------

         printf("\nBefore sort:\n");
         for (i = 0; i < nPrime; i++)
         printf("No. %ld prime Number: %ld\n",i,*(vecPrime+i));


  
         indexBucket = sortBucket(vecPrime,nElementInBucket,nBucket);
  
         printf("Bucket sort index are showed below:\n");
         for (i=0; i < nBucket; i++)
         {
            printf("%ld\t", *(indexBucket + i));
         }
         printf("\n");

         vecPrime = sortAllElements(vecPrime,nElementInBucket,nBucket,indexBucket,nPrime);

   
         printf("\nAfter sort:\n");
         for (i = 0; i < nPrime; i++)
         printf("No. %ld prime Number: %ld\n",i,*(vecPrime+i)); 


//----------------------------
         
//         qsort(vecPrime, nPrime, sizeof(long int), cmpLongInt);

                  
         lbound = ubound + 2;
     }
     lbound = MAXIMUM;
     pthread_cond_broadcast(&cMaster);
     pthread_mutex_unlock(&mtx);
     
     for(i=0; i<thread_num; i++) pthread_join(threads[i].id, NULL);
     pthread_mutex_destroy(&mtx);
     pthread_cond_destroy(&cWorker);
     pthread_cond_destroy(&cMaster);
     return;
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------








// atomic worker thread
//------------------------------------------------------------------------------------------------------------
void *atomic_worker(void *arg) {
     long int i, j, k, *temp;
     long int loc_lbound, loc_ubound;
     WorkerStatus  *pMyStatus=threads;
     
     while ( pMyStatus->id!=pthread_self() ) pMyStatus++; 
     pMyStatus->maximum = BLOCK_SIZE;
     pMyStatus->vecPrime = (long int*) malloc(pMyStatus->maximum*sizeof(long int));
     
     pthread_mutex_lock(&mtx);     
     totalThread++;          
     pthread_cond_broadcast(&cWorker); 
     j = 2;    
     while(lbound<MAXIMUM) {
          pthread_cond_wait(&cMaster, &mtx);
          if ( lbound==MAXIMUM ) continue;
          pthread_mutex_unlock(&mtx);
          
          pMyStatus->nPrime = 0;
          while ( true ) {
          	loc_lbound = __sync_fetch_and_add(&lbound, task_size);
          	if ( loc_lbound >= ubound ) break;
          	loc_ubound = loc_lbound + task_size;
          	if ( loc_ubound > ubound ) loc_ubound = ubound;
          	
          	for ( i=loc_lbound; i<loc_ubound; i+=2 ) {
          	    while ( i>vecPrime[j-1]*vecPrime[j-1] ) j++;
          	    for ( k=1; k<j; k++ ) if ( i%vecPrime[k] == 0 ) break;
          	    if ( k<j ) continue;
          	    pMyStatus->vecPrime[pMyStatus->nPrime]=i;
          	    pMyStatus->nPrime++;
          	    if ( pMyStatus->nPrime == pMyStatus->maximum ) {
          	       temp = pMyStatus->vecPrime; 
          	       pMyStatus->maximum += BLOCK_SIZE;
          	       pMyStatus->vecPrime = (long int*) malloc(pMyStatus->maximum*sizeof(long int));
          	       memcpy(pMyStatus->vecPrime, temp, pMyStatus->nPrime*sizeof(long int));
          	       free(temp);
          	    }          	    
          	}
          }         
          
          pthread_mutex_lock(&mtx);
          totalThread++;
          pthread_cond_broadcast(&cWorker);
     }
     pthread_mutex_unlock(&mtx);         
     free(pMyStatus->vecPrime);
    return (void*)0;
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------





// atomic master thread
//------------------------------------------------------------------------------------------------------------
void pthread_prime_atomic(long int arg) {
     int thread_num=sysconf(_SC_NPROCESSORS_ONLN);
     long int  i, j, k, *temp;     

     maximum = BLOCK_SIZE;
     vecPrime = (long int*)malloc(maximum*sizeof(long int));
     vecPrime[0] = 2;
     vecPrime[1] = 3;
     j = 2;
     nPrime = 2;
     lbound = 5;

     totalThread = 0;
     pthread_mutex_init(&mtx, NULL);
     pthread_cond_init(&cWorker, NULL);
     pthread_cond_init(&cMaster, NULL);
     if ( thread_num>Max_Thread_Num ) thread_num = Max_Thread_Num;
     for(i=0; i<thread_num; i++) pthread_create(&(threads[i].id), NULL, atomic_worker, NULL);
     pthread_mutex_lock(&mtx);
     while ( totalThread!=thread_num ) pthread_cond_wait(&cWorker, &mtx);    
     
     while (lbound<arg) {
         ubound = vecPrime[nPrime-1]*vecPrime[nPrime-1];
         if ( ubound<0 || ubound>arg ) ubound = arg;
         task_size = (ubound-lbound)/(10*thread_num);
         if ( task_size<10 ) task_size = (ubound-lbound+thread_num-1)/thread_num;
         if ( task_size%2==1 ) task_size++;
         
         totalThread = 0;
         pthread_cond_broadcast(&cMaster);         
         while ( totalThread!=thread_num ) pthread_cond_wait(&cWorker, &mtx);
         for (i=0; i<thread_num; i++) {
             if ( threads[i].nPrime==0 ) continue;
             if ( nPrime+threads[i].nPrime>maximum ) {
             	temp = vecPrime;
             	while( nPrime+threads[i].nPrime>maximum ) maximum += BLOCK_SIZE;
             	vecPrime = (long int*) malloc(maximum*sizeof(long int));
             	memcpy(vecPrime, temp, nPrime*sizeof(long int));
             	free(temp);
             }
             memcpy(vecPrime+nPrime, threads[i].vecPrime, threads[i].nPrime*sizeof(long int)); 
             nPrime += threads[i].nPrime;       
         }
         
         qsort(vecPrime, nPrime, sizeof(long int), cmpLongInt);
         lbound = ubound + 2;
     }
     lbound = MAXIMUM;
     pthread_cond_broadcast(&cMaster);
     pthread_mutex_unlock(&mtx);
     
     for(i=0; i<thread_num; i++) pthread_join(threads[i].id, NULL);
     pthread_mutex_destroy(&mtx);
     pthread_cond_destroy(&cWorker);
     pthread_cond_destroy(&cMaster);
     return;
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------





// dup master thread
//------------------------------------------------------------------------------------------------------------
void *dup_worker(void *arg) {
     long int i, j, k, *temp, loc_vecPrime[BLOCK_SIZE];
     long int loc_lbound, loc_ubound, loc_nPrime;
     WorkerStatus  *pMyStatus=threads;
     
     while ( pMyStatus->id!=pthread_self() ) pMyStatus++;
     
     pthread_mutex_lock(&mtx);     
     totalThread++;          
     pthread_cond_broadcast(&cWorker); 
     j = 2;    
     while(lbound<MAXIMUM) {
          pthread_cond_wait(&cMaster, &mtx);
          if ( lbound==MAXIMUM ) continue;
          pthread_mutex_unlock(&mtx);
          
          loc_nPrime = 0;
          while ( true ) {
          	loc_lbound = __sync_fetch_and_add(&lbound, task_size);
          	if ( loc_lbound >= ubound ) break;
          	loc_ubound = loc_lbound + task_size;
          	if ( loc_ubound > ubound ) loc_ubound = ubound;
          	
          	for ( i=loc_lbound; i<loc_ubound; i+=2 ) {
          	    while ( i>pMyStatus->vecPrime[j-1]*pMyStatus->vecPrime[j-1] ) j++;
          	    for ( k=1; k<j; k++ ) if ( i%pMyStatus->vecPrime[k] == 0 ) break;
          	    if ( k<j ) continue;
          	    loc_vecPrime[loc_nPrime]=i;
          	    loc_nPrime++;
          	    if ( loc_nPrime == BLOCK_SIZE ) {
          	       pthread_mutex_lock(&mtx);
          	       temp = vecPrime;          	       
          	       maximum += BLOCK_SIZE;
          	       vecPrime = (long int*) malloc(maximum*sizeof(long int));
          	       memcpy(vecPrime, temp, nPrime*sizeof(long int));
          	       memcpy(vecPrime+nPrime, loc_vecPrime, BLOCK_SIZE*sizeof(long int));
          	       nPrime += BLOCK_SIZE;
          	       pthread_mutex_unlock(&mtx);
          	       free(temp);
          	       loc_nPrime = 0;
          	    }          	    
          	}
          } 
          pthread_mutex_lock(&mtx);        
          if ( loc_nPrime>0 ) {
             if ( nPrime + loc_nPrime > maximum ) {
                temp = vecPrime;
                maximum += BLOCK_SIZE;
                vecPrime = (long int*) malloc(maximum*sizeof(long int));
                memcpy(vecPrime, temp, nPrime*sizeof(long int));
             }
             memcpy(vecPrime+nPrime, loc_vecPrime, loc_nPrime*sizeof(long int));
             nPrime += loc_nPrime;
          }
          totalThread++;
          pthread_cond_broadcast(&cWorker);
     }
     pthread_mutex_unlock(&mtx);         
     return (void*)0;
}

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------






// dup master thread
//------------------------------------------------------------------------------------------------------------
void pthread_prime_dup(long int arg) {
     int thread_num=sysconf(_SC_NPROCESSORS_ONLN);
     long int  i, j, k, *temp;     

     maximum = BLOCK_SIZE;
     vecPrime = (long int*)malloc(maximum*sizeof(long int));
     vecPrime[0] = 2;
     vecPrime[1] = 3;
     j = 2;
     nPrime = 2;
     lbound = 5;

     totalThread = 0;
     pthread_mutex_init(&mtx, NULL);
     pthread_cond_init(&cWorker, NULL);
     pthread_cond_init(&cMaster, NULL);
     if ( thread_num>Max_Thread_Num ) thread_num = Max_Thread_Num;
     for(i=0; i<thread_num; i++) {
        threads[i].vecPrime = NULL;
        pthread_create(&(threads[i].id), NULL, dup_worker, NULL);
     }
     pthread_mutex_lock(&mtx);                                                         
     while ( totalThread!=thread_num ) pthread_cond_wait(&cWorker, &mtx);    
     
     while (lbound<arg) {
         ubound = vecPrime[nPrime-1]*vecPrime[nPrime-1];
         if ( ubound<0 || ubound>arg ) ubound = arg;
         task_size = (ubound-lbound)/(10*thread_num);
         if ( task_size<10 ) task_size = (ubound-lbound+thread_num-1)/thread_num;
         if ( task_size%2==1 ) task_size++; 
         
         totalThread = 0;
         for (i=0; i<thread_num; i++) {
             if ( threads[i].vecPrime!=NULL ) free(threads[i].vecPrime);
             threads[i].nPrime = nPrime;
             threads[i].vecPrime = (long int*) malloc(nPrime*sizeof(long int));
             memcpy(threads[i].vecPrime, vecPrime, nPrime*sizeof(long int));
         }
         pthread_cond_broadcast(&cMaster);
         
         while ( totalThread!=thread_num ) pthread_cond_wait(&cWorker, &mtx);
         
         qsort(vecPrime, nPrime, sizeof(long int), cmpLongInt);
         lbound = ubound + 2;
     }
     lbound = MAXIMUM;
     pthread_cond_broadcast(&cMaster);
     pthread_mutex_unlock(&mtx);
     
     for(i=0; i<thread_num; i++) {
        pthread_join(threads[i].id, NULL);
        if ( threads[i].vecPrime != NULL ) free(threads[i].vecPrime);
     }
     pthread_mutex_destroy(&mtx);
     pthread_cond_destroy(&cWorker);
     pthread_cond_destroy(&cMaster);
     return;
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------






// main function 
//------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[] ){
    struct timespec ts,te;
    double serial_cost, mtx_cost, atomic_cost, dup_cost;
    
    if (n<=0) n=MAXIMUM;
    
    clock_gettime(CLOCK_REALTIME, &ts);
    serial_prime(n);
    free(vecPrime);
    clock_gettime(CLOCK_REALTIME, &te);
    serial_cost = te.tv_sec - ts.tv_sec + (double)(te.tv_nsec-ts.tv_nsec)/NANO;
    printf("serial: found %ld primes  cost = %15.10f \n", nPrime, serial_cost);

    clock_gettime(CLOCK_REALTIME, &ts);
    pthread_prime_mtx(n);
    free(vecPrime);
    clock_gettime(CLOCK_REALTIME, &te);
    mtx_cost = te.tv_sec - ts.tv_sec + (double)(te.tv_nsec-ts.tv_nsec)/NANO;
    printf("mtx   : found %ld primes  cost = %15.10f    speedup = %f \n", nPrime, mtx_cost, serial_cost/mtx_cost);
    
    clock_gettime(CLOCK_REALTIME, &ts);
    pthread_prime_atomic(n);
    free(vecPrime);
    clock_gettime(CLOCK_REALTIME, &te);
    atomic_cost = te.tv_sec - ts.tv_sec + (double)(te.tv_nsec-ts.tv_nsec)/NANO;
    printf("atomic: found %ld primes  cost = %15.10f    speedup = %f \n", nPrime, atomic_cost, serial_cost/atomic_cost);

    clock_gettime(CLOCK_REALTIME, &ts);
    pthread_prime_dup(n);
    free(vecPrime);
    clock_gettime(CLOCK_REALTIME, &te);
    dup_cost = te.tv_sec - ts.tv_sec + (double)(te.tv_nsec-ts.tv_nsec)/NANO;
    printf("dup   : found %ld primes  cost = %15.10f    speedup = %f \n", nPrime, dup_cost, serial_cost/dup_cost);
    
    return EXIT_SUCCESS;
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------


