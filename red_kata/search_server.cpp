#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <unordered_map>

vector<string> SplitIntoWords(const string& line) {
  istringstream words_input(line);
  return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

SearchServer::SearchServer(istream& document_input) {
  UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {
  InvertedIndex new_index;

  for (string current_document; getline(document_input, current_document); ) {
    new_index.Add(move(current_document));
  }

  swap(index, new_index);
}

void SearchServer::AddQueriesStream(
  istream& query_input, ostream& search_results_output
) {
  for (string current_query; getline(query_input, current_query); ) {
    const auto words = SplitIntoWords(current_query);
    size_t mysize = max(size_t(6), index.get_docs_size());
    vector<size_t> docid_count(mysize, 0);
    for (const auto& word : words) {
      if (index.is_word_in_Lookup(word)){
        for (auto it : index.Lookup(word)) {
          docid_count[it.first] += it.second;
        }
      }
    }

    vector<pair<size_t, size_t>> search_results;
    for (size_t i = 0; i < docid_count.size(); i++){
      search_results.push_back(make_pair(i, docid_count[i]));
    }
    partial_sort(
      begin(search_results), begin(search_results) + 5,
      end(search_results),
      [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
        int64_t lhs_docid = lhs.first;
        auto lhs_hit_count = lhs.second;
        int64_t rhs_docid = rhs.first;
        auto rhs_hit_count = rhs.second;
        return make_pair(lhs_hit_count, -lhs_docid) > make_pair(rhs_hit_count, -rhs_docid);
      }
    );

    search_results_output << current_query << ':';
    for (auto [docid, hitcount] : Head(search_results, 5)) {
      if (hitcount == 0){
        break;
      }
      search_results_output << " {"
        << "docid: " << docid << ", "
        << "hitcount: " << hitcount << '}';
    }
    search_results_output << endl;
  }
}

void InvertedIndex::Add(const string& document) {
  docs.push_back(document);

  const size_t docid = docs.size() - 1;
  unordered_map<string, vector<size_t>> th;
  for (const auto& word : SplitIntoWords(document)) {
    th[word].push_back(1);
  }
  for (auto i : th){
    index[i.first].push_back(make_pair(docid, i.second.size()));
  }
}

bool InvertedIndex::is_word_in_Lookup(const string& word){
  if (index.find(word) != index.end())
    return true;
  else
    return false;
}

const vector<pair<size_t, size_t>>& InvertedIndex::Lookup(const string& word) const {
    return index.at(word);
}
