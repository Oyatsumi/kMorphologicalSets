#include <stdio.h>
#include <stdlib.h>
//#include <LinkedList.h>
#include<ArrayList.h>
#include <Common.h>


#define INI -1

inline bool closer(int x1, int y1, int x2, int y2, int centerX, int centerY){//true if the first is closer
	return ((abs(centerX - x1) + abs(centerY - y1)) < (abs(centerX - x2) + abs(centerY - y2)));
}

int naiveKMeans(bool * img, int width, int height, int K){
	int *kX = new int[K];
	int *kY = new int[K];

	int *xCloser = new int[K];
	int *yCloser = new int[K];

	ArrayList<int>* clusterPoints = new ArrayList<int>[K];

	char *clusterIndex = new char[width*height];
	for (int k = 0; k < width*height; k++) clusterIndex[k] = INI;

	bool difCenters = true;
	//initial random cluster centers
	for (int k = 0; k < K; k++)	{
		do {
			kX[k] = rand() % width;
			kY[k] = rand() % height;
			difCenters = true;
			for (int k2 = 0; k2 < k; k2++) difCenters &= (abs(kX[k] - kX[k2]) > width/100 || abs(kY[k] - kY[k2]) > height/100) && (kX[k] != kX[k2] || kY[k] != kY[k2]);
		} while (!difCenters);
		clusterIndex[kY[k] * width + kX[k]] = k; //indexing the centers of the clusters
		xCloser[k] = INT_MAX/2 - width;
		yCloser[k] = INT_MAX/2 - height;
	}

	int xAux, yAux, x, y, aux;
	bool finished = false;
	bool foundCloser = false;
	int iterations = 0;
	while (!finished){
		finished = true;
		for (int k = 0; k < K; k++){//for each cluster
			foundCloser = false;
			xCloser[k] = INT_MAX/2 - width; yCloser[k] = INT_MAX/2 - height;
			//scans the 2d dimension (or image) for a closer element to the center of the cluster (kX and kY)
			for (int i = 0; i < height; i++){
				for (int j = 0; j < width; j++){
					if (closer(j, i, xCloser[k], yCloser[k], kX[k], kY[k]) && clusterIndex[i*width + j] == INI){
						//close element that has not been already clusterized
						xCloser[k] = j;
						yCloser[k] = i;
						foundCloser = true; 
					}
				}
			}
			if (foundCloser){
				clusterIndex[yCloser[k] * width + xCloser[k]] = k;  //indexing the element to the index of the cluster

				//update clusters and center (centroid) information
				clusterPoints[k].add(yCloser[k] * width + xCloser[k]); 

				xAux = 0; yAux = 0;
				//computing the center of the clusters
				for (int e = 0; e < clusterPoints[k].size(); e++){
					aux = clusterPoints[k].get(e);
					x = aux % width;
					y = (int)(aux / width);
					xAux += x;
					yAux += y;
				}
				if (kX[k] != xAux || kY[k] != yAux) {
					kX[k] = xAux / clusterPoints[k].size();
					kY[k] = yAux / clusterPoints[k].size();
				}
			}
			
			finished &= !foundCloser;
		}
		iterations++;
	}

	printf("Finished in %d iterations\n", iterations);

	delete(clusterIndex);
	return 0;
}

