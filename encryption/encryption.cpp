#define _CRT_SECURE_NO_WARNINGS
#include <CL/opencl.h>
#include <iostream>
#include "Util.h"
#include "OCL_Device.h"
#include "temp.h"
#include <ctime>
#include <fstream>
#include <string>

#define FAILURE 1

void readFile(const char* filename, std::string& s)
{
	std::string line;

	std::ifstream in(filename);
	if (in.is_open())
	{
		while (std::getline(in, line))
		{
			s.append(line);
		}
	}
	in.close();
}

int main(int argc, char** argv)
{
	const char* filename = argv[1];
	std::string s = " ";
	readFile(filename, s);
	unsigned int MemorySize = 250;
	std::cout << argv[0] << " " << argv[1] << " " << argv[2] << std::endl;
	cl_context context = NULL;
	cl_command_queue commandQueue = NULL;
	cl_device_id device = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_int errNum;
	context = ocl::creteContext();
	ocl::DisplayContextInfo(context);
	commandQueue = ocl::createCommandQueue(context, &device);
	program = ocl::CreateProgram(context, device, "kernel.cl");
	kernel = clCreateKernel(program, "encrypt", &errNum);
	ocl::CHECK_OPENCL_ERROR(errNum);
	unsigned char plainText[10000];
	strcpy((char*)plainText, s.c_str());
	const char* filenameKey = argv[2];
	std::string key = " ";
	readFile(filenameKey, key);
	unsigned char cipherKey[1000];
	strcpy((char*)cipherKey, key.c_str());
	unsigned int* roundkey = new unsigned int[44];
	tmp::expandEncryptKey(roundkey, cipherKey, 16);

	unsigned long textLen = sizeof(plainText);
	unsigned int BlockSize = 256 * 16;
	unsigned long memLen = (unsigned long)((MemorySize * 1024 * 1024 + BlockSize - 1) / BlockSize) * BlockSize;
	unsigned char* kuznechik = new unsigned char[memLen];
	unsigned char* destkuznechik = new unsigned char[memLen];
	size_t globalSize[1] = { memLen / 16 };
	size_t localSize[1] = { 256 };
	for (unsigned long i = 0; i < memLen; ++i) {
		int value = (i / 16) % 3;
		if (value == 0)
			kuznechik[i] = plainText[i % textLen];
		else if (value == 1)
			kuznechik[i] = 0x00;
		else
			kuznechik[i] = 0x01;
	}
	std::clock_t time = clock();
	cl_mem srcMem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(unsigned char) * memLen, kuznechik, NULL);
	cl_mem keyMem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(unsigned int) * 44, roundkey, NULL);
	cl_mem destMem = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(unsigned char) * memLen, NULL, NULL);

	errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &srcMem);
	ocl::CHECK_OPENCL_ERROR(errNum);
	errNum = clSetKernelArg(kernel, 1, sizeof(cl_mem), &destMem);
	ocl::CHECK_OPENCL_ERROR(errNum);
	errNum = clSetKernelArg(kernel, 2, sizeof(cl_mem), &keyMem);
	ocl::CHECK_OPENCL_ERROR(errNum);
	errNum = clSetKernelArg(kernel, 3, sizeof(cl_uint) * 44, NULL);
	ocl::CHECK_OPENCL_ERROR(errNum);
	errNum = clSetKernelArg(kernel, 4, sizeof(cl_uint) * 1024, NULL);
	ocl::CHECK_OPENCL_ERROR(errNum);
	errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, globalSize, localSize, 0,
		NULL, NULL);
	ocl::CHECK_OPENCL_ERROR(errNum);
	errNum = clFinish(commandQueue);
	ocl::CHECK_OPENCL_ERROR(errNum);
	errNum = clEnqueueReadBuffer(commandQueue, destMem, CL_TRUE, 0, sizeof(unsigned char) * memLen,
		destkuznechik, 0, NULL, NULL);
	ocl::CHECK_OPENCL_ERROR(errNum);
	time = clock() - time;
	double totalTime = (double)time / CLOCKS_PER_SEC;
	std::cout << std::dec;
	std::cout << "time: " << totalTime << std::endl;
	std::cout << "speed: " << (double)(memLen / totalTime / 1024.0f / 1024.0f) << "  MBytes/s" << std::endl;;
	std::cout << std::hex;

	delete[] roundkey;
	delete[] kuznechik;
	delete[] destkuznechik;
	int a = 0;
	std::cin >> a;
	return 0;
}
