
#include <assert.h>
#include <gsl/gsl_permutation.h>
#include <math.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "corpus.h"
#include "topic.h"
#include "author.h"

#define REP_NO_GEM 100
#define GEM_STDEV 0.05
#define GEM_MEAN_STDEV 0.05
#define BUF_SIZE 10000

namespace hatm {

// =======================================================================
// Corpus
// =======================================================================

Corpus::Corpus()
    : gem_mean_(0.0),
      gem_scale_(0.0),
      word_no_(0),
      author_no_(0) {
}

Corpus::Corpus(double gem_mean, double gem_scale)
    : gem_mean_(gem_mean),
      gem_scale_(gem_scale),
      word_no_(0),
      author_no_(0) {
}



// =======================================================================
// CorpusUtils
// =======================================================================

void CorpusUtils::ReadCorpus(
    const std::string& docs_filename,
    const std::string& authors_filename,
    Corpus* corpus,
    int depth) {

  ifstream infile(docs_filename.c_str());
  char buf[BUF_SIZE];

  ifstream authors_infile(authors_filename.c_str());
  char authors_buf[BUF_SIZE];

  int author_no = 0;
  int doc_no = 0;
  int word_no = 0;
  int total_word_count = 0;
  int words;

  AllWords& all_words = AllWords::GetInstance();

  while (infile.getline(buf, BUF_SIZE) && 
  			 authors_infile.getline(authors_buf, BUF_SIZE)) {
  	
  	istringstream s_line_author(authors_buf);
    std::vector<int> author_ids;
  	while (s_line_author.getline(authors_buf, BUF_SIZE, ' ')) {
  		int author_id = atoi(authors_buf);
  		if (author_id >= author_no) {
  			author_no = author_id + 1;
  		}
  		author_ids.push_back(author_id);
  	}

  	if (author_ids.empty()) {
  		continue;
  	}

    istringstream s_line(buf);
    // Consider each line at a time.
    int word_count_pos = 0;
    Document document(doc_no);
    // Set author ids
    document.setAuthorIds(author_ids);
    while (s_line.getline(buf, BUF_SIZE, ' ')) {
      if (word_count_pos == 0) {
        words = atoi(buf);
      } else {
        int word_id, word_count;
        istringstream s_word_count(buf);
        string str;
        getline(s_word_count, str, ':');
        word_id = atoi(str.c_str());
        getline(s_word_count, str, ':');
        word_count = atoi(str.c_str());
        total_word_count += word_count;
       
        for (int i = 0; i < word_count; i++) {
          all_words.addWord(word_id);
          document.addWord(all_words.getWordNo() - 1);
        }
       
        if (word_id >= word_no) {
          word_no = word_id + 1;
        }
      }
      word_count_pos++;
    }
    corpus->addDocument(move(document));
    doc_no += 1;
  }

  infile.close();
  authors_infile.close();

  AllAuthors& all_authors = AllAuthors::GetInstance();
  for (int i = 0; i < author_no; i++) {
    all_authors.addAuthor(i, depth);
  }

  corpus->setWordNo(word_no);
  corpus->setAuthorNo(author_no);

  cout << "Number of documents in corpus: " << doc_no << endl;
  cout << "Number of authors in corpus: " << author_no << endl;
  cout << "Number of distinct words in corpus: " << word_no << endl;
  cout << "Number of words in corpus: " << total_word_count << " = " 
       << all_words.getWordNo() << endl;
}

double CorpusUtils::GemScore(
    Corpus* corpus) {
  double score = 0.0;

  AllAuthors& all_authors = AllAuthors::GetInstance();

  // Get depth of the tree.
  // Look at the topic in the topic path of the document.
  int depth =
      all_authors.getMutableAuthor(0)->
      getMutablePathTopic(0)->getMutableTree()->getDepth();

  // GEM distribution priors.
  double prior_a = (1 - corpus->getGemMean()) * corpus->getGemScale();
  double prior_b = corpus->getGemMean() * corpus->getGemScale();

  for (int i = 0; i < all_authors.getAuthors(); i++) {
    Author* author = all_authors.getMutableAuthor(i);
    assert(author != NULL);

    double author_score = 0.0;

    // Get an aggregated level count composed of all the level counts
    // up to the current level.
    vector<double> agreg_level_count(depth);
    for (int j = 0; j < depth; j++) {
      agreg_level_count[j] = 0.0;
      double count = author->getLevelCounts(j);
      for (int k = 0; k < j; k++) {
        agreg_level_count[k] += count;
      }
    }
    double sum_log_prob = 0.0;

    // Sum up all the level counts.
    double sum_levels = author->getSumLevelCounts(depth);
    double last_log_prob = 0.0;
    for (int j = 0; j < depth - 1; j++) {
      double a = author->getLevelCounts(j) + prior_a;
      double b = agreg_level_count[j] + prior_b;

      author_score += lgamma(a) + lgamma(b) - lgamma(a + b) -
          lgamma(prior_b) - lgamma(prior_a) +
          lgamma(prior_a + prior_b);

      sum_levels -= author->getLevelCounts(j);

      double expected_stick_len = (prior_a + author->getLevelCounts(j)) /
          (corpus->getGemScale() + author->getLevelCounts(j) + sum_levels);

      double log_prob = log(expected_stick_len) + sum_log_prob;

      if (j == 0) {
        last_log_prob = log_prob;
      } else {
        last_log_prob += Utils::LogSum(log_prob, last_log_prob);
      }

      sum_log_prob += log(1 - expected_stick_len);
    }

    last_log_prob = log(1 - exp(last_log_prob));

    // The bottom levels are conditionally independent.
    author_score += author->getLevelCounts(depth - 1) * last_log_prob;
    score += author_score;
    author->setScore(author_score);
  }
  // ???
  score += -corpus->getGemScale();
  return score;
}

void CorpusUtils::UpdateGemScale(Corpus* corpus) {
  double current_gem_score = GemScore(corpus);

  int score_change = 0;

  for (int i = 0; i < REP_NO_GEM; i++) {
    double old_gem_scale = corpus->getGemScale();

    // A new GEM scale parameter based on Gaussian random variates.
    double new_gem_scale = Utils::RandGauss(old_gem_scale, GEM_STDEV);

    // Decide if to keep the new GEM scale value.
    if (new_gem_scale > 0) {
      corpus->setGemScale(new_gem_scale);
      double new_gem_score = GemScore(corpus);
      double rand = Utils::RandNo();
      if (rand > exp(new_gem_score - current_gem_score)) {
        corpus->setGemScale(old_gem_scale);
      } else {
        current_gem_score = new_gem_score;
        score_change++;
      }
    }
  }
  cout << "Gem scale: (1) score_change: " << score_change <<
        " (2) new_gem_scale: " << corpus->getGemScale() << endl;
}

void CorpusUtils::UpdateGemMean(Corpus* corpus) {
  double current_gem_score = GemScore(corpus);

  int score_change = 0;

  for (int i = 0; i < REP_NO_GEM; i++) {
    double old_gem_mean = corpus->getGemMean();

    // A new GEM mean parameter based on Gaussian random variates.
    double new_gem_mean = Utils::RandGauss(old_gem_mean, GEM_MEAN_STDEV);

    // Decide if to keep the new GEM mean value.
    if (new_gem_mean > 0 && new_gem_mean < 1) {
      corpus->setGemMean(new_gem_mean);
      double new_gem_score = GemScore(corpus);
      double rand = Utils::RandNo();
      if (rand > exp(new_gem_score - current_gem_score)) {
        corpus->setGemMean(old_gem_mean);
      } else {
        current_gem_score = new_gem_score;
        score_change++;
      }
    }
  }
  cout << "Gem mean: (1) score_change: " << score_change <<
      " (2) new_gem_mean: " << corpus->getGemMean() << endl;
}

void CorpusUtils::PermuteDocuments(Corpus* corpus) {
  int size = corpus->getDocuments();
  vector<Document> permuted_documents;

  // Permute the values in perm.
  // These values correspond to the indices of the documents in the
  // document vector of the corpus.
  gsl_permutation* perm = gsl_permutation_calloc(size);
  Utils::Shuffle(perm, size);
  int perm_size = perm->size;
  assert(size == perm_size);

  for (int i = 0; i < perm_size; i++) {
    Document* document = corpus->getMutableDocument(perm->data[i]);
    permuted_documents.emplace_back(move(*document));
  }

  corpus->setDocuments(move(permuted_documents));

  gsl_permutation_free(perm);
}

}  // namespace hatm

