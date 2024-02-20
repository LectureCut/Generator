// local
#include "generator.h"
#include "definitions.h"
#include "uuid.h"
#include "pcm.h"

// 3rdparty
#include "fvad.h"

// std
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <thread>

#ifdef _WIN32
#include "Windows.h"
#endif

const char* version()
{
  return VERSION;
}

void init() {
}

result generate(
  const char *file,
  int aggressiveness,
  bool invert,
  progress_callback* progress
)
{
  // ================
  // INPUT VALIDATION
  // ================

  // check if file exists
  if (!std::filesystem::exists(file)) {
    std::cout << "File does not exist" << std::endl;
    return error_result;
  }

  // check if aggressiveness is valid
  if (aggressiveness < 0 && aggressiveness > 3) {
    std::cout << "Aggressiveness must be between 0 and 3" << std::endl;
    return error_result;
  }

  // ============
  // MEDIA -> PCM
  // ============

  PCM_QUEUE pcm_queue;

  std::thread pcm_thread(file_to_pcm, file, &pcm_queue);

  // =============
  // PCM -> SPEECH
  // =============

  Fvad *vad = fvad_new();
  
  fvad_set_mode(vad, aggressiveness);
  fvad_set_sample_rate(vad, 16000);

  std::vector<cut> cuts;
  cut current_cut;
  current_cut.start = 0;
  current_cut.end = 0;

  int16_t buffer[160];
  int vad_result;

  double total_video_length = 0;

  // for (int i = 0; i < audio_data.size(); i += 160) {
  while (pcm_queue.pop(buffer, 160) == 160) {
    // memcpy(buffer, audio_data.data() + i, sizeof(int16_t) * 160);

    vad_result = fvad_process(vad, buffer, 160);

    if (vad_result == 1) {
      if (current_cut.start == 0) {
        current_cut.start = current_cut.end;
      }
    } else {
      if (current_cut.start != 0) {
        cuts.push_back(current_cut);
        current_cut.start = 0;
      }
    }

    current_cut.end += 0.01;
    total_video_length += 0.01;
  }

  pcm_thread.join();

  if (current_cut.start != 0) {
    cuts.push_back(current_cut);
  }

  fvad_free(vad);

  // ======
  // INVERT
  // ======

  if (invert) {
    // create a list of cuts that are the inverse of the cuts
    std::vector<cut> inverse_cuts;
    cut inverse_cut;
    inverse_cut.start = 0;
    inverse_cut.end = 0;
    for (size_t i = 0; i < cuts.size(); i++) {
      inverse_cut.end = cuts[i].start;
      inverse_cuts.push_back(inverse_cut);
      inverse_cut.start = cuts[i].end;
    }
    inverse_cut.end = total_video_length;
    inverse_cuts.push_back(inverse_cut);

    cuts = inverse_cuts;
  }

  // ===========
  // FILTER CUTS
  // ===========
  
  // join cuts that are less than .2 seconds apart
  std::vector<cut> filtered_cuts;
  for (int i = 0; i < cuts.size(); i++) {
    if (i == cuts.size() - 1) {
      filtered_cuts.push_back(cuts[i]);
    } else if (cuts[i + 1].start - cuts[i].end < 0.2) {
      cuts[i + 1].start = cuts[i].start;
    } else {
      filtered_cuts.push_back(cuts[i]);
    }
  }
  cuts = filtered_cuts;

  // remove cuts that are shorter than .2 seconds
  cuts.erase(std::remove_if(cuts.begin(), cuts.end(), [](const cut &c) {
    return c.end - c.start < 0.2;
  }), cuts.end());

  // return the cuts
  cut* result_cuts = (cut*) malloc(sizeof(cut) * cuts.size());
  for (size_t i = 0; i < cuts.size(); i++) {
    result_cuts[i] = cuts[i];
  }

  cut_list cutlist = { (long) cuts.size(), result_cuts };

  double total_cut_length = 0;
  #pragma omp parallel for reduction(+:total_cut_length)
  for (int i = 0; i < cuts.size(); i++) {
    total_cut_length += cuts[i].end - cuts[i].start;
  }

  result result = { cutlist, {total_video_length, total_cut_length} };

  return result;
}