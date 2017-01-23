import java.awt.image.BufferedImage;
import java.awt.image.Raster;
import java.awt.image.WritableRaster;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Random;

import javax.imageio.ImageIO;


public class kMorphologicalSetsSequential {
	private static final int C = -1;
	
	public static void main(String[] args) throws IOException{
		
		BufferedImage bi = ImageIO.read(new File(args[3]));
		Raster r = bi.getRaster();
		
		final int numDilations = Integer.parseInt(args[0]),
				numClusters = Integer.parseInt(args[1]),
				thresNoise = Integer.parseInt(args[2]),
				WIDTH = r.getWidth(), HEIGHT = r.getHeight(),
				IMG_SIZE = r.getWidth()*r.getHeight();
		
		
		final boolean[] imgP = new boolean[IMG_SIZE], imgP2 = new boolean[IMG_SIZE];
		
		int[] data = new int[IMG_SIZE];
		
		for (int i=0; i<HEIGHT; i++){
			for (int j=0; j<WIDTH; j++){
				imgP[i*WIDTH + j] = r.getSample(j, i, 0) > 0;
				imgP2[i*WIDTH + j] = imgP[i*WIDTH + j];
			}
		}
		
		final double start = System.nanoTime();
		
		
		//starting the dilation threads
		int leap = 1;
		int dilCounter = 0;
		if (numDilations > 0){
			while (dilCounter < numDilations){
				for (int k = 0; k < IMG_SIZE; k++){
					
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
				
				//copying back
				for (int i=0; i<HEIGHT; i++){
					for (int j=0; j<WIDTH; j++){
						imgP2[i*WIDTH + j] = imgP[i*WIDTH + j];
					}
				}
				
				dilCounter++;
				
			}
			
		}
		
		for (int i=0; i<HEIGHT; i++){
			for (int j=0; j<WIDTH; j++){
				if (imgP[i*WIDTH + j]) data[i*WIDTH + j] = i*WIDTH + j;
				else data[i*WIDTH + j] = C;
			}
		}
		
		//kms algorithm
		int[] kArray;
		kArray = new int[numClusters];
		boolean finished, idempotence;
		int counter = 0;
		int cValue;
		boolean still, success;
		do{
			counter++;
			
			finished = true; idempotence = true;
			
			//reset karray
			for (int k=0; k<kArray.length; k++){
				kArray[k] = C;
			}
			
			
			for (int id=0; id<IMG_SIZE; id++){//for every pixel in the image
				
				cValue = data[id];
				if (cValue == C) continue;
		
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
				if (!idempotence) continue;
				
				//checks idempotence
				if (cValue != previousValue){//idempotencia, um diferente, não atingiu
					idempotence = false;
				}
				
			
				//if we already know that the k-ms did not end, then continue to dilate the image
				if (!finished) continue;
				
				//reset
				still = true; success = true;
	
				//checks if ended
				for (int k = 0; k < numClusters; k++){
					success = kArray[k] == C;
					if (success) kArray[k] = cValue;
						
					if (success || kArray[k] == cValue){
						still = false;
						break;
					}
				}
				
				//if there is still something to do then finished is false
				if (still) {
					finished = false;
				}
	
				
				
			}

			
			//changing the leap variable according to the idempotence
			if (idempotence)
				leap++;
			else
				leap = 1;
			
			
			//finished also depends on idempotence
			finished = (idempotence && finished);
			
		}while(!finished);
		
		
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
				final int cValuef = data[i*WIDTH + j];
				
				if (cValuef == C)
					continue;
				
				int red = 0, green = 0, blue = 0;
				boolean contains = false;
				
				for (int k=0; k<colours.size(); k++){
					if (colours.get(k).value == cValuef){
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
					
					

				
					Vector3 nVec = new kMorphologicalSetsSequential().new Vector3();
					nVec.r = red; nVec.g = green; nVec.b = blue; nVec.value = cValuef;
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
		
		
		
		//output final information
		System.out.printf("The clustering ended with %d iterations and %d valid clusters. \n", counter, colours.size());
		System.out.printf("Cluster values/identifiers: [");
		for (int k = 0; k < numClusters - 1; k++){
			System.out.printf("%d, ", kArray[k]);
		}
		System.out.printf("%d]\n", kArray[numClusters - 1]);
		
	}
	
	public class Vector3{
		
		public int r, g, b, value, counter;
		
	}

}
