#include "./storage/store.cpp"
#include "./storage/communication.cpp"

template<typename K, typename V>
class KVS{
private:
Storage<K, V> store;



public:
Response<V> process_request(Request<KVParam> req){
  switch(req.command){
    case Commands::PUT:{
      if(!store.put(req.params.key, req.params.value)){
        return create_response<V>("Key already exists.", false);
      }
      return create_response<V>("", true);
    }
    case Commands::GET:{
      auto tmp = store.get(req.params.key);
      if(tmp == -1){
        return create_response<V>("KV not found", false);
      }
      return create_response<V>("", tmp, true);
    }
    default:
      return create_response<V>("Unknown command", false);
  }
}




};