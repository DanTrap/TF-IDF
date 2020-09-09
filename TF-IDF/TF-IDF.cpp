#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result;
	cin >> result;
	ReadLine();
	return result;
}

vector<string> SplitIntoWords(const string& text) {
	vector<string> words;
	string word;
	for (const char c : text) {
		if (c == ' ') {
			words.push_back(word);
			word = "";
		}
		else {
			word += c;
		}
	}
	words.push_back(word);
	return words;
}

struct Document {
	int id;
	double relevance;
};

struct Query {
	vector<string> plus_words;
	vector<string> minus_words;
};

class SearchServer {
public:
	int document_count_ = 0;

	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document) {
		for (const string& word : SplitIntoWordsNoStop(document)) {
			id_to_doc_[document_id].push_back(word);
			word_to_documents_[word].insert(document_id);
		}
		++document_count_;
	}

	vector<Document> FindTopDocuments(const string& query) const {
		auto matched_documents = FindAllDocuments(query);

		sort(
			matched_documents.begin(),
			matched_documents.end(),
			[](const Document& lhs, const Document& rhs) {
				return lhs.relevance > rhs.relevance;
			}
		);
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

private:
	map<int, vector<string>> id_to_doc_;
	map<string, set<int>> word_to_documents_;
	set<string> stop_words_;

	vector<string> SplitIntoWordsNoStop(const string& text) const {
		vector<string> words;
		for (const string& word : SplitIntoWords(text)) {
			if (stop_words_.count(word) == 0) {
				words.push_back(word);
			}
		}
		return words;
	}

	Query ParseQuery(const string& query) const {
		Query parsing_query;
		vector<string> words = SplitIntoWords(query);
		for (string& str : words) {
			if (str[0] == '-') {
				str.erase(0, 1);
				parsing_query.minus_words.push_back(str);
			}
			else {
				if (stop_words_.count(str) == 0) {
					parsing_query.plus_words.push_back(str);
				}
			}
		}
		return parsing_query;
	}

	map<string, double> FindIDF(Query& query_words) const {
		map<string, double> idf;
		for (auto it : query_words.plus_words) {
			if (word_to_documents_.at(it).size() != 0) {
				idf[it] = log(static_cast<double>(document_count_) / static_cast<double>(word_to_documents_.at(it).size()));
			}
		}
		return idf;
	}

	map<string, map<int, double>> FindTF(Query& query_words) const {
		map<string, map<int, double>> word_to_document_freqs_;
		for (auto word : query_words.plus_words) {
			for (int id = 0; id < document_count_; ++id) {
				int count = count_if(id_to_doc_.at(id).begin(), id_to_doc_.at(id).end(),
					[&word](string str) {
						if (str == word) return 1;
						else return 0;
					});
				word_to_document_freqs_[word][id] = count / static_cast<double>(id_to_doc_.at(id).size());
			}
		}
		return word_to_document_freqs_;
	}

	map<int, double> FindTF_IDF(Query& query_words) const {
		map<string, double> word_to_idf = FindIDF(query_words);
		map<string, map<int, double>> word_to_document_tf = FindTF(query_words);
		map<int, double> id_to_rel;
		for (int id = 0; id < document_count_; ++id) {
			double rel = 0;
			for (auto word : query_words.plus_words) {
				rel += word_to_idf[word] * word_to_document_tf[word][id];
			}
			if (rel != 0) id_to_rel[id] = rel;
		}
		return id_to_rel;
	}

	vector<Document> FindAllDocuments(const string& query) const {
		Query query_words = ParseQuery(query);
		map<int, double> document_to_relevance = FindTF_IDF(query_words);
		vector<int> delete_this_doc;
		for (auto it : query_words.minus_words) {
			if (word_to_documents_.at(it).empty()) {
				continue;
			}
			else {
				for (auto i : word_to_documents_.at(it)) {
					delete_this_doc.push_back(i);
				}
			}
		}
		for (auto i : delete_this_doc) {
			document_to_relevance.erase(i);
		}
		vector<Document> matched_documents;
		for (auto it : document_to_relevance) {
			matched_documents.push_back({ it.first, it.second });
		}
		return matched_documents;
	}
};

SearchServer CreateSearchServer() {
	SearchServer search_server;
	search_server.SetStopWords(ReadLine());

	const int document_count = ReadLineWithNumber();
	for (int document_id = 0; document_id < document_count; ++document_id) {
		search_server.AddDocument(document_id, ReadLine());
	}

	return search_server;
}


int main() {
	const SearchServer search_server = CreateSearchServer();

	const string query = ReadLine();
	for (auto it : search_server.FindTopDocuments(query)) {
		cout << "{ document_id = " << it.id << ", relevance = " << it.relevance << " }" << endl;
	}
	return 0;
}