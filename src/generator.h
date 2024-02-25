#pragma once

#if defined(_MSC_VER)
  //  Microsoft 
  #define EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
  //  GCC
  #define EXPORT __attribute__((visibility("default")))
#else
  #define EXPORT
  #pragma warning Unknown dynamic link export semantics.
#endif

#ifdef __cplusplus
extern "C" {
#endif

  EXPORT const char *version();

  EXPORT void init();

  struct cut
  {
    double start;
    double end;
  };

  struct cut_list
  {
    long num_cuts;
    cut* cuts;
  };

  struct generator_stats
  {
    double len_pre_cuts;
    double len_post_cuts;
  };

  struct result
  {
    cut_list cuts;
    generator_stats stats;
  };

  result error_result = {
    { 0, nullptr },
    { 0, 0 }
  };

  typedef void progress_callback(const char*, double);

  struct ArgumentResult {
    const char* name;
    const char* value;
  };

  struct ArgumentResultList {
    long num_args;
    ArgumentResult* args;
  };

  // takes the given file and converts it to pcm audio
  // the audio is then processed by webrtcvad to
  // determine the speech segments
  EXPORT result generate(
    const char *file,
    ArgumentResultList* args,
    progress_callback* progress
  );

  struct Argument {
    const char short_name;
    const char* long_name;
    const char* description;
    bool required;
    bool is_flag;
  };

  struct ArgumentList {
    long num_args;
    Argument* args;
  };

  EXPORT ArgumentList get_arguments();

#ifdef __cplusplus
}
#endif