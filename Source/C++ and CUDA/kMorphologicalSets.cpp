#include <stdlib.h>
#include <Common.h>
#include<stdio.h>

#define C -1

#define SQUARED 1
#define DIAMOND 2
#define CROSS 3
#define CROSS_S 4
#define SQUARED_10 5

//define the chosen structuring element
#define STRUCT_ELEMENT SQUARED //it can be SQUARED, SQUARED_10, DIAMOND, CROSS or CROSS_S. Different structuring elements can be implemented as well.

//this structuring element refers to how the image is dilated before the clusterization is performed
#define STRUCT_ELEMENT_PREV SQUARED //it can be SQUARED, DIAMOND, CROSS or CROSS_S. Different structuring elements can be implemented as well.



bool*
dilateImgCPU(const bool* __restrict data, const int timesToDilate, const int WIDTH, const int HEIGHT){
	size_t BOOL_SIZE = sizeof(bool)* WIDTH*HEIGHT;

	bool *imgP = (bool*)malloc(BOOL_SIZE), *imgP2 = (bool*)malloc(BOOL_SIZE);

	//copying to another image
	for (int k = 0; k < WIDTH*HEIGHT; k++){
		imgP[k] = data[k];
		imgP2[k] = data[k];
	}

	int counter = 0; const int leap = 1;
	while (counter < timesToDilate){
		for (int k = 0; k < WIDTH*HEIGHT; k++){
			const int cX = k % WIDTH, cY = k / WIDTH;

#if STRUCT_ELEMENT_PREV == SQUARED
			if (cX - leap >= 0) if (imgP2[k - leap] > 0) imgP[k] = true;
			if (cX + leap < WIDTH) if (imgP2[k + leap] > 0) imgP[k] = true;
			if (cY - leap >= 0) if (imgP2[k - WIDTH*leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT) if (imgP2[k + WIDTH*leap] > 0) imgP[k] = true;
			//digonals
			if (cX - leap >= 0 && cY - leap >= 0) if (imgP2[k - WIDTH*leap - leap] > 0) imgP[k] = true;
			if (cY - leap >= 0 && cX + leap < WIDTH) if (imgP2[k - WIDTH*leap + leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT && cX - leap >= 0) if (imgP2[k + WIDTH*leap - leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT && cX + leap < WIDTH) if (imgP2[k + WIDTH*leap + leap] > 0) imgP[k] = true;
#elif STRUCT_ELEMENT_PREV == DIAMOND
			if (cX - leap >= 0) if (imgP2[k - leap] > 0) imgP[k] = true;
			if (cX + leap < WIDTH) if (imgP2[k + leap] > 0) imgP[k] = true;
			if (cY - leap >= 0) if (imgP2[k - WIDTH*leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT) if (imgP2[k + WIDTH*leap] > 0) imgP[k] = true;
#elif STRUCT_ELEMENT_PREV == CROSS
			if (cX - leap >= 0 && cY - leap >= 0) if (imgP2[k - WIDTH*leap - leap] > 0) imgP[k] = true;
			if (cY - leap >= 0 && cX + leap < WIDTH) if (imgP2[k - WIDTH*leap + leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT && cX - leap >= 0) if (imgP2[k + WIDTH*leap - leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT && cX + leap < WIDTH) if (imgP2[k + WIDTH*leap + leap] > 0) imgP[k] = true;
#elif STRUCT_ELEMENT_PREV == CROSS_S
			if (cX - leap >= 0) if (imgP2[k - leap] > 0) imgP[k] = true;
			if (cX + leap < WIDTH) if (imgP2[k + leap] > 0) imgP[k] = true;
			if (cY - leap >= 0) if (imgP2[k - WIDTH*leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT) if (imgP2[k + WIDTH*leap] > 0) imgP[k] = true;
			//digonals
			if (cX - leap >= 0 && cY - leap >= 0) if (imgP2[k - WIDTH*leap - leap] > 0) imgP[k] = true;
			if (cY - leap >= 0 && cX + leap < WIDTH) if (imgP2[k - WIDTH*leap + leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT && cX - leap >= 0) if (imgP2[k + WIDTH*leap - leap] > 0) imgP[k] = true;
			if (cY + leap < HEIGHT && cX + leap < WIDTH) if (imgP2[k + WIDTH*leap + leap] > 0) imgP[k] = true;
			//
			if (cX - leap * 3 >= 0 && cY - leap * 3 >= 0) if (imgP2[k - WIDTH*leap * 3 - leap * 3] > 0) imgP[k] = true;
			if (cY - leap * 3 >= 0 && cX + leap * 3 < WIDTH) if (imgP2[k - WIDTH*leap * 3 + leap * 3] > 0) imgP[k] = true;
			if (cY + leap * 3 < HEIGHT && cX - leap * 3 >= 0) if (imgP2[k + WIDTH*leap * 3 - leap * 3] > 0) imgP[k] = true;
			if (cY + leap * 3 < HEIGHT && cX + leap * 3 < WIDTH) if (imgP2[k + WIDTH*leap * 3 + leap * 3] > 0) imgP[k] = true;
#endif
		}

		//copy
		for (int k = 0; k < WIDTH*HEIGHT; k++){
			imgP2[k] = imgP[k];
		}

		counter++;
	}

	free(imgP2);

	return imgP;
}

int*
kMorphologicalSetsCPU(const bool* __restrict img, const int WIDTH, const int HEIGHT, const int K)
{
	const size_t DATA_SIZE = sizeof(int) * WIDTH*HEIGHT;
	int* __restrict data = (int*)malloc(DATA_SIZE);
	//int* __restrict dataP = (int*)malloc(DATA_SIZE);

	//indexing grey image
	for (int k = 0; k < WIDTH*HEIGHT; k++){
		if (img[k]) {
			data[k] = k;
			//dataP[k] = k;
		}
		else{
			data[k] = C;
			//dataP[k] = C;
		}
	}


	//ArrayList<int> clusterId = ArrayList<int>();
	int *kArray = (int*)malloc(sizeof(int)*K);
	bool idempotence = false, finished = false;
	int leap = 1;
	int counter = 0, clustersQnt = 0;
	bool contains, added;
	while (!finished){
		counter++;

		idempotence = true; finished = true;
		clustersQnt = 0;

		//reset clusters kArray
		for (int k = 0; k < K; k++) kArray[k] = C;
		//process pixels
		int cValue, pValue;
		for (int k = 0; k < WIDTH*HEIGHT; k++){

			if (!img[k]) continue;

			const int cX = k % WIDTH, cY = k / WIDTH;
			
			cValue = data[k];  pValue = cValue;

#if STRUCT_ELEMENT == SQUARED
			if (cX - leap >= 0) if (data[k - leap] > cValue) cValue = data[k - leap];
			if (cX + leap < WIDTH) if (data[k + leap] > cValue) cValue = data[k + leap];
			if (cY - leap >= 0) if (data[k - WIDTH*leap] > cValue) cValue = data[k - WIDTH*leap];
			if (cY + leap < HEIGHT) if (data[k + WIDTH*leap] > cValue) cValue = data[k + WIDTH*leap];
			//digonals
			if (cX - leap >= 0 && cY - leap >= 0) if (data[k - WIDTH*leap - leap] > cValue) cValue = data[k - WIDTH*leap - leap];
			if (cY - leap >= 0 && cX + leap < WIDTH) if (data[k - WIDTH*leap + leap] > cValue) cValue = data[k - WIDTH*leap + leap];
			if (cY + leap < HEIGHT && cX - leap >= 0) if (data[k + WIDTH*leap - leap] > cValue) cValue = data[k + WIDTH*leap - leap];
			if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[k + WIDTH*leap + leap] > cValue) cValue = data[k + WIDTH*leap + leap];
#elif STRUCT_ELEMENT == DIAMOND
			//dilates the image
			if (cX - leap >= 0) if (data[k - leap] > cValue) cValue = data[k - leap];
			if (cX + leap < WIDTH) if (data[k + leap] > cValue) cValue = data[k + leap];
			if (cY - leap >= 0) if (data[k - WIDTH*leap] > cValue) cValue = data[k - WIDTH*leap];
			if (cY + leap < HEIGHT) if (data[k + WIDTH*leap] > cValue) cValue = data[k + WIDTH*leap];
#elif STRUCT_ELEMENT == CROSS
			if (cX - leap >= 0 && cY - leap >= 0) if (data[k - WIDTH*leap - leap] > cValue) cValue = data[k - WIDTH*leap - leap];
			if (cY - leap >= 0 && cX + leap < WIDTH) if (data[k - WIDTH*leap + leap] > cValue) cValue = data[k - WIDTH*leap + leap];
			if (cY + leap < HEIGHT && cX - leap >= 0) if (data[k + WIDTH*leap - leap] > cValue) cValue = data[k + WIDTH*leap - leap];
			if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[k + WIDTH*leap + leap] > cValue) cValue = data[k + WIDTH*leap + leap];
#elif STRUCT_ELEMENT == CROSS_S
			if (cX - leap >= 0) if (data[k - leap] > cValue) cValue = data[k - leap];
			if (cX + leap < WIDTH) if (data[k + leap] > cValue) cValue = data[k + leap];
			if (cY - leap >= 0) if (data[k - WIDTH*leap] > cValue) cValue = data[k - WIDTH*leap];
			if (cY + leap < HEIGHT) if (data[k + WIDTH*leap] > cValue) cValue = data[k + WIDTH*leap];
			//diag
			if (cX - leap >= 0 && cY - leap >= 0) if (data[k - WIDTH*leap - leap] > cValue) cValue = data[k - WIDTH*leap - leap];
			if (cY - leap >= 0 && cX + leap < WIDTH) if (data[k - WIDTH*leap + leap] > cValue) cValue = data[k - WIDTH*leap + leap];
			if (cY + leap < HEIGHT && cX - leap >= 0) if (data[k + WIDTH*leap - leap] > cValue) cValue = data[k + WIDTH*leap - leap];
			if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[k + WIDTH*leap + leap] > cValue) cValue = data[k + WIDTH*leap + leap];
			//more diags
			leap *= 3;
			if (cX - leap >= 0 && cY - leap >= 0) if (data[k - WIDTH*leap - leap] > cValue) cValue = data[k - WIDTH*leap - leap];
			if (cY - leap >= 0 && cX + leap < WIDTH) if (data[k - WIDTH*leap + leap] > cValue) cValue = data[k - WIDTH*leap + leap];
			if (cY + leap < HEIGHT && cX - leap >= 0) if (data[k + WIDTH*leap - leap] > cValue) cValue = data[k + WIDTH*leap - leap];
			if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[k + WIDTH*leap + leap] > cValue) cValue = data[k + WIDTH*leap + leap];
#elif STRUCT_ELEMENT == SQUARED_10
			//dilates the image
			if (cX - leap >= 0) if (data[k - leap] > cValue) cValue = data[k - leap];
			if (cX + leap < WIDTH) if (data[k + leap] > cValue) cValue = data[k + leap];
			if (cY - leap >= 0) if (data[k - WIDTH*leap] > cValue) cValue = data[k - WIDTH*leap];
			if (cY + leap < HEIGHT) if (data[k + WIDTH*leap] > cValue) cValue = data[k + WIDTH*leap];
			//digonals
			if (cX - leap * 10 >= 0 && cY - leap * 10 >= 0) if (data[k - WIDTH*leap * 10 - leap * 10] > cValue) cValue = data[k - WIDTH*leap * 10 - leap * 10];
			if (cY - leap * 10 >= 0 && cX + leap * 10 < WIDTH) if (data[k - WIDTH*leap * 10 + leap * 10] > cValue) cValue = data[k - WIDTH*leap * 10 + leap * 10];
			if (cY + leap * 10 < HEIGHT && cX - leap * 10 >= 0) if (data[k + WIDTH*leap * 10 - leap * 10] > cValue) cValue = data[k + WIDTH*leap * 10 - leap * 10];
			if (cY + leap * 10 < HEIGHT && cX + leap * 10 < WIDTH) if (data[k + WIDTH*leap * 10 + leap * 10] > cValue) cValue = data[k + WIDTH*leap * 10 + leap * 10];
#endif

			data[k] = cValue;

			//speeding up
			if (!idempotence ) continue;

			//check idempotence
			idempotence &= cValue == pValue; //per pixel idempotence
			
			if (!finished) continue;

			//check the number of clusters
			//currently O(K) in the worst case, it can be optimized to O(log K), but it will only be really efficient for large values of K
			contains = false; added = false;
			for (int k = 0; k < K; k++){
				contains = kArray[k] == cValue;
				if (contains) break;

				if (kArray[k] == C) {
					kArray[k] = cValue;
					clustersQnt++;
					added = true;
					break;
				}
			}

			if (!contains && !added) clustersQnt++; //if clustersQnt > K, then the number of clusters is not the exact amount


			finished = clustersQnt <= K;
		}

		//copy back the image
		/*if (!finished){
			for (int k = 0; k < WIDTH*HEIGHT; k++){
				data[k] = dataP[k];
			}
		}*/

		if (!idempotence) leap = 1;
		else leap++;

		finished &= idempotence;
	}

	printf("The clustering ended with %d iterations and %d valid clusters. \n", counter, clustersQnt);
	printf("Cluster values/identifiers: [");
	for (int k = 0; k < K - 1; k++){
		printf("%d, ", kArray[k]);
	}
	printf("%d]\n", kArray[K - 1]);


	return data;
}
