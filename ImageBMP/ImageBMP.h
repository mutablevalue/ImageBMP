#pragma once

#include<algorithm>
#include<array>
#include<cassert>
#include<filesystem> 
#include<fstream> 
#include<iomanip> 
#include<iostream>
#include<map> 
#include<string>
#include<unordered_map>
#include <vector>


#ifdef __cplusplus
#if __cplusplus >= 201703L
using std::cout, std::ifstream, std::ofstream, std::vector, std::string, std::ios, std::ostream, std::array;
using std::swap, std::map, std::pair, std::setw, std::left, std::to_string;
using std::unordered_map;
#endif
#endif

using std::map; 
using std::vector; 
using std::array; 
using std::ofstream; 
using std::ifstream; 
using std::string; 

class FileHeader
{
	/*will make PRIVATE all of the bmp fields that (probably) never change
	- ex: 1st two bytes should always be B and M*/

	array<char, 2> filetype = { 'B', 'M' }; //indices 0 and 1  
	//const char* filetype = "BM"; //indices 0 and 1  -> problematic type!

	unsigned int reserved1And2 = 0x00'00'00'00; //(unclear meaning) indices 6 - 9 
	unsigned int indexOfPixelData = 0x00'00'00'36; //indices 10 - 13

public:
	unsigned int fileSize{}; //indices 2 - 5 (size is in bytes)
	//should perhaps be set by the ImageBMP class //(since ImageBMP will have info on pixel data and infoheader) 

	FileHeader() = default;

	friend class ImageBMP;

};

class InfoHeader
{
	unsigned int infoHeaderSize = 0x00'00'00'28; //indices 14 - 17, in bytes
	short planes = 0x00'01; //indices 26 - 27 ["always" 1 (meaning unclear)] 

	short bitsPerPixel = 0x00'20; //CAREFUL! not always 32! -> indices 28 - 29 (32 bits - 24 for G,B,R, and 8 for Alpha)
	unsigned int compressionMethod = 0x00'00'00'00; //indices 30 - 33
	unsigned int sizeOfPixelData{};// = imageWidth * imageHeight * (bitsPerPixel / 8); //indices 34 - 37
	std::vector<int> remainingHeaderFields =
	{
		0x00'00'00'00,//xPixelsPerMeter
		0x00'00'00'00,//yPixelsPerMeter
		0x00'00'00'00,//color indexing thing
		0x00'00'00'00//"important" color count 
	};

public:
	unsigned int imageWidth = 0; //indices 18 - 21
	unsigned int imageHeight = 0; //indices 22 - 25

	InfoHeader() = default;

	unsigned int getInfoHeaderSize() const;
	unsigned int getSizeOfPixelData() const;

	friend class ImageBMP;
};

/*NOTE: little-endian BGRA order is used here*/
enum class ColorEnum : unsigned int
{
	//A, R, G, B
	Black = 0xFF'00'00'00,
	White = 0xFF'FF'FF'FF,

	Red = 0xFF'FF'00'00,
	Green = 0xFF'00'FF'00,
	Blue = 0xFF'00'00'FF,

	Yellow = 0xFF'FF'FF'00,
	Cyan = 0xFF'00'FF'FF,
	Magenta = 0xFF'FF'00'FF,

	//chessboard-specific colors: 
	DarkSquareColor = 0xFF'BA'61'34,
	LightSquareColor = 0xFF'EF'D7'B5,
	BoardBorder = 0xFF'6C'1E'1C,

	//using some images from this page: https://commons.wikimedia.org/wiki/Category:Chess_bitmap_pieces
	WKnightBgrdColor = 0xFF'FF'FF'CB,

	RedBgrd = 0xFF'ED'1C'24
	//CopyWKnightBgrdColor = 0x00'00'00'00

};

struct Color
{
	//should be unsigned because 1) no "negative" colors and 2) having alpha = 255 (FF) is desirable
	unsigned int bgra = 0x00'00'00'00;

	Color() = default;
	Color(unsigned int bgra);
	Color(unsigned int b, unsigned int g, unsigned int r); // New constructor for 24-bit color
	Color(unsigned int b, unsigned int g, unsigned int r, unsigned int a);
	Color(ColorEnum colorEnum);

	unsigned int convertToUnsignedInt();
};

class PixelData
{
public:
	std::vector<vector<Color>> pixelMatrix;

	PixelData() = default;
};

class ImageBMP
{
	/*made private, I suppose, to prevent overwhelming client with large number of functions*/
	void readFileHeaderFromFile(ifstream& fin);
	void readInfoHeaderFromFile(ifstream& fin);
	void readPixelDataFromFile(ifstream& fin);
public:
	FileHeader fileHeader;
	InfoHeader infoHeader;
	PixelData pixelData;

	ImageBMP() = default;

	ImageBMP(unsigned int imageWidth, unsigned int imageHeight, const Color& fillColor, const Color& middleDotColor);

	ImageBMP(unsigned int imageWidth, unsigned int imageHeight, const Color& fillColor);

	ImageBMP(const string& filepath);

	void readImageBMP(string inputFilename);

	void doublescaleImageBMP();

	void drawRectangleOutline(unsigned int x0, unsigned int y0,
		unsigned int rectangleWidth, unsigned int rectangleHeight, const Color& color);

	void fillRectangleWithColor(unsigned int x0, unsigned int y0,
		unsigned int rectangleWidth, unsigned int rectangleHeight, const Color& color);

	void setPixelToColor_withThickness(unsigned int x, unsigned int y, const Color& color, unsigned int thickness);

	/*NOTE! this function is intentionally left empty*/
	void drawAndFillAnIrregularShape();

	void writeImageFile(std::string filename);


};






#pragma region auxillary functions 
//"auxillary" method: 
vector<vector<char>> rotateMatrixClockwise
(vector<vector<char>>& originalMatrix, int originalNumberOfRows, int originalNumberOfCols);

vector<vector<int>> rotateIntMatrixClockwise(vector<vector<int>>& originalMatrix, int originalNumberOfRows, int originalNumberOfCols);

/*NOTE: this function requires C++17!
And caution: potentially returning "large" amount of data*/
vector<ImageBMP> getAllImagesInFolder(string folderName);


//for pixelated letters (for labeling chessboard A1, C3, etc.)
map<char, vector<vector<char>>> makeMapOfPixelLetters();

map<int, vector<vector<int>>> makeMapOfPixelNumbers();

#pragma endregion