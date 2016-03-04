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

void DocumentUtils::SampleAuthors(
			Document* document,
			bool remove) {
	int size = documnet->getAuthors();
	double log_pr = log(1.0 / size);
	std::vector<double> log_pr_sample(size, log_pr);
	AllAuthors& all_authors = AllAuthors::GetInstance();

	for (int i = 0; i < document->getWords(); i++) {
		Word* word = document->getMutableWord(i);

		if (remove) {
			Author* author = all_authors.getMutableAuthor(word->getAuthorId());
			author->removeWord(*word);
		}
		// Sample author id uniformly.
		int author_id = Utils::SampleFromLogPr(log_pr_sample);
		word->setAuthorId(author_id);

		Author* author = all_authors.getMutableAuthor(author_id);
		author->addWord(*word);
	}
}



}  // namespace hatm