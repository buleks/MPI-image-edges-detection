#include "edges.h"
#include <mpi.h>
#include <sstream>

using namespace std;

constexpr int INFO_TAG = 13;
constexpr int DATA_TAG = 14;
constexpr int RESULT_TAG = 15;
constexpr uint8_t THRESHOLD = 30;

int main(int argc,char **argv)
{
	int size,rank;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	//cout<<"World size:"<<size<<endl;
	cout<<"WRank:"<<rank<<endl;
	
	MPI_Datatype ProcessDataInfo;
	MPI_Datatype types[6] = { MPI_INT, MPI_INT, MPI_INT,MPI_INT,MPI::BOOL,MPI::BOOL };
	MPI_Aint     offsets[6];
	int blocklengths[6] = { 1, 1, 1,1,1,1 };
	offsets[0] = offsetof(Image::fragment_info,width);
	offsets[1] = offsetof(Image::fragment_info,rows);
	offsets[2] = offsetof(Image::fragment_info,bufferStart);
	offsets[3] = offsetof(Image::fragment_info,bufferlength);
	offsets[4] = offsetof(Image::fragment_info,first);
	offsets[5] = offsetof(Image::fragment_info,last);
		
	MPI_Type_create_struct(6, blocklengths, offsets, types, &ProcessDataInfo);
	MPI_Type_commit(&ProcessDataInfo);
	
	int imageSize=0;
	Image *img;
	Image::fragment_info *dataInfo;
	if(rank == 0)
	{
		if(argc != 2)
		{
			cout<<"Needs file name as argument\n";
			exit(1);
		}
		img = new Image(size);
		
		img->readfile(argv[1]);
		imageSize = img->w*img->h;
		dataInfo = img->divideImage();
		img->printprocessinfo();
		
		
		
		for(int i = 0; i < size;i++)
		{
			MPI_Send(&dataInfo[i],1,ProcessDataInfo,i,INFO_TAG,MPI_COMM_WORLD);
		}
		
		uint8_t *image = img->getsourcebuffer();
		for(int i = 0; i < size;i++)
		{
			MPI_Send(&image[dataInfo[i].bufferStart], dataInfo[i].bufferlength, MPI_UINT8_T,i,DATA_TAG,MPI_COMM_WORLD);
		}
		
	}
	
	Image::fragment_info processDatainfo;
	MPI_Status status;
	MPI_Recv(&processDatainfo,1,ProcessDataInfo,0,INFO_TAG,MPI_COMM_WORLD,&status);
	cout<<"Rank"<<rank<<" rows:"<<processDatainfo.rows<<" start:"<<processDatainfo.bufferStart<<" length:"<< processDatainfo.bufferlength<<" first:"<<processDatainfo.first<<" last:"<<processDatainfo.last<<std::endl;
	uint8_t *imagePart = new uint8_t[processDatainfo.bufferlength];

	//FreeImage_Initialise();
	//FIBITMAP *tempedge = FreeImage_Allocate(processDatainfo.width, processDatainfo.rows+2, 8);
	//uint8_t *tempImage = (uint8_t *)FreeImage_GetBits(tempedge);

	//MPI_Recv(tempImage,processDatainfo.bufferlength,MPI_UINT8_T,0,DATA_TAG,MPI_COMM_WORLD,&status);
	MPI_Recv(imagePart,processDatainfo.bufferlength,MPI_UINT8_T,0,DATA_TAG,MPI_COMM_WORLD,&status);
	cout<<"Received image fragment"<<endl;
	//FreeImage_Initialise();	
	int w = processDatainfo.width;
	int partImageSize = processDatainfo.width*processDatainfo.rows;
	//FIBITMAP *edge = FreeImage_Allocate(processDatainfo.width, processDatainfo.rows, 8);
	//uint8_t *edgepixels = (uint8_t *)FreeImage_GetBits(edge);
	//memcpy(edgepixels,imagePart,processDatainfo.width* processDatainfo.rows);
	uint8_t *processImagePart = new uint8_t[partImageSize];
	
	
	
	for(int i =0 ; i < partImageSize;i++)
	{
		
		//if(i%w == 0) //first column
		//((i%w == 0) ?  : 0)
		
		//(((i-1)%w == 0) ? 0 : ) //last column
	
		int32_t temp=0;
		////calculations based on first line of coefficients matrix
		if(true == processDatainfo.first )//first fragment
		{
		 //first line of coefficients matrix shouldnt be calculated
		 if(i >= w)
		 {
			temp+=Image::conv[0]*(((i-1-w) >=0) ? imagePart[i-1-w] : 0);
			temp+=Image::conv[1]*imagePart[i-w];
			temp+=Image::conv[2]*(((i-1-w)%w == 0) ? 0 : imagePart[i+1-w]);
		 }
		} else
		{//(((i-1) >=0) ?  : 0)
			temp+=Image::conv[0]*(((i-1-w) >=0) ? imagePart[i-1-w] : 0);
			temp+=Image::conv[1]*imagePart[i];
			temp+=Image::conv[2]*(((i-1-w)%w == 0) ? 0 : imagePart[i+1-w]);
		}
		////end of first line of coefficients matrix
		
		////calculations based on second line of coefficients matrix
		if(true == processDatainfo.first )//first fragment
		{
			temp+=Image::conv[3]*((i%w == 0) ? 0 : imagePart[i-1]);
			temp+=Image::conv[4]*imagePart[i];
			temp+=Image::conv[5]*(((i-1)%w == 0) ? 0 : imagePart[i+2]);
		}
		else
		{
			temp+=Image::conv[3]*((i%w == 0) ? 0 : imagePart[i+w-1]);
			temp+=Image::conv[4]*imagePart[i+w];
			temp+=Image::conv[5]*(((i-1)%w == 0) ? 0 : imagePart[i+w+2]);
		}
		
		////end of calculations based on first line of coefficients matrix
		
		
		//third line of matrix
		if(true == processDatainfo.first )//first fragment
		{
			temp+=Image::conv[6]*((i%w == 0) ? 0 : imagePart[i+w-1]);
			temp+=Image::conv[7]*imagePart[i+w];
			temp+=Image::conv[8]*(((i-1)%w == 0) ? 0 : imagePart[i+w+1]);
		}else
		{
			if(!(true == processDatainfo.last && i>= (partImageSize-w))) //all lines without last line in last fragment
			{
				temp+=Image::conv[6]*((i%w == 0) ? 0 : imagePart[i+w-1]);
				temp+=Image::conv[7]*imagePart[i+2*w];
				temp+=Image::conv[8]*(((i-1)%w == 0) ? 0 : imagePart[i+w+1]);
			}
			
		}
			
		
		////////////////
		//copy raw image into image part
		//if(true == processDatainfo.first )
		//{
		//	processImagePart[i]=imagePart[i];
		//}else
		//{
		//	processImagePart[i]=imagePart[i+w];
		//}
		//end raw image
		////////
		if(temp>THRESHOLD)
		{
			processImagePart[i]=0xff;
		}
		else
		{
			processImagePart[i]=0;
		}
	}

	//std::ostringstream file;
	//file << "e"<<rank;
		//FreeImage_Save(FIF_PNG,tempedge,file.str().c_str(),0);
		//FreeImage_Unload(tempedge);
	delete imagePart;

	int *receiveCounts = new int[size];
	int *displacement = new int[size];
	if(rank == 0)
	{
		displacement[0]=0;
		for(int i=0;i < size; i++)
		{
			receiveCounts[i] = dataInfo[i].width*dataInfo[i].rows;
			if(i>0)
			{
				displacement[i]= displacement[i-1]+receiveCounts[i-1];
			}
		}
	}
	
	FIBITMAP *edge = FreeImage_Allocate(img->w, img->h, 8);
	uint8_t *gatheredImage = (uint8_t *)FreeImage_GetBits(edge);
	
	MPI_Gatherv(processImagePart, partImageSize,MPI_UINT8_T,gatheredImage, receiveCounts, displacement, MPI_UINT8_T,0, MPI_COMM_WORLD);
	if(rank == 0)
	{
		cout<<"Received gathered data"<<endl;
	}
	delete processImagePart; //remove process image buffer
	
	if(rank == 0)
	{
		std::ostringstream file;
		file << "result"<<rank;
		FreeImage_Save(FIF_PNG,edge,file.str().c_str(),0);
		FreeImage_Unload(edge);
		//FreeImage_DeInitialise();
	}
	if(rank == 0)
	{
		delete img;
	}
	MPI_Type_free(&ProcessDataInfo);
	MPI_Finalize();
	return 0;
}
