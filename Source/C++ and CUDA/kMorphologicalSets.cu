/**
kMorphologicalSets.cu
Purpose: Cluster data using the k-Morphological Sets algorithm on the GPU. 
It contains the actual algorithm as a whole, with GPU and CPU functions in it.

@author Érick Oliveira Rodrigues
@version 1.0 9/9/2016
*/

#include <stdio.h>
#include <math.h>

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>
#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

//to call from .cpp
#include "Common.h"

//a constant for the background value in the images
#define C -1

//setting the index of your CUDA device
#define CUDA_DEVICE_INDEX 0 

#define SQUARED 1
#define DIAMOND 2
#define CROSS 3
#define CROSS_S 4
#define SQUARED_10 5

//define the chosen structuring element
#define STRUCT_ELEMENT SQUARED //it can be SQUARED, SQUARED_10, DIAMOND, CROSS or CROSS_S. Different structuring elements can be implemented as well.

//this structuring element refers to how the image is dilated before the clusterization is performed
#define STRUCT_ELEMENT_PREV SQUARED //it can be SQUARED, DIAMOND, CROSS or CROSS_S. Different structuring elements can be implemented as well.

//indexes each and every pixel of a input boolean image
__global__ void
indexImg(const bool* __restrict__ mask, int* __restrict__ data, const int WIDTH, const int HEIGHT){
	const int id = blockDim.x * blockIdx.x + threadIdx.x;

	if (id < WIDTH*HEIGHT){
		if (mask[id]) data[id] = id;
		else data[id] = C;
	}
}

//kernel for dilating an input boolean/binary image
__global__ void
dilate(const bool* __restrict__ inputImg, const int WIDTH, const int HEIGHT, bool* __restrict__ outputImg){
	const int id = blockDim.x * blockIdx.x + threadIdx.x;
	const int leap = 1;
	
	const int cX = id % WIDTH, cY = id / WIDTH;
	if (cX >= 0 && cX < WIDTH && cY >= 0 && cY < HEIGHT){
#if STRUCT_ELEMENT_PREV == SQUARED
		if (cX - leap >= 0) if (inputImg[id - leap] > 0) outputImg[id] = true;
		if (cX + leap < WIDTH) if (inputImg[id + leap] > 0) outputImg[id] = true;
		if (cY - leap >= 0) if (inputImg[id - WIDTH*leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT) if (inputImg[id + WIDTH*leap] > 0) outputImg[id] = true;
		//digonals
		if (cX - leap >= 0 && cY - leap >= 0) if (inputImg[id - WIDTH*leap - leap] > 0) outputImg[id] = true;
		if (cY - leap >= 0 && cX + leap < WIDTH) if (inputImg[id - WIDTH*leap + leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT && cX - leap >= 0) if (inputImg[id + WIDTH*leap - leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT && cX + leap < WIDTH) if (inputImg[id + WIDTH*leap + leap] > 0) outputImg[id] = true;
#elif STRUCT_ELEMENT_PREV == DIAMOND
		if (cX - leap >= 0) if (inputImg[id - leap] > 0) outputImg[id] = true;
		if (cX + leap < WIDTH) if (inputImg[id + leap] > 0) outputImg[id] = true;
		if (cY - leap >= 0) if (inputImg[id - WIDTH*leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT) if (inputImg[id + WIDTH*leap] > 0) outputImg[id] = true;
#elif STRUCT_ELEMENT_PREV == CROSS
		if (cX - leap >= 0 && cY - leap >= 0) if (inputImg[id - WIDTH*leap - leap] > 0) outputImg[id] = true;
		if (cY - leap >= 0 && cX + leap < WIDTH) if (inputImg[id - WIDTH*leap + leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT && cX - leap >= 0) if (inputImg[id + WIDTH*leap - leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT && cX + leap < WIDTH) if (inputImg[id + WIDTH*leap + leap] > 0) outputImg[id] = true;
#elif STRUCT_ELEMENT_PREV == CROSS_S
		if (cX - leap >= 0) if (inputImg[id - leap] > 0) outputImg[id] = true;
		if (cX + leap < WIDTH) if (inputImg[id + leap] > 0) outputImg[id] = true;
		if (cY - leap >= 0) if (inputImg[id - WIDTH*leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT) if (inputImg[id + WIDTH*leap] > 0) outputImg[id] = true;
		//digonals
		if (cX - leap >= 0 && cY - leap >= 0) if (inputImg[id - WIDTH*leap - leap] > 0) outputImg[id] = true;
		if (cY - leap >= 0 && cX + leap < WIDTH) if (inputImg[id - WIDTH*leap + leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT && cX - leap >= 0) if (inputImg[id + WIDTH*leap - leap] > 0) outputImg[id] = true;
		if (cY + leap < HEIGHT && cX + leap < WIDTH) if (inputImg[id + WIDTH*leap + leap] > 0) outputImg[id] = true;
		//
		if (cX - leap*3 >= 0 && cY - leap*3 >= 0) if (inputImg[id - WIDTH*leap*3 - leap*3] > 0) outputImg[id] = true;
		if (cY - leap*3 >= 0 && cX + leap*3 < WIDTH) if (inputImg[id - WIDTH*leap*3 + leap*3] > 0) outputImg[id] = true;
		if (cY + leap*3 < HEIGHT && cX - leap*3 >= 0) if (inputImg[id + WIDTH*leap*3 - leap*3] > 0) outputImg[id] = true;
		if (cY + leap*3 < HEIGHT && cX + leap*3 < WIDTH) if (inputImg[id + WIDTH*leap*3 + leap*3] > 0) outputImg[id] = true;
#endif
	}
}


//Dilates and verifies if the processing ended.
//This function is somewhat random, it may return slightly different solutions from run to run.
//However, we have tested it and in our experiments the results were always the same.
//The randomness can be mitigated if more memory is consumed to duplicate the data array.
__global__ void
dilateAndVerify(int* __restrict__ data/*img*/, volatile int* __restrict__ kArray, int *finished, 
int *idempotence, const int leap, const int WIDTH, const int HEIGHT, const int K)
{
	const int id = blockDim.x * blockIdx.x + threadIdx.x;
	int cValue = 0, old;
	int previousValue = 0;
	bool still = true;

	const int cX = id % WIDTH, cY = id / WIDTH;
	cValue = data[id];
	previousValue = cValue;
	if (id < WIDTH*HEIGHT && cValue != C){
		
		//dilate according to the chosen structuring element
#if STRUCT_ELEMENT == SQUARED
		//dilates the image
		if (cX - leap >= 0) if (data[id - leap] > cValue) cValue = data[id - leap];
		if (cX + leap < WIDTH) if (data[id + leap] > cValue) cValue = data[id + leap];
		if (cY - leap >= 0) if (data[id - WIDTH*leap] > cValue) cValue = data[id - WIDTH*leap];
		if (cY + leap < HEIGHT) if (data[id + WIDTH*leap] > cValue) cValue = data[id + WIDTH*leap];
		//digonals
		if (cX - leap >= 0 && cY - leap >= 0) if (data[id - WIDTH*leap - leap] > cValue) cValue = data[id - WIDTH*leap - leap];
		if (cY - leap >=0 && cX + leap < WIDTH) if (data[id - WIDTH*leap + leap] > cValue) cValue = data[id - WIDTH*leap + leap];
		if (cY + leap < HEIGHT && cX - leap >= 0) if (data[id + WIDTH*leap - leap] > cValue) cValue = data[id + WIDTH*leap - leap];
		if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[id + WIDTH*leap + leap] > cValue) cValue = data[id + WIDTH*leap + leap];
#elif STRUCT_ELEMENT == DIAMOND
		//dilates the image
		if (cX - leap >= 0) if (data[id - leap] > cValue) cValue = data[id - leap];
		if (cX + leap < WIDTH) if (data[id + leap] > cValue) cValue = data[id + leap];
		if (cY - leap >= 0) if (data[id - WIDTH*leap] > cValue) cValue = data[id - WIDTH*leap];
		if (cY + leap < HEIGHT) if (data[id + WIDTH*leap] > cValue) cValue = data[id + WIDTH*leap];
#elif STRUCT_ELEMENT == CROSS
		if (cX - leap >= 0 && cY - leap >= 0) if (data[id - WIDTH*leap - leap] > cValue) cValue = data[id - WIDTH*leap - leap];
		if (cY - leap >= 0 && cX + leap < WIDTH) if (data[id - WIDTH*leap + leap] > cValue) cValue = data[id - WIDTH*leap + leap];
		if (cY + leap < HEIGHT && cX - leap >= 0) if (data[id + WIDTH*leap - leap] > cValue) cValue = data[id + WIDTH*leap - leap];
		if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[id + WIDTH*leap + leap] > cValue) cValue = data[id + WIDTH*leap + leap];
#elif STRUCT_ELEMENT == CROSS_S
		if (cX - leap >= 0) if (data[id - leap] > cValue) cValue = data[id - leap];
		if (cX + leap < WIDTH) if (data[id + leap] > cValue) cValue = data[id + leap];
		if (cY - leap >= 0) if (data[id - WIDTH*leap] > cValue) cValue = data[id - WIDTH*leap];
		if (cY + leap < HEIGHT) if (data[id + WIDTH*leap] > cValue) cValue = data[id + WIDTH*leap];
		//diag
		if (cX - leap >= 0 && cY - leap >= 0) if (data[id - WIDTH*leap - leap] > cValue) cValue = data[id - WIDTH*leap - leap];
		if (cY - leap >= 0 && cX + leap < WIDTH) if (data[id - WIDTH*leap + leap] > cValue) cValue = data[id - WIDTH*leap + leap];
		if (cY + leap < HEIGHT && cX - leap >= 0) if (data[id + WIDTH*leap - leap] > cValue) cValue = data[id + WIDTH*leap - leap];
		if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[id + WIDTH*leap + leap] > cValue) cValue = data[id + WIDTH*leap + leap];
		//more diags
		leap *= 3;
		if (cX - leap >= 0 && cY - leap >= 0) if (data[id - WIDTH*leap - leap] > cValue) cValue = data[id - WIDTH*leap - leap];
		if (cY - leap >= 0 && cX + leap < WIDTH) if (data[id - WIDTH*leap + leap] > cValue) cValue = data[id - WIDTH*leap + leap];
		if (cY + leap < HEIGHT && cX - leap >= 0) if (data[id + WIDTH*leap - leap] > cValue) cValue = data[id + WIDTH*leap - leap];
		if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[id + WIDTH*leap + leap] > cValue) cValue = data[id + WIDTH*leap + leap];
#elif STRUCT_ELEMENT == SQUARED_10
		//dilates the image
		if (cX - leap >= 0) if (data[id - leap] > cValue) cValue = data[id - leap];
		if (cX + leap < WIDTH) if (data[id + leap] > cValue) cValue = data[id + leap];
		if (cY - leap >= 0) if (data[id - WIDTH*leap] > cValue) cValue = data[id - WIDTH*leap];
		if (cY + leap < HEIGHT) if (data[id + WIDTH*leap] > cValue) cValue = data[id + WIDTH*leap];
		//digonals
		if (cX - leap*10 >= 0 && cY - leap*10 >= 0) if (data[id - WIDTH*leap*10 - leap*10] > cValue) cValue = data[id - WIDTH*leap*10 - leap*10];
		if (cY - leap*10 >= 0 && cX + leap*10 < WIDTH) if (data[id - WIDTH*leap*10 + leap*10] > cValue) cValue = data[id - WIDTH*leap*10 + leap*10];
		if (cY + leap*10 < HEIGHT && cX - leap*10 >= 0) if (data[id + WIDTH*leap*10 - leap*10] > cValue) cValue = data[id + WIDTH*leap*10 - leap*10];
		if (cY + leap*10 < HEIGHT && cX + leap*10 < WIDTH) if (data[id + WIDTH*leap*10 + leap*10] > cValue) cValue = data[id + WIDTH*leap*10 + leap*10];
#endif
		
		//cValue = dilatePixel(data, leap, cValue, id, cX, cY, WIDTH, HEIGHT);
		
		//synchronizes the threads in the group at this point so that it theoretically diminishes the randomness
		//__syncthreads(); //diminuir o erro com uma Imgm só

		//updates the value for the pixel in data
		data[id] = cValue;

		
		//if (idempotence){//speeding up, early break
			if (cValue != previousValue){//idempotencia, um diferente, não atingiu
				atomicAnd(idempotence, false);
			}


			//if (finished){//speeding up, early break
				//rapidly and innacurately checks if ended
				still = true;
				for (int k = 0; k < K; k++){
					if (kArray[k] == cValue) {
						still = false;
						break;
					}
				}
				//more properly checks if ended
				for (int k = 0; k < K && still; k++){
					//if (kArray[k] == C) {//ou true no booleano
					old = atomicCAS((int*)&kArray[k], C, cValue); //duas threads ao mesmo tempo?

					if (old == C || kArray[k] == cValue){
						still = false;
						k = K; //out of loop
						__threadfence(); //necessary
					}
					//}
				}
				//if there is still something to do then finished is false
				if (still) atomicAnd(finished, false);
				//__threadfence();

				//checks idempotence
			//}
		//}
	}


}


/**
Auxiliar private function for printing cuda errors.
*/
inline void
print(cudaError_t err, char* msg){
	if (err != cudaSuccess)
	{
		printf("Error on %s ", msg);
		fprintf(stderr, "Error code: %s!\n", cudaGetErrorString(err));
		exit(EXIT_FAILURE);
	}
}

//private global kArray
int *kArray;



/**
* Host main routine
*/
int*
kMorphologicalSets(const bool* __restrict img, const int WIDTH, const int HEIGHT, const int K, const bool disposeKArray)
{
	//error code to check return values for CUDA calls
	cudaError_t err = cudaSuccess;

	//setting the sizes
	const size_t DATA_SIZE = WIDTH*HEIGHT*sizeof(int),
		K_ARRAY_SIZE = K*sizeof(int),
		FINISHED_SIZE = sizeof(int),
		IMG_SIZE = WIDTH*HEIGHT*sizeof(bool);

	//kernel variables, in respect to the device at CUDA_DEVICE_INDEX constant
	cudaDeviceProp props;
	cudaGetDeviceProperties(&props, CUDA_DEVICE_INDEX); //device index = 0, you can change it if you have more CUDA devices
	const int threadsPerBlock = props.maxThreadsPerBlock / 2;
	const int blocksPerGrid = (HEIGHT*WIDTH + threadsPerBlock - 1) / threadsPerBlock;


	//allocate boolean matrix (mask)
	bool *d_mask = NULL;
	err = cudaMalloc((void **)&d_mask, IMG_SIZE);
	print(err, "Failed to allocate device vector d_mask!\n");

	//copy the data to this boolean matrix
	err = cudaMemcpy(d_mask, img, IMG_SIZE, cudaMemcpyHostToDevice);
	print(err, "Failed to copy vector d_img from host to device!\n");


	//allocate the grey image (data)
	int *d_data = NULL;
	err = cudaMalloc((void **)&d_data, DATA_SIZE);
	print(err, "Failed to allocate device vector d_data!\n");


	//call first Kernel, populate data, indexes every pixel uniquely
	indexImg << <blocksPerGrid, threadsPerBlock >> >
		(d_mask, d_data, WIDTH, HEIGHT);

	
	//erase the boolean img
	cudaFree(d_mask);

	//allocate the vector kArray
	int *d_kArray = NULL;
	err = cudaMalloc((void **)&d_kArray, K_ARRAY_SIZE);
	print(err, "Failed to allocate device vector d_kArray!");


	//allocate the kArrayAux
	int *d_kArrayAux = NULL;
	err = cudaMalloc((void **)&d_kArrayAux, K_ARRAY_SIZE);
	print(err, "Failed to allocate device vector d_kArrayAux!");
	

	//allocate the variable finished
	int *d_finished = NULL;
	err = cudaMalloc((void **)&d_finished, FINISHED_SIZE);
	print(err, "Failed to allocate device vector d_finished!");


	//allocate the device variable idempotence
	int *d_idempotence = NULL;
	err = cudaMalloc((void **)&d_idempotence, FINISHED_SIZE);
	print(err, "Failed to allocate device vector d_idempotence!");

	
	//Host variables
	kArray = (int*)malloc(K_ARRAY_SIZE);
	for (int k = 0; k < K; k++) kArray[k] = C; 
	int finished = false;
	int *data = (int*)malloc(DATA_SIZE);



	//copy k array to device
	err = cudaMemcpy(d_kArray, kArray, K_ARRAY_SIZE, cudaMemcpyHostToDevice);
	print(err, "Failed to copy vector d_kArray from host to device!");
	

	int iterations = 0;
	int idempotence = false;
	bool idempotenceAuxVar = false;
	int leap = 1;
	int lastLeap = 1;
	// Launch the Vector Add CUDA Kernel
	//printf("CUDA kernel launch with %d blocks of %d threads\n", blocksPerGrid, threadsPerBlock);
	while (!(idempotence && finished)){//while not idempotent and not finished (i.e., while there are more than k clusters and it is not idempotent)
		//reset the cuda variables at each kernel launch
		cudaMemset(d_finished, true, FINISHED_SIZE);
		cudaMemset(d_idempotence, true, FINISHED_SIZE);
		cudaMemset(d_kArray, C, K_ARRAY_SIZE);

		//dilate and verify if finished
		dilateAndVerify << <blocksPerGrid, threadsPerBlock>> >
			(d_data, d_kArray, d_finished, d_idempotence, leap, WIDTH, HEIGHT, K);
		print(cudaGetLastError(), "Failed to launch kernel diloteAndVerify!");
			

		//Copy finished back to host
		err = cudaMemcpy(&finished, d_finished, FINISHED_SIZE, cudaMemcpyDeviceToHost);
		print(err, "Failed to copy vector d_finished from device to host!");
	

		//Copy idempotence back to host
		err = cudaMemcpy(&idempotence, d_idempotence, FINISHED_SIZE, cudaMemcpyDeviceToHost);
		print(err, "Failed to copy vector d_idempotence from device to host!");


		if (idempotence) {
			idempotenceAuxVar = true;
			if (lastLeap <= leap) leap++;
			else leap = lastLeap;
		}
		if (idempotenceAuxVar && !idempotence){
			lastLeap = leap;
			leap = 1;
		}

		iterations++;
	}

	

	//copying back the clustered image, this part can be removed for faster processing
	int* outImg = (int*)malloc(DATA_SIZE);
	err = cudaMemcpy(outImg, d_data, DATA_SIZE, cudaMemcpyDeviceToHost);
	print(err, "Failed to copy vector outImg from device to host!");


	//copying the kArray back
	err = cudaMemcpy(kArray, d_kArray, K_ARRAY_SIZE, cudaMemcpyDeviceToHost);
	print(err, "Failed to copy vector kArray from device to host!");

	//counting found clusters
	int totalClusters = 0;
	for (int k = 0; k < K; k++){
		if (kArray[k] != C) totalClusters++;
	}
	printf("The clustering ended with %d iterations and %d valid clusters. \n", iterations, totalClusters);
	printf("Cluster values/identifiers: [");
	for (int k = 0; k < K - 1; k++){
		printf("%d, ", kArray[k]);
	}
	printf("%d]\n", kArray[K - 1]);
	
	print(err, "Error on copying kArray back to host memory!");


	//free device global memory
	err = cudaFree(d_data);
	print(err, "Failed to free device vector d_data!");
	err = cudaFree(d_kArray);
	print(err, "Failed to free device vector d_kArray!");
	err = cudaFree(d_finished);
	print(err, "Failed to free device d_idempotence!");
	err = cudaFree(d_idempotence);

	//free host memory
	if (disposeKArray) free(kArray);

	//reset device
	err = cudaDeviceReset();
	print(err, "Failed to deinitialize the device!");


	return outImg;
}

int*
kMorphologicalSets(const bool* __restrict img, const int WIDTH, const int HEIGHT, const int K, int* kArray)
{
	return kMorphologicalSets(img, WIDTH, HEIGHT, K, false);
}

int*
kMorphologicalSets(const bool* __restrict img, const int WIDTH, const int HEIGHT, const int K)
{
	return kMorphologicalSets(img, WIDTH, HEIGHT, K, true);
}


/**
Dilates an input boolean image and returns a boolean dilated version.

@return the pointer to the dilated image.
*/
bool*
dilateImg(const bool* __restrict inputImg, const int timesToDilate, const int WIDTH, const int HEIGHT){
	size_t SIZE = WIDTH*HEIGHT*sizeof(bool);
	cudaError_t err;

	bool *outputImg = (bool*) malloc(SIZE);
	bool *d_inputImg, *d_outputImg;

	err = cudaMalloc(&d_outputImg, SIZE);
	print(err, "Error on allocating d_outputImg in device memory.");

	err = cudaMalloc(&d_inputImg, SIZE);
	print(err, "Error on allocating d_inputImg in device memory.");

	err = cudaMemcpy(d_inputImg, inputImg, SIZE, cudaMemcpyHostToDevice);
	print(err, "Error on copying inputImg to device memory.");

	err = cudaMemcpy(d_outputImg, d_inputImg, SIZE, cudaMemcpyDeviceToDevice);
	print(err, "Error on copying d_outputImg from d_intputImg in device memory.");


	cudaDeviceProp props;
	cudaGetDeviceProperties(&props, CUDA_DEVICE_INDEX); //device index = 0, you can change it if you have more CUDA devices
	const int threadsPerBlock = props.maxThreadsPerBlock / 2;
	const int blocksPerGrid = (HEIGHT*WIDTH + threadsPerBlock - 1) / threadsPerBlock;

	//dilate
	int counter = 0;
	while (counter < timesToDilate){
		dilate << <blocksPerGrid, threadsPerBlock >> >
			(d_inputImg, WIDTH, HEIGHT, d_outputImg);

		err = cudaMemcpy(d_inputImg, d_outputImg, SIZE, cudaMemcpyDeviceToDevice);
		print(err, "Error on copying d_outputImg to d_inputImg.");

		counter++;
	}

	err = cudaMemcpy(outputImg, d_outputImg, SIZE, cudaMemcpyDeviceToHost);
	print(err, "Error on copying d_outputImg to host.");


	cudaFree(d_inputImg);
	cudaFree(d_outputImg);

	return outputImg;
}




