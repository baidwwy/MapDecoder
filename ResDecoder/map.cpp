#include "pch.h"
#include "map.h"
#include "jpegint.h"
#include "jpeg.h"
#include <iostream>
#include <memory>
#include <functional>
#include <algorithm>
#include <memory>
#include <cmath>
#include <string>
#include <sstream>
#include <assert.h>
using std::cerr;
using std::endl;
using std::ios;

#define MEM_READ_WITH_OFF(off,dst,src,len) if(off+len<=src.size()){  memcpy((uint8_t*)dst,(uint8_t*)(src.data()+off),len);off+=len;   }
#define MEM_COPY_WITH_OFF(off,dst,src,len) {  memcpy(dst,src+off,len);off+=len;   }


#define BITMAPFILE_ID  0x4D42
#define BITMAPFILE_PAL_SIZE  256

#pragma pack(push) 
#pragma pack(1)  


void RGB555toRGB888(PIXEL s, unsigned char* t, int i, int j) {
	int cur = ((i * 320) + j) * 3;
	const unsigned int r = s & 0x7C00, g = s & 0x03E0, b = s & 0x1F;
	const unsigned int rgb = (r << 9) | (g << 6) | (b << 3);
	unsigned int temp = rgb | ((rgb >> 5) & 0x070707);
	t[cur] = (unsigned char)(temp >> 16) & 0xff;
	t[cur + 1] = (unsigned char)(temp >> 8) & 0xff;
	t[cur + 2] = (unsigned char)temp & 0xff;
}


MAP::MAP(std::string filename) :m_FileName(filename)
{
	std::fstream fs(m_FileName, ios::in | ios::binary);
	if (!fs) {
		std::cerr << "Map file open error!" << m_FileName << std::endl;
		return;
	}
	std::cerr << "InitMAP:" << m_FileName.c_str() << std::endl;

	auto fpos = fs.tellg();
	fs.seekg(0, std::ios::end);
	m_FileSize = fs.tellg() - fpos;

	m_FileData.resize(m_FileSize);
	fs.seekg(0, std::ios::beg);
	fs.read((char*)m_FileData.data(), m_FileSize);
	fs.close();

	uint32_t fileOffset = 0;
	MEM_READ_WITH_OFF(fileOffset, &m_Header, m_FileData, sizeof(MapHeader));
	if (m_Header.Flag == 0x4D312E30) {
		m_MapType = 2;
	}
	else if (m_Header.Flag == 0x4D415058) {
		m_MapType = 1;
	}
	else {
		cerr << "Map file format error!" << endl;
		return;
	}

	m_Width = m_Header.Width;
	m_Height = m_Header.Height;
	//	cout << "Width:" << m_Width << "\tHeight:" << m_Height << endl;

	m_BlockWidth = 320;
	m_BlockHeight = 240;

	m_ColCount = (uint32_t)std::ceil(m_Header.Width * 1.0f / m_BlockWidth);
	m_RowCount = (uint32_t)std::ceil(m_Header.Height * 1.0f / m_BlockHeight);
	//cout << "Row:" << m_RowCount << " Col:" << m_ColCount << endl;

	m_MapWidth = m_ColCount * m_BlockWidth;
	m_MapHeight = m_RowCount * m_BlockHeight;

	// m_MapPixelsRGB24 = new uint8_t[m_RowCount*m_ColCount * 320 * 240 * 3];

	// Read Unit
	m_UnitSize = m_RowCount * m_ColCount;
	m_MapUnits.resize(m_UnitSize);
	m_UnitIndecies.resize(m_UnitSize, 0);
	MEM_READ_WITH_OFF(fileOffset, m_UnitIndecies.data(), m_FileData, m_UnitSize * 4);

	if (m_MapType == 1) {  // 旧地图，读取JPEG Header
		MEM_READ_WITH_OFF(fileOffset, &m_JPEGHeaderInfo, m_FileData, sizeof(JPEGHeader));
		if (m_JPEGHeaderInfo.Flag == 0x4A504748) {
			m_JPEGHeader.resize(m_JPEGHeaderInfo.Size);
			MEM_READ_WITH_OFF(fileOffset, m_JPEGHeader.data(), m_FileData, m_JPEGHeaderInfo.Size);
		}
	}
	else if (m_MapType == 2) {  // 新地图，读取Mask索引
		// Read Mask
		MEM_READ_WITH_OFF(fileOffset, &m_MaskHeader, m_FileData, sizeof(MaskHeader));
		m_MaskSize = m_MaskHeader.Size;
		m_MaskInfos.resize(m_MaskSize);
		m_MaskIndecies.resize(m_MaskSize, 0);
		MEM_READ_WITH_OFF(fileOffset, m_MaskIndecies.data(), m_FileData, m_MaskSize * 4);

		DecodeMapMasks();
	}

	DecodeMapUnits();  // 读取JPEG基本信息和旧地图Mask索引

	std::cout << "MAP init success!" << std::endl;
}

MAP::~MAP()
{
}


// 2 bytes high bit swap 
void MAP::ByteSwap(uint16_t & value)
{
	uint16_t tempvalue = value >> 8;
	value = (value << 8) | tempvalue;
}

size_t MAP::DecompressMask(void* in, void* out)
{
	uint8_t* op;
	uint8_t* ip;
	unsigned t;
	uint8_t* m_pos;

	op = (uint8_t*)out;
	ip = (uint8_t*)in;

	if (*ip > 17) {
		t = *ip++ - 17;
		if (t < 4)
			goto match_next;
		do *op++ = *ip++; while (--t > 0);
		goto first_literal_run;
	}

	while (1) {
		t = *ip++;
		if (t >= 16) goto match;
		if (t == 0) {
			while (*ip == 0) {
				t += 255;
				ip++;
			}
			t += 15 + *ip++;
		}

		*(unsigned*)op = *(unsigned*)ip;
		op += 4; ip += 4;
		if (--t > 0)
		{
			if (t >= 4)
			{
				do {
					*(unsigned*)op = *(unsigned*)ip;
					op += 4; ip += 4; t -= 4;
				} while (t >= 4);
				if (t > 0) do *op++ = *ip++; while (--t > 0);
			}
			else do *op++ = *ip++; while (--t > 0);
		}

	first_literal_run:

		t = *ip++;
		if (t >= 16)
			goto match;

		m_pos = op - 0x0801;
		m_pos -= t >> 2;
		m_pos -= *ip++ << 2;

		*op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;

		goto match_done;

		while (1)
		{
		match:
			if (t >= 64)
			{

				m_pos = op - 1;
				m_pos -= (t >> 2) & 7;
				m_pos -= *ip++ << 3;
				t = (t >> 5) - 1;

				goto copy_match;

			}
			else if (t >= 32)
			{
				t &= 31;
				if (t == 0) {
					while (*ip == 0) {
						t += 255;
						ip++;
					}
					t += 31 + *ip++;
				}

				m_pos = op - 1;
				m_pos -= (*(unsigned short*)ip) >> 2;
				ip += 2;
			}
			else if (t >= 16) {
				m_pos = op;
				m_pos -= (t & 8) << 11;
				t &= 7;
				if (t == 0) {
					while (*ip == 0) {
						t += 255;
						ip++;
					}
					t += 7 + *ip++;
				}
				m_pos -= (*(unsigned short*)ip) >> 2;
				ip += 2;
				if (m_pos == op)
					goto eof_found;
				m_pos -= 0x4000;
			}
			else {
				m_pos = op - 1;
				m_pos -= t >> 2;
				m_pos -= *ip++ << 2;
				*op++ = *m_pos++; *op++ = *m_pos;
				goto match_done;
			}

			if (t >= 6 && (op - m_pos) >= 4) {
				*(unsigned*)op = *(unsigned*)m_pos;
				op += 4; m_pos += 4; t -= 2;
				do {
					*(unsigned*)op = *(unsigned*)m_pos;
					op += 4; m_pos += 4; t -= 4;
				} while (t >= 4);
				if (t > 0) do *op++ = *m_pos++; while (--t > 0);
			}
			else {
			copy_match:
				*op++ = *m_pos++; *op++ = *m_pos++;
				do *op++ = *m_pos++; while (--t > 0);
			}

		match_done:

			t = ip[-2] & 3;
			if (t == 0)	break;

		match_next:
			do *op++ = *ip++; while (--t > 0);
			t = *ip++;
		}
	}

eof_found:
	return (op - (uint8_t*)out);
}

void MAP::MapHandler(uint8_t * Buffer, uint32_t inSize, uint8_t * outBuffer, uint32_t * outSize)
{
	// JPEG数据处理原理
	// 1、复制D8到D9的数据到缓冲区中
	// 2、删除第3、4个字节 FFA0
	// 3、修改FFDA的长度00 09 为 00 0C
	// 4、在FFDA数据的最后添加00 3F 00
	// 5、替换FFDA到FF D9之间的FF数据为FF 00

	uint32_t TempNum = 0;						// 临时变量，表示已读取的长度
	uint16_t TempTimes = 0;					// 临时变量，表示循环的次数
	uint32_t Temp = 0;

	// 当已读取数据的长度小于总长度时继续
	while (TempNum < inSize && *Buffer++ == 0xFF)
	{
		*outBuffer++ = 0xFF;
		TempNum++;
		switch (*Buffer)
		{
		case 0xD8:
			*outBuffer++ = 0xD8;
			Buffer++;
			TempNum++;
			break;
		case 0xA0:
			Buffer++;
			outBuffer--;
			TempNum++;
			break;
		case 0xC0:
			*outBuffer++ = 0xC0;
			Buffer++;
			TempNum++;

			memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
			ByteSwap(TempTimes); // 将长度转换为Intel顺序


			for (int i = 0; i < TempTimes; i++)
			{
				*outBuffer++ = *Buffer++;
				TempNum++;
			}

			break;
		case 0xC4:
			*outBuffer++ = 0xC4;
			Buffer++;
			TempNum++;
			memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
			ByteSwap(TempTimes); // 将长度转换为Intel顺序

			for (int i = 0; i < TempTimes; i++)
			{
				*outBuffer++ = *Buffer++;
				TempNum++;
			}
			break;
		case 0xDB:
			*outBuffer++ = 0xDB;
			Buffer++;
			TempNum++;

			memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
			ByteSwap(TempTimes); // 将长度转换为Intel顺序

			for (int i = 0; i < TempTimes; i++)
			{
				*outBuffer++ = *Buffer++;
				TempNum++;
			}
			break;
		case 0xDA:
			*outBuffer++ = 0xDA;
			*outBuffer++ = 0x00;
			*outBuffer++ = 0x0C;
			Buffer++;
			TempNum++;

			memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
			ByteSwap(TempTimes); // 将长度转换为Intel顺序
			Buffer++;
			TempNum++;
			Buffer++;

			for (int i = 2; i < TempTimes; i++)
			{
				*outBuffer++ = *Buffer++;
				TempNum++;
			}
			*outBuffer++ = 0x00;
			*outBuffer++ = 0x3F;
			*outBuffer++ = 0x00;
			Temp += 1; // 这里应该是+3的，因为前面的0xFFA0没有-2，所以这里只+1。

					   // 循环处理0xFFDA到0xFFD9之间所有的0xFF替换为0xFF00
			for (; TempNum < inSize - 2;)
			{
				if (*Buffer == 0xFF)
				{
					*outBuffer++ = 0xFF;
					*outBuffer++ = 0x00;
					Buffer++;
					TempNum++;
					Temp++;
				}
				else
				{
					*outBuffer++ = *Buffer++;
					TempNum++;
				}
			}
			// 直接在这里写上了0xFFD9结束Jpeg图片.
			Temp--; // 这里多了一个字节，所以减去。
			outBuffer--;
			*outBuffer-- = 0xD9;
			break;
		case 0xD9:
			// 算法问题，这里不会被执行，但结果一样。
			*outBuffer++ = 0xD9;
			TempNum++;
			break;
		default:
			break;
		}
	}
	Temp += inSize;
	*outSize = Temp;
}


bool MAP::ReadJPEG(uint32_t offset, uint32_t size, uint32_t index)
{
	std::vector<uint8_t> jpegData(size, 0);
	MEM_READ_WITH_OFF(offset, jpegData.data(), m_FileData, size);

	m_MapUnits[index].JPEGRGB24.resize(size * 2, 0);
	uint32_t tmpSize = 0;
	
	init_jpeg();
	BMP* tmp;
	if (m_MapType == 1) {
		std::vector<uint8_t> temp(m_JPEGHeader);
		temp.insert(temp.end(), jpegData.begin(), jpegData.end());
		temp.push_back(0xff);
		temp.push_back(0xd9);
		tmp = Unpak_mapx(temp.data(), temp.size());
	}
	else {
		MapHandler(jpegData.data(), size, m_MapUnits[index].JPEGRGB24.data(), &tmpSize);
		tmp = Unpak_jpg(m_MapUnits[index].JPEGRGB24.data(), tmpSize);
	}                                    
	m_MapUnits[index].JPEGRGB24.resize(230400);
	for (int i = 0; i < 240; i += 1) {
		for (int j = 0; j < 320; j += 1) {
			RGB555toRGB888(tmp->line[i][j], m_MapUnits[index].JPEGRGB24.data(), i, j);
		}
	}
	free(tmp);
	return true;
}


void MAP::DecodeMapUnits()
{
	int maskIndex = 0;
	for (size_t i = 0; i < m_MapUnits.size(); i++)
	{
		uint32_t fileOffset = m_UnitIndecies[i];
		uint32_t eat_num;
		MEM_READ_WITH_OFF(fileOffset, &eat_num, m_FileData, sizeof(uint32_t));
		if (m_MapType == 2)
			fileOffset += eat_num * 4;
		bool loop = true;
		while (loop)
		{
			MapUnitHeader unitHeader{ 0 };
			MEM_READ_WITH_OFF(fileOffset, &unitHeader, m_FileData, sizeof(MapUnitHeader));
			switch (unitHeader.Flag)
			{
			case 0x4A504547:  // JPEG
			{
				m_MapUnits[i].JpegOffset = fileOffset;
				m_MapUnits[i].JpegSize = unitHeader.Size;
				fileOffset += unitHeader.Size;
				break;
			}
			case 0x4D415332:  // MASK
			case 0x4D41534B:  // MASK
				DecodeOldMapMask(fileOffset, i, unitHeader.Size);
				fileOffset += unitHeader.Size;
				break;
			case 0x43454C4C:  // CELL
				// ReadCELL(fileOffset, (uint32_t)unitHeader.Size, (uint32_t)i);
				fileOffset += unitHeader.Size;
				break;
			case 0x42524947:  // BRIG
				fileOffset += unitHeader.Size;
				break;
			default:
				loop = false;
				break;
			}
		}
	}
}


void MAP::ReadUnit(int index)
{
	if (m_MapUnits[index].bHasLoad || m_MapUnits[index].bLoading) {
		return;
	}
	m_MapUnits[index].bLoading = true;

	ReadJPEG(m_MapUnits[index].JpegOffset, m_MapUnits[index].JpegSize, index);
	ReadMasksByUnit(index);

	m_MapUnits[index].Index = index;
	m_MapUnits[index].bHasLoad = true;
	m_MapUnits[index].bLoading = false;
}

void MAP::DecodeOldMapMask(uint32_t offset, int unit, int size) {
	BBaseMaskInfo bbaseMaskInfo{ };
	MEM_READ_WITH_OFF(offset, &bbaseMaskInfo, m_FileData, sizeof(BBaseMaskInfo));

	MaskInfo maskInfo;

	int row = unit / m_ColCount;
	int col = unit % m_ColCount;
	maskInfo.StartX = (col * 320) + bbaseMaskInfo.StartX;
	maskInfo.StartY = (row * 240) + bbaseMaskInfo.StartY;
	maskInfo.Width = bbaseMaskInfo.Width;
	maskInfo.Height = bbaseMaskInfo.Height;
	maskInfo.Size = size - sizeof(BBaseMaskInfo);
	maskInfo.MaskOffset = offset - sizeof(BBaseMaskInfo);
	m_MapUnits[unit].Masks.push_back(maskInfo);
}


void MAP::DecodeMapMasks()
{
	for (size_t index = 0; index < m_MaskSize; index++)
	{
		uint32_t offset = m_MaskIndecies[index];

		BaseMaskInfo baseMaskInfo;//& maskInfo = m_MaskInfos[index];
		MEM_READ_WITH_OFF(offset, &baseMaskInfo, m_FileData, sizeof(BaseMaskInfo));

		MaskInfo& maskInfo = m_MaskInfos[index];
		maskInfo.StartX = baseMaskInfo.StartX;
		maskInfo.StartY = baseMaskInfo.StartY;
		maskInfo.Width = baseMaskInfo.Width;
		maskInfo.Height = baseMaskInfo.Height;
		maskInfo.Size = baseMaskInfo.Size;

		int occupyRowStart = maskInfo.StartY / m_BlockHeight;
		int occupyRowEnd = (maskInfo.StartY + maskInfo.Height) / m_BlockHeight;

		int occupyColStart = maskInfo.StartX / m_BlockWidth;
		int occupyColEnd = (maskInfo.StartX + maskInfo.Width) / m_BlockWidth;

		for (int i = occupyRowStart; i <= occupyRowEnd; i++)
			for (int j = occupyColStart; j <= occupyColEnd; j++)
			{
				int unit = i * m_ColCount + j;
				if (unit >= 0 && unit < m_MapUnits.size())
				{
					maskInfo.OccupyUnits.insert(unit);
					m_MapUnits[unit].OwnMasks.insert((int)index);
				}
			}
	}
}

bool MAP::ReadCELL(uint32_t & offset, uint32_t size, uint32_t index)
{
	m_MapUnits[index].Cell.resize(size, 0);
	MEM_READ_WITH_OFF(offset, m_MapUnits[index].Cell.data(), m_FileData, size);
	return true;
}

uint8_t* MAP::GetCell() {
	std::vector<uint8_t> whole_cell;
	for (int i = 0; i < m_UnitSize; i++) {
		whole_cell.insert(whole_cell.end(), m_MapUnits[i].Cell.begin(), m_MapUnits[i].Cell.end());
	}
	return whole_cell.data();
}

bool MAP::ReadBRIG(uint32_t & offset, uint32_t size, uint32_t index)
{
	offset += size;
	return true;
}


void MAP::ReadMasksByUnit(int index) {
	if (m_MapType == 1) {
		ReadMasksOld(index);
	}
	else {
		ReadMasksNew(index);
	}
}

int* MAP::GetMasksIndexByUnit(int index) {
	std::set<int>::iterator it;
	
	std::vector<int> indecies = {-1, -1, -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 };
	if (m_MapType == 1) {
		for (int i = 0; i < m_MapUnits[index].Masks.size(); i++) {
			indecies[i] = i;
		}
	}
	else {
		int i = 0;
		for (it = m_MapUnits[index].OwnMasks.begin(); it != m_MapUnits[index].OwnMasks.end(); it++)
		{
			indecies[i] = *it;
			i++;
		}
	}
	
	return indecies.data();
}


MAP::BaseMaskInfo* MAP::GetMaskInfo(int unit, int index) {
	if (m_MapType == 1) {
		return &m_MapUnits[unit].Masks[index];
	}
	else {
		return &m_MaskInfos[index];
	}
};

void MAP::ReadMasksNew(int unit) {
	std::set<int>::iterator it;
	for (it = m_MapUnits[unit].OwnMasks.begin(); it != m_MapUnits[unit].OwnMasks.end(); it++) {
		ReadMask(m_MaskInfos[*it], *it);
	}
}

void MAP::ReadMasksOld(int unit) {
	for (int i = 0; i < m_MapUnits[unit].Masks.size(); i++) {
		ReadMask(m_MapUnits[unit].Masks[i], i);
	}
}

void MAP::ReadMask(MAP::MaskInfo& maskInfo, int index)
{
	if (maskInfo.bHasLoad || maskInfo.bLoading) {
		return;
	}
	maskInfo.bLoading = true;

	uint32_t fileOffset = 0;
	if (m_MapType == 1) {
		fileOffset = maskInfo.MaskOffset;
		fileOffset += sizeof(BBaseMaskInfo);
	}
	else {
		fileOffset = m_MaskIndecies[index];
		fileOffset += sizeof(BaseMaskInfo);
	}

	std::vector<uint8_t> pData(maskInfo.Size, 0);
	MEM_READ_WITH_OFF(fileOffset, pData.data(), m_FileData, maskInfo.Size);

	int align_width = maskInfo.Width / 4 + (maskInfo.Width % 4 != 0);	// align 4 bytes
	int size = align_width * maskInfo.Height;
	std::vector<uint8_t> pMaskDataDec(size, 0);

	DecompressMask(pData.data(), pMaskDataDec.data());

	maskInfo.Data.resize(size * 4, 0);
	for (int i = 0; i < size; i++) {
		char byte = pMaskDataDec[i];
		maskInfo.Data[4 * i] = (byte >> 6) & 3;
		maskInfo.Data[4 * i + 1] = (byte >> 4) & 3;
		maskInfo.Data[4 * i + 2] = (byte >> 2) & 3;
		maskInfo.Data[4 * i + 3] = byte & 3;
	}
	maskInfo.bHasLoad = true;
	maskInfo.bLoading = false;
}

void MAP::PrintCellMap()
{
	int** cells;
	int** mat;
	int mat_row, mat_col;
	int row = m_RowCount;
	int col = m_ColCount;
	cells = new int* [row * col];
	for (int i = 0; i < row * col; i++) {
		cells[i] = new int[192];
	}

	mat_row = row * 12;
	mat_col = col * 16;

	//printf("%d %d \n", mat_row, mat_col);

	mat = new int* [row * 12];
	for (int i = 0; i < row * 12; i++) {
		mat[i] = new int[16 * col];
	}

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			ReadUnit(i * col + j);
			for (int k = 0; k < 192; k++) {
				cells[i * col + j][k] = (m_MapUnits)[i * col + j].Cell[k];
			}
			int startMat_i = i * 12;
			int startMat_j = j * 16;
			for (int p = 0; p < 12; p++) {
				for (int q = 0; q < 16; q++) {
					mat[startMat_i + p][startMat_j + q] = cells[i * col + j][p * 16 + q];
				}
			}
		}
	}

	for (int i = 0; i < mat_row; i++) {
		for (int j = 0; j < mat_col; j++) {
			//	printf("%d", mat[i][j]);
		}
		//printf("\n");
	}
}


uint8_t* MAP::GetMaskBitmap(int unit, int index) {
	if (m_MapType == 1) {
		return m_MapUnits[unit].Masks[index].Data.data();
	}
	else {
		return m_MaskInfos[index].Data.data();
	}
}

void MAP::EraseUnitBitmap(int index) {
	std::vector<uint8_t> v;
	m_MapUnits[index].JPEGRGB24.swap(v);
	m_MapUnits[index].bHasLoad = false;
}


void MAP::EraseMaskBitmap(int unit, int index) {
	MaskInfo& maskInfo = m_MapType == 1 ? m_MapUnits[unit].Masks[index] : m_MaskInfos[index];
	std::vector<uint8_t> v;
	v.swap(maskInfo.Data);
	maskInfo.bHasLoad = false;
}