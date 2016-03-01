#include <assert.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_sf.h>
#include <math.h>

#include "document.h"
#include "utils.h"

namespace hatm {

// =======================================================================
// Word
// =======================================================================

Word::Word(int id, int count, int author_id, int level)
		: id_(id),
		  count_(count),
		  author_id_(author_id),
		  level_(level) {
}

// =======================================================================
// Document
// =======================================================================
Document::Documnet(int id)
		: id_(id){
}

void Document::addWord(int word_id, int word_count, int author_id, int level) {
	Word word(word_id, word_count, author_id, level);
	for (int i = 0; i < word_count; i++) {
		words_.push_back(word);
	}
}

// =======================================================================
// DocumentUtils
// =======================================================================

void DocumentUtils::PermuteWords(Document* document) {
  int size = document->getWords();
  vector<Word> permuted_words;

  // Permute the values in perm.
  // These values correspond to the indices of the words in the
  // word vector of the document.
  gsl_permutation* perm = gsl_permutation_calloc(size);
  Utils::Shuffle(perm, size);
  int perm_size = perm->size;
  assert(size == perm_size);

  for (int i = 0; i < perm_size; i++) {
    permuted_words.push_back(*document->getMutableWord(perm->data[i]));
  }

  document->setWords(permuted_words);

  gsl_permutation_free(perm);
}

void DocumnetUtils::PermuteAuthors(Document* document) {
	int size = document->getAuthors();
	vector<int> permuted_author_ids;

	// Permute the values in perm.
	// These values correspond to the indices of the author_id in 
	// the author id vector of the document.
	gsl_permutation* perm = gsl_permutation_calloc(size);
	Utils::Shuffle(perm, size);
	int perm_size = perm->size;
	assert(size == perm_size);

	for (int i = 0; i < perm_size; i++) {
		permuted_author_ids.push_back(document->getAuthorId(perm->data[i]));
	}

	document->setAuthorIds(permuted_author_ids);
	gsl_permutation_free(perm);
}

void DocumentUtils::SampleAuthorId(
			Document* document,
			int permute_words,
			bool remove) {
	int size = documnet->getAuthors();
	double log_pr = log(1.0 / size);
	std::vector<double> log_pr_sample(size, log_pr);

	for (int i = 0; i < document->getWords(); i++) {
		Word* word = document->getMutableWord(i);
		// Sample author id uniformly.
		int author_id = Utils::SampleFromLogPr(log_pr_sample);
		word->setAuthorId(author_id);

		Author* author = Author::GetMutableAuthor(author_id);
		author->addWord(*word);
	}
}

void DocumentUtils::SampleLevels(
			Document* document,
      int permute_words,
      bool remove,
      double gem_mean,
      double gem_scale) {
	
	int depth = author->getMutablePathTopic(0)->getMutableTree()->getDepth();
	vector<double> log_pr(depth);

	// Permute the words in the document.
	if (permute_words == 1) {
		PermuteWords(document);
	}

	for (int i = 0; i < document->getWords(); i++) {
		Word* word = document->getMutableWord(i);
		Author* author = Author::GetMutableAuthor(author_id);
		if (remove) {
			int level = word->getLevel();
			// Update the word level.
			author->updateLevelCounts(level, -1);
			// Remove the word from author.
			author->removeWord(*word);
			// Decrease the word count.
			author->getMutablePathTopic(level)->updateWordCount(word->getId(), -1);
		}

		// Compute probabilities.
		// Compute log prbabilities for all levels.
		// Use the corpus GEM mean and scale.
		author->computeLogPrLevel(gem_mean, gem_scale, depth);

		for (int j = 0; j < depth; j++) {
			double log_pr_level = author_.getLogPrLevel(j);
			double log_pr_word = 
					author->getMutablePathTopic(j)->getLogPrWord(word->getId());

			// Keep for each level the log probability of the word +
      // log probability of the level.
      // Use these values to sample the new level.
      log_pr.at(j) = log_value;
		}

		// Sample the new level and update.
    int new_level = Utils::SampleFromLogPr(log_pr);
    author->getMutablePathTopic(new_level)->updateWordCount(word->getId(), 1);
    word->setLevel(new_level);
    author->updateLevelCounts(new_level, 1);
    author->addWord(*word);
	}
}



}  // namespace hatm