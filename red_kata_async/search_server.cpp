#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <future>
#include <string_view>

string_view get_word(string_view& line){
    while (!line.empty() && isspace(line[0])){
        line.remove_prefix(1);
    }
    auto pos = line.find(' ');
    auto word = line.substr(0, pos);
    line.remove_prefix(pos != line.npos? pos: line.size());
    return word;
}

vector<string_view> SplitIntoWords(string_view line) {
    vector<string_view> res;

    for (string_view word = get_word(line); !word.empty(); word = get_word(line)){
        res.push_back(word);
    }
    return res;
}

SearchServer::SearchServer(istream& document_input) {
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {

    index = InvertedIndex{document_input};
}

ostringstream  AddQueriesStreamMulti(istream& query_input, InvertedIndex& index){
    ostringstream search_results_output;
    for (string current_query; getline(query_input, current_query); ) {
        const vector<string_view>& words = SplitIntoWords(current_query);
        size_t mysize = max(size_t(6), index.get_docs_size());
        vector<size_t> docid_count(mysize, 0);
        for (string_view word : words) {
            if (index.is_word_in_Lookup(word)){
                for (auto& it : index.Lookup(word)) {
                    docid_count[it.first] += it.second;
                }
            }
        }

        vector<int64_t> docids(docid_count.size());
        iota(docids.begin(), docids.end(), 0);
        partial_sort(
                begin(docids), Head(docids, 5).end(),
                end(docids),
                [&docid_count](int64_t lhs, int64_t rhs) {
                    return make_pair(docid_count[lhs], -lhs) > make_pair(docid_count[rhs], -rhs);
                }
        );

        search_results_output << current_query << ':';
        for (size_t docid : Head(docids, 5)) {
            const size_t hitcount = docid_count[docid];
            if (hitcount == 0){
                break;
            }
            search_results_output << " {"
                                  << "docid: " << docid << ", "
                                  << "hitcount: " << hitcount << '}';
        }
        search_results_output << endl;
    }
    return search_results_output;
}

void SearchServer::AddQueriesStream(
        istream& query_input, ostream& search_results_output
)
{
    vector<future<ostringstream>> futures;
    vector<string> strems;
    vector<istringstream> strstrems;
    int64_t ii = 0;
    for (string current_query; getline(query_input, current_query); ){
        strems.push_back(move(current_query));
    }
    auto strems_size = strems.size() / 5 + 1;
    ii = 0;
    for (int64_t i = 0; i < max(strems.size(), size_t(4));){
        string th = "";
        for (int64_t j = 0; j < strems_size; j++){
            if (j == strems.size()) {
                i++;
                break;
            }
            if (i >= strems.size()) {
                i++;
                break;
            }
            th += move(strems[i]) + "\n";
            i++;
        }
        strstrems.push_back(move(istringstream(th)));
    }
    size_t a;
    if ((a = strstrems.size()) < 5){
        while (a < 5){
            strstrems.push_back(move(istringstream("")));
            a++;
        }
    }

    futures.push_back(async(AddQueriesStreamMulti, ref(strstrems[0]), ref(index)));
    futures.push_back(async(AddQueriesStreamMulti, ref(strstrems[1]), ref(index)));
    futures.push_back(async(AddQueriesStreamMulti, ref(strstrems[2]), ref(index)));
    futures.push_back(async(AddQueriesStreamMulti, ref(strstrems[3]), ref(index)));
    futures.push_back(async(AddQueriesStreamMulti, ref(strstrems[4]), ref(index)));

    for (auto &i : futures){
        search_results_output << move(i.get().str());
    }
}

InvertedIndex::InvertedIndex(istream& document) {

    for (string cur_doc; getline(document, cur_doc);){
        docs.push_back(move(cur_doc));

        size_t docid = docs.size() - 1;
        for (string_view word : SplitIntoWords(docs.back())){
            auto& docids = index[word];
            if (!docids.empty() && docids.back().first == docid){
                docids.back().second++;
            }
            else{
                docids.push_back({docid, 1});
            }
        }
    }
}

bool InvertedIndex::is_word_in_Lookup(string_view word){
    if (index.find(word) != index.end())
        return true;
    else
        return false;
}

const vector<pair<size_t, size_t>>& InvertedIndex::Lookup(string_view word) const {
    return index.at(word);
}
