#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"
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

  /* Initialize OpenCL runtime system and find devices */
  if (!init_opencl(argv[1]))
    return -1;

  int N = 8;
  
  /* Load the kernel */
  { cl_int status;
    kernel = clCreateKernel(program, "vec_add10", &status);
    checkError(status, "Failed to create kernel"); }

  /* Let's test our device */
  float Vector_on_CPU[N];
  for (int i = 0; i < N; i++) Vector_on_CPU[i] = i; // Fill it with 0
  printf("Vector on CPU before kernel execution: ");
  for (int i = 0; i < N; i++) printf("%f ", Vector_on_CPU[i]); printf("\n");

  /* Allocate memory on the FPGA */
  cl_mem Vector_on_FPGA;
  { cl_int status;
    Vector_on_FPGA = clCreateBuffer(context, CL_MEM_READ_WRITE, N * sizeof(float), NULL, &status);
    checkError(status, "Failed to allocate memory on device");
  }

  /* Copy Vector_on_CPU to Vector_on_FPGA */
  { cl_int status;
    status = clEnqueueWriteBuffer(queue, Vector_on_FPGA, CL_TRUE, 0,
				  N * sizeof(float), Vector_on_CPU, 0, NULL, NULL);
    checkError(status, "Failed to transfer data from host to device"); }

  /* Run the FPGA */
  {
    const size_t global_work_size = N;
    cl_int status;
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &Vector_on_FPGA);
    checkError(status, "Failed to set argument 0");
    //status = clEnqueueTask(queue,kernel,0,NULL,NULL);
    status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    checkError(status, "Failed launching kernel");  
  }

 
  /* Copy Back Vector_on_FPGA to Vector_on_CPU */
  { cl_int status;
    status = clEnqueueReadBuffer(queue, Vector_on_FPGA, CL_TRUE, 0,
				  N * sizeof(float), Vector_on_CPU, 0, NULL, NULL);
    checkError(status, "Failed to transfer data from host to device"); }
  
  /* Let's wait until all commands finished */
  clFinish(queue);

  /* Print results */
  printf("Vector on CPU after kernel execution: ");
  for (int i = 0; i < N; i++) printf("%f ", Vector_on_CPU[i]); printf("\n");
  
  return 0;
}


