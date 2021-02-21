#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <string>

#include <numeric>
#include <thread>
#include <mutex>
#include <deque>
using namespace std;

template <typename T>
class Synchronized {
public:
    explicit Synchronized(T initial = T())
            : value(move(initial)){

    }

    struct Access {
        T& ref_to_value;
        lock_guard<mutex> guard;
    };

    Access GetAccess(){
        return {value, lock_guard(m)};
    }

private:
    T value;
    mutex m;
};

class InvertedIndex {
public:
    InvertedIndex() = default;
    explicit InvertedIndex(istream& stream);
    // void Add(const string& document);
    const vector<pair<size_t, size_t>>& Lookup(string_view word) const;
    bool is_word_in_Lookup(string_view word);

    string_view GetDocument(size_t id) const {
        return docs[id];
    }

    size_t  get_docs_size(){
        return docs.size();
    }

private:
    map<string_view, vector<pair<size_t, size_t>>> index;
    deque<string> docs;
};

class SearchServer {
public:
    SearchServer() = default;
    explicit SearchServer(istream& document_input);
    void UpdateDocumentBase(istream& document_input);
    void AddQueriesStream(istream& query_input, ostream& search_results_output);

private:
    InvertedIndex index;
};
