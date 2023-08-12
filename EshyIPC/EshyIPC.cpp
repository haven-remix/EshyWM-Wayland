
#include "EshyIPC.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <map>

static std::map<int, eipcSharedMemory> SharedMemories;

namespace EshyIPC
{
int MakeSharedMemoryBlock(const std::string& Filename, int Size)
{
    remove(Filename.c_str());
    FILE* file = fopen(Filename.c_str(), "w");
    fclose(file);

    key_t key = ftok(Filename.c_str(), SharedMemories.size());

    eipcSharedMemory SharedMemory = {nullptr, Size};
    int id = shmget(key, Size, 0644 | IPC_CREAT);
    SharedMemories.emplace(id, SharedMemory);
    return id;
}

eipcSharedMemory& AttachSharedMemoryBlock(int id)
{
    SharedMemories[id].Block = (char*)shmat(id, NULL, 0);
    return SharedMemories[id];
}

void DetachSharedMemoryBlock(int id)
{
    shmdt(SharedMemories[id].Block);
}

void DestroySharedMemoryBlock(int id)
{
    shmctl(id, IPC_RMID, NULL);
    SharedMemories.erase(id);
}

void InsertIntoMemory(int id, const std::string& Data)
{
    strncpy(SharedMemories[id].Block, Data.c_str(), SharedMemories[id].Size);
}

void InsertIntoMemoryUnprotected(int id, const std::string& Data)
{
    strcpy(SharedMemories[id].Block, Data.c_str());
}
}