#include <unistd.h>
#include <stdint.h>
#include <assert.h> 
#include <cstring>
#include <vector>
#include <set>
#include <fstream>
#include <set>
#include <random>
#include <algorithm>

#include "util.hpp"

namespace proteus {

// For RocksDB
uint64_t sliceToUint64(const char* data) {
    uint64_t out = 0ULL;
    std::memcpy(&out, data, 8);
    return __builtin_bswap64(out);
}

std::string util_uint64ToString(const uint64_t& word) {
    uint64_t endian_swapped_word = __builtin_bswap64(word);
    return std::string(reinterpret_cast<const char*>(&endian_swapped_word), 8);
}

uint64_t util_stringToUint64(const std::string& str_word) {
    uint64_t int_word = 0;
    memcpy(reinterpret_cast<char*>(&int_word), str_word.data(), 8);
    return __builtin_bswap64(int_word);
}

template <>
void FIFOSampleQueryCache<std::string>::add(const std::pair<std::string, std::string> q) {
    // Add every {sample_freq}-th query
    std::lock_guard<std::mutex> guard(mut_);
    counter_ = (counter_ == (sample_freq_ - 1)) ? 0 : (counter_ + 1);
    if (counter_ == 0) {
        sample_queries_[pos_] = q;
        pos_ = (pos_ == (sample_queries_.size() - 1)) ? 0 : (pos_ + 1);
    }
}

template <>
void FIFOSampleQueryCache<uint64_t>::add(const std::pair<std::string, std::string> q) {
    // Add every {sample_freq}-th query
    std::lock_guard<std::mutex> guard(mut_);
    counter_ = (counter_ == (sample_freq_ - 1)) ? 0 : (counter_ + 1);
    if (counter_ == 0) {
        sample_queries_[pos_] = std::make_pair(util_stringToUint64(q.first), util_stringToUint64(q.second));
        pos_ = (pos_ == (sample_queries_.size() - 1)) ? 0 : (pos_ + 1);
    }
}

void intLoadKeys(std::string keyFilePath,
                 std::vector<uint64_t>& keys,
                 std::vector<std::string>& skeys,
                 std::set<uint64_t>& keyset) {
    
    std::ifstream keyFile;
    uint64_t key;

    keyFile.open(keyFilePath);
    while (keyFile >> key) {
        keyset.insert(key);
        keys.push_back(key);        
    }
    keyFile.close();

    std::sort(keys.begin(), keys.end());
    for (const auto & k : keys) {
        skeys.push_back(util_uint64ToString(k));
    }
}


void intLoadQueries(std::string lQueryFilePath, 
                    std::string uQueryFilePath,
                    std::vector<std::pair<uint64_t, uint64_t>>& range_queries,
                    std::vector<std::pair<std::string, std::string>>& squeries) {
    
    std::ifstream lQueryFile, uQueryFile;    
    uint64_t lq, uq;

    lQueryFile.open(lQueryFilePath);
    uQueryFile.open(uQueryFilePath);
    while((lQueryFile >> lq) && (uQueryFile >> uq)) {
        assert(lq <= uq);
        range_queries.push_back(std::make_pair(lq, uq));
    }
    lQueryFile.close();
    uQueryFile.close();

    std::sort(range_queries.begin(), range_queries.end());
    for (const auto & q : range_queries) {
        squeries.push_back(std::make_pair(util_uint64ToString(q.first), util_uint64ToString(q.second)));
    }
}


size_t strLoadKeys(std::string keyFilePath,
                   std::vector<std::string>& skeys,
                   std::set<std::string>& keyset) {
    
    std::ifstream keyFile(keyFilePath, std::ifstream::binary);
    uint32_t sz;
    keyFile.read(reinterpret_cast<char*>(&sz), sizeof(uint32_t));

    char* k_arr = new char[sz];
    while (keyFile.peek() != EOF) {
        keyFile.read(k_arr, sz);
        keyset.insert(std::string(k_arr, sz));
        skeys.push_back(std::string(k_arr, sz)); 
    }

    keyFile.close();

    std::sort(skeys.begin(), skeys.end());
    delete[] k_arr;

    return static_cast<size_t>(sz);
}

void strLoadQueries(std::string lQueryFilePath, 
                    std::string rQueryFilePath,
                    std::vector<std::pair<std::string, std::string>>& squeries) {
    
    std::ifstream lQueryFile(lQueryFilePath, std::ifstream::binary);
    std::ifstream rQueryFile(rQueryFilePath, std::ifstream::binary);
    
    uint32_t lsz, rsz;
    lQueryFile.read(reinterpret_cast<char*>(&lsz), sizeof(uint32_t));
    rQueryFile.read(reinterpret_cast<char*>(&rsz), sizeof(uint32_t));
    assert(lsz == rsz);

    std::string lq, rq;
    char* lq_arr = new char[lsz];
    char* rq_arr = new char[rsz];

    while (lQueryFile.peek() != EOF && rQueryFile.peek() != EOF) {
        lQueryFile.read(lq_arr, lsz);
        rQueryFile.read(rq_arr, rsz);
        lq = std::string(lq_arr, lsz);
        rq = std::string(rq_arr, rsz);
        assert(lq <= rq);
        squeries.push_back(std::make_pair(lq, rq));
    }

    lQueryFile.close();
    rQueryFile.close();

    std::sort(squeries.begin(), squeries.end());
    delete[] lq_arr;
    delete[] rq_arr;
}

    void intGenerateAdversarialQueries(const size_t n_queries, const std::vector<uint64_t> &keys,
                                          std::vector<std::pair<uint64_t, uint64_t>> &queries,
                                          std::vector<std::pair<std::string, std::string>> &squeries) {
        std::srand(42);
        std::mt19937 gen(42); // seed the generator
        std::uniform_int_distribution<> distr0(0, keys.size() - 1); // define the range
        queries.reserve(n_queries);
        for (auto i = 0; i < n_queries; i++) {
            size_t j = distr0(gen);
            queries.emplace_back(keys[j] + 1, keys[j + 1] - 1);
        }

        squeries.reserve(n_queries);
        for (const auto &q: queries) {
            squeries.emplace_back(util_uint64ToString(q.first), util_uint64ToString(q.second));
        }

    }

    void intLoadKeysSOSD(std::string keyFilePath,
                         std::vector<uint64_t> &keys,
                         std::vector<std::string> &skeys,
                         std::set<uint64_t> &keyset,
                         size_t max_num_keys) {

        std::ifstream keyFile;

        keyFile.open(keyFilePath);

        // Read size
        uint64_t size;
        keyFile.read(reinterpret_cast<char *>(&size), sizeof(uint64_t));
        size = std::min(size, max_num_keys);
        keys.resize(size);

        // Read values
        keyFile.read(reinterpret_cast<char *>(keys.data()), size * sizeof(uint64_t));

        for (auto k: keys) {
            keyset.insert(k);
        }


        // Shuffle data
        //std::default_random_engine rng(rand());
        //std::shuffle(std::begin(keys), std::end(keys), rng);

        std::sort(keys.begin(), keys.end());

        // skeys is needed for FST
        for (const auto &k: keys) {
            skeys.push_back(util_uint64ToString(k));
        }
    }

}