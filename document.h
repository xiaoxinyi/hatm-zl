#ifndef DOCUMENT_H_
#define DOCUMENT_H_

#include <string>
#include <vector>

#include "author.h"

namespace hatm {

// A word having an id, a word count, author id,
// and the level in the tree the word is assigned to.
class Word {
public:
	Word(int id, int count, int author_id, int level);

	void setLevel(int level) { level = level_; }
	int getLevel() const { return level_; }
	void updateLevel(int value) const { level_ += value; }

	void setId(int id) { id_ = id; }
	int getId() const { return id_; }

	void setAuthorId(int author_id) { author_id_ = author_id; }
	int getAuthorId() const { return author_id_; }

	bool operator==(const Word& word) {
		return (word.id_ == id_) &&
					 (word.count_ == count_) &&
					 (word.author_id_ == author_id) &&
					 (word.level_ == level_);
	}

private:
	// Word id.
	int id_;

	// Word count in the document.
	int count_;

	// Author id the word is assigned to.
	int author_id_;

	// Each word has assigned a level in the tree.
	int level_;
};

// The document containing a number of words and authors.
// A document has an id.
class Document {
public:
	Documnet(int id);
	int getWodds() const { return words_.size(); }
	int getAuthors() const { return author_ids_.size(); }

	void addWord(int word_id, int word_count, int author_id, int level);
	Word* getMutableWord(int i) { return &words_.at(i); }
	void setWord(int i, const Word& word) { words_.at(i) = word; }

	int getAuthorId(int i) const { return author_ids_.at(i); }
	void addAuthorId(const int author_id) { author_ids_.push_back(author_id); } 

	void setAuthorIds(const std::vector<int>& author_ids) { author_ids_ = author_ids; }
private:
	// Document id.
	int id_;

	// The words in the documnet
	std::vector<word> words_;

	// Author ids of the document.
	std::vector<int> author_ids_;
};

// The class provides functionality for permuting words
// and author ids.
class DocumentUtils {
public:
	// Permute the words in a document.
	static void PermuteWords(Document* document);

	// Permute author ids in a document.
	static void PermuteAuthors(Document* document);

	// Sample author id
	static void SampleAuthorId(Document* document);

	// Sample the word levels for a given author,
  // for the current path assignments of the author.
  // Sampling can be with (permute = 1) or without (permute != 1)
  // permuting the words in the document.
  // Words can or cannot be removed from levels in the tree (set/unset
  // the bool remove variable).
  // The GEM distribution mean and scale parameters determined at corpus
  // level are provided as input.
	static void SampleLevels(
			Document* document,
      int permute_words,
      bool remove,
      double gem_mean,
      double gem_scale)
};

}  // namespace hatm