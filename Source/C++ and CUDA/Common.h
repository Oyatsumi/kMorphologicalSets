/*
A header for importing the .cu properly
*/

#pragma once

int* kMorphologicalSets(const bool *img, const int WIDTH, const int HEIGHT, const int K);

int naiveKMeans(bool * img, int width, int height, int K);

bool*
dilateImg(const bool *inputImg, const int timesToDilate, const int WIDTH, const int HEIGHT);

int*
kMorphologicalSets(const bool *img, const int WIDTH, const int HEIGHT, const int K, int* kArray);

int*
kMorphologicalSets(const bool *img, const int WIDTH, const int HEIGHT, const int K);

bool*
dilateImgCPU(const bool *data, const int timesToDilate, const int WIDTH, const int HEIGHT);

int*
kMorphologicalSetsCPU(const bool *img, const int WIDTH, const int HEIGHT, const int K);
