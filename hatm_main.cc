#include <iostream>

#include "gibbs.h"

using hatm::GibbsSampler;
using hatm::GibbsState;

using std::string;

#define MAX_ITERATIONS 10000

int main(int argc, char** argv) {
  if (argc == 4) {
    // The random number generator seed.
    // For testing an example seed is: t = 1147530551;
    long rng_seed;
    (void) time(&rng_seed);

    string filename_corpus = argv[1];
    string filename_authors = argv[2];
    string filename_settings = argv[3];
    hatm::GibbsState* gibbs_state = hatm::GibbsSampler::InitGibbsStateRep(
        filename_corpus, filename_authors, filename_settings, rng_seed);

    for (int i = 0; i < MAX_ITERATIONS; i++) {
      hatm::GibbsSampler::IterateGibbsState(gibbs_state);
    }

    delete gibbs_state;
  } else {
    cout << "Arguments: "
        "(1) corpus filename "
        "(2) authors filename "
        "(3) settings filename" << endl;
  }
  return 0;
}

