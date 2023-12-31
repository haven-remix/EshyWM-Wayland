
#include "Output.h"
#include "Server.h"
#include "EshyWM.h"

#include "EshyIPC.h"

#define static

extern "C"
{
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
}

#undef static

#include <string>
#include <iostream>

static eipcSharedMemory SharedMemory;
static std::string CurrentShm;

void OutputFrame(struct wl_listener* listener, void* data)
{
	//This function is called every time an output is ready to display a frame, generally at the output's refresh rate (e.g. 60Hz).
	class EshyWMOutput* output = wl_container_of(listener, output, FrameListener);
	struct wlr_scene* scene = Server->Scene;

	struct wlr_scene_output* scene_output = wlr_scene_get_scene_output(scene, output->WlrOutput);

	//Only runs when shared memory changes
	SharedMemory = EshyIPC::AttachSharedMemoryBlock(EshybarShmID);
	if(CurrentShm != std::string(SharedMemory.Block))
	{
		CurrentShm = std::string(SharedMemory.Block);
		SharedMemoryUpdated(CurrentShm);
	}

	//Render the scene if needed and commit the output
	wlr_scene_output_commit(scene_output, nullptr);

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

void OutputRequestState(struct wl_listener* listener, void* data)
{
	/*This function is called when the backend requests a new state for
	*  the output. For example, Wayland and X11 backends request a new mode
	*  when the output window is resized.*/
	class EshyWMOutput* output = wl_container_of(listener, output, RequestStateListener);
	const struct wlr_output_event_request_state* event = (wlr_output_event_request_state*)data;
	wlr_output_commit_state(output->WlrOutput, event->state);
}

void OutputDestroy(struct wl_listener* listener, void* data)
{
	class EshyWMOutput* output = wl_container_of(listener, output, DestroyListener);

	wl_list_remove(&output->FrameListener.link);
	wl_list_remove(&output->RequestStateListener.link);
	wl_list_remove(&output->DestroyListener.link);
	free(output);
}