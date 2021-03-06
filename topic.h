#ifndef TOPIC_H_
#define TOPIC_H_

#include <map>
#include <vector>

#include "tree.h"

using namespace std;

namespace hatm {
class Tree;

// The topic in the HATM implementation.
// Each topic contains word statistics,
// the number of authors it is assigned to,
// the topic id, the level in the tree, a scaling factor,
// pointers to the parent and children topics,
// a pointer to the tree this topic belongs to,
// and a probability for sampling the path.
class Topic {
public:
	Topic(int level, Topic* parent, Tree* tree, int corpus_word_no);
  Topic(const Topic& from) = delete;
  Topic& operator=(const Topic& from) = delete;
	~Topic();
	
	Topic(const Topic& from, Topic* parent, Tree* tree);

  const Tree& getTree() const { return *tree_; }
  Tree* getMutableTree() { return tree_; }

  int getLevel() const { return level_; }

  double getLogPrWord(int word_id) const { return log_pr_word_[word_id]; }

  int getChildren() const { return children_.size(); }
  Topic* getMutableChild(int i) { return children_.at(i); }
  void addChild(Topic* child) { children_.push_back(child); }
  void setChild(int i, Topic* child) { children_.at(i) = child; }
  void removeLastChild() { children_.pop_back(); }

	Topic* getMutableParent() { return parent_; }
  void setParent(Topic* parent) { parent_ = parent; }

  void incAuthorNo(int val) { author_no_ += val; }
  int getAuthorNo() const { return author_no_; }

  int getWordCount(int word_id) const { return word_counts_[word_id]; }

  // Update the count of a word in a given topic.
  void updateWordCount(int word_id, int update);

  double getProbability() const { return probability_; }
  void setProbability(double probability) { probability_ = probability; }

  double getScaling() const { return scaling_; }

  int getTopicWordNo() const { return topic_word_no_; }

  double getLgamWordCountEta(int word_id) const {
    return lgam_word_count_eta_[word_id];
  }

  int getCorpusWordNo() const { return corpus_word_no_; }

private:
	// Total number of words assigned to this topic.
	int topic_word_no_;

	// Total number of words in the corpus.
	int corpus_word_no_;

	// Word counts.
	vector<int> word_counts_;

	// Log probabilities for words.
	vector<double> log_pr_word_;

	// Precomputed lngamma(word_count + eta), 
	// where Eta is topic Dirichlet parameter.
	vector<double> lgam_word_count_eta_;

	// Total number of authors;
	int author_no_;

	// Id of this topic.
	int id_;

	// Level in the tree.
	int level_;

	// Scaling factor.
	double scaling_;

	// Parent topic.
	Topic* parent_;

	// Children topics.
	vector<Topic*> children_;

	// The tree which this topic belongs to.
	Tree* tree_;

	// Probability used to sample a path.
	double probability_;

};

// This class provides functionality for calculating Eta and Gamma scores,
// for adding and removing topics from the tree and sampling topics.
class TopicUtils {
 public:
  // Compute the Eta score which is a topic score.
  // The Eta parameter represents the expected variance of the
  // underlying topics.
  static double EtaScore(Topic* topic);

  // Computes the Gamma score given the topic.
  // The Gamma parameter is related to the CRP, and shows
  // if the customers in each restaurant will share tables or not.
  static double GammaScore(Topic* topic);

  // If the level of the parent topic (received as parameter) < depth
  // of the tree - 1 creates new children for the parent topic
  // and recursively calls AddTopic for the children.
  // Otherwise it returns a pointer to the the parent topic.
  static Topic* AddTopic(Topic* parent_topic);

  // Creates a new child topic and adds it to the parent topic
  // Received as parameter.
  // Returns the newly created child topic.
  static Topic* AddChildTopic(Topic* parent_topic);

  // Prunes the tree at the topic node.
  static void Prune(Topic* topic);

  // Sample topic draws a random number and calls SampleDfs.
  static Topic* SampleTopic(Topic* root, double log_sum);

 private:
  // Removes the topic node from the tree.
  static void Remove(Topic* topic);

  // Selects a topic node in the tree given the random number and the
  // topic probability. It recursively repreats the process for all the
  // topic children.
  static Topic* SampleDfs(
      Topic* topic,
      double* sum,
      double rand_no,
      double log_sum);
};

}  // namespace hatm

#endif  // TOPIC_H_