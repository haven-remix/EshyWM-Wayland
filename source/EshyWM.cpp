
#include "EshyWM.h"
#include "Server.h"
#include "Window.h"
#include "Config.h"
#include "Util.h"

#include "EshyIPC.h"

extern "C" {
#include <wlr/util/log.h>
}

#include <nlohmann/json.hpp>

#include <fstream>

int EshybarShmID;

static std::ofstream LogFile;

static void Callback(enum wlr_log_importance importance, const char *fmt, va_list args)
{
	char* log = (char*)malloc(10000 * sizeof(char));
	vsprintf(log, fmt, args);
	LogFile << log << "\n";
	free(log);
}

int main(int argc, char* argv[])
{
	LogFile.open("eshywmlogfile.txt");
	wlr_log_init(WLR_INFO, Callback);

	EshyWMConfig::InitializeKeys();
	EshyWMConfig::ReadConfigFromFile("/home/eshy/eshywm/eshywm.conf");
	
	//Make shared memory for communication with Eshybar
	EshybarShmID = EshyIPC::MakeSharedMemoryBlock("eshybarshm", 4096);
	EshyIPC::AttachSharedMemoryBlock(EshybarShmID);
	EshyIPC::InsertIntoMemory(EshybarShmID, "");

	Server = new EshyWMServer;
	Server->BeginEventLoop();
	Server->Shutdown();

	EshyIPC::DetachSharedMemoryBlock(EshybarShmID);
	LogFile.close();
	return 0;
}
