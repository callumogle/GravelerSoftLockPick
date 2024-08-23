#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#define uint64 unsigned long long

// Mersenne Twister psuedorandom number generator implementation
// taken from: https://github.com/ESultanik/mtwister
#define UPPER_MASK		0x80000000
#define LOWER_MASK		0x7fffffff
#define TEMPERING_MASK_B	0x9d2c5680
#define TEMPERING_MASK_C	0xefc60000

#include <stdint.h>
#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M      397 /* changes to STATE_VECTOR_LENGTH also require changes to this */

typedef struct tagMTRand {
  uint32_t mt[STATE_VECTOR_LENGTH];
  int32_t index;
} MTRand;

MTRand seedRand(uint32_t seed);
uint32_t genRandLong(MTRand* rand);

inline static void m_seedRand(MTRand* rand, uint32_t seed) {
  /* set initial seeds to mt[STATE_VECTOR_LENGTH] using the generator
   * from Line 25 of Table 1 in: Donald Knuth, "The Art of Computer
   * Programming," Vol. 2 (2nd Ed.) pp.102.
   */
  rand->mt[0] = seed & 0xffffffff;
  for(rand->index=1; rand->index<STATE_VECTOR_LENGTH; rand->index++) {
    rand->mt[rand->index] = (6069 * rand->mt[rand->index-1]) & 0xffffffff;
  }
}

/**
* Creates a new random number generator from a given seed.
*/
MTRand seedRand(uint32_t seed) {
  MTRand rand;
  m_seedRand(&rand, seed);
  return rand;
}

/**
 * Generates a pseudo-randomly generated long.
 */
uint32_t genRandLong(MTRand* rand) {

  uint32_t y;
  static uint32_t mag[2] = {0x0, 0x9908b0df}; /* mag[x] = x * 0x9908b0df for x = 0,1 */
  if(rand->index >= STATE_VECTOR_LENGTH || rand->index < 0) {
    /* generate STATE_VECTOR_LENGTH words at a time */
    int32_t kk;
    if(rand->index >= STATE_VECTOR_LENGTH+1 || rand->index < 0) {
      m_seedRand(rand, 4357);
    }
    for(kk=0; kk<STATE_VECTOR_LENGTH-STATE_VECTOR_M; kk++) {
      y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
      rand->mt[kk] = rand->mt[kk+STATE_VECTOR_M] ^ (y >> 1) ^ mag[y & 0x1];
    }
    for(; kk<STATE_VECTOR_LENGTH-1; kk++) {
      y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
      rand->mt[kk] = rand->mt[kk+(STATE_VECTOR_M-STATE_VECTOR_LENGTH)] ^ (y >> 1) ^ mag[y & 0x1];
    }
    y = (rand->mt[STATE_VECTOR_LENGTH-1] & UPPER_MASK) | (rand->mt[0] & LOWER_MASK);
    rand->mt[STATE_VECTOR_LENGTH-1] = rand->mt[STATE_VECTOR_M-1] ^ (y >> 1) ^ mag[y & 0x1];
    rand->index = 0;
  }
  y = rand->mt[rand->index++];
  y ^= (y >> 11);
  y ^= (y << 7) & TEMPERING_MASK_B;
  y ^= (y << 15) & TEMPERING_MASK_C;
  y ^= (y >> 18);
  return y;
}



void* thread_function(void* arg) {
    int *numLoops = (int*)arg; 
    int *result = malloc(sizeof(int)); 
    int numDice = 231;
    int oneCount = 0;
    int maxOnes = 0;
    unsigned int seed = time(NULL) ^ getpid() ^ pthread_self();
    MTRand rand = seedRand(seed);
    for (uint64 i = 0; i < *numLoops; i++){
        if (oneCount >= 177){
            break;
        }
        oneCount = 0;
        for (int j = 0; j < numDice; j++){
            int roll = genRandLong(&rand) % 4 + 1;
            if (roll == 1){
                oneCount++;
            }
            
        }
        if (oneCount > maxOnes){
            maxOnes = oneCount;
        }
    }
    *result = maxOnes;
    pthread_exit((void*)result); 
}


int main() {
    int numThreads = 8;  
    pthread_t threads[numThreads];
    int inputs[numThreads];
    void* results[numThreads];
    int totalLoops = 1000000000;
   
    for (int i = 0; i < numThreads; i++) {
        inputs[i] = totalLoops/numThreads;  
        pthread_create(&threads[i], NULL, thread_function, (void*)&inputs[i]);
    }

    int largestOneCount = 0;
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], &results[i]);
        if (largestOneCount < *(int *)results[i]){
            largestOneCount = *(int *)results[i];
        }
        free(results[i]);
    }
    printf("Highest Ones Roll: %d\n",largestOneCount);
    printf("Number of Roll Sessions: %d\n",totalLoops);
    return 0;
}

