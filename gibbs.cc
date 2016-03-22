#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>

#include "gibbs.h"

#define REP_NO 100
#define DEFAULT_HYPER_LAG 0
#define DEFAULT_SHUFFLE_LAG 100
#define DEFAULT_LEVEL_LAG -1
#define DEFAULT_SAMPLE_GAM 0
#define BUF_SIZE 100

namespace hatm {

// =======================================================================
// GibbsState
// =======================================================================

GibbsState::GibbsState()
    : score_(0.0),
      gem_score_(0.0),
      eta_score_(0.0),
      gamma_score_(0.0),
      max_score_(0.0),
      iteration_(0),
      shuffle_lag_(DEFAULT_SHUFFLE_LAG),
      hyper_lag_(DEFAULT_HYPER_LAG),
      level_lag_(DEFAULT_LEVEL_LAG),
      sample_eta_(0),
      sample_gem_(0),
      sample_gam_(DEFAULT_SAMPLE_GAM) {
}


double GibbsState::computeGibbsScore() {
  // Compute the GEM, Eta and Gamma scores.
  gem_score_ = CorpusUtils::GemScore(&corpus_);
  eta_score_ = TopicUtils::EtaScore((&tree_)->getMutableRootTopic());
  gamma_score_ = TopicUtils::GammaScore((&tree_)->getMutableRootTopic());
  score_ = gem_score_ + eta_score_ + gamma_score_;
  cout << "Gem_score: " << gem_score_ << endl;
  cout << "Eta_score: " << eta_score_ << endl;
  cout << "Gamma_score: " << gamma_score_ << endl;
  cout << "Score: " << score_ << endl;

  // Update the maximum score if necessary.
  if (score_ > max_score_ || iteration_ == 0) {
    max_score_ = score_;
  }

  return score_;
}

// =======================================================================
// GibbsUtils
// =======================================================================

void GibbsSampler::ReadGibbsInput(
    GibbsState* gibbs_state,
    const std::string& filename_corpus,
    const std::string& filename_authors,
    const std::string& filename_settings) {
  // Read hyperparameters from file
  ifstream infile(filename_settings.c_str());
  char buf[BUF_SIZE];

  int depth, sample_eta, sample_gem;
  vector<double> eta;
  double gem_mean = 0.0, gem_scale = 0.0,
				 scaling_shape = 0.0, scaling_scale = 0.0;

  while (infile.getline(buf, BUF_SIZE)) {
    istringstream s_line(buf);
    // Consider each line at a time.
    std::string str;
    getline(s_line, str, ' ');
    std::string value;
    getline(s_line, value, ' ');
    if (str.compare("DEPTH") == 0) {
      depth = atoi(value.c_str());
    } else if (str.compare("ETA") == 0) {
      do {
        double flt_value = atof(value.c_str());
        eta.push_back(flt_value);
      } while (getline(s_line, value, ' '));
    } else if (str.compare("GEM_MEAN") == 0) {
      gem_mean = atof(value.c_str());
    } else if (str.compare("GEM_SCALE") == 0) {
      gem_scale = atof(value.c_str());
    } else if (str.compare("SCALING_SHAPE") == 0) {
      scaling_shape = atof(value.c_str());
    } else if (str.compare("SCALING_SCALE") == 0) {
      scaling_scale = atof(value.c_str());
    } else if (str.compare("SAMPLE_ETA") == 0) {
      sample_eta = atoi(value.c_str());
    } else if (str.compare("SAMPLE_GEM") == 0) {
      sample_gem = atoi(value.c_str());
    }
  }

  infile.close();

  // Create corpus.
  Corpus corpus(gem_mean, gem_scale);
  CorpusUtils::ReadCorpus(filename_corpus, filename_authors, &corpus, depth);

  // Create tree of topics.
  Tree tree(depth, corpus.getWordNo(), eta, scaling_shape, scaling_scale);

  gibbs_state->setSampleEta(sample_eta);
  gibbs_state->setSampleGem(sample_gem);
  gibbs_state->setCorpus(corpus);
  gibbs_state->setTree(tree);
}

void GibbsSampler::InitGibbsState(
    GibbsState* gibbs_state) {

  Corpus* corpus = gibbs_state->getMutableCorpus();
  Tree* tree = gibbs_state->getMutableTree();
  int depth = tree->getDepth();

  // Permute Authors in the corpus.
  CorpusUtils::PermuteDocuments(corpus);

  for (int i = 0; i < corpus->getDocuments(); i++) {
    Document* document = corpus->getMutableDocument(i);
    DocumentUtils::SampleAuthors(document);
  }

  AllAuthors& all_authors = AllAuthors::GetInstance();

  for (int i = 0; i < all_authors.getAuthors(); i++) {
    Author* author = all_authors.getMutableAuthor(i);

    // Initialize the level counts to 0.
    author->initLevelCounts(depth);

    // Permute the words in the current author.
    AuthorUtils::PermuteWords(author);

    // Add a topic to the root topic of the tree.
    Topic* topic = TopicUtils::AddTopic(tree->getMutableRootTopic());

    // Increase the author count of the topic.
    topic->incAuthorNo(1);

    // Set this topic on the path at level depth - 1.
    author->setPathTopic(depth - 1, topic);
    for (int j = depth - 2; j >= 0; j--) {
      Topic* parent = author->getMutablePathTopic(j+1)->getMutableParent();
      parent->incAuthorNo(1);
      author->setPathTopic(j, parent);
    }

    // Sample levels for this author, without permuting the words
    // in the author and without removing words from levels.
    AuthorUtils::SampleLevels(author,
                                0,
                                false,
                                corpus->getGemMean(),
                                corpus->getGemScale());

    // Sample the author path starting at level 0 and removing words from
    // levels.
    if (i > 0) {
      AuthorTreeUtils::SampleAuthorPath(tree, author, true, 0);
    }

    // Sample levels for this author, and permute the words in the author.
    AuthorUtils::SampleLevels(author,
                                0,
                                true,
                                corpus->getGemMean(),
                                corpus->getGemScale());
  }

  // Compute the Gibbs score.
  double gibbs_score = gibbs_state->computeGibbsScore();

  cout << "Gibbs score = " << gibbs_score << endl;
}

GibbsState* GibbsSampler::InitGibbsStateRep(
    const std::string& filename_corpus,
    const string& filename_authors,
    const std::string& filename_settings,
    long random_seed) {
  double best_score = 0.0;
  GibbsState* best_gibbs_state = NULL;

  for (int i = 0; i < REP_NO; i++) {
    // Initialize the random number generator.
    Utils::InitRandomNumberGen(random_seed);

    GibbsState* gibbs_state = new GibbsState();
    ReadGibbsInput(gibbs_state, filename_corpus, filename_authors, filename_settings);

    // Initialize the Gibbs state.
    InitGibbsState(gibbs_state);

    // Update Gibbs best state if necessary.
    if (gibbs_state->getScore()  > best_score || i == 0) {
      if (best_gibbs_state != NULL) {
        delete best_gibbs_state;
      }
      best_gibbs_state = gibbs_state;
      best_score = gibbs_state->getScore();
      cout << "Best initial state at iteration: " <<
          i << " score " << best_score << endl;
    } else {
      delete gibbs_state;
    }
  }

  return best_gibbs_state;
}

void GibbsSampler::IterateGibbsState(GibbsState* gibbs_state) {
  assert(gibbs_state != NULL);

  Tree* tree = gibbs_state->getMutableTree();
  Corpus* corpus = gibbs_state->getMutableCorpus();
  gibbs_state->incIteration(1);
  int current_iteration = gibbs_state->getIteration();

  cout << "Start iteration..." << gibbs_state->getIteration() << endl;

  int level_lag = gibbs_state->getLevelLag();

  // Determine the level in the tree for sampling.
  int sampling_level = 0;
  if (level_lag == -1) {
    sampling_level = 0;
  } else if (current_iteration % level_lag == 0) {
    int level_inc = current_iteration / level_lag;
    sampling_level = level_inc % (tree->getDepth() - 1);
  }

  // Determine value for permute.
  int permute = 0;
  int shuffle_lag = gibbs_state->getShuffleLag();
  if (shuffle_lag > 0) {
    permute = 1 - (current_iteration % shuffle_lag);
  }

  // Permute documents in corpus.
  if (permute == 1) {
    CorpusUtils::PermuteDocuments(corpus);
  }

  for (int i = 0; i < corpus->getDocuments(); i++) {
    Document* document = corpus->getMutableDocument(i);
    DocumentUtils::SampleAuthors(document);
  }

  AllAuthors& all_authors = AllAuthors::GetInstance();

  // Sample author path and word levels.
  for (int i = 0; i < all_authors.getAuthors(); i++) {
    Author* author = all_authors.getMutableAuthor(i);
    AuthorTreeUtils::SampleAuthorPath(
        tree, author, true, sampling_level);
  }
  for (int i = 0; i < all_authors.getAuthors(); i++) {
    Author* author = all_authors.getMutableAuthor(i);
    AuthorUtils::SampleLevels(author,
                              permute,
                              true,
                              corpus->getGemMean(),
                              corpus->getGemScale());
  }

  // Sample hyper-parameters.
  if (gibbs_state->getHyperLag() > 0 &&
      (current_iteration % gibbs_state->getHyperLag() == 0)) {
    if (gibbs_state->getSampleEta() == 1) {
      TreeUtils::UpdateEta(tree);
    }
    if (gibbs_state->getSampleGem() == 1) {
      CorpusUtils::UpdateGemScale(corpus);
      CorpusUtils::UpdateGemMean(corpus);
    }
    // No gamma sampling.
  }

  // Compute the Gibbs score with the new parameter values.
  double gibbs_score = gibbs_state->computeGibbsScore();

  cout << "Gibbs score at iteration "
       << gibbs_state->getIteration() << " = " << gibbs_score << endl;
}

}  // namespace hlda


