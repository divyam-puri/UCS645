/**
 * cuda_utils.cu  —  GPU utility / diagnostic functions
 *
 * Provides device-enumeration and runtime diagnostic helpers.
 * All symbols declared in cuda_interface.h that are not in cuda_hash.cu
 * belong here; currently that is just cuda_print_device_info (already
 * implemented inline in cuda_hash.cu — kept here as well for completeness
 * when building separately).
 *
 * Nothing in this file is performance-critical; it is called only once
 * at start-up from the MPI master process.
 */

#include <cuda_runtime.h>
#include <cstdio>
#include <cstring>
#include "cuda_interface.h"

extern "C" {

/**
 * Warm up the CUDA runtime on the given device.
 * Calling cudaSetDevice() + a no-op kernel initialises the CUDA context,
 * which otherwise happens lazily on the first real kernel call and
 * inflates the first-launch latency.
 */
void cuda_warmup(int device_id)
{
    cudaSetDevice(device_id);
    cudaFree(nullptr);   /* forces context creation */
}

/**
 * Returns the peak theoretical memory bandwidth (GB/s) for the device.
 * Useful for reporting in the notebook.
 */
double cuda_peak_bandwidth_gbps(int device_id)
{
    cudaDeviceProp prop;
    if (cudaGetDeviceProperties(&prop, device_id) != cudaSuccess) return 0.0;

    /* bandwidth = 2 * memClockRate (kHz) * memBusWidth (bits) / 8 / 1e6 GB/s */
    double bw = 2.0
              * (double)prop.memoryClockRate         /* kHz */
              * (double)(prop.memoryBusWidth / 8)    /* bytes / clock */
              / 1.0e6;                               /* kHz → GB/s  */
    return bw;
}

} /* extern "C" */
