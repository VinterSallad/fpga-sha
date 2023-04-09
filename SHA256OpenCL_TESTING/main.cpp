#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <time.h>

using namespace std;
using namespace aocl_utils;

cl_platform_id platform = NULL;
unsigned num_devices = 0;
scoped_array<cl_device_id> device; // num_devices elements
cl_context context = NULL;
cl_command_queue queue;
cl_program program = NULL;
cl_kernel kernel;


void cleanup() {
  if(program)
    clReleaseProgram(program);

  if(context) 
    clReleaseContext(context);
  exit(0);
}


bool init_opencl (const char *aocxFile)
{
  cl_int status;
  fprintf(stderr,"Initializing OpenCL runtime...\n");
  if(!setCwdToExeDir())
    { return false; }
  
  platform = findPlatform("Intel(R) FPGA Emulation Platform for OpenCL(TM)");
  if(platform == NULL) {
    printf("ERROR: Unable to find FPGA OpenCL platform.\n");
    return false; }
  
  device.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));
  printf("Platform: %s\n", getPlatformName(platform).c_str());
  printf("Using %d device(s)\n", num_devices);
  for(unsigned i = 0; i < num_devices; ++i) 
    printf("  %s\n", getDeviceName(device[i]).c_str());
  context = clCreateContext(NULL, num_devices, device, &oclContextCallback, NULL, &status);
  checkError(status, "Failed to create context");
  std::string binary_file = getBoardBinaryFile(aocxFile, device[0]);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), device, num_devices);
  
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");
  
  queue = clCreateCommandQueue(context, device[0], 0, &status);
  checkError(status, "Failed to initialize queue");   //}
  return true;
}	



int main(int argc, char **argv) {
  cl_int status;
  /* Initialize OpenCL runtime system and find devices */
  if (!init_opencl(argv[1]))
    return -1;


  
  /* Load the kernel */
  { status;
    kernel = clCreateKernel(program, "sha256", &status);
    checkError(status, "Failed to create kernel"); }



    ifstream file;
    file.open("./lipsum.txt");
    string str;
    if(file) {
        ostringstream stream;
        stream << file.rdbuf();
        str = stream.str();
    }
  vector<uint8_t> vec(str.begin(), str.end());
  uint8_t *msgOnCPU = &vec[0];


  


  /* Let's test our device */

  uint64_t outlen = 32;
  uint8_t outputHash[outlen];
  size_t messageLength = vec.size();

  outputHash[0] = 3;


  clock_t time = clock();

  /* Allocate memory on the FPGA */
  cl_mem msgOnFPGA;
  { status;
    msgOnFPGA = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint8_t)*messageLength, NULL, &status);
    checkError(status, "Failed to allocate memory on device");
  }
  cl_mem outOnFPGA;
  { status;
    outOnFPGA = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint8_t)*outlen, NULL, &status);
    checkError(status, "Failed to allocate memory on device");
  }
  cl_mem msgLenOnFPGA;
  { status;
    msgLenOnFPGA = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(size_t), NULL, &status);
    checkError(status, "Failed to allocate memory on device");
  }


  /* Copy data to fpga */
  { status;
    status = clEnqueueWriteBuffer(queue, msgOnFPGA, CL_TRUE, 0, sizeof(uint8_t) * messageLength, msgOnCPU, 0, NULL, NULL);
    checkError(status, "Failed to transfer data from host to device"); 
  }
  { status;
    status = clEnqueueWriteBuffer(queue, outOnFPGA, CL_TRUE, 0, sizeof(uint8_t)*outlen, outputHash, 0, NULL, NULL);
    checkError(status, "Failed to transfer data from host to device"); 
  }
  { status;
    status = clEnqueueWriteBuffer(queue, msgLenOnFPGA, CL_TRUE, 0, sizeof(size_t), &messageLength, 0, NULL, NULL);
    checkError(status, "Failed to transfer data from host to device"); 
  }

  /* Run the FPGA */
  {
    const size_t global_work_size = 1;
    status = clSetKernelArg(kernel, 0, sizeof(outOnFPGA), &outOnFPGA);
        checkError(status, "Failed to set argument 0");
    status = clSetKernelArg(kernel, 1, sizeof(msgOnFPGA), &msgOnFPGA);
        checkError(status, "Failed to set argument 1");
    status = clSetKernelArg(kernel, 2, sizeof(msgLenOnFPGA), &msgLenOnFPGA);
        checkError(status, "Failed to set argument 2");
    status = clSetKernelArg(kernel, 3, sizeof(uint8_t)*40, NULL);
        checkError(status, "Failed to set argument 3");
    status = clSetKernelArg(kernel, 4, sizeof(uint8_t)*128, NULL);
        checkError(status, "Failed to set argument 4");
  
    //status = clEnqueueTask(queue,kernel,0,NULL,NULL);
    status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    checkError(status, "Failed launching kernel");  
  }

  /* Copy Back Vector_on_FPGA to Vector_on_CPU */
  { cl_int status;
    status = clEnqueueReadBuffer(queue, outOnFPGA, CL_TRUE, 0, sizeof(uint8_t)*outlen, outputHash, 0, NULL, NULL);
    checkError(status, "Failed to transfer data from host to device"); }

//DEBUG
    { cl_int status;
    status = clEnqueueReadBuffer(queue, msgOnFPGA, CL_TRUE, 0, sizeof(uint8_t)*messageLength, msgOnCPU, 0, NULL, NULL);
    checkError(status, "Failed to transfer data from host to device"); }

    { cl_int status;
    status = clEnqueueReadBuffer(queue, msgOnFPGA, CL_TRUE, 0, sizeof(uint8_t)*messageLength, msgOnCPU, 0, NULL, NULL);
    checkError(status, "Failed to transfer data from host to device"); }
  
  /* Let's wait until all commands finished */
  clFinish(queue);
  printf("Time taken: %.10fs\n", (double)(clock() - time)/CLOCKS_PER_SEC);

  printf("\n");
  for(int i = 0; i < outlen; i++)
  {
    printf("%02x", outputHash[i]);
  }
  printf("\n");

  return 0;
}


