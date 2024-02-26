#pragma once

#include "definitions.h"
#include "generator.h"

bool file_to_pcm(const char* file, int stream_index, PCM_QUEUE* queue, error_callback* error);