#include "pch.h"
#include "res_decoder.h"


ResDecoder::ResDecoder() {
	std::cerr << "*********Init Res Decoder*********" << std::endl;
}

ResDecoder::~ResDecoder() {

}

void ResDecoder::AddTask(Task task) {
	TaskQueue.push(task);
}

bool ResDecoder::HasMap(std::string filename) {
	std::unordered_map<std::string, MAP>::iterator it;
	it = MapPool.find(filename);
	if (it != MapPool.end())
		return true;
	else
		return false;
}

void ResDecoder::AddMap(std::string filename) {
	if (HasMap(filename)) {
		return;
	}
	MAP map = MAP(filename);
	MapPool.insert(std::pair<std::string, MAP>(filename, map));
}


MAP* ResDecoder::GetMap(std::string filename) {
	if (!HasMap(filename)) {
		AddMap(filename);
	}
	return &MapPool.at(filename);
}

bool ResDecoder::HasTask() {
	return TaskQueue.size() > 0;
}

ResDecoder::Task ResDecoder::GetTask() {
	Task task = TaskQueue.front();
	TaskQueue.pop();
	return task;
}

bool ResDecoder::HasUnitLoaded(std::string filename, int index) {
	MAP* map = GetMap(filename);
	return map->HasUnitLoad(index);
}

uint8_t* ResDecoder::GetUnitBitmap(std::string filename, int index) {
	MAP* map = GetMap(filename);
	return map->GetUnitBitmap(index);
}

int* ResDecoder::GetMasksIndexByUnit(std::string filename, int index) {
	MAP* map = GetMap(filename);
	return map->GetMasksIndexByUnit(index);
}

MAP::BaseMaskInfo* ResDecoder::GetMaskInfo(std::string filename, int unit, int index) {
	MAP* map = GetMap(filename);
	return map->GetMaskInfo(unit, index);
}

void ResDecoder::EraseUnitBitmap(std::string filename, int index) {
	MAP* map = GetMap(filename);
	map->EraseUnitBitmap(index);
}

uint8_t* ResDecoder::GetMaskBitmap(std::string filename, int unit, int index) {
	MAP* map = GetMap(filename);
	return map->GetMaskBitmap(unit, index);
}

void ResDecoder::EraseMaskBitmap(std::string filename, int unit, int index) {
	MAP* map = GetMap(filename);
	map->EraseMaskBitmap(unit, index);
}

void ResDecoder::Loop() {
	running = true;
	while (running) {
		if (HasTask()) {
			Task task = GetTask();
			MAP* map = GetMap(task.filename);
			map->ReadUnit(task.index);
		}
		else {
			Sleep(100);
		}
	}
}

void ResDecoder::EndLoop() {
	running = false;
}

uint8_t* ResDecoder::GetMapCell(std::string filename) {
	MAP* map = GetMap(filename);
	return map->GetCell();
}

void ResDecoder::DropMap(std::string filename) {
	if (HasMap(filename)) {
		MapPool.erase(filename);
	}
}