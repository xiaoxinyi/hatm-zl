#ifndef AUTHOR_H_
#define AUTHOR_H_

#include <vector>
#include <string>
#include <unordered_map>

#include "topic.h"

namespace hatm {

// An author has an id, a author score, a topic path
// from the root of the tree to the leaf and
// statistics for words assigned to different levels in
// the tree - level counts and log probabilities for the
// levels.
// The class records map from id to author.
class Author {
public:
	Author();
	Author(int id, int depth);

	int getId() const { return id_; }

	// Set the word level counts and 
	// log probabilities for the level
	// (level_counts__ and log_pr_level_) to 0.
	void initLevelCounts(int depth);

	int getLevelCounts(int level) const { return level_counts_.at(i); }
	int getSumLevelCounts(int depth) const;
	void updateLevelCounts(int level, int value) {
		level_counts_.at(level) += value;
	}

	double getScore() const { return score_; }
	void setScore(double score) { score_ = score; }

	void setPathTopic(int level, Topic* topic) {
		path_[level] = topic;
	}
	Topic* getMutablePathTopic(int level) { return path_[level]; }

	double getLogPrLevel(int depth) const { return log_pr_level_[depth]; }
	void computeLogPrLevel(double gem_mean, double gem_scale, int depth);

	int getWodds() const { return words_.size(); }
	
	void addWord(const Word& word) { words.push_back(word); }
	void removeWord(const Word& word);
	
	Word* getMutableWord(int i) { return &words_.at(i); }
	void setWord(int i, const Word& word) { words_.at(i) = word; }

	// Get author correpond to author_id
	static Author* GetMutableAuthor(int author_id);
private:
	// Author id;
	int id_;

	// Topic path from the root of the tree to leaf.
	std::vector<Topic*> path_;

	// Depth of the tree.
	int depth_;

	std::vector<word> words;
	// Word counts.
	// std::vector<int> word_counts_;

	// Level counts.
	std::vector<int> level_counts_;

	// Log p(level) which is unnormalized.
	std::vector<double> log_pr_level_;

	// Author score.
	double score_;

	// Map id to author
	static std::unordered_map<int, Author*> id_to_author_;
};

// This class provides functionality for sampling the
// path in the tree for an author,
// for removing a path assigned to a documnet from a tree
// and updating the tree topics given a author.
class AuthorTreeUtils {
public:
	// Sample the path of an author starting from a particular level.
	// Given the level allocation, sample the path associated
	// with each author conditioned on all other paths and the oberserved words.
	static void SampleAuthorPath(
      Tree* tree,
      Author* author,
      bool remove,
      int start_level);

	// Remove a path assigned to an author from a tree,
  // given a particular start level.
  static void RemoveAuthorFromPath(
      Tree* tree,
      Author* author,
      int start_level);

  // Update the topics from a author beginning at a specified level,
  // by updating the word and author counts.
  static void UpdateTreeFromAuthor(
      Author* author,
      int update,
      int start_level);
};


// This class provides functionality for building the topic path
// for a document, and for computing path probabilities.
class AuthorTopicUtils {
 public:
  // Fill in the topic path for this document.
  static void AddPathToDocument(
      Topic* topic,
      Author* Author,
      int start_level);

  // Compute path probabilities by traversing the tree depth-first.
  static void ProbabilitiesDfs(
      Topic* topic,
      Author* Author,
      double* log_sum,
      vector<double>* path_pr,
      int start_level);

 private:
  // Log gamma ratio computation used to compute the
  // path probabilities.
  static double LogGammaRatio(
      Author* Author,
      Topic* topic,
      int level,
      double eta,
      int word_no);
};

}  // namespace hatm
#endif // AUTHOR_H_