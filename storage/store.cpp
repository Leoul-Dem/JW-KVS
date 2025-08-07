#include <mutex>
#include <vector>
#include <shared_mutex>
#include <functional>
#include <optional>
#include <atomic>
#include <iostream>

const int STARTER_SIZE = 100000;
const int MAX_CUCKOO_COUNT = 8;

using namespace std;

template <typename K, typename V>
struct KV_Pair{
  K key;
  V value;
  bool is_empty = true;

  KV_Pair() : is_empty(true) {}
  KV_Pair(const K& k, const V& v) : key(k), value(v), is_empty(false) {}
};

template <typename K, typename V>
class Storage{

private:
  vector<KV_Pair<K, V>> table_1;
  vector<KV_Pair<K, V>> table_2;

  vector<shared_mutex> rw_mutex_1;
  vector<shared_mutex> rw_mutex_2;

  function<size_t(const K&)> h1;
  function<size_t(const K&)> h2;

  size_t size;
  atomic<long> n_counter;
  mutable shared_mutex resize_mutex;

  void init_hash_functions() {
    h1 = [this](const K& key) -> size_t {
      return hash<K>{}(key) % size;
    };
    h2 = [this](const K& key) -> size_t {
      return ((hash<K>{}(key)) ^ 0x9e3779b9) % size;
    };
  }

  bool cuckoo_insert(const K& key, const V& value) {
    KV_Pair<K, V> item(key, value);

    for (int count = 0; count < MAX_CUCKOO_COUNT; count++) {
      size_t idx1 = h1(item.key);

      // Try to place in table_1
      {
        unique_lock<shared_mutex> lock(rw_mutex_1[idx1]);
        if (table_1[idx1].is_empty) {
          table_1[idx1] = item;
          n_counter++;
          return true;
        }
        // Evict existing item
        swap(item, table_1[idx1]);
      }

      size_t idx2 = h2(item.key);

      // Try to place in table_2
      {
        unique_lock<shared_mutex> lock(rw_mutex_2[idx2]);
        if (table_2[idx2].is_empty) {
          table_2[idx2] = item;
          n_counter++;
          return true;
        }
        // Evict existing item
        swap(item, table_2[idx2]);
      }
    }

    return false; // Failed to insert after MAX_CUCKOO_COUNT attempts
  }

  void rehash() {
    // Simple rehashing strategy - double the size
    size_t old_size = size;
    size *= 2;

    vector<KV_Pair<K, V>> old_table_1 = move(table_1);
    vector<KV_Pair<K, V>> old_table_2 = move(table_2);

    // Reinitialize tables
    table_1.clear();
    table_2.clear();
    table_1.resize(size);
    table_2.resize(size);

    rw_mutex_1.clear();
    rw_mutex_2.clear();
    rw_mutex_1.resize(size);
    rw_mutex_2.resize(size);

    init_hash_functions();
    n_counter = 0;

    // Reinsert all elements
    for (const auto& item : old_table_1) {
      if (!item.is_empty) {
        if (!cuckoo_insert(item.key, item.value)) {
          // This shouldn't happen with a good hash function
          throw runtime_error("Failed to rehash");
        }
      }
    }

    for (const auto& item : old_table_2) {
      if (!item.is_empty) {
        if (!cuckoo_insert(item.key, item.value)) {
          throw runtime_error("Failed to rehash");
        }
      }
    }
  }

public:
  Storage() : size(STARTER_SIZE), n_counter(0) {
    table_1.resize(size);
    table_2.resize(size);
    rw_mutex_1.resize(size);
    rw_mutex_2.resize(size);
    init_hash_functions();
  }

  Storage(size_t n) : size(n), n_counter(0) {
    table_1.resize(size);
    table_2.resize(size);
    rw_mutex_1.resize(size);
    rw_mutex_2.resize(size);
    init_hash_functions();
  }

  optional<V> get_value(const K& key) {
    shared_lock<shared_mutex> resize_lock(resize_mutex);

    size_t idx1 = h1(key);
    {
      shared_lock<shared_mutex> lock(rw_mutex_1[idx1]);
      if (!table_1[idx1].is_empty && table_1[idx1].key == key) {
        return table_1[idx1].value;
      }
    }

    size_t idx2 = h2(key);
    {
      shared_lock<shared_mutex> lock(rw_mutex_2[idx2]);
      if (!table_2[idx2].is_empty && table_2[idx2].key == key) {
        return table_2[idx2].value;
      }
    }

    return nullopt;
  }

  void set_KV(const K& key, const V& value) {
    while (true) {
      {
        shared_lock<shared_mutex> resize_lock(resize_mutex);

        // Check if key already exists and update
        size_t idx1 = h1(key);
        {
          unique_lock<shared_mutex> lock(rw_mutex_1[idx1]);
          if (!table_1[idx1].is_empty && table_1[idx1].key == key) {
            table_1[idx1].value = value;
            return;
          }
        }

        size_t idx2 = h2(key);
        {
          unique_lock<shared_mutex> lock(rw_mutex_2[idx2]);
          if (!table_2[idx2].is_empty && table_2[idx2].key == key) {
            table_2[idx2].value = value;
            return;
          }
        }

        // Try to insert new key-value pair
        if (cuckoo_insert(key, value)) {
          return;
        }
      }

      // If insertion failed, need to rehash
      {
        unique_lock<shared_mutex> resize_lock(resize_mutex);
        // Double-check that we still need to rehash
        if (n_counter.load() >= size * 0.75) { // Load factor check
          rehash();
        }
      }
    }
  }

  bool delete_KV(const K& key) {
    shared_lock<shared_mutex> resize_lock(resize_mutex);

    size_t idx1 = h1(key);
    {
      unique_lock<shared_mutex> lock(rw_mutex_1[idx1]);
      if (!table_1[idx1].is_empty && table_1[idx1].key == key) {
        table_1[idx1].is_empty = true;
        n_counter--;
        return true;
      }
    }

    size_t idx2 = h2(key);
    {
      unique_lock<shared_mutex> lock(rw_mutex_2[idx2]);
      if (!table_2[idx2].is_empty && table_2[idx2].key == key) {
        table_2[idx2].is_empty = true;
        n_counter--;
        return true;
      }
    }

    return false;
  }

  bool contains(const K& key) {
    shared_lock<shared_mutex> resize_lock(resize_mutex);

    size_t idx1 = h1(key);
    {
      shared_lock<shared_mutex> lock(rw_mutex_1[idx1]);
      if (!table_1[idx1].is_empty && table_1[idx1].key == key) {
        return true;
      }
    }

    size_t idx2 = h2(key);
    {
      shared_lock<shared_mutex> lock(rw_mutex_2[idx2]);
      if (!table_2[idx2].is_empty && table_2[idx2].key == key) {
        return true;
      }
    }

    return false;
  }

  size_t get_size() const {
    return n_counter.load();
  }

  double get_load_factor() const {
    return static_cast<double>(n_counter.load()) / size;
  }
};