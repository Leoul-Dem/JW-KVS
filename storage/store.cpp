#include <mutex>
#include <vector>
#include <shared_mutex>
#include <functional>
#include <optional>

int STARTER_SIZE = 100000;


using namespace std;

template <typename K, typename V>
struct KV_Pair{
  K key;
  V value;
};

template <typename K, typename V>
class Storage{

private:

  vector<KV_Pair<K, V>> table_1;
  vector<KV_Pair<K, V>> table_2;

  vector<shared_mutex> rw_mutex_1;
  vector<shared_mutex> rw_mutex_2;

  size_t h1;
  size_t h2;

  size_t size;

  long n_counter;

public:
  Storage(){
    table_1.reserve(STARTER_SIZE);
    table_2.reserve(STARTER_SIZE);
    rw_mutex_1.reserve(STARTER_SIZE);
    rw_mutex_2.reserve(STARTER_SIZE);

    size = STARTER_SIZE;

    h1 = [size](const K& key){
      return hash<K>{}(key) % size;
    }
    h2 = [size](const K& key){
      return ((hash<K>{}(key)) ^ 0x9e3779b9) % size;
    }

    n_counter = 0;
  }

  Storage(size_t n){
    table_1.reserve(n);
    table_2.reserve(n);
    rw_mutex_1.reserve(n);
    rw_mutex_2.reserve(n);

    size = n;

    h1 = [size](const K& key){
      return hash<K>{}(key) % size;
    }
    h2 = [size](const K& key){
      return ((hash<K>{}(key)) ^ 0x9e3779b9) % size;
    }

    n_counter = 0;
  }

  std::optional<V> get_value(K& key){
    size_t idx1 = h1(key);
    size_t idx2 = h2(key);
    shared_lock<shared_mutex> lock(rw_mutex_1[idx1]);
    auto temp_value = table_1[idx1];
    if(temp_value.key == key){
      return temp_value.value;
    }
    lock.unlock();

    lock(rw_mutex_2[idx2]);
    temp_value = table_2[idx2];
    if(temp_value.key == key){
      return temp_value.value;
    }

    return nullopt;
  }

  void set_KV(K& key, V& value){



  }

  bool delete_KV(K& key){
    size_t idx1 = h1(key);
    size_t idx2 = h2(key);
    shared_lock<shared_mutex> lock(rw_mutex_1[idx1]);
    auto temp_value = table_1[idx1];
    if(temp_value.key == key){
      table_1.erase(table_1.begin()+idx1);
      return true;
    }
    lock.unlock();

    lock(rw_mutex_2[idx2]);
    temp_value = table_2[idx2];
    if(temp_value.key == key){
      table_2.erase(table_2.begin()+idx2);
      return true;
    }

    return false;


  }

  bool contains(K& key){
    s size_t idx1 = h1(key);
    size_t idx2 = h2(key);
    shared_lock<shared_mutex> lock(rw_mutex_1[idx1]);
    auto temp_value = table_1[idx1];
    if(temp_value.key == key){
      return true;
    }
    lock.unlock();

    lock(rw_mutex_2[idx2]);
    temp_value = table_2[idx2];
    if(temp_value.key == key){
      return true;
    }

    return false;

  }



};