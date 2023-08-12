
#include "EshyWM.h"
#include "Server.h"
#include "Window.h"
#include "Util.h"

#include "EshyIPC.h"

#include <wayland-client-core.h>

#include <nlohmann/json.hpp>

int EshybarShmID;

static char* DetermineStartupCommand(int argc, char* argv[])
{
	char* StartupCmd = nullptr;

	int c;
	while ((c = getopt(argc, argv, "s:h")) != -1)
	{
		switch (c)
		{
		case 's':
			StartupCmd = optarg;
			break;
		default:
			printf("Usage: %s [-s startup command]\n", argv[0]);
			return 0;
		}
	}

	return StartupCmd;
}

int main(int argc, char* argv[])
{
	wlr_log_init(WLR_SILENT, NULL);
	
	//Make shared memory for communication with Eshybar
	EshybarShmID = EshyIPC::MakeSharedMemoryBlock("eshybarshm", 4096);
	EshyIPC::AttachSharedMemoryBlock(EshybarShmID);
	EshyIPC::InsertIntoMemory(EshybarShmID, "");

	Server = new EshyWMServer;
	Server->Initialize();
	Server->BeginEventLoop(DetermineStartupCommand(argc, argv));
	Server->Shutdown();
	
	EshyIPC::DetachSharedMemoryBlock(EshybarShmID);
	return 0;
}
