#ifndef EDGES_H
#define EDGES_H
#include <string>
#include <iostream>
#include <FreeImage.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <mpi.h>

class Image 
{
	FIBITMAP *bitmap;
	FIBITMAP *edge;
	
	unsigned int bpp;
	int scan_width;
	
	
	uint8_t *bitmappixels;
	int world_size;
	
	public:
	
	Image()
	{
		FreeImage_Initialise();	
	}

	std::string inputFileName;
	static constexpr int32_t conv[9] = {0,1,0,1,-4,1,0,1,0};
	//static constexpr int32_t conv[9] = {0,0,0,1,-1,0,0,0,0};
	int w,h;
	
	struct fragment_info {
		int width;
		int rows;
		int bufferStart;
		int bufferlength;
		bool first;
		bool last;
	};
	
	struct imageBlock_info 
	{
		int start;
		int size;
	};
	
	
	Image(int world_size): world_size(world_size)
	{
		
	}
	
	imageBlock_info getBlockinfo(int rank)
	{
		imageBlock_info block;
		if(rank == 0)
		{
			block.start = 0;
		}
		else
		{
			block.start = rank*w-h; //because it must have one line more before to calculations
		}
		
	}
	
	fragment_info process_info[10]; // assumes max 10 processes
	
	fragment_info* divideImage()
	{
		//calculate width and height for each row
		int rem = h  % world_size;
		if(rem == 0)
		{
			int rowsperprocess = h/world_size;
			for(int i =0; i < world_size;i++)
			{
				process_info[i].width = w;
				process_info[i].rows = rowsperprocess;
				
			}
		}
		else
		{
			int rowsperprocess = h/world_size;
			for(int i =0; i < world_size-1;i++)
			{
				process_info[i].width = w;
				process_info[i].rows = rowsperprocess;
			}
			process_info[world_size-1].width = w;
			process_info[world_size-1].rows = h-(world_size-1)*rowsperprocess;
			
		}
		
		
		///fill buffer info
		int previousRawBufferStart =0;
		int previousRawBufferLength =0;
		
		process_info[0].bufferStart = 0;
		previousRawBufferLength = process_info[0].width*(process_info[0].rows);
		process_info[0].bufferlength = previousRawBufferLength+process_info[0].width;//needs one row more to claculate filtered values
		process_info[0].first = true;
		process_info[0].last = false;
		
		for(int i =1; i < world_size;i++)
		{
	
			previousRawBufferStart = process_info[i].bufferStart = previousRawBufferStart+previousRawBufferLength;
			process_info[i].bufferStart -= w; //one line before
			previousRawBufferLength = process_info[i].width*(process_info[i].rows);
			process_info[i].bufferlength = previousRawBufferLength+w; //one more because one line before
			if(i != world_size - 1) //if not last
			{
				process_info[i].bufferlength  +=w;//one line more after
				process_info[i].first = false;
				process_info[i].last = false;
			}
			if(i == world_size - 1) //if last
			{
				process_info[i].first = false;
				process_info[i].last = true;
			}
			
		}
		return process_info;
	}
	
	
	void printprocessinfo()
	{
		for(int i =0; i < world_size;i++)
		{
			std::cout<<"[List]Rank"<<i<<" rows:"<<process_info[i].rows<<" Start:"<<process_info[i].bufferStart<<" length"<<process_info[i].bufferlength<<std::endl;
		}
	}
	
	bool readfile(std::string img_filename)
	{
		FREE_IMAGE_FORMAT format;
		inputFileName = img_filename;
		format = FreeImage_GetFileType(img_filename.c_str());
		if(format == FIF_UNKNOWN )
		{
			std::cout<<"File: "<<img_filename<<" unknown format"<<std::endl;
			return false;
		}

		bitmap = FreeImage_Load(format,img_filename.c_str(),BMP_DEFAULT);
		if(!bitmap)
		{
			std::cout<<"Cannot open image file:"<<img_filename<<std::endl;
			return false;
		}
		
		
		w = FreeImage_GetWidth(bitmap);
		h = FreeImage_GetHeight(bitmap);
		bpp = FreeImage_GetBPP(bitmap);
		scan_width = FreeImage_GetPitch(bitmap);
		std::cout<<"Width:"<<w<<"Height"<<h<<std::endl;
		std::cout<<"Bpp:"<<bpp<<std::endl;
		std::cout<<"Pitch:"<<scan_width<<std::endl;
		
		FIBITMAP *old_bitmap = bitmap;
		bitmap = FreeImage_ConvertToGreyscale(bitmap);
		bitmappixels = (uint8_t*)FreeImage_GetBits(bitmap);
		FreeImage_Unload(old_bitmap);
		return true;
	}
	
	uint8_t* getsourcebuffer()
	{
		
		return bitmappixels;
	}
	
	~Image()
	{
		FreeImage_Unload(bitmap);
		FreeImage_DeInitialise();
	}
};

#endif
