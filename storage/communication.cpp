#include <string>
#include <vector>
#include <unistd.h>
#include <cstdint>
#include <iostream>
#include <utility>
#include <optional>

enum class Commands{
  PUT,
  GET,
  DELETE,
  POST,
  ERROR_
};

template <typename T>
struct Request{
  Commands command;
  T params;
  //Request(Commands c, const T& p) : command(c), params(p) {}
};


// Extracts the Key from the input
// Returns empty vector if unsuccessful, or the Key if successful
std::pair<std::vector<uint8_t>, size_t> extract_key(Commands comm, const std::vector<uint8_t>& input){
  size_t starting_point;
  if (comm == Commands::PUT || comm == Commands::GET){
    starting_point = 5;
  }else if (comm == Commands::POST){
    starting_point = 6;
  }else if (comm == Commands::DELETE){
    starting_point = 8;
  }

  // check for the key opening brace
  if (input[starting_point - 1] != '{'){
    return {};
  }

  // extract the key
  std::vector<uint8_t> to_be_returned;
  int counter = starting_point;
  for(auto i = input.begin() + starting_point; i != input.end() || *i != '}' ; ++i){
    to_be_returned.push_back(*i);
    counter++;
  }

  // if Command is PUT or POST, check if input has value after key
  if(comm == Commands::PUT || comm == Commands::POST){
    if (counter == input.size()){
      return {};
    }
  }

  return {to_be_returned, counter};
}

// Extracts the Value from the input
// Returns empty vector if unsuccessful, or the Value if successful
std::vector<uint8_t> extract_value(size_t val_start, const std::vector<uint8_t>& input){
  if(val_start != '{'){
    return {};
  }

  std::vector<uint8_t> to_be_returned;
  for(auto i = input.begin() + val_start + 1; i != input.end() || *i != '}' ; ++i){
    to_be_returned.push_back(*i);
  }

  if(*input.end() != '}'){
    return {};
  }

  return to_be_returned;

}

Commands command_from_string(std::string text){
  if(text == "PUT") return Commands::PUT;
  if(text == "GET") return Commands::GET;
  if(text == "POST") return Commands::POST;
  if(text == "DELETE") return Commands::DELETE;

  return Commands::ERROR_;
}

// A request will have the following form:
//  PUT {Key} {Value} (since the key and value of generic, the beginning and
//                     end of the key and value if determined byt he braces)
//  GET {Key}
//  DELETE {Key}
//  POST {Key} {Value}

struct KVParam {
  std::vector<uint8_t> key;
  std::optional<std::vector<uint8_t>> value; // nullopt => only key present
};

Request<KVParam> create_request(const std::vector<uint8_t>& input){

  std::string cmd_string;
  for (uint8_t c : input){
    if(c == ' '){
      break;
    }
    cmd_string.push_back(static_cast<char>(c));
  }

  Commands cmd = command_from_string(cmd_string);

  switch(cmd){
    case Commands::PUT:
    case Commands::POST:{
      auto tmp = extract_key(cmd, input);
      std::vector<uint8_t> key = tmp.first;
      if(key.empty()){
        std::cerr << "Invalid Request. PUT or POST request format: PUT {Key} {Value} or POST {Key} {Value}. (BRACES REQUIRED)" << std::endl;
        return {Commands::ERROR_, {{}, std::nullopt}};
      };

      size_t val_start = tmp.second + 2;
      std::vector<uint8_t> value = extract_value(val_start, input);
      if(value.empty()){
        std::cerr << "Invalid Request. PUT or POST request format: PUT {Key} {Value} or POST {Key} {Value}. (BRACES REQUIRED)" << std::endl;
        return {Commands::ERROR_, {{}, std::nullopt}};
      }

      return {cmd, {std::move(key), std::make_optional(std::move(value))}};
    }

    case Commands::GET:
    case Commands::DELETE:{
      std::vector<uint8_t> key = extract_key(cmd, input).first;
      if(key.empty()){
        std::cerr << "Invalid Request. GET or DELETE request format: GET {Key} or DELETE {Key}. (BRACES REQUIRED)" << std::endl;
        return {Commands::ERROR_, {{}, std::nullopt}};
      };

      return {cmd, {std::move(key), std::nullopt}};
    }

    default:
      return {Commands::ERROR_, {{}, std::nullopt}};
  }
}



template <typename V>
struct Response{
  std::optional<std::string> error;
  std::optional<V>* return_val;
  bool successful;
};

template<typename V>
Response<V> create_response(std::optional<std::string> err_msg, bool success){
  return {err_msg, nullptr, success};
}

template<typename V>
Response<V> create_response(std::optional<std::string> err_msg, V& value, bool success){
  return {err_msg, value, success};
}


