#include "ImageBMP.h"

void ImageBMP::writeImageFile(std::string filename)
{
	ofstream fout{ filename, std::ios::binary };

	//first comes the 14-byte file header: 
	fout.write(reinterpret_cast<const char*>(fileHeader.filetype.data()), 2); //no sizeof here, since filetype is a pointer
	fout.write(reinterpret_cast<const char*>(&fileHeader.fileSize), sizeof(fileHeader.fileSize));
	fout.write(reinterpret_cast<const char*>(&fileHeader.reserved1And2), sizeof(fileHeader.reserved1And2));
	fout.write(reinterpret_cast<const char*>(&fileHeader.indexOfPixelData), sizeof(fileHeader.indexOfPixelData));

	//next, the 40-byte info header: 
	fout.write(reinterpret_cast<const char*>(&infoHeader.infoHeaderSize), sizeof(infoHeader.infoHeaderSize));
	fout.write(reinterpret_cast<const char*>(&infoHeader.imageWidth), sizeof(infoHeader.imageWidth));
	fout.write(reinterpret_cast<const char*>(&infoHeader.imageHeight), sizeof(infoHeader.imageHeight));
	fout.write(reinterpret_cast<const char*>(&infoHeader.planes), sizeof(infoHeader.planes));
	fout.write(reinterpret_cast<const char*>(&infoHeader.bitsPerPixel), sizeof(infoHeader.bitsPerPixel));
	fout.write(reinterpret_cast<const char*>(&infoHeader.compressionMethod), sizeof(infoHeader.compressionMethod));
	fout.write(reinterpret_cast<const char*>(&infoHeader.sizeOfPixelData), sizeof(infoHeader.sizeOfPixelData));

	for (const auto& remainingHeaderField : infoHeader.remainingHeaderFields)
	{
		fout.write(reinterpret_cast<const char*>(&remainingHeaderField), sizeof(remainingHeaderField));
	}

	// Calculate the number of bytes per pixel based on the bit depth
	int bytesPerPixel = infoHeader.bitsPerPixel / 8;

	// Calculate the padding bytes per row (each row is padded to a multiple of 4 bytes)
	int paddingBytes = (4 - (infoHeader.imageWidth * bytesPerPixel) % 4) % 4;

	//now the pixel data: 
	for (unsigned int row = 0; row < infoHeader.imageHeight; ++row)
	{
		for (unsigned int col = 0; col < infoHeader.imageWidth; ++col)
		{
			if (infoHeader.bitsPerPixel == 32)
			{


				unsigned int colorOfCurrentPixel = pixelData.pixelMatrix.at(row).at(col).convertToUnsignedInt();

				fout.write(reinterpret_cast<const char*>(&colorOfCurrentPixel), sizeof(colorOfCurrentPixel));
			}

			else if (infoHeader.bitsPerPixel == 24)
			{
				unsigned int colorOfCurrentPixel = pixelData.pixelMatrix.at(row).at(col).convertToUnsignedInt();
				char rgb[3] = {
					static_cast<char>((colorOfCurrentPixel >> 0) & 0xFF),
					static_cast<char>((colorOfCurrentPixel >> 8) & 0xFF),
					static_cast<char>((colorOfCurrentPixel >> 16) & 0xFF)
				};
				fout.write(rgb, 3);
			}

			else
			{
				std::cout << "Hey! Neither 32 nor 24 bits per pixel? What is this file?\n";
				std::cin.get();
			}
		}

		// Write the padding bytes at the end of each row
		char padding[3] = { 0, 0, 0 };
		fout.write(padding, paddingBytes);
	}

	fout.close();
}

ImageBMP::ImageBMP(unsigned int imageWidth, unsigned int imageHeight, const Color& fillColor, const Color& middleDotColor)
{
	infoHeader.imageWidth = imageWidth;
	infoHeader.imageHeight = imageHeight;
	infoHeader.sizeOfPixelData = imageWidth * imageHeight * (infoHeader.bitsPerPixel / 8);
	//NOTE: fileheader size should always be 14 (I think) 
	fileHeader.fileSize = 14 + infoHeader.getInfoHeaderSize() + infoHeader.sizeOfPixelData;



	//fill pixelData with given fill color:
	for (unsigned int row = 0; row < imageHeight; ++row)
	{
		std::vector<Color> currentRow;
		for (unsigned int col = 0; col < imageWidth; ++col)
		{
			currentRow.push_back(fillColor);
		}
		pixelData.pixelMatrix.push_back(currentRow);
	}

	//add the middle dot (having different color): 
	pixelData.pixelMatrix.at(imageWidth / 2).at(imageHeight / 2) = middleDotColor;

}

ImageBMP::ImageBMP(unsigned int imageWidth, unsigned int imageHeight, const Color& fillColor)
{
	infoHeader.imageWidth = imageWidth;
	infoHeader.imageHeight = imageHeight;
	infoHeader.sizeOfPixelData = imageWidth * imageHeight * (infoHeader.bitsPerPixel / 8);
	//NOTE: fileheader size should always be 14 (I think) 
	fileHeader.fileSize = 14 + infoHeader.getInfoHeaderSize() + infoHeader.sizeOfPixelData;



	//fill pixelData with given fill color:
	for (unsigned int row = 0; row < imageHeight; ++row)
	{
		vector<Color> currentRow;
		for (unsigned int col = 0; col < imageWidth; ++col)
		{
			currentRow.push_back(fillColor);
		}
		pixelData.pixelMatrix.push_back(currentRow);
	}
}

ImageBMP::ImageBMP(const string& filepath)
{
	readImageBMP(filepath);
}

void ImageBMP::readImageBMP(string inputFilename)
{
	ifstream fin{ inputFilename, std::ios::binary };

	if (!fin)
	{
		std::cout << "File " << inputFilename << " not found.\n";
		std::cin.get();
		return;
	}

	//first read the file header info: 
	readFileHeaderFromFile(fin);

	//now read info header: 
	readInfoHeaderFromFile(fin);

	readPixelDataFromFile(fin);

	//pixelData.pixelMatrix; 


	//0xb6
	//0x03'A9'B6

	fin.close();
}

//only allow integer scaling (no 1.5x) 
void ImageBMP::doublescaleImageBMP()
{
	unsigned int scalingFactor = 2;

	//first, make the needed updates to the headers: 
	fileHeader.fileSize = fileHeader.fileSize + scalingFactor * scalingFactor * infoHeader.sizeOfPixelData;

	infoHeader.imageWidth = infoHeader.imageWidth * scalingFactor;
	infoHeader.imageHeight = infoHeader.imageHeight * scalingFactor;

	infoHeader.sizeOfPixelData = scalingFactor * scalingFactor * infoHeader.sizeOfPixelData;
	//ex: if 3 rows and 3 cols (9) pixels originally, then 36 pixels for scalingFactor = 2
	//since now 6 rows and 6 cols 

	//now, modify pixel data (the more complicated/interesting part of this function): 

	std::vector<std::vector<Color>> newPixelMatrix(infoHeader.imageHeight, std::vector < Color>(infoHeader.imageWidth));
	//int a;

	for (int row = 0; row < pixelData.pixelMatrix.size(); ++row)
	{
		for (int col = 0; col < pixelData.pixelMatrix.at(0).size(); ++col)
		{
			std::vector<std::pair<int, int>> newIndices =
			{
				std::pair<int, int> {2 * row,			2 * col }, //newIndexCenter
				std::pair<int, int> {2 * row + 1,		2 * col}, //newIndexAbove
				std::pair<int, int> {2 * row + 1,		2 * col + 1}, //newIndexAboveAndRight
				std::pair<int, int> {2 * row,			2 * col + 1}//newIndexToRight
			};

			for (auto& newIndex : newIndices)
			{
				newPixelMatrix.at(newIndex.first).at(newIndex.second) = pixelData.pixelMatrix.at(row).at(col);
			}
			//newPixelMatrix.at(newIndexCenter.first).at(newIndexCenter.second) = 
			//	pixelData.pixelMatrix.at(row).at(col); 

			//newPixelMatrix.at(newIndexAbove.first).at(newIndexAbove.second) =
			//	pixelData.pixelMatrix.at(row).at(col);

			//newPixelMatrix.at(newIndexAboveAndRight.first).at(newIndexAboveAndRight.second) =
			//	pixelData.pixelMatrix.at(row).at(col);

			//newPixelMatrix.at(newIndexCenter.first).at(newIndexCenter.second) =
			//	pixelData.pixelMatrix.at(row).at(col);

		}
	}
	//resize and copy new into old (the member variable that will live beyond this function scope): 
	pixelData.pixelMatrix.resize(infoHeader.imageHeight, vector<Color>(infoHeader.imageWidth));

	pixelData.pixelMatrix = newPixelMatrix;

}


void ImageBMP::readFileHeaderFromFile(ifstream& fin)
{
	char filetype[2];
	fin.read(filetype, 2); //NOTE: fin.GET() appends null terminator! (\0)
	fileHeader.filetype.at(0) = filetype[0];
	fileHeader.filetype.at(1) = filetype[1];

	char filesize[4];
	fin.read(filesize, 4);

	//DETAILED approach without bitshifting and bitwise OR: 
	//auto first = (unsigned char)filesize[0];
	//auto second= (unsigned char)filesize[1];
	//auto third = (unsigned char)filesize[2];
	//auto fourth = (unsigned char)filesize[3];
	//cout << first + (second * pow(2, 8)) + (third*pow(2, 16)) + (fourth*pow(2, 24)) << "\n";

	//equivalently (faster, probably): 
	fileHeader.fileSize =
		(
			(unsigned char)filesize[0] << 0 |
			(unsigned char)filesize[1] << 8 |
			(unsigned char)filesize[2] << 16 |
			(unsigned char)filesize[3] << 24
			);
	char reserved1And2[4];
	fin.read(reserved1And2, 4);

	fileHeader.reserved1And2 = (
		(unsigned char)reserved1And2[0] << 0 |
		(unsigned char)reserved1And2[1] << 8 |
		(unsigned char)reserved1And2[2] << 16 |
		(unsigned char)reserved1And2[3] << 24
		);

	char indexOfPixelData[4];
	fin.read(indexOfPixelData, 4);
	fileHeader.indexOfPixelData = (
		(unsigned char)indexOfPixelData[0] << 0 |
		(unsigned char)indexOfPixelData[1] << 8 |
		(unsigned char)indexOfPixelData[2] << 16 |
		(unsigned char)indexOfPixelData[3] << 24
		);

}

void ImageBMP::readInfoHeaderFromFile(ifstream& fin)
{
	char infoHeaderSize[4];
	fin.read(infoHeaderSize, 4);
	infoHeader.infoHeaderSize =
		(
			(unsigned char)infoHeaderSize[0] << 0 |
			(unsigned char)infoHeaderSize[1] << 8 |
			(unsigned char)infoHeaderSize[2] << 16 |
			(unsigned char)infoHeaderSize[3] << 24
			);

	/*a "safety check" here:*/
	if (infoHeader.infoHeaderSize != 40)
	{
		//cout << "Hey! Listen!\n"; 
		//cout << "Info header size is not 40! - it is " << infoHeader.infoHeaderSize << "\n";
		//std::cin.get(); 
	}


	char imageWidth[4];
	fin.read(imageWidth, 4);
	infoHeader.imageWidth =
		(
			(unsigned char)imageWidth[0] << 0 |
			(unsigned char)imageWidth[1] << 8 |
			(unsigned char)imageWidth[2] << 16 |
			(unsigned char)imageWidth[3] << 24
			);

	char imageHeight[4];
	fin.read(imageHeight, 4);
	infoHeader.imageHeight =
		(
			(unsigned char)imageHeight[0] << 0 |
			(unsigned char)imageHeight[1] << 8 |
			(unsigned char)imageHeight[2] << 16 |
			(unsigned char)imageHeight[3] << 24
			);

	char planes[2];
	fin.read(planes, 2);
	infoHeader.planes =
		(
			(unsigned char)planes[0] << 0 |
			(unsigned char)planes[1] << 8
			);

	char bitsPerPixel[2];
	fin.read(bitsPerPixel, 2);
	infoHeader.bitsPerPixel =
		(
			(unsigned char)bitsPerPixel[0] << 0 |
			(unsigned char)bitsPerPixel[1] << 8
			);

	/*another "safety check" here:*/
	if (infoHeader.bitsPerPixel != 32)
	{
		//cout << "Hey! Listen!\n";
		//cout << "bitsPerPixel is not 32 (gbra)! - it is " << infoHeader.bitsPerPixel << "\n";
		//std::cin.get();
	}


	char compressionMethod[4];
	fin.read(compressionMethod, 4);
	infoHeader.compressionMethod =
		(
			(unsigned char)compressionMethod[0] << 0 |
			(unsigned char)compressionMethod[1] << 8 |
			(unsigned char)compressionMethod[2] << 16 |
			(unsigned char)compressionMethod[3] << 24
			);

	char sizeOfPixelData[4];
	fin.read(sizeOfPixelData, 4);
	infoHeader.sizeOfPixelData =
		(
			(unsigned char)sizeOfPixelData[0] << 0 |
			(unsigned char)sizeOfPixelData[1] << 8 |
			(unsigned char)sizeOfPixelData[2] << 16 |
			(unsigned char)sizeOfPixelData[3] << 24
			);


	assert(infoHeader.remainingHeaderFields.size() == 4); //useless assertion? 

	for (int i = 0; i < infoHeader.remainingHeaderFields.size(); ++i)
	{
		char remainingHeaderFields[4];
		fin.read(remainingHeaderFields, 4);

		infoHeader.remainingHeaderFields.at(i) =
			(
				(unsigned char)remainingHeaderFields[0] << 0 |
				(unsigned char)remainingHeaderFields[1] << 8 |
				(unsigned char)remainingHeaderFields[2] << 16 |
				(unsigned char)remainingHeaderFields[3] << 24
				);

	}
}


/*Note: method is made ghastly long by handling 24 and 32 bit color*/
void ImageBMP::readPixelDataFromFile(ifstream& fin)
{

	if (infoHeader.bitsPerPixel == 32)
	{
		pixelData.pixelMatrix.resize(infoHeader.imageHeight,
			std::vector<Color>(infoHeader.imageWidth));	//CAREFUL to resize as a TWO-d array - NOT 1D!

		for (unsigned int row = 0; row < infoHeader.imageHeight; ++row)
		{
			for (unsigned int col = 0; col < infoHeader.imageWidth; ++col)
			{
				char bgra[4];
				fin.read(bgra, 4);

				if (fin.fail())
					//fin.fail gets set to true if, for example, ... the `row` counter variable goes too far
					//ex: 	for (int row = 0; row < infoHeader.imageHeight + 1; ++row)
				{
					std::cout << "Error: Attempted to read beyond the end of the file at row " << row << ", col " << col << ".\n";
					std::cin.get();
					return;
				}

				Color currentPixelColor
				{ (unsigned int)(unsigned char)bgra[0], (unsigned int)(unsigned char)bgra[1] ,
					(unsigned int)(unsigned char)bgra[2] , (unsigned int)(unsigned char)bgra[3] };
				if (col < infoHeader.imageWidth)
				{
					pixelData.pixelMatrix.at(row).at(col) = currentPixelColor;
				}

				else
				{
					std::cout << "Error: Attempted to access out-of-bounds pixel at row " << row << ", col " << col << ".\n";
					std::cin.get();
					return;
				}

			}
		}

		int lastThingInFile = fin.get(); //should be -1, I think

		//confirm that the end of the file was reached:
		if (!fin.eof())
		{
			std::cout << "Hey!\nListen\n EOF was not reached? Is there more pixel data? \n";
			std::cin.get();
		}
	}

	else
	{
		// Calculate the number of bytes per pixel based on the bit depth
		int bytesPerPixel = infoHeader.bitsPerPixel / 8;

		// Calculate the padding bytes per row (each row is padded to a multiple of 4 bytes)
		int paddingBytes = (4 - (infoHeader.imageWidth * bytesPerPixel) % 4) % 4;

		//cout << "not 32 bits per pixel\n";
		pixelData.pixelMatrix.resize(infoHeader.imageHeight,
			std::vector<Color>(infoHeader.imageWidth));	//CAREFUL to resize as a TWO-d array - NOT 1D!

		for (unsigned int row = 0; row < infoHeader.imageHeight; ++row)
		{
			for (unsigned int col = 0; col < infoHeader.imageWidth; ++col)
			{
				char bgr[3];
				fin.read(bgr, 3);

				if (fin.fail())
					//fin.fail gets set to true if, for example, ... the `row` counter variable goes too far
					//ex: 	for (int row = 0; row < infoHeader.imageHeight + 1; ++row)
				{
					std::cout << "Error: Attempted to read beyond the end of the file at row " << row << ", col " << col << ".\n";
					std::cin.get();
					return;
				}

				Color currentPixelColor
				{ (unsigned int)(unsigned char)bgr[0], (unsigned int)(unsigned char)bgr[1] ,
					(unsigned int)(unsigned char)bgr[2] };
				if (col < infoHeader.imageWidth)
				{
					pixelData.pixelMatrix.at(row).at(col) = currentPixelColor;
				}

				else
				{
					std::cout << "Error: Attempted to access out-of-bounds pixel at row " << row << ", col " << col << ".\n";
					std::cin.get();
					return;
				}

			}
			// Skip the padding bytes at the end of each row
			fin.ignore(paddingBytes);

		}

		//int counter = 0; 
		//while (!fin.eof())
		//{
			//counter++; 
		int lastThingInFile = fin.get(); //should be -1, I think
		//}

		//confirm that the end of the file was reached:
		if (!fin.eof())
		{
			std::cout << "Hey!\nListen\n EOF was not reached? Is there more pixel data? \n";
			std::cin.get();
		}
	}
}

/*Modifies pixelData - no change to fileHeader or infoHeader*/
void ImageBMP::drawRectangleOutline(unsigned int x0, unsigned int y0,
	unsigned int rectangleWidth, unsigned int rectangleHeight, const Color& color)
{

	assert(x0 + rectangleWidth <= infoHeader.imageWidth);
	assert(y0 + rectangleHeight <= infoHeader.imageHeight);

	// Top line
	for (unsigned int i = x0; i < x0 + rectangleWidth; ++i)
	{
		pixelData.pixelMatrix.at(y0).at(i) = color;
	}

	// Bottom line
	for (unsigned int i = x0; i < x0 + rectangleWidth; ++i)
	{
		pixelData.pixelMatrix.at(y0 + rectangleHeight - 1).at(i) = color;
	}

	// Left line
	for (unsigned int i = y0; i < y0 + rectangleHeight; ++i)
	{
		pixelData.pixelMatrix.at(i).at(x0) = color;
	}

	// Right line
	for (unsigned int i = y0; i < y0 + rectangleHeight; ++i)
	{
		pixelData.pixelMatrix.at(i).at(x0 + rectangleWidth - 1) = color;
	}
}

void ImageBMP::fillRectangleWithColor(unsigned int x0, unsigned int y0, unsigned int rectangleWidth, unsigned int rectangleHeight, const Color& color)
{
	std::swap(x0, y0); //stupid, but ah well -> images use image[row][col], where row is y value and col is x value

	for (unsigned int row = x0; row < x0 + rectangleWidth; ++row)
	{
		for (unsigned int col = y0; col < y0 + rectangleHeight; ++col)
		{
			pixelData.pixelMatrix.at(row).at(col) = color;
		}
	}

}

/*NOTE: this method will be swapping x and y */
void ImageBMP::setPixelToColor_withThickness(unsigned int x, unsigned int y, const Color& color, unsigned int thickness)
{
	if (x >= infoHeader.imageWidth || y >= infoHeader.imageHeight)
	{
		std::cout << "Error: Center pixel out of bounds.\n";
		return;
	}

	//center pixel 
	pixelData.pixelMatrix[x][y] = color;


	if (thickness > 1)
	{
		//neighbors within thickness:
		for (unsigned int row = -1 * (int)thickness; row <= (int)thickness; ++row)
		{
			for (unsigned int col = -1 * (int)thickness; col < (int)thickness; ++col)
			{
				int newX = x + col;
				int newY = y + row;

				// Ensure the new coordinates are within bounds
				if (newX >= 0
					&& newX < static_cast<int>(infoHeader.imageWidth)
					&& newY >= 0
					&& newY < static_cast<int>(infoHeader.imageHeight))
				{
					pixelData.pixelMatrix[newX][newY] = color;
				}

			}
		}
	}

}

/*purpose: to gain experience with "scanline" algorithms*/
void ImageBMP::drawAndFillAnIrregularShape()
{
}


unsigned int InfoHeader::getInfoHeaderSize() const
{
	return infoHeaderSize;
}

unsigned int InfoHeader::getSizeOfPixelData() const
{
	//return sizeOfPixelData;
	return imageWidth * imageHeight * (bitsPerPixel / 8);
}

Color::Color(unsigned int bgra)
	:bgra(bgra)
{
}

Color::Color(unsigned int b, unsigned int g, unsigned int r)
{
	bgra = (b << 0) | (g << 8) | (r << 16) | (0xFF << 24); // Set alpha to 255
}

Color::Color(unsigned int b, unsigned int g, unsigned int r, unsigned int a)
{
	bgra =
		(
			b << 0 |
			g << 8 |
			r << 16 |
			a << 24
			);
}

Color::Color(ColorEnum colorEnum)
	:bgra((unsigned int)colorEnum) //note the typecast
{

}

unsigned int Color::convertToUnsignedInt()
{
	return bgra;
}





#pragma region auxillary functions

/*[1, 2, 3] will become
[
1
2
3
]
*/
std::vector<std::vector<char>> rotateMatrixClockwise
	(std::vector<std::vector<char>>& originalMatrix, int originalNumberOfRows, int originalNumberOfCols)
{
	std::vector<std::vector<char>> rotatedMatrix;

	//note the switched order of rows and cols: 
	rotatedMatrix.resize(originalNumberOfCols, vector<char>(originalNumberOfRows));

	for (int row = 0; row < originalNumberOfRows; ++row)
	{
		for (int col = 0; col < originalNumberOfCols; ++col)
		{
			//I didn't work through this equation - only walked through in debug mode...
			rotatedMatrix[col][originalNumberOfRows - 1 - row] = originalMatrix[row][col];
		}
	}


	return rotatedMatrix;
}

/*No great need for this one - just being goofy*/
std::vector<std::vector<int>> rotateIntMatrixClockwise
	(std::vector<std::vector<int>>& originalMatrix, int originalNumberOfRows, int originalNumberOfCols)
{
	std::vector<std::vector<int>> rotatedMatrix;

	//note the switched order of rows and cols: 
	rotatedMatrix.resize(originalNumberOfCols, std::vector<int>(originalNumberOfRows));

	for (int row = 0; row < originalNumberOfRows; ++row)
	{
		for (int col = 0; col < originalNumberOfCols; ++col)
		{
			//I didn't work through this equation - only walked through in debug mode...
			rotatedMatrix[col][originalNumberOfRows - 1 - row] = originalMatrix[row][col];
		}
	}


	return rotatedMatrix;
}



#ifdef __cplusplus
#if __cplusplus >= 201703L
std::vector<ImageBMP> getAllImagesInFolder(std::string folderName)
{
	std::vector<ImageBMP> allImagesInFolder;

	auto startPath = std::filesystem::current_path();
	//set current directory to folder name to get images from: 
	auto newPath = startPath.string() + folderName;
	std::filesystem::current_path(newPath);

	for (auto& entry : std::filesystem::directory_iterator(newPath))
	{
		//cout << entry.path() << "\n";
		//ImageBMP currentPieceImage{}
		ImageBMP currentPieceImage{ entry.path().string() };

		allImagesInFolder.push_back(currentPieceImage);
	}


	//reset path to where main cpp file is
	std::filesystem::current_path(startPath);

	return allImagesInFolder;
}

#endif
#endif

map<char, vector<vector<char>>> makeMapOfPixelLetters()
{
	map<char, vector<vector<char>>> mapOfPixelLetters;

	vector<vector<char>> matrixForC =
	{
		{'C','C','C','C','C','C','C','C',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'C','C','C','C','C','C','C','C',' ',' ',' ',' ',' ',' ',' ',' '}
	};

	mapOfPixelLetters.insert({ 'C', matrixForC });

	vector<vector<char>> matrixForD =
	{
		{ 'D','D','D','D','D','D','D',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D',' ',' ',' ',' ',' ',' ','D',' ',' ',' ',' ',' ',' ',' ',' ' },
		{ 'D','D','D','D','D','D','D',' ',' ',' ',' ',' ',' ',' ',' ',' ' }
	};

	mapOfPixelLetters.insert({ 'D', matrixForD });

	vector<vector<char>> matrixForE =
	{
		{'E','E','E','E','E','E','E','E','E','E','E','E','E','E','E','E'},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E','E','E','E','E','E','E','E','E','E','E','E','E','E','E',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'E','E','E','E','E','E','E','E','E','E','E','E','E','E','E','E'}
	};

	mapOfPixelLetters.insert({ 'E', matrixForE });

	vector<vector<char>> matrixForF =
	{
		{'F','F','F','F','F','F','F','F','F','F','F','F','F','F','F','F'},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F','F','F','F','F','F','F','F','F','F','F','F','F','F','F',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'F',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
	};

	mapOfPixelLetters.insert({ 'F', matrixForF });

	vector<vector<char>> matrixForG =
	{
		{' ',' ',' ',' ',' ','G','G','G','G','G','G','G',' ',' ',' ',' '},
		{' ',' ',' ',' ','G',' ',' ',' ',' ',' ',' ',' ','G',' ',' ',' '},
		{' ',' ',' ','G',' ',' ',' ',' ',' ',' ',' ',' ',' ','G',' ',' '},
		{' ',' ','G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G',' '},
		{' ','G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G'},
		{'G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
		{'G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G','G','G','G','G'},
		{'G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G','G','G','G','G'},
		{'G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G'},
		{'G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G'},
		{' ','G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G'},
		{' ',' ','G',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G',' '},
		{' ',' ',' ','G',' ',' ',' ',' ',' ',' ',' ',' ',' ','G',' ',' '},
		{' ',' ',' ',' ','G','G','G','G','G','G','G','G','G',' ',' ',' '},
		{' ',' ',' ',' ',' ','G','G','G','G','G','G','G',' ',' ',' ',' '}
	};

	mapOfPixelLetters.insert({ 'G', matrixForG });

	vector<vector<char>> matrixForH =
	{
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H','H','H','H','H','H','H','H','H','H','H','H','H','H','H','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'},
		{'H',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','H'}
	};

	mapOfPixelLetters.insert({ 'H', matrixForH });

	return mapOfPixelLetters;


}

map<int, vector<vector<int>>> makeMapOfPixelNumbers()
{
	map<int, vector<vector<int>>> mapOfPixelNumbers;

	vector<vector<int>> matrixFor1 =
	{
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
	};

	mapOfPixelNumbers.insert({ 1, matrixFor1 });


	vector<vector<int>> matrixFor2 =
	{
		{ 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	mapOfPixelNumbers.insert({ 2, matrixFor2 });

	vector<vector<int>> matrixFor3 =
	{
		{ 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	mapOfPixelNumbers.insert({ 3, matrixFor3 });

	vector<vector<int>> matrixFor4 =
	{
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 4, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 4, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 4, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	mapOfPixelNumbers.insert({ 4, matrixFor4 });

	vector<vector<int>> matrixFor5 =
	{
		{ 0, 0, 5, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	mapOfPixelNumbers.insert({ 5, matrixFor5 });

	vector<vector<int>> matrixFor6 =
	{
		{ 0, 0, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 6, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 6, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 6, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 6, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 6, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 6, 6, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	mapOfPixelNumbers.insert({ 6, matrixFor6 });

	vector<vector<int>> matrixFor7 =
	{
		{ 0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	mapOfPixelNumbers.insert({ 7, matrixFor7 });

	vector<vector<int>> matrixFor8 =
	{
		{ 0, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 8, 8, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 8, 8, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 8, 8, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 8, 8, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 8, 8, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 8, 8, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 8, 8, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 8, 8, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 8, 8, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 8, 8, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	mapOfPixelNumbers.insert({ 8, matrixFor8 });

	return mapOfPixelNumbers;

}

#pragma endregion
