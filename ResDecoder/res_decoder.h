#pragma once
#include "pch.h"
#include "map.h"
#include <queue>
#include <unordered_map>
#include <string>
#include <vector> 
#include<iostream>


class ResDecoder {
public:
	struct Task {
		std::string filename;
		int index;
		Task(char* filename, int index)
			:filename(filename),
			index(index)
		{

		}
	};

	std::queue<Task> TaskQueue;

	std::unordered_map<std::string, MAP> MapPool;

	ResDecoder();

	~ResDecoder();

	void AddTask(Task task);

	bool HasMap(std::string filename);

	bool HasUnitLoaded(std::string filename, int index);

	uint8_t* GetMapCell(std::string filename);

	uint8_t* GetUnitBitmap(std::string filename, int index);

	int* GetMasksIndexByUnit(std::string filename, int index);

	uint8_t* GetMaskBitmap(std::string filename, int unit, int index);

	MAP::BaseMaskInfo* GetMaskInfo(std::string filename, int unit, int index);

	void EraseUnitBitmap(std::string filename, int index);

	void EraseMaskBitmap(std::string filename, int unit, int index);

	void Loop();

	void EndLoop();

	void DropMap(std::string filename);

private:
	bool running = false;

	MAP* GetMap(std::string filename);

	void AddMap(std::string filename);

	bool HasTask();

	Task GetTask();
};