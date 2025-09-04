//
// Created by MyPC on 9/3/2025.
//

#ifndef STORE_H
#define STORE_H

#include <mutex>
#include <vector>
#include <shared_mutex>
#include <functional>
#include <optional>
#include <atomic>

extern const int STARTER_SIZE;
extern const int MAX_CUCKOO_COUNT;

template <typename K, typename V>
struct KV_Pair{
    K key;
    V value;
    bool is_empty = true;

    KV_Pair();
    KV_Pair(const K& k, const V& v);
};

template <typename K, typename V>
class Storage {
private:
    std::vector<KV_Pair<K, V>> table_1;
    std::vector<KV_Pair<K, V>> table_2;

    std::vector<std::shared_mutex> rw_mutex_1;
    std::vector<std::shared_mutex> rw_mutex_2;

    std::function<size_t(const K&)> h1;
    std::function<size_t(const K&)> h2;

    size_t size;
    std::atomic<long> n_counter;
    mutable std::shared_mutex resize_mutex;

    void init_hash_functions();
    bool cuckoo_insert(const K& key, const V& value);
    void rehash();

public:
    Storage();
    Storage(size_t n);

    std::optional<V> get_value(const K& key);
    void set_KV(const K& key, const V& value);
    bool delete_KV(const K& key);
    bool contains(const K& key);
    size_t get_size() const;
    double get_load_factor() const;
};

// Include template implementation
#include "store.cpp"

#endif // STORE_H