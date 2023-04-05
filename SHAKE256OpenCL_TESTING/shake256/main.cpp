#include <stddef.h>
#include <stdint.h>

#include <stdio.h>

#include <time.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

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


int main(int argc, char const *argv[])
{
    /* Initialize OpenCL runtime system and find devices */
    if (!init_opencl(argv[1])) return -1;

    cl_int status;

    /* Load the kernel */
    { 
        kernel = clCreateKernel(program, "shake256", &status);
        checkError(status, "Failed to create kernel"); 
    }

    //////////

    
    uint64_t outlen = 32;
    uint8_t hash[outlen];

    /*
    uint8_t input[] = {
        'f', 'i', 's', 'h', 'e', 's'
    };
    */

    ifstream file;
    file.open("../../lipsum.txt");
    string str;
    if(file) {
        ostringstream stream;
        stream << file.rdbuf();
        str = stream.str();
    }
    vector<uint8_t> vec(str.begin(), str.end());
    uint8_t *input = &vec[0];

    size_t inputLength = vec.size();

    /* Allocate memory on the FPGA */
    clock_t time = clock(); //TODO
    cl_mem hash_on_FPGA;
    { 
        hash_on_FPGA = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint8_t)*outlen, NULL, &status);
        checkError(status, "Failed to allocate memory on device");
    }
    cl_mem input_on_FPGA;
    { 
        input_on_FPGA = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint8_t)*inputLength, NULL, &status);
        checkError(status, "Failed to allocate memory on device");
    }

    /* Copy */
    { 
        status = clEnqueueWriteBuffer(queue, hash_on_FPGA, CL_TRUE, 0, sizeof(uint8_t)*outlen, hash, 0, NULL, NULL);
        checkError(status, "Failed to transfer data from host to device"); 
    }
    { 
        status = clEnqueueWriteBuffer(queue, input_on_FPGA, CL_TRUE, 0, sizeof(uint8_t)*inputLength, input, 0, NULL, NULL);
        checkError(status, "Failed to transfer data from host to device"); 
    }

    /* Run the FPGA */
    {
        const size_t global_work_size = 1;

        status = clSetKernelArg(kernel, 0, sizeof(hash_on_FPGA), &hash_on_FPGA);
        checkError(status, "Failed to set argument 0");

	    status = clSetKernelArg(kernel, 1, sizeof(uint64_t), &outlen);
        checkError(status, "Failed to set argument 1");

	    status = clSetKernelArg(kernel, 2, sizeof(input_on_FPGA), &input_on_FPGA);
        checkError(status, "Failed to set argument 2");

	    status = clSetKernelArg(kernel, 3, sizeof(uint64_t), &inputLength);
        checkError(status, "Failed to set argument 3");

        //status = clEnqueueTask(queue,kernel,0,NULL,NULL);
        status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
        checkError(status, "Failed launching kernel");  
    }

    /* Copy Back */
    { 
        status = clEnqueueReadBuffer(queue, hash_on_FPGA, CL_TRUE, 0, sizeof(uint8_t)*outlen, hash, 0, NULL, NULL);
        checkError(status, "Failed to transfer data from host to device"); 
    }
    
    /* Let's wait until all commands finished */
    clFinish(queue);
    printf("Time taken: %.2fs\n", (double)(clock() - time)/CLOCKS_PER_SEC);


    for(int i=0; i<32; i++) {
        printf("%02x", hash[i]);
    }


    return 0;
}