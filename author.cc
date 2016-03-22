#include <assert.h>
#include <math.h>
#include <algorithm>
#include <gsl/gsl_sf.h>

#include "utils.h"
#include "author.h"
#include "topic.h"

namespace hatm {

// =======================================================================
// Author
// =======================================================================

Author::Author() {	
}

Author::Author(int id, int depth) 
		: id_(id),
		  depth_(depth) {
	initLevelCounts(depth);
}

void Author::removeWord(int word) {
	auto found = find(begin(words_), end(words_), word);
	if (found == end(words_)) {

	} else {
		words_.erase(found);
	}
}

void Author::initLevelCounts(int depth) {
	level_counts_ = vector<int>(depth, 0);
	log_pr_level_ = vector<double>(depth, 0.0);
}

int Author::getSumLevelCounts(int depth) const {
	int sum = 0;
	for (int i = 0; i < depth; i++) {
		sum += level_counts_[i];
	}
	return sum;
}

void Author::computeLogPrLevel(double gem_mean, 
															 double gem_scale,
															 int depth) {
	int sum_level_counts = getSumLevelCounts(depth);
	double sum_log_pr = 0.0;
	double last_section = 0.0;

	for (int i = 0; i < depth - 1; i++) {
		sum_level_counts -= level_counts_[i];

		double expected_stick_len = 
				((1 - gem_mean) * gem_scale + level_counts_[i]) / 
				(gem_scale + level_counts_[i] + sum_level_counts);

		log_pr_level_[i] = log(expected_stick_len) + sum_log_pr;

		if (i == 0) {
			last_section = log_pr_level_[i];
		} else {
			last_section = Utils::LogSum(log_pr_level_[i], last_section);
		}
		sum_log_pr += log(1.0 - expected_stick_len);
	}

	last_section = log(1.0 - exp(last_section));
	log_pr_level_[depth - 1] = last_section;
}



// =======================================================================
// AllAuthors
// =======================================================================

AllAuthors& AllAuthors::GetInstance() {
	static AllAuthors instance;
	return instance;
}



// =======================================================================
// AuthorUtils
// =======================================================================

void AuthorUtils::PermuteWords(Author* author) {
  int size = author->getWords();
  vector<int> permuted_words;

  // Permute the values in perm.
  // These values correspond to the indices of the words in the
  // word vector of the author.
  gsl_permutation* perm = gsl_permutation_calloc(size);
  Utils::Shuffle(perm, size);
  int perm_size = perm->size;
  assert(size == perm_size);

  for (int i = 0; i < perm_size; i++) {
    permuted_words.push_back(author->getWord(perm->data[i]));
  }

  author->setWords(move(permuted_words));

  gsl_permutation_free(perm);
}



void AuthorUtils::SampleLevels(
			Author* author,
      int permute_words,
      bool remove,
      double gem_mean,
      double gem_scale) {
	int depth = author->getMutablePathTopic(0)->getMutableTree()->getDepth();
	vector<double> log_pr(depth);

	// Permute the words in the author.
	if (permute_words == 1) {
		PermuteWords(author);
	}

	AllWords& all_words = AllWords::GetInstance();

	for (int i = 0; i < author->getWords(); i++) {
		int word_idx = author->getWord(i);
		Word* word = all_words.getMutableWord(word_idx);

		if (remove) {
			int level = word->getLevel();
			if (level != -1) {
				// Update the word level.
				author->updateLevelCounts(level, -1);
			
				// Decrease the word count.
				author->getMutablePathTopic(level)->updateWordCount(word->getId(), -1);
			}
		}

		// Compute probabilities.
		// Compute log prbabilities for all levels.
		// Use the corpus GEM mean and scale.
		author->computeLogPrLevel(gem_mean, gem_scale, depth);

		for (int j = 0; j < depth; j++) {
			double log_pr_level = author->getLogPrLevel(j);
			double log_pr_word = 
					author->getMutablePathTopic(j)->getLogPrWord(word->getId());

			double log_value = log_pr_level + log_pr_word;
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
 	}
}

// =======================================================================
// AuthorTreeUtils
// =======================================================================

void AuthorTreeUtils::RemoveAuthorFromPath(
      Tree* tree,
      Author* author,
      int start_level) {
	UpdateTreeFromAuthor(author, -1, start_level);
	Topic* topic = author->getMutablePathTopic(tree->getDepth() - 1);
	TopicUtils::Prune(topic);
}

void AuthorTreeUtils::UpdateTreeFromAuthor(
      Author* author,
      int update,
      int start_level) {
	// The depth of the tree.
	int depth = author->getMutablePathTopic(0)->getMutableTree()->getDepth();

	AllWords& all_words = AllWords::GetInstance();

	// Update the word count of the topic for all the words in the author.
	for (int i = 0; i < author->getWords(); i++) {
		int word_idx = author->getWord(i);
		Word* word = all_words.getMutableWord(word_idx);
		int level = word->getLevel();
		if (level > start_level) {
			Topic* topic = author->getMutablePathTopic(level);
			topic->updateWordCount(word->getId(), update);
		}
	}

	// Update the author count for the topic in the path.
	for (int i = start_level + 1; i < depth; i++) {
		Topic* topic = author->getMutablePathTopic(i);
		topic->incAuthorNo(update);
	}

}


// =======================================================================
// AuthorTopicUtils
// =======================================================================

void AuthorTopicUtils::AddPathToAuthor(
      Topic* topic,
      Author* author,
      int start_level) {
	int depth = topic->getMutableTree()->getDepth();
  int level = depth - 1;

  // Set the path for this author.
  do {
    author->setPathTopic(level, topic);
    topic = topic->getMutableParent();
    level--;
  } while (level >= start_level);

  // Update the topics from the author.
  AuthorTreeUtils::UpdateTreeFromAuthor(author, 1, start_level);

}

void AuthorTopicUtils::ProbabilitiesDfs(
      Topic* topic,
      Author* author,	
      double* log_sum,
      vector<double>* path_pr,
      int start_level) {
	int level = topic->getLevel();
	int depth = topic->getMutableTree()->getDepth();

	double eta = topic->getMutableTree()->getEta(level);
	int term_no = topic->getCorpusWordNo();

	// Set path probability for current toipic node in the tree.
	path_pr->at(level) = LogGammaRatio(author, topic, level, eta, term_no);

	double parent_log_val = 0.0;

	if (level > start_level) {
		parent_log_val = log(topic->getMutableParent()->getAuthorNo() + 
			topic->getMutableParent()->getScaling());
		path_pr->at(level) += log(topic->getAuthorNo() - parent_log_val);
	}

	// Set path probabilities for level below this topic.
	if (level < depth - 1) {
    for (int i = level + 1; i < depth; i++) {
      eta = topic->getMutableTree()->getEta(i);
      path_pr->at(i) = LogGammaRatio(author, NULL, i, eta, term_no);
    }

    path_pr->at(level+1) += log(topic->getScaling());
    path_pr->at(level+1) -= log(topic->getAuthorNo() + topic->getScaling());
  }

  // Set probability for the topic.
  double probability = 0.0;
  for (int i = start_level; i < depth; i++) {
    probability += path_pr->at(i);
  }
  topic->setProbability(probability);

  // Update the normalizing constant.
  if (level == start_level) {
    *log_sum = probability;
  } else {
    *log_sum = Utils::LogSum(*log_sum, probability);
  }

  // Recursive call for the children.
  for (int i = 0; i < topic->getChildren(); i++) {
    ProbabilitiesDfs(
        topic->getMutableChild(i), author, log_sum, path_pr, start_level);
  }
}

double AuthorTopicUtils::LogGammaRatio(
      Author* author,
      Topic* topic,
      int level,
      double eta,
      int term_no) {
	std::vector<int> count(term_no, 0);
	AllWords& all_words = AllWords::GetInstance();

	for (int i = 0; i < author->getWords(); i++) {
		int word_idx = author->getWord(i);
		Word* word = all_words.getMutableWord(word_idx);
		if (word->getLevel() == level) {
			count[word->getId()]++;
		}
	}

	int word_no = 0;

	// Topic can be NULL, in which case the result doesn't include the
  // Word count for the topic.
  if (topic != NULL) {
    word_no = topic->getTopicWordNo();
  }


  double result = gsl_sf_lngamma(word_no + term_no * eta); 
  double value = word_no + author->getLevelCounts(level) + term_no * eta; 
  result -= gsl_sf_lngamma(value);

  for (int i = 0; i < author->getWords(); i++) {
  	int word_idx = author->getWord(i);
  	int word_id = all_words.getMutableWord(word_idx)->getId();
    if (count[word_id] > 0) {
      int word_count = 0;
      if (topic != NULL) {
        word_count = topic->getWordCount(word_id);
      }
      result -= gsl_sf_lngamma(word_count + eta);
      result += gsl_sf_lngamma(word_count + count[word_id] + eta);
      count[word_id] = 0;
    } 
  }

  return result;
}

}  // namespace hatm