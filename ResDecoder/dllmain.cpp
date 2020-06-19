// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "map.h"
#include "res_decoder.h"
#include<iostream>
#define EXPORT __declspec(dllexport)

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


ResDecoder res_decoder = ResDecoder();


extern "C" {
    EXPORT uint8_t* load(char* filename, int index) {
        MAP map = MAP(filename);
        map.ReadUnit(index);
        return map.GetUnitBitmap(index);
    }

    EXPORT void loop() {
        res_decoder.Loop();
    }

    EXPORT void end_loop() {
        res_decoder.EndLoop();
    }

    EXPORT bool has_map_loaded(char* filename) {
        return res_decoder.HasMap(filename);
    }

    EXPORT uint8_t* get_map_cell(char* filename) {
        return res_decoder.GetMapCell(filename);
    }

    EXPORT void add_task(char* filename, int index) {
        ResDecoder::Task task = ResDecoder::Task(filename, index);
        res_decoder.AddTask(task);
    }

    EXPORT bool has_unit_loaded(char* filename, int index) {
        return res_decoder.HasUnitLoaded(filename, index);
    }

    EXPORT uint8_t* get_unit_bitmap(char* filename, int index) {
        return res_decoder.GetUnitBitmap(filename, index);
    }

    EXPORT int* get_masks_index_by_unit(char* filename, int index) {
        return res_decoder.GetMasksIndexByUnit(filename, index);
    }

    EXPORT MAP::BaseMaskInfo* get_mask_info(char* filename, int unit, int index) {
        return res_decoder.GetMaskInfo(filename, unit, index);
    }

    EXPORT uint8_t* get_mask_data(char* filename, int unit, int index) {
        return res_decoder.GetMaskBitmap(filename, unit, index);
    }

    EXPORT void erase_unit_bitmap(char* filename, int index) {
        res_decoder.EraseUnitBitmap(filename, index);
    }

    EXPORT void erase_mask_bitmap(char* filename, int unit, int index) {
        res_decoder.EraseMaskBitmap(filename, unit, index);
    }

    EXPORT void drop_map(char* filename) {
        res_decoder.DropMap(filename);
    }
}