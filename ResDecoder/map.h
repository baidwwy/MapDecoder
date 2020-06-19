#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>


class MAP
{
public:
	struct MapHeader
	{
		uint32_t		Flag;
		uint32_t		Width;
		uint32_t		Height;
	};

	struct MapUnitHeader
	{
		uint32_t		Flag;
		uint32_t		Size;
	};

	struct MaskHeader
	{
		uint32_t	Flag;
		int	Size;
		MaskHeader()
			:Flag(0),
			Size(0)
		{

		}
	};

	struct JPEGHeader {
		uint32_t MapSize;
		uint32_t Flag;
		int	Size;
		JPEGHeader()
			:MapSize(0),
			Flag(0),
			Size(0)
		{

		}
	};

	struct BBaseMaskInfo
	{
		int	StartX;
		int	StartY;
		uint32_t	Width;
		uint32_t	Height;
		BBaseMaskInfo()
			:StartX(0),
			StartY(0),
			Width(0),
			Height(0)
		{

		}
	};

	struct BaseMaskInfo: BBaseMaskInfo
	{
		uint32_t	Size;
		BaseMaskInfo()
			:Size(0)
		{

		}
	};

	struct MaskInfo : BaseMaskInfo
	{
		std::vector<uint8_t> Data;
		std::set<int> OccupyUnits;
		uint32_t MaskOffset;
		bool bHasLoad = false;
		bool bLoading = false;
	};

	struct MapUnit
	{
		std::vector<uint8_t>  Cell;
		std::vector<uint8_t> JPEGRGB24;
		std::vector<MaskInfo> Masks;
		uint32_t Size;
		uint32_t Index;
		bool bHasLoad = false;
		bool bLoading = false;
		uint32_t JpegSize;
		uint32_t JpegOffset;
		std::set<int> OwnMasks;
	};

	MAP(std::string filename);

	~MAP();

	void DecodeMapUnits();
	void DecodeMapMasks();
	void DecodeOldMapMask(uint32_t offset, int index, int size);

	void ReadUnit(int index);

	void ReadUnit(int row, int col) { ReadUnit(row * m_ColCount + col); };

	void ReadMasksNew(int index);

	void ReadMasksOld(int index);

	void ReadMasksByUnit(int index);

	void PrintCellMap();

	int MapWidth() { return m_MapWidth; };
	int MapHeight() { return m_MapHeight; };
	int SliceWidth() { return m_Width; };
	int SliceHeight() { return m_Height; };
	int Row() { return m_RowCount; };
	int Col() { return m_ColCount; };
	int UnitSize() { return m_UnitSize; };
	int MaskSize() { return m_MaskSize; };

	int GetMaskWidth(int index) { return m_MaskInfos[index].Width; };
	int GetMaskHeight(int index) { return m_MaskInfos[index].Height; };

	int* GetMasksIndexByUnit(int index);
	BaseMaskInfo* GetMaskInfo(int unit, int index);
	MapUnit& GetUnit(int index) { return m_MapUnits[index]; };
	bool HasUnitLoad(int index) { return m_MapUnits[index].bHasLoad; };
	bool IsUnitLoading(int index) { return m_MapUnits[index].bLoading; };

	uint8_t* GetCell();
	
	uint8_t* GetUnitBitmap(int index) { return m_MapUnits[index].JPEGRGB24.data(); };
	size_t GetUnitBitmapSize(int index) { return m_MapUnits[index].JPEGRGB24.size(); };

	uint8_t* GetMaskBitmap(int unit, int index);

	void EraseUnitBitmap(int index);

	void EraseMaskBitmap(int unit, int index);
private:

	void ByteSwap(uint16_t& value);

	size_t DecompressMask(void* in, void* out);

	void MapHandler(uint8_t* Buffer, uint32_t inSize, uint8_t* outBuffer, uint32_t* outSize);

	bool ReadJPEG(uint32_t offset, uint32_t size, uint32_t index);

	bool ReadCELL(uint32_t& offset, uint32_t size, uint32_t index);

	bool ReadBRIG(uint32_t& offset, uint32_t size, uint32_t index);

	void ReadMask(MaskInfo& maskInfo, int index);

	std::string m_FileName;

	int m_MapType;

	int m_Width;

	int m_Height;

	int m_MapWidth;

	int m_MapHeight;

	int m_BlockWidth;

	int m_BlockHeight;

	std::unordered_map<int, int> m_MaskIndex;

	std::vector<uint8_t> m_JPEGHeader;

	JPEGHeader m_JPEGHeaderInfo;

	uint32_t m_RowCount;

	uint32_t m_ColCount;

	MapHeader m_Header;

	std::vector<uint32_t> m_UnitIndecies;

	uint32_t m_UnitSize;

	MaskHeader m_MaskHeader;

	std::vector<uint32_t> m_MaskIndecies;

	uint32_t m_MaskSize;

	std::vector<MapUnit> m_MapUnits;

	std::vector<MaskInfo> m_MaskInfos;

	std::vector<uint8_t> m_FileData;

	std::uint64_t m_FileSize;

	std::vector<std::vector<uint8_t>> m_CellData;
};