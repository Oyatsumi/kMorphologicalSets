#include <iostream>
#include <Image.h>
#include <Common.h>
#include <chrono>


#include <ArrayList.h>

using namespace cv;
using namespace std;

struct rgbv{
	int r, g, b, v;
};
struct vc{
	int v, c;
};


int main(int argc, char** argv)
{
	if (argc < 4){//help

		printf("!!!!Usage!!!! \n\n\
kMS-CPU.exe -numberOfTimesToDilate -K -thresholdForNoiseRemoval -pathToTheImage \n \
\n   \
	- numberOfTimesToDilate: number of times to dilate the image prior to the clusterization, \n\
		it may help the algorithm to find better clusters or to converge faster \n\
	- K : the desired amount of clusters(the final result can be <= k) \n\
	- thresholdForNoiseRemoval : a minimum quantity of pixels that a set must have, \n\
		otherwise it is erased from the image \n\
	- pathToTheImage : the path where the image is located \n\
\n\
For instance: \n\
\n\
	kMS-CPU.exe 1 450 200 \"./Chameleon/t7.10k.dat.png\" \n\
	kMS-GPU.exe 1 450 200 \"./Chameleon/t7.10k.dat.png\" \n");

		exit(0);
	}
	
	//printf("Device %d \n", *argv[1] - '0');
	//cudaSetDevice(*argv[1] - '0');

	/*Algorithm Parameters*/
	const int dilTimes = atoi(argv[1]), K = atoi(argv[2]), threshold = atoi(argv[3]);

	//String dir = "C:\\Users\\erick\\Documents\\Viterbo\\Trabalho Final\\Chameleon\\New Datasets\\8192\\";
	//String dir = ".\\Datasets Per Amount of Instances\\";

	Image img = Image(argv[4]);

	const int WIDTH = img.getWidth(), HEIGHT = img.getHeight();

	int* __restrict kArray = NULL;
	int* outImg = NULL;

	bool* __restrict imgB = img.getBooleanLinearImage(0);

	auto begin = std::chrono::high_resolution_clock::now();
		
	bool* __restrict imgAux = NULL;
	if (dilTimes > 0)
		//imgAux = dilateImg(imgB, dilTimes, img.getWidth(), img.getHeight());
		imgAux = dilateImgCPU(imgB, dilTimes, img.getWidth(), img.getHeight());
	else
		imgAux = imgB;

	Image *out2 = new Image(imgAux, img.getWidth(), img.getHeight());
	(*out2).writeImage("./dilatedVersionIfAny.bmp");

	//outImg = kMorphologicalSets(imgAux, img.getWidth(), img.getHeight(), K);
	outImg = kMorphologicalSetsCPU(imgAux, img.getWidth(), img.getHeight(), K);
		//naiveKMeans(imgB, img.getWidth(), img.getHeight(), 5);
		//cvMeans(256, 256, 15);
		//img.setPixel(1, 0, 0, 255);
		//img.writeImage("./bla.bmp");

		//printf("bla %d", img1.getPixel(1, 0, 0));
		//img1.printBooleanLinearImage(0);
	
	auto end = std::chrono::high_resolution_clock::now();

	printf("Total elapsed time (k = %d): ", K);
	//std::cout << "Total elapsed time (k = : ";
	std::cout << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count()/(double)1000000000) << "s" << std::endl;

	//exit(0);

	printf("Coloring the clusters with a unique color. The algorithm may get stuck for too long here for large images and values of K, since the process of choosing the colors is random. Please remove or change this part within the algorithm if it is the case.\n");
	
	ArrayList<rgbv> colorList = ArrayList<rgbv>();
	Image *out = new Image(WIDTH, HEIGHT, 3);
	srand(time(NULL));
	for (int k = 0; k < WIDTH*HEIGHT; k++){
		const int x = k % WIDTH, y = k / WIDTH;

		if (outImg[k] < 0) {
			(*out).setPixel(x, y, 0, 255);
			(*out).setPixel(x, y, 1, 255);
			(*out).setPixel(x, y, 2, 255);
			rgbv aux;
			aux.r = 255; aux.g = 255; aux.b = 255; aux.v = -1;
			colorList.add(aux);
			continue;
		}

		bool found = false;
		for (int l = 0; l < colorList.size(); l++){
			if (colorList.get(l).v == outImg[k]){
				(*out).setPixel(x, y, 0, colorList.get(l).r);
				(*out).setPixel(x, y, 1, colorList.get(l).g);
				(*out).setPixel(x, y, 2, colorList.get(l).b);

				found = true;
				break;
			}
		}
		if (!found){
			rgbv aux;
			bool uniqueColor = true;
			do{
				aux.r = rand() % 256;
				aux.g = rand() % 256;
				aux.b = rand() % 256;
				for (int l = 0; l < colorList.size(); l++){
					uniqueColor &= !(colorList.get(l).r == aux.r && colorList.get(l).g == aux.g && colorList.get(l).b == aux.b);
				}
			} while (!uniqueColor);
			aux.v = outImg[k];

			colorList.add(aux);
			(*out).setPixel(x, y, 0, aux.r);
			(*out).setPixel(x, y, 1, aux.g);
			(*out).setPixel(x, y, 2, aux.b);
		}

	}

	//Image *out = new Image(outImg, img.getWidth(), img.getHeight());
	(*out).writeImage("./colouredVersionPostClusterization.bmp");
	//if (outImg != NULL) delete(outImg);

	ArrayList<vc> eraseList = ArrayList<vc>();
	vc aux;
	for (int k = 0; k < WIDTH*HEIGHT; k++){
		
		bool found = false;
		for (int l = 0; l < eraseList.size(); l++){
			if (outImg[k] == eraseList.get(l).v){
				aux = eraseList.remove(l);
				aux.c = aux.c + 1;
				eraseList.add(aux);
				found = true;
			}
		}
		if (!found){
			vc aux2;
			aux2.c = 1; aux2.v = outImg[k];
			eraseList.add(aux2);
		}

	}

	Image *outE = new Image(out); //clonning

	//output original
	int instances = 0;
	for (int y = 0; y < HEIGHT; y++){
		for (int x = 0; x < WIDTH; x++){
			if (!imgB[y*WIDTH + x]){
				(*out).setPixel(x, y, 0, 255);
				(*out).setPixel(x, y, 1, 255);
				(*out).setPixel(x, y, 2, 255);
			}
			else instances++;
		}
	}
	printf("The dataset contains a total of %d instances. \n", instances);

	(*out).writeImage("./colouredVersionPostClusterizationNonDilated.bmp");

	for (int k = 0; k < WIDTH*HEIGHT; k++){
		const int x = k % WIDTH, y = k / WIDTH;

		for (int l = 0; l < eraseList.size(); l++){
			if (outImg[k] == eraseList.get(l).v){
				if (eraseList.get(l).c < threshold){
					(*outE).setPixel(x, y, 0, 255);
					(*outE).setPixel(x, y, 1, 255);
					(*outE).setPixel(x, y, 2, 255);
				}
			}
		}

	}

	(*outE).writeImage("./colouredVersionPostClusterizationWithNoiseRemoval.bmp");


	
	delete(out2);
	delete(out);
	delete(outE);

	if (kArray != NULL) free(kArray);
	if (outImg != NULL) free(outImg);
	if (imgAux != NULL && imgAux != imgB) free(imgAux);
	if (imgB != NULL) free(imgB);
	

	return 0;
}










