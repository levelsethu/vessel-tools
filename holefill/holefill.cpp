/*
 * To fill holes in a segmented binary image.
 * If the filler program is not able to fill all vessels completely voids will remain.
 * These holes create "bubbles" in the skeletonised image, and the resulting network
 * generated by the topology program will have many densely-clustered spurious edges at these locations.
 */

#include <cstdio>
#include <vector>
/*
#include <algorithm>
#include <math.h>
#include <string.h>
#include <string>
#include <sstream>
#include <assert.h>
#include <ctime>
*/
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkSize.h"
#include <itkBinaryFillholeImageFilter.h>

typedef itk::Image<unsigned char,3> ImageType;
ImageType::Pointer im;
unsigned char *p, *p2;

#define V(a,b,c)  p[(c)*xysize+(b)*width+(a)]
#define V2(a,b,c)  p2[(c)*xysize2+(b)*width2+(a)]

int main(int argc, char**argv)
{
	long long width, height, depth, xysize;
	bool use_compression = true;

	if (argc != 3) {
		printf("Usage: holefill input_tiff output_tiff\n");
		return 1;
	}

	printf("Input image file: %s\n",argv[1]);
	printf("Output image file: %s\n",argv[2]);

	typedef itk::ImageFileReader<ImageType> FileReaderType;
	FileReaderType::Pointer reader = FileReaderType::New();

	reader->SetFileName(argv[1]);
	try
	{
		reader->Update();
	}
	catch (itk::ExceptionObject &e)
	{
		std::cout << e << std::endl;
		return 2;
	}

	im = reader->GetOutput();

	width = im->GetLargestPossibleRegion().GetSize()[0];
	height = im->GetLargestPossibleRegion().GetSize()[1];
	depth = im->GetLargestPossibleRegion().GetSize()[2];
	xysize = width*height;
	printf("Image dimensions: width, height, depth: %d %d %d\n",width,height,depth);
	p = (unsigned char *)(im->GetBufferPointer());

	typedef itk::BinaryFillholeImageFilter<ImageType> BinFillType;
	BinFillType::Pointer binFill=BinFillType::New();
	binFill->SetInput(im);
	binFill->Update();

	typedef itk::ImageFileWriter<ImageType> FileWriterType;
	FileWriterType::Pointer writer = FileWriterType::New();
	writer->SetFileName(argv[2]);
	writer->SetInput(binFill->GetOutput());
	if (use_compression) {
		writer->UseCompressionOn();
	}
	try
	{
		writer->Update();
	}
	catch (itk::ExceptionObject &e)
	{
		std::cout << e << std::endl;
		return 4;
	}
	if (use_compression) {
		printf("Created compressed image file: %s\n",argv[2]);
	} else {
		printf("Created uncompressed image file: %s\n",argv[2]);
	}

	return 0;
}