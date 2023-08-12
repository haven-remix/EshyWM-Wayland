
#pragma once

#include <string>

struct eipcSharedMemory
{
    char* Block;
    int Size;
};

namespace EshyIPC
{
int MakeSharedMemoryBlock(const std::string& Filename, int Size);
eipcSharedMemory& AttachSharedMemoryBlock(int id);
void DetachSharedMemoryBlock(int id);
void DestroySharedMemoryBlock(int id);

void InsertIntoMemory(int id, const std::string& Data);
void InsertIntoMemoryUnprotected(int id, const std::string& Data);
}