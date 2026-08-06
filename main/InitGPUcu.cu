#include "InitGPUcu.hpp"

#include <iostream>

//
//  Initialize fastest GPU device
//
int InitGPU(int verbose)
{
   //  Get number of CUDA devices
   int num;
   if (cudaGetDeviceCount(&num))
   {
      printf("Cannot get number of CUDA devices\n");
      exit(-1);
   }
   if (num < 1)
   {
      printf("No CUDA devices found\n");
      exit(-1);
   }

   //  Get fastest device
   cudaDeviceProp prop;
   int MaxDevice = -1;
   int MaxGflops = -1;
   for (int dev = 0; dev < num; dev++)
   {
      if (cudaGetDeviceProperties(&prop, dev))
      {
         printf("Error getting device %d properties\n", dev);
         exit(-1);
      }
      int Gflops = prop.multiProcessorCount * prop.clockRate;
      if (verbose)
         printf("CUDA Device %d: %s Gflops %f Processors %d Threads/Block %d\n", dev, prop.name, 1e-6 * Gflops, prop.multiProcessorCount, prop.maxThreadsPerBlock);
      if (Gflops > MaxGflops)
      {
         MaxGflops = Gflops;
         MaxDevice = dev;
      }
   }

   //  Print and set device
   if (cudaGetDeviceProperties(&prop, MaxDevice))
   {
      printf("Error getting device %d properties\n", MaxDevice);
      exit(-1);
   }
   printf("Fastest CUDA Device %d: %s\n", MaxDevice, prop.name);
   cudaSetDevice(MaxDevice);

   //  Return max thread count
   return prop.maxThreadsPerBlock;
}
