/**********
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

#include <cstdio>
#include <iostream>
#include "nearest_neighbor.h"
#include <limits.h>
#include "sds_lib.h"

#define MAX_DIMS 5

class perf_counter
{
public:
	uint64_t tot, cnt, calls;
	perf_counter() : tot(0), cnt(0), calls(0) {};
	inline void reset() { tot = cnt = calls = 0; }
	inline void start() { cnt = sds_clock_counter(); calls++; };
	inline void stop() { tot += (sds_clock_counter() - cnt); };
	inline uint64_t avg_cpu_cycles() {return (tot / calls); };
};

void find_nearest_neighbor(int *out, const int dim,
                           const int *search_points,
                           const int *points, const int len);

void verify(int *gold, int *test, int size) {
    bool match = true;

    for(int i = 0; i < size; i++) {
    
        if(gold[i] == test[i])
            continue;
        else {
            match = false;
            break;
        }
  
    }

    if (!match) {
        printf("\n TEST FAILED\n");
        exit(EXIT_FAILURE);
    }
}

void print_point(int *point, int size) {
    
    for(int i = 0; i < size; i++)
        printf("%d", point[i]);

    printf("\n");  
}

// This example illustrates the algorithm of nearest neighbor search for a given
// point (x, y) from a list of points.
int main(int argc, char **argv) {

    static const int num_points = 512;
    static const int num_dims = 2;

    // Allocate PL buffers using sds_alloc
    int *data  = (int *)sds_alloc(sizeof(int) * num_points * num_dims);
    int *input = (int *)sds_alloc(sizeof(int) * num_dims);
    int *out   = (int *)sds_alloc(sizeof(int) * num_dims);

    for(int i = 0; i < num_points * num_dims; i++){
        data[i] = i  + i;
        if(i < num_dims)
            input[i] = i * 2;
    }

    printf("Number Of Points: %d\n", num_points);
    printf("Dimensions: %d\n", num_dims);
    printf("\nInput Point:      ");
    print_point(input, num_dims);

    perf_counter hw_ctr, sw_ctr;
    
    int gold[num_dims];
    
    
    sw_ctr.start();
    //Launch the Software Solution
    find_nearest_neighbor(gold, num_dims, input, data, num_points);
    sw_ctr.stop();

    size_t array_size_bytes = num_points * num_dims * sizeof(int);
 
     
    hw_ctr.start();
    //Launch the Hardware Solution
    nearest_neighbor_loop_fusion_accel(out, data, input, num_points, num_dims); 
    hw_ctr.stop();
    
    verify(gold, out, num_dims);
   
    uint64_t sw_cycles = sw_ctr.avg_cpu_cycles();
    uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
    double speedup = (double) sw_cycles / (double) hw_cycles;
    
    std::cout << "Average number of CPU cycles running mmult in software: "
			 << sw_cycles << std::endl;
    std::cout << "Average number of CPU cycles running mmult in hardware: "
				 << hw_cycles << std::endl;
    std::cout << "Speed up: " << speedup << std::endl;
    
    printf("Nearest Neighbor: ");
    print_point(gold, num_dims);
    
    sds_free(out);
    sds_free(data);
    sds_free(input);
   
    printf("TEST PASSED\n");
    return EXIT_SUCCESS;
}

void find_nearest_neighbor(int *out, const int dim,
                           const int *search_points,
                           const int *points, const int num_points) {
  // points is the list of data points that need to be searched for the given
  // point (x, y)
  //
  // output array out has two values - the closest point to given input point
  int best_i = 0;
  int best_dist = INT_MAX;
  int s_point[MAX_DIMS];
    
  for(int d = 0; d < dim; ++d) {
     s_point[d] = search_points[d];
  }

  find_best:
  for(int p = 0; p < num_points; ++p) {
     int dist = 0;
     
        // Calculate the distance in a n-dimensional space
        dist_calc:
        for (int c = 0; c < dim; ++c) {
            int dx = abs(points[dim * p + c] - s_point[c]);
            dist += dx * dx;
        }

        if (dist < best_dist) {
            best_i = p;
            best_dist = dist;
        }   
  }    
    write_best:
    for (int c = 0; c < dim; ++c) {
    #pragma HLS PIPELINE
        out[c] = points[best_i * dim + c];
    }

}
