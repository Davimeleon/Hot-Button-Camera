/*Copyright (c) [2020] [Seek Thermal, Inc.]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The Software may only be used in combination with Seek cores/products.

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Project:	 Seek Thermal SDK Demo
 * Purpose:	 Demonstrates how to communicate with Seek Thermal Cameras
 * Author:	 Seek Thermal, Inc.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

 // C includes
#include <cstring>
#include <signal.h>
// C++ includes
#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <condition_variable>
#include <mutex>

// SDL includes
#if defined(__linux__) || defined(__APPLE__)
#	include <SDL2/SDL.h>
#elif defined(_WIN32)
#	define SDL_MAIN_HANDLED
#	include <SDL.h>
#endif

// Seek SDK includes
#include "seekcamera/seekcamera.h"
#include "seekcamera/seekcamera_manager.h"
#include "seekframe/seekframe.h"

//DAQ
//#include "daq/cbw.h"
//#include "daq/Counters.h"

#define MAX_FILENAME_LENGTH 64
#define FRAME_FORMAT SEEKCAMERA_FRAME_FORMAT_COLOR_RGB565
#define SDL_FRAME_FORMAT SDL_PIXELFORMAT_RGB565

//#define SAVE_CSV_FORMAT

using namespace std;

// Structure representing a renderering interface.
// It uses SDL and all rendering is done on the calling thread.
struct seekrenderer_t
{
	seekcamera_t* camera{};

	FILE* log;

	// Rendering data
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* texture{};

	// Synchronization data
	std::atomic<bool> is_active;
	std::atomic<bool> is_dirty;
	std::atomic<bool> save_data;

	seekcamera_frame_t* frame{};
};

// Define the global variables
static std::atomic<bool> g_exit_requested;                       // Controls application shutdown.
static std::map<seekcamera_t*, seekrenderer_t*> g_renderers;     // Tracks all renderers.
static std::mutex g_mutex;										 // Synchronizes camera events that need to be coordinated with main thread.
static std::condition_variable g_condition_variable;			 // Synchronizes camera events that need to be coordinated with main thread.
static std::atomic<bool> g_is_dirty;
int choice = 0;

/*int BoardNum = 0;
int ULStat = 0;
int CounterNum = 0;
ULONG previousCounter = 0;
int numCounters, defaultCtr;
ULONG Count;
float RevLevel = (float)CURRENTREVNUM;
int ctrType = CTREVENT;
char BoardName[BOARDNAMELEN];
*/
string userFileName = "";
int choiceShutter;
char* char_array;
char* directoryPath;
//C:/Seek/DAQ/DAQTest/lib

// Switches Image Processing Filter Pipeline Mode
// Filter options = 0,1,2
bool seekrenderer_switch_pipeline_mode(seekrenderer_t* renderer)
{

	if (!renderer->is_active.load())
	{
		return false;
	}

	// get value for current filter mode
	seekcamera_pipeline_mode_t current_mode;
	if (seekcamera_get_pipeline_mode(renderer->camera, &current_mode) != SEEKCAMERA_SUCCESS)
		return false;

	// Rotate through smoothing filter values and cycle back to the beginning once SEEKVISION is hit
	current_mode = (seekcamera_pipeline_mode_t)((current_mode + 1) % SEEKCAMERA_IMAGE_LASTVALUE);
	std::cout << "Filter pipeline mode: " << seekcamera_pipeline_mode_get_str(current_mode) << std::endl;
	return seekcamera_set_pipeline_mode(renderer->camera, current_mode) == SEEKCAMERA_SUCCESS;
}

// Toggles the sharpen filter state
// 0 = disable, 1 = enable
bool seekrenderer_toggle_sharpen_filter(seekrenderer_t* renderer)
{
	if (!renderer->is_active.load())
	{
		return false;
	}
	// get value for current filter mode
	seekcamera_pipeline_mode_t current_mode;
	if (seekcamera_get_pipeline_mode(renderer->camera, &current_mode) != SEEKCAMERA_SUCCESS)
	{
		std::cout << "Failed to get the filter mode" << std::endl;
		return false;
	}

	if (current_mode == SEEKCAMERA_IMAGE_SEEKVISION)
	{
		std::cout << "Cannot update the sharpen correction filter while using SeekVision mode" << std::endl;
		return false;
	}
	//Get the current sharpen correction state
	seekcamera_filter_state_t current_state;
	if (seekcamera_get_filter_state(renderer->camera, SEEKCAMERA_FILTER_SHARPEN_CORRECTION, &current_state) != SEEKCAMERA_SUCCESS)
		return false;

	// Toggle the sharpen correction state, circling around from disabled to enabled state
	current_state = (seekcamera_filter_state_t)((current_state + 1) % SEEKCAMERA_FILTER_STATE_LASTVALUE);
	std::cout << "sharpen correction: " << seekcamera_get_filter_state_str(SEEKCAMERA_FILTER_SHARPEN_CORRECTION, current_state) << std::endl;
	return seekcamera_set_filter_state(renderer->camera, SEEKCAMERA_FILTER_SHARPEN_CORRECTION, current_state) == SEEKCAMERA_SUCCESS;
}

// Switches the current color palette.
// Settings will be refreshed between frames.
bool seekrenderer_switch_color_palette(seekrenderer_t* renderer)
{
	if (!renderer->is_active.load())
	{
		return false;
	}
	seekcamera_color_palette_t current_palette;
	if (seekcamera_get_color_palette(renderer->camera, &current_palette) != SEEKCAMERA_SUCCESS)
		return false;

	// Not including the user palettes so we will cycle back to the beginning once GREEN is hit
	current_palette = (seekcamera_color_palette_t)((current_palette + 1) % SEEKCAMERA_COLOR_PALETTE_USER_0);
	std::cout << "color palette: " << seekcamera_color_palette_get_str(current_palette) << std::endl;
	return seekcamera_set_color_palette(renderer->camera, current_palette) == SEEKCAMERA_SUCCESS;
}

// Closes the SDL window associated with a renderer.
void seekrenderer_close_window(seekrenderer_t* renderer)
{
	if (renderer == NULL)
		return;
	if (renderer->is_active.load())
		seekcamera_capture_session_stop(renderer->camera);
	renderer->is_active.store(false);
	renderer->is_dirty.store(false);
	renderer->save_data.store(false);
	renderer->frame = nullptr;

	if (renderer->texture != NULL)
	{
		SDL_DestroyTexture(renderer->texture);
		renderer->texture = nullptr;
	}

	if (renderer->renderer != NULL)
	{
		SDL_DestroyRenderer(renderer->renderer);
		renderer->renderer = nullptr;
	}

	if (renderer->window != NULL)
	{
		SDL_DestroyWindow(renderer->window);
		renderer->window = nullptr;
	}
	// Invalidate references that rely on the camera lifetime.
	renderer->camera = nullptr;

	// Close the log.
	if (renderer->log != NULL)
	{
		fclose(renderer->log);
		renderer->log = NULL;
	}
}

void seekrenderer_close_all()
{
	for (auto& kvp : g_renderers)
	{
		if (kvp.second != nullptr)
		{
			seekrenderer_close_window(kvp.second);
		}
	}

}

seekrenderer_t* seekrenderer_find_by_window_id(Uint32 id)
{
	// Find the renderer associated with this window.
	for (auto& kvp : g_renderers)
	{
		auto renderer = kvp.second;
		if (renderer == nullptr || !renderer->is_active.load())
			continue;
		if (SDL_GetWindowID(renderer->window) == id)
			return renderer;
	}
	return nullptr;
}

//Logs frame data to binary format
void logtoBinary(seekrenderer_t* renderer, seekcamera_chipid_t cid, seekframe_t* frame) {
	const size_t width = seekframe_get_width(frame);
	const size_t height = seekframe_get_height(frame);

	fprintf(stdout, "frame available: %s (size: %zux%zu)\n", cid, width, height);

	// Log each header value to the CSV file.
	// See the documentation for a description of the header.
	seekcamera_frame_header_t* header = (seekcamera_frame_header_t*)seekframe_get_header(frame);

	size_t count = 0;

	//fwrite(&header->sentinel, sizeof(uint32_t), 1, renderer->log);
	//++count;

	//fwrite(&header->version, sizeof(uint8_t), 1, renderer->log);
	//++count;

	//fwrite(&header->type, sizeof(uint32_t), 1, renderer->log);
	//++count;

	fwrite(&header->width, sizeof(uint16_t), 1, renderer->log);
	++count;

	fwrite(&header->height, sizeof(uint16_t), 1, renderer->log);
	++count;

	//fwrite(&header->channels, sizeof(uint8_t), 1, renderer->log);
	//++count;

	//fwrite(&header->pixel_depth, sizeof(uint8_t), 1, renderer->log);
	//++count;

	//fwrite(&header->pixel_padding, sizeof(uint8_t), 1, renderer->log);
	//++count;

	//fwrite(&header->line_padding, sizeof(uint16_t), 1, renderer->log);
	//++count;

	//fwrite(&header->header_size, sizeof(uint16_t), 1, renderer->log);
	//++count;

	fwrite(&header->timestamp_utc_ns, sizeof(uint64_t), 1, renderer->log);
	++count;

	//fwrite(&header->chipid, sizeof(char[16]), 1, renderer->log);
	//++count;

	//fwrite(&header->serial_number, sizeof(char[16]), 1, renderer->log);
	//++count;

	//fwrite(&header->core_part_number, sizeof(char[32]), 1, renderer->log);
	//++count;

	////fwrite(renderer->log, "firmware_version=%u.%u.%u.%u,", header->firmware_version[0], header->firmware_version[1], header->firmware_version[2], header->firmware_version[3]);
	////++count;


	//fwrite(&header->io_type, sizeof(uint8_t), 1, renderer->log);
	//++count;

	//fwrite(&header->fpa_frame_count, sizeof(uint32_t), 1, renderer->log);
	//++count;

	//fwrite(&header->fpa_diode_count, sizeof(uint32_t), 1, renderer->log);
	//++count;

	//fwrite(&header->environment_temperature, sizeof(float), 1, renderer->log);
	//++count;

	//fwrite(&header->thermography_min_x, sizeof(uint16_t), 1, renderer->log);
	//++count;

	//fwrite(&header->thermography_min_y, sizeof(uint16_t), 1, renderer->log);;
	//++count;

	/*
	fwrite(&header->thermography_min_value, sizeof(float), 1, renderer->log);
	++count;

	fwrite(&header->thermography_max_x, sizeof(uint16_t), 1, renderer->log);
	++count;

	fwrite(&header->thermography_max_y, sizeof(uint16_t), 1, renderer->log);
	++count;

	fwrite(&header->thermography_max_value, sizeof(float), 1, renderer->log);
	++count;

	fwrite(&header->thermography_spot_x, sizeof(uint16_t), 1, renderer->log);
	++count;

	fwrite(&header->thermography_spot_y, sizeof(uint16_t), 1, renderer->log);
	++count;

	fwrite(&header->thermography_spot_value, sizeof(float), 1, renderer->log);
	++count;

	fwrite(&header->agc_mode, sizeof(uint8_t), 1, renderer->log);
	++count;

	fwrite(&header->histeq_agc_num_bins, sizeof(uint16_t), 1, renderer->log);
	++count;

	fwrite(&header->histeq_agc_bin_width, sizeof(uint16_t), 1, renderer->log);
	++count;

	fwrite(&header->histeq_agc_gain_limit_factor, sizeof(float), 1, renderer->log);
	++count;

	fwrite(&header->linear_agc_min, sizeof(float), 1, renderer->log);
	++count;

	fwrite(&header->linear_agc_max, sizeof(float), 1, renderer->log);
	++count;

	fwrite(&header->gradient_correction_filter_state, sizeof(uint8_t), 1, renderer->log);
	++count;

	fwrite(&header->flat_scene_correction_filter_state, sizeof(uint8_t), 1, renderer->log);
	++count; */

	/*for (size_t i = count; i < width; ++i)
	{
		fwrite(renderer->log, "blank,");
	}*/

	//fputc('\n', renderer->log);

	// Log each temperature value to the CSV file.
	// See the documentation for a description of the frame layout.
	for (size_t y = 0; y < height; ++y)
	{
		//float* pixels = (float*)seekframe_get_row(frame, y);
		//fwrite(pixels, sizeof(float), width, renderer->log);

		uint16_t* pixels = (uint16_t*)seekframe_get_row(frame, y);
		fwrite(pixels, sizeof(uint16_t), width, renderer->log);
	}

	if (choice == 3) {
		renderer->save_data.store(false);
	}
}

//Logs frame data to CSV File
void logtoCSV(seekrenderer_t* renderer, seekcamera_chipid_t cid, seekframe_t* frame) {
	const size_t width = seekframe_get_width(frame);
	const size_t height = seekframe_get_height(frame);

	fprintf(stdout, "frame available: %s (size: %zux%zu)\n", cid, width, height);

	// Log each header value to the CSV file.
	// See the documentation for a description of the header.
	seekcamera_frame_header_t* header = (seekcamera_frame_header_t*)seekframe_get_header(frame);

	size_t count = 0;

	fprintf(renderer->log, "sentinel=%u,", header->sentinel);
	++count;

	fprintf(renderer->log, "version=%u,", header->version);
	++count;

	fprintf(renderer->log, "type=%u,", header->type);
	++count;

	fprintf(renderer->log, "width=%u,", header->width);
	++count;

	fprintf(renderer->log, "height=%u,", header->height);
	++count;

	fprintf(renderer->log, "channels=%u,", header->channels);
	++count;

	fprintf(renderer->log, "pixel_depth=%u,", header->pixel_depth);
	++count;

	fprintf(renderer->log, "pixel_padding=%u,", header->pixel_padding);
	++count;

	fprintf(renderer->log, "line_padding=%u,", header->line_padding);
	++count;

	fprintf(renderer->log, "header_size=%u,", header->header_size);
	++count;

	fprintf(renderer->log, "timestamp_utc_ns=%zu,", header->timestamp_utc_ns);
	++count;

	fprintf(renderer->log, "chipid=%s,", header->chipid);
	++count;

	fprintf(renderer->log, "serial_number=%s,", header->serial_number);
	++count;

	fprintf(renderer->log, "core_part_number=%s,", header->core_part_number);
	++count;

	fprintf(renderer->log, "firmware_version=%u.%u.%u.%u,", header->firmware_version[0], header->firmware_version[1], header->firmware_version[2], header->firmware_version[3]);
	++count;

	fprintf(renderer->log, "io_type=%u,", header->io_type);
	++count;

	fprintf(renderer->log, "fpa_frame_count=%u,", header->fpa_frame_count);
	++count;

	fprintf(renderer->log, "fpa_diode_count=%u,", header->fpa_diode_count);
	++count;

	fprintf(renderer->log, "environment_temperature=%f,", header->environment_temperature);
	++count;

	fprintf(renderer->log, "thermography_min_x=%u,", header->thermography_min_x);
	++count;

	fprintf(renderer->log, "thermography_min_y=%u,", header->thermography_min_y);
	++count;

	fprintf(renderer->log, "thermography_min_value=%f,", header->thermography_min_value);
	++count;

	fprintf(renderer->log, "thermography_max_x=%u,", header->thermography_max_x);
	++count;

	fprintf(renderer->log, "thermography_max_y=%u,", header->thermography_max_y);
	++count;

	fprintf(renderer->log, "thermography_max_value=%f,", header->thermography_max_value);
	++count;

	fprintf(renderer->log, "thermography_spot_x=%u,", header->thermography_spot_x);
	++count;

	fprintf(renderer->log, "thermography_spot_y=%u,", header->thermography_spot_y);
	++count;

	fprintf(renderer->log, "thermography_spot_value=%f,", header->thermography_spot_value);
	++count;

	fprintf(renderer->log, "agc_mode=%u,", header->agc_mode);
	++count;

	fprintf(renderer->log, "histeq_agc_num_bins=%u,", header->histeq_agc_num_bins);
	++count;

	fprintf(renderer->log, "histeq_agc_bin_width=%u,", header->histeq_agc_bin_width);
	++count;

	fprintf(renderer->log, "histeq_agc_gain_limit_factor=%f,", header->histeq_agc_gain_limit_factor);
	++count;

	for (size_t i = 0; i < sizeof(header->histeq_agc_reserved) / sizeof(header->histeq_agc_reserved[0]); ++i)
	{
		fprintf(renderer->log, "histeq_agc_reserved,");
		++count;
	}

	fprintf(renderer->log, "linear_agc_min=%f,", header->linear_agc_min);
	++count;

	fprintf(renderer->log, "linear_agc_max=%f,", header->linear_agc_max);
	++count;

	for (size_t i = 0; i < sizeof(header->linear_agc_reserved) / sizeof(header->linear_agc_reserved[0]); ++i)
	{
		fprintf(renderer->log, "linear_agc_reserved,");
		++count;
	}

	fprintf(renderer->log, "gradient_correction_filter_state=%u,", header->gradient_correction_filter_state);
	++count;

	fprintf(renderer->log, "flat_scene_correction_filter_state=%u,", header->flat_scene_correction_filter_state);
	++count;

	for (size_t i = count; i < width; ++i)
	{
		fprintf(renderer->log, "blank,");
	}
	fputc('\n', renderer->log);

	// Log each temperature value to the CSV file.
	// See the documentation for a description of the frame layout.
	for (size_t y = 0; y < height; ++y)
	{
		float* pixels = (float*)seekframe_get_row(frame, y);
		for (size_t x = 0; x < width; ++x)
		{
			const float temperature_degrees_c = pixels[x];
			fprintf(renderer->log, "%.1f,", temperature_degrees_c);
		}
		fputc('\n', renderer->log);
	}
}

// Handles frame available events.
void handle_camera_frame_available(seekcamera_t* camera, seekcamera_frame_t* camera_frame, void* user_data)
{
	(void)camera;
	auto* renderer = (seekrenderer_t*)user_data;

	if (renderer->log == NULL)
	{
		fprintf(stderr, "unable to continue: log handle is invalid\n");
		return;
	}

	seekcamera_chipid_t cid;
	seekcamera_get_chipid(camera, &cid);

	//Update DAQ Counter & Check Choices
	/*ULStat = cbCIn32(BoardNum, CounterNum, &Count);

	if (choice == 2 && Count != previousCounter) {
		if (renderer->save_data.load()) {
			renderer->save_data.store(false);
		}
		else {
			renderer->save_data.store(true);
		}
	}
	else if (choice == 3 && Count != previousCounter) {
		renderer->save_data.store(true);
	}
	previousCounter = Count;*/


	seekframe_t* frame = NULL;
	/*const seekcamera_error_t status = seekcamera_frame_get_frame_by_format(
		camera_frame,
		SEEKCAMERA_FRAME_FORMAT_THERMOGRAPHY_FLOAT,
		&frame);*/

	const seekcamera_error_t status = seekcamera_frame_get_frame_by_format(
		camera_frame,
		SEEKCAMERA_FRAME_FORMAT_CORRECTED,
		&frame);


	if (status != SEEKCAMERA_SUCCESS)
	{
		fprintf(stderr, "failed to get thermal frame: %s (%s)", cid, seekcamera_error_get_str(status));
		return;
	}

	//Logs frame to CSV File
	if (renderer->save_data.load()) {
#if defined(SAVE_CSV_FORMAT)
		logtoCSV(renderer, cid, frame);
#else
		logtoBinary(renderer, cid, frame);
#endif
	}


	// Lock the seekcamera frame for safe use outside of this callback.
	seekcamera_frame_lock(camera_frame);
	renderer->is_dirty.store(true);

	// Note that this will always render the most recent frame. There could be better buffering here but this is a simple example.
	renderer->frame = camera_frame;
	g_is_dirty.store(true);
	g_condition_variable.notify_one();
}

// Handles camera connect events.
void handle_camera_connect(seekcamera_t* camera, seekcamera_error_t event_status, void* user_data)
{
	(void)event_status;
	(void)user_data;
	seekcamera_chipid_t cid{};
	seekcamera_get_chipid(camera, &cid);
	seekrenderer_t* renderer = g_renderers[camera] == nullptr ? new seekrenderer_t() : g_renderers[camera];
	renderer->is_active.store(true);
	renderer->save_data.store(false);
	renderer->camera = camera;
	renderer->log = NULL;

	//seekcamera_set_thermography_window(camera, 0, 0, 200, 200);

	if (choiceShutter == 1)
		seekcamera_set_shutter_mode(camera, SEEKCAMERA_SHUTTER_MODE_AUTO);
	else {
		seekcamera_set_shutter_mode(camera, SEEKCAMERA_SHUTTER_MODE_MANUAL);
	}

	// Register a frame available callback function.
	seekcamera_error_t status = seekcamera_register_frame_available_callback(camera, handle_camera_frame_available, (void*)renderer);
	if (status != SEEKCAMERA_SUCCESS)
	{
		std::cerr << "failed to register frame callback: " << seekcamera_error_get_str(status) << std::endl;
		renderer->is_active.store(false);
		return;
	}

	// Start the capture session.
	//status = seekcamera_capture_session_start(camera, FRAME_FORMAT | SEEKCAMERA_FRAME_FORMAT_THERMOGRAPHY_FLOAT);
	status = seekcamera_capture_session_start(camera, FRAME_FORMAT | SEEKCAMERA_FRAME_FORMAT_CORRECTED);

	if (status != SEEKCAMERA_SUCCESS)
	{
		std::cerr << "failed to start capture session: " << seekcamera_error_get_str(status) << std::endl;
		renderer->is_active.store(false);
		return;
	}
	g_renderers[camera] = renderer;

	// Create the thermography log file.
	// Each log file is associated with a camera by its unique chip id.
	// Log files will be overwritten if the camera is repeatedly connected and disconnected -- or if the application is repeatedly launched.
	char filename[MAX_FILENAME_LENGTH] = { 0 };

	time_t rawtime;
	struct tm* timeinfo;
	char buffer[20];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 20, "%F_%H-%M-%S", timeinfo);

#if defined(SAVE_CSV_FORMAT)
	snprintf(filename, MAX_FILENAME_LENGTH, "%s/%s-%s.bin", directoryPath, char_array, buffer);
	renderer->log = fopen(filename, "w");
#else
	snprintf(filename, MAX_FILENAME_LENGTH, "%s/%s-%s.bin", directoryPath, char_array, buffer);
	renderer->log = fopen(filename, "wb");
#endif
	cout << filename << endl;


	if (renderer->log != NULL)
	{
		fprintf(stdout, "opened log file: %s (%s)\n", cid, filename);
	}
	else
	{
		fprintf(stderr, "failed to open log file: %s\n", cid);
	}
}

// Handles camera disconnect events.
void handle_camera_disconnect(seekcamera_t* camera, seekcamera_error_t event_status, void* user_data)
{
	(void)event_status;
	(void)user_data;
	auto renderer = g_renderers[camera];
	if (renderer != nullptr)
	{
		renderer->is_active.store(false);
		renderer->save_data.store(false);
		g_is_dirty.store(true);
	}


}

// Handles camera error events.
void handle_camera_error(seekcamera_t* camera, seekcamera_error_t event_status, void* user_data)
{
	(void)user_data;
	seekcamera_chipid_t cid{};
	seekcamera_get_chipid(camera, &cid);
	std::cerr << "unhandled camera error: (CID: " << cid << ")" << seekcamera_error_get_str(event_status) << std::endl;
}

// Handles camera ready to pair events
void handle_camera_ready_to_pair(seekcamera_t* camera, seekcamera_error_t event_status, void* user_data)
{
	// Attempt to pair the camera automatically.
	// Pairing refers to the process by which the sensor is associated with the host and the embedded processor.
	const seekcamera_error_t status = seekcamera_store_calibration_data(camera, nullptr, nullptr, nullptr);
	if (status != SEEKCAMERA_SUCCESS)
	{
		std::cerr << "failed to pair device: " << seekcamera_error_get_str(status) << std::endl;
	}

	// Start imaging.
	handle_camera_connect(camera, event_status, user_data);
}

// Callback function for the camera manager; it fires whenever a camera event occurs.
void camera_event_callback(seekcamera_t* camera, seekcamera_manager_event_t event, seekcamera_error_t event_status, void* user_data)
{
	seekcamera_chipid_t cid{};
	seekcamera_get_chipid(camera, &cid);
	std::cout << seekcamera_manager_get_event_str(event) << " (CID: " << cid << ")" << std::endl;

	// Handle the event type.
	switch (event)
	{
	case SEEKCAMERA_MANAGER_EVENT_CONNECT:
		handle_camera_connect(camera, event_status, user_data);
		break;
	case SEEKCAMERA_MANAGER_EVENT_DISCONNECT:
		handle_camera_disconnect(camera, event_status, user_data);
		break;
	case SEEKCAMERA_MANAGER_EVENT_ERROR:
		handle_camera_error(camera, event_status, user_data);
		break;
	case SEEKCAMERA_MANAGER_EVENT_READY_TO_PAIR:
		handle_camera_ready_to_pair(camera, event_status, user_data);
		break;
	default:
		break;
	}
}

// Application entry point.
int main()
{
	//Var Declarations
	string tempPath;

	//DAQ Device Setup
	/* Declare UL Revision Level */
	//ULStat = cbDeclareRevision(&RevLevel);

	//InitUL();	// Set up error handling

	// get the name of the board
	//if (!GetNameOfBoard(BoardNum, BoardName)) {
		//cerr << "failed to connect to DAQ" << endl;
		//return 0;
	//}

	// Determine if device is compatible with this example
	//numCounters = FindCountersOfType(BoardNum, ctrType, &defaultCtr);
	/*if (numCounters == 0) {
		//some scan counters can also work with this example
		ctrType = CTRSCAN;
		numCounters = FindCountersOfType(BoardNum, ctrType, &defaultCtr);
		if (numCounters == 0) {
			printf("%s (board %u) does not have Event counters.\n", BoardName, BoardNum);
			cerr << "No errors" << endl;
			return 0;
		}
	}

	CounterNum = defaultCtr;
	ULStat = cbCClear(BoardNum, CounterNum);*/

	//DAQ Init Done

	//Set Window Size


	// Initialize global variables.
	g_exit_requested.store(false);
	g_is_dirty.store(false);

	//Explain Program to User/Intro
	std::cout << "seekcamera-sdl starting\n";
	std::cout << "user controls:\n\t1) mouse click: next color palette\n\t2) q: quit" << std::endl << endl;

	cout << "What do you want to name the file? ";
	cin >> userFileName;
	cout << endl;
	char_array = new char[userFileName.length() + 1];
	strcpy(char_array, userFileName.c_str());

	cout << "Do you want to use automatic or no shuttering? " << endl;
	cout << "Press 1 for Automatic, 2 for No: ";
	cin >> choiceShutter;
	cout << endl;

	cout << "Where do you want to save the file? " << endl;
	cout << "For default path press 1, else input file path(forward slashes): ";
	cin >> tempPath;
	if (tempPath != "1") {
		directoryPath = new char[tempPath.length() + 1];
		strcpy(directoryPath, tempPath.c_str());
	}
	else {
		tempPath = "C:/Seek/seekcamera_capture/src/build/Debug";
		directoryPath = new char[tempPath.length() + 1];
		strcpy(directoryPath, tempPath.c_str());
	}

	cout << "3 User Modes" << endl;
	cout << "1) Keyboard mode: Press t to activate and stop saving the frame data" << endl;
	cout << "2) Analog mode: Activate an analog signal to activate and stop saving the frame data" << endl;
	cout << "3) Shutter mode: For each analog signal, camera will save a frame" << endl;
	cout << "Enter which mode you want: ";
	cin >> choice;
	cout << endl << endl;

	// Initialize SDL and enable bilinear stretching.
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	std::cout << "display driver: " << SDL_GetVideoDriver(0) << std::endl;

	// Create the camera manager.
	// This is the structure that owns all Seek camera devices.
	seekcamera_manager_t* manager = nullptr;
	seekcamera_error_t status = seekcamera_manager_create(&manager, SEEKCAMERA_IO_TYPE_USB);
	if (status != SEEKCAMERA_SUCCESS)
	{
		std::cerr << "failed to create camera manager: " << seekcamera_error_get_str(status) << std::endl;
		return 1;
	}

	// Register an event handler for the camera manager to be called whenever a camera event occurs.
	status = seekcamera_manager_register_event_callback(manager, camera_event_callback, nullptr);
	if (status != SEEKCAMERA_SUCCESS)
	{
		std::cerr << "failed to register camera event callback: " << seekcamera_error_get_str(status) << std::endl;
		return 1;
	}

	// Poll for events until told to stop.
	// Both renderer events and SDL events are polled.
	// Events are polled on the main thread because SDL events must be handled on the main thread.
	while (!g_exit_requested.load())
	{
		std::unique_lock<std::mutex> event_lock(g_mutex);
		if (g_condition_variable.wait_for(event_lock, std::chrono::milliseconds(150), [=] { return g_is_dirty.load(); }))
		{
			g_is_dirty.store(false);
			for (auto& kvp : g_renderers)
			{
				auto renderer = kvp.second;
				if (renderer == NULL)
					continue;

				// Close inactive windows
				if (!renderer->is_active.load())
				{
					seekrenderer_close_window(renderer);
					continue;
				}


				// Create window if not created yet
				if (renderer->is_active.load() && renderer->window == NULL && renderer->renderer == NULL)
				{
					// Set the window title.
					seekcamera_chipid_t cid{};
					seekcamera_get_chipid(renderer->camera, &cid);
					std::stringstream window_title;
					window_title << "Seek Thermal - SDL Sample (CID: " << cid << ")";

					// Setup the window handle.
					SDL_Window* window = SDL_CreateWindow(window_title.str().c_str(), 100, 100, 0, 0, SDL_WINDOW_HIDDEN);
#if SDL_VERSION_ATLEAST(2, 0, 5)
					SDL_SetWindowResizable(window, SDL_TRUE);
#endif
					// Setup the window renderer.
					SDL_Renderer* window_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

					// Setup the renderer area.
					renderer->window = window;
					renderer->renderer = window_renderer;
				}

				// Render frame if necessary
				if (renderer->is_dirty.load())
				{
					if (renderer->frame == NULL || !renderer->is_active.load())
						continue;

					// Get the frame to draw.
					seekframe_t* frame = nullptr;
					status = seekcamera_frame_get_frame_by_format(renderer->frame, FRAME_FORMAT, &frame);
					if (status != SEEKCAMERA_SUCCESS)
					{
						std::cerr << "failed to get frame: " << seekcamera_error_get_str(status) << std::endl;
						seekcamera_frame_unlock(renderer->frame);
						continue;
					}

					// Get the frame dimensions.
					const int frame_width = (int)seekframe_get_width(frame);
					const int frame_height = (int)seekframe_get_height(frame);
					const int frame_stride = (int)seekframe_get_line_stride(frame);

					// Lazy allocate the texture data.
					if (renderer->texture == nullptr)
					{
						// Resize and show the window -- upscaling by two.
						const int scale_factor = 2;
						SDL_RenderSetLogicalSize(renderer->renderer, frame_width * scale_factor, frame_height * scale_factor);
						renderer->texture = SDL_CreateTexture(renderer->renderer, SDL_FRAME_FORMAT, SDL_TEXTUREACCESS_TARGET, frame_width, frame_height);
						SDL_SetWindowSize(renderer->window, frame_width * scale_factor, frame_height * scale_factor);
						SDL_ShowWindow(renderer->window);
					}

					// Update the SDL windows and renderers.
					SDL_UpdateTexture(renderer->texture, nullptr, seekframe_get_data(frame), frame_stride);
					SDL_RenderCopy(renderer->renderer, renderer->texture, nullptr, nullptr);
					SDL_RenderPresent(renderer->renderer);

					// Unlock the camera frame.
					seekcamera_frame_unlock(renderer->frame);
					renderer->is_dirty.store(false);
					renderer->frame = nullptr;
				}
			}
		}
		else
		{
			event_lock.unlock();
		}

		// Handle the SDL window events.
		// The events need to be polled in order for the window to be responsive.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			seekrenderer_t* renderer = seekrenderer_find_by_window_id(event.window.windowID);

			if (renderer == nullptr)
			{
				continue;
			}

			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
			{
				seekrenderer_close_window(renderer);
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				seekrenderer_switch_color_palette(renderer);
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_n)
			{
				seekrenderer_switch_pipeline_mode(renderer);
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s)
			{
				seekrenderer_toggle_sharpen_filter(renderer);
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_t && choice == 1) {
				if (renderer->save_data.load()) {
					renderer->save_data.store(false);
				}
				else {
					renderer->save_data.store(true);
				}
			}
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q)
			{
				g_exit_requested.store(true);
			}
		}
	}

	std::cout << "Destroying camera manager" << std::endl;

	// Teardown the camera manager.
	seekcamera_manager_destroy(&manager);

	seekrenderer_close_all();
	g_renderers.clear();

	// Teardown SDL.
	SDL_Quit();
	std::cout << "done" << std::endl;
	return 0;
}