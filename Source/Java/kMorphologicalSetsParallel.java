import java.awt.image.BufferedImage;
import java.awt.image.Raster;
import java.awt.image.WritableRaster;
import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import javax.imageio.ImageIO;

public class kMorphologicalSetsParallel {
	private static final int C = -1;

	private static AtomicBoolean finished = new AtomicBoolean(), idempotence = new AtomicBoolean();
	private static AtomicInteger[] kArray;
	
	public static void main(String[] args) throws IOException, URISyntaxException{
		
		BufferedImage bi = ImageIO.read(new File(args[4]));
		Raster r = bi.getRaster();
		
		final int numThreads = Integer.parseInt(args[0]),
				numDilations = Integer.parseInt(args[1]),
				numClusters = Integer.parseInt(args[2]),
				thresNoise = Integer.parseInt(args[3]),
				WIDTH = r.getWidth(), HEIGHT = r.getHeight(),
				IMG_SIZE = r.getWidth()*r.getHeight(),
				CHUNK = (IMG_SIZE + numThreads) / numThreads;
		
		
		final boolean[] imgP = new boolean[IMG_SIZE], imgP2 = new boolean[IMG_SIZE];
		
		int[] data = new int[IMG_SIZE];
		
		for (int i=0; i<HEIGHT; i++){
			for (int j=0; j<WIDTH; j++){
				imgP[i*WIDTH + j] = r.getSample(j, i, 0) > 0;
				imgP2[i*WIDTH + j] = imgP[i*WIDTH + j];
			}
		}
		
		final double start = System.nanoTime();
		
		DilateThread[] dThreads = new DilateThread[numThreads];
		
		//starting the dilation threads
		int dilCounter = 0;
		if (numDilations > 0){
			while (dilCounter < numDilations){
				for (int k=0; k<dThreads.length; k++){
					dThreads[k] = new kMorphologicalSetsParallel().new DilateThread(imgP, imgP2, k*CHUNK, CHUNK, WIDTH, HEIGHT);
					
					dThreads[k].start();
				}
			
				dilCounter++;
			
				//waiting for every thread to finish			
				boolean allFinished = false;
				while (!allFinished){
					allFinished = true;
					for (int k=0; k<dThreads.length; k++){
						allFinished &= !dThreads[k].isAlive();
					}
					if (!allFinished) continue;
				}
				
				//copying back
				for (int i=0; i<HEIGHT; i++){
					for (int j=0; j<WIDTH; j++){
						imgP2[i*WIDTH + j] = imgP[i*WIDTH + j];
					}
				}
				
			}
			
		}
		
		for (int i=0; i<HEIGHT; i++){
			for (int j=0; j<WIDTH; j++){
				if (imgP[i*WIDTH + j]) data[i*WIDTH + j] = i*WIDTH + j;
				else data[i*WIDTH + j] = C;
			}
		}
		
		//kms algorithm
		kMSThread[] kmsThreads = new kMSThread[numThreads];
		int leap = 1;
		kArray = new AtomicInteger[numClusters];
		for (int k=0; k<numClusters; k++){
			kArray[k] = new AtomicInteger();
			kArray[k].set(C);
		}
		int counter = 0;
		do{
			counter++;
			finished.set(true); idempotence.set(true);
			
			//reset karray
			for (int k=0; k<kArray.length && counter > 1; k++){
				kArray[k].set(C);
			}
			
			
			for (int k=0; k<kmsThreads.length; k++){
				kmsThreads[k] = new kMorphologicalSetsParallel().new kMSThread(data, leap, k*CHUNK, CHUNK, WIDTH, HEIGHT, numClusters);
				kmsThreads[k].start();
			}
			
			//waiting for every thread to finish
			boolean allFinished = false;
			while (!allFinished){
				allFinished = true;
				for (int k=0; k<kmsThreads.length; k++){
					allFinished &= !kmsThreads[k].isAlive();
				}
				if (!allFinished) continue;
			}
			
			//changing the leap variable according to the idempotence
			if (idempotence.get())
				leap++;
			else
				leap = 1;
			
		
			finished.set(idempotence.get() && finished.get());
			
		}while(!finished.get());
		
		
		System.out.printf("The threaded k-MS algorithm finished in %f seconds.\n", (System.nanoTime() - start)/1000000000f);
		


		
		
		
		//save images
		BufferedImage dilImg = new BufferedImage(WIDTH, HEIGHT, BufferedImage.TYPE_BYTE_GRAY);
		WritableRaster dilImgR = dilImg.getRaster();
		for (int i=0; i<HEIGHT; i++){
			for (int j=0; j<WIDTH; j++){
				dilImgR.setSample(j, i, 0, (imgP[i*WIDTH + j] ? 255 : 0));
			}
		}
		ImageIO.write(dilImg, "png", new File("./dilatedVersionIfAny.png"));
		
		
		
		BufferedImage colouredImg = new BufferedImage(WIDTH, HEIGHT, BufferedImage.TYPE_INT_RGB);
		WritableRaster colouredImgR = colouredImg.getRaster();
		for (int i=0; i<HEIGHT; i++)
			for (int j=0; j<WIDTH; j++) 
				for (int b=0; b<3; b++) 
					colouredImgR.setSample(j, i, b, 255); //paint background white
		ArrayList<Vector3> colours = new ArrayList<Vector3>();
		Random ran = new Random();
		for (int i=0; i<HEIGHT; i++){
			for (int j=0; j<WIDTH; j++){
				final int cValue = data[i*WIDTH + j];
				
				if (cValue == C)
					continue;
				
				int red = 0, green = 0, blue = 0;
				boolean contains = false;
				
				for (int k=0; k<colours.size(); k++){
					if (colours.get(k).value == cValue){
						red = colours.get(k).r; green = colours.get(k).g;
						blue = colours.get(k).b;
						colours.get(k).counter++;
						contains = true;
						break;
					}
				}
				
				if (!contains){
					//gen a unique color
					boolean containsColor = false;
					do{
						containsColor = false;
						red = ran.nextInt(256); green = ran.nextInt(256); blue = ran.nextInt(256);
						for (int k=0; k<colours.size(); k++){
							if (colours.get(k).r == red && colours.get(k).g == green && colours.get(k).b == blue){
								containsColor = true;
								break;
							}
						}
					}while(containsColor);
					
					

				
					Vector3 nVec = new kMorphologicalSetsParallel().new Vector3();
					nVec.r = red; nVec.g = green; nVec.b = blue; nVec.value = cValue;
					nVec.counter = 1;
					colours.add(nVec);
				}
				
				//paint
				colouredImgR.setSample(j, i, 0, red);
				colouredImgR.setSample(j, i, 1, green);
				colouredImgR.setSample(j, i, 2, blue);
			}
		}
		ImageIO.write(colouredImg, "png", new File("./colouredVersionPostClusterization.png"));
		
		
		//noise removal img
		BufferedImage colouredImgNoise = new BufferedImage(WIDTH, HEIGHT, BufferedImage.TYPE_INT_RGB);
		WritableRaster colouredImgNoiseR = colouredImgNoise.getRaster();
		int red, green, blue;
		for (int i=0; i<HEIGHT; i++){
			for (int j=0; j<HEIGHT; j++){
				red = colouredImgR.getSample(j, i, 0); green = colouredImgR.getSample(j, i, 1); blue = colouredImgR.getSample(j, i, 2);
			
				for (int k=0; k<colours.size(); k++){
					if (data[i*WIDTH + j] == colours.get(k).value){
						if (colours.get(k).counter < thresNoise){
							red = 255; green = 255; blue = 255;
						}
						break;
					}
				}
				
				colouredImgNoiseR.setSample(j, i, 0, red);
				colouredImgNoiseR.setSample(j, i, 1, green);
				colouredImgNoiseR.setSample(j, i, 2, blue);
			}
		}
		ImageIO.write(colouredImgNoise, "png", new File("./colouredVersionPostClusterizationWithNoiseRemoval.png"));
		
		
		//nondilated img
		BufferedImage colouredImgNonDil = new BufferedImage(WIDTH, HEIGHT, BufferedImage.TYPE_INT_RGB);
		WritableRaster colouredImgNonDilR = colouredImgNonDil.getRaster();
		for (int i=0; i<HEIGHT; i++){
			for (int j=0; j<WIDTH; j++){
				red = colouredImgR.getSample(j, i, 0); green = colouredImgR.getSample(j, i, 1); blue = colouredImgR.getSample(j, i, 2);
				
				if (r.getSample(j, i, 0) == 0){
					red = 255; green = 255; blue = 255;
				}
				
				colouredImgNonDilR.setSample(j, i, 0, red);
				colouredImgNonDilR.setSample(j, i, 1, green);
				colouredImgNonDilR.setSample(j, i, 2, blue);
			}
		}
		ImageIO.write(colouredImgNonDil, "png", new File("./colouredVersionPostClusterizationNonDilated.png"));
		
		
		
		//output information
		System.out.printf("The clustering ended with %d iterations and %d valid clusters. \n", counter, colours.size());
		System.out.printf("Cluster values/identifiers: [");
		for (int k = 0; k < numClusters - 1; k++){
			System.out.printf("%d, ", kArray[k].get());
		}
		System.out.printf("%d]\n", kArray[numClusters - 1].get());
		
		
	}
	
	public class Vector3{
		
		public int r, g, b, value, counter;
		
	}
	
	
	public class DilateThread extends Thread{
		
		//
		private final boolean[] imgP, imgP2;
		private final int INDEX, SIZE, WIDTH, HEIGHT;
		
		public DilateThread(final boolean[] imgP, final boolean[] imgP2, final int INDEX, final int SIZE, final int WIDTH, final int HEIGHT){
			this.imgP = imgP;
			this.imgP2 = imgP2;
			this.INDEX = INDEX;
			this.SIZE = SIZE;
			this.WIDTH = WIDTH;
			this.HEIGHT = HEIGHT;
		}
			
		public void run(){
			dilateImgCPU();
		}
		
		private boolean[] dilateImgCPU(){

			final int leap = 1;
			
			for (int k = INDEX; k < INDEX + SIZE; k++){
				if (k >= WIDTH*HEIGHT) break;
				
				final int cX = k % WIDTH, cY = k / WIDTH;

				if (cX - leap >= 0) if (imgP2[k - leap] == true) imgP[k] = true;
				if (cX + leap < WIDTH) if (imgP2[k + leap] == true) imgP[k] = true;
				if (cY - leap >= 0) if (imgP2[k - WIDTH*leap] == true) imgP[k] = true;
				if (cY + leap < HEIGHT) if (imgP2[k + WIDTH*leap] == true) imgP[k] = true;
				//digonals
				if (cX - leap >= 0 && cY - leap >= 0) if (imgP2[k - WIDTH*leap - leap] == true) imgP[k] = true;
				if (cY - leap >= 0 && cX + leap < WIDTH) if (imgP2[k - WIDTH*leap + leap] == true) imgP[k] = true;
				if (cY + leap < HEIGHT && cX - leap >= 0) if (imgP2[k + WIDTH*leap - leap] == true) imgP[k] = true;
				if (cY + leap < HEIGHT && cX + leap < WIDTH) if (imgP2[k + WIDTH*leap + leap] == true) imgP[k] = true;

			}

			//copy
			//for (int k = INDEX; k < INDEX + SIZE && k < WIDTH*HEIGHT; k++){
			//	imgP2[k] = imgP[k];
			//}

	
			return imgP;
		}
	
	}
	
	
	
	public class kMSThread extends Thread{
		
		private int[] data;
		private final int leap, INDEX, SIZE, WIDTH, HEIGHT, K;
		
		kMSThread(int[] data, final int leap, final int INDEX, final int SIZE, final int WIDTH, final int HEIGHT, final int K){
			this.data = data;
			this.leap = leap;
			this.INDEX = INDEX;
			this.SIZE = SIZE;
			this.WIDTH = WIDTH;
			this.HEIGHT = HEIGHT;
			this.K = K;
		}
		
		public void run(){
			kMorphologicalSets();
		}
		
		private int[] kMorphologicalSets()
		{
			int cValue = 0;
			boolean still, success;
			for (int id=INDEX; id<INDEX+SIZE; id++){
				if (id >= WIDTH * HEIGHT) break;
				
				cValue = data[id];
				if (cValue == C) continue;
				
				still = true; success = true;
				final int cX = id % WIDTH, cY = id / WIDTH;
				
				final int previousValue = cValue;
				
				
				//dilate according to the chosen structuring element
				if (cX - leap >= 0) if (data[id - leap] > cValue) cValue = data[id - leap];
				if (cX + leap < WIDTH) if (data[id + leap] > cValue) cValue = data[id + leap];
				if (cY - leap >= 0) if (data[id - WIDTH*leap] > cValue) cValue = data[id - WIDTH*leap];
				if (cY + leap < HEIGHT) if (data[id + WIDTH*leap] > cValue) cValue = data[id + WIDTH*leap];
				//digonals
				if (cX - leap >= 0 && cY - leap >= 0) if (data[id - WIDTH*leap - leap] > cValue) cValue = data[id - WIDTH*leap - leap];
				if (cY - leap >=0 && cX + leap < WIDTH) if (data[id - WIDTH*leap + leap] > cValue) cValue = data[id - WIDTH*leap + leap];
				if (cY + leap < HEIGHT && cX - leap >= 0) if (data[id + WIDTH*leap - leap] > cValue) cValue = data[id + WIDTH*leap - leap];
				if (cY + leap < HEIGHT && cX + leap < WIDTH) if (data[id + WIDTH*leap + leap] > cValue) cValue = data[id + WIDTH*leap + leap];
				//
				
				//updates the value for the pixel in data
				data[id] = cValue;
				
				//if we already know that the k-ms did not end, then continue to dilate the image
				if (!idempotence.get()) continue;

				//checks idempotence
				if (cValue != previousValue){//idempotencia, um diferente, não atingiu
					idempotence.set(false);
				}
				
				//if we already know that the k-ms did not end, then continue to dilate the image
				if (!finished.get()) continue;
	
				//checks if ended
				for (int k = 0; k < K && still; k++){
						success = kArray[k].compareAndSet(C, cValue);
							
						if (success || kArray[k].get() == cValue){
							still = false;
							break;
						}
				}
				
				//if there is still something to do then finished is false
				if (still) finished.set(false);
	
			}

			return data;
		}
		
	}
	
}
