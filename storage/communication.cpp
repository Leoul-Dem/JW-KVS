#include <string>
#include <sstream>
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


// A request will have the following form:
//  PUT {Key} {Value} (since the key and value of generic, the beginning and
//                     end of the key and value if determined byt he braces)
//  GET {Key}
//  DELETE {Key}
//  POST {Key} {Value}

struct KVParam {
  std::vector<uint8_t>* key;
  std::optional<std::vector<uint8_t>*> value; // nullopt => only key present
};

Request<KVParam> create_request(const std::vector<uint8_t>& input){


  std::string cmd_string;
  for (uint8_t i : input){
    if(i == ' '){
      break;
    }
    cmd_string.push_back(static_cast<char>(i));
  }

  Commands cmd;

  if(cmd_string == "PUT"){
    cmd = Commands::PUT;

    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
    size_t val_start;
    auto tmp = extract_key(Commands::PUT, input);


    key = tmp.first;
    if(key.size() == 0){
      std::cerr << "Invalid Request. PUT request format: PUT {Key} {Value}. (BRACES REQUIRED)" << std::endl;
      return {Commands::ERROR_, {nullptr, nullptr}};
    };

    val_start = tmp.second + 2;

    value = extract_value(val_start, input);
    if(value.size() == 0){
      std::cerr << "Invalid Request. PUT request format: PUT {Key} {Value}. (BRACES REQUIRED)" << std::endl;
      return {Commands::ERROR_, {nullptr, nullptr}};
    }

    return {cmd, {&key, &value}};

  }else if (cmd_string == "GET"){
    cmd = Commands::GET;

    std::vector<uint8_t> key = extract_key(Commands::GET, input).first;
    if(key.size() == 0){
      std::cerr << "Invalid Request. GET request format: GET {Key}. (BRACES REQUIRED)" << std::endl;
      return {Commands::ERROR_, {nullptr, nullptr}};
    };

    return {cmd, {&key, nullptr}};

  }else if (cmd_string == "DELETE"){
    cmd = Commands::DELETE;

    std::vector<uint8_t> key = extract_key(Commands::DELETE, input).first;
    if(key.size() == 0){
      std::cerr << "Invalid Request. DELETE request format: DELETE {Key}. (BRACES REQUIRED)" << std::endl;
      return {Commands::ERROR_, {nullptr, nullptr}};
    };

    return {cmd, {&key, nullptr}};

  }else if (cmd_string == "POST"){
    cmd = Commands::POST;

    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
    size_t val_start;
    auto tmp = extract_key(Commands::POST, input);


    key = tmp.first;
    if(key.size() == 0){
      std::cerr << "Invalid Request. POST request format: POST {Key} {Value}. (BRACES REQUIRED)" << std::endl;
      return {Commands::ERROR_, {nullptr, nullptr}};
    };

    val_start = tmp.second + 2;

    value = extract_value(val_start, input);
    if(value.size() == 0){
      std::cerr << "Invalid Request. POST request format: POST {Key} {Value}. (BRACES REQUIRED)" << std::endl;
      return {Commands::ERROR_, {nullptr, nullptr}};
    }

    return {cmd, {&key, &value}};


  }else{
    cmd = Commands::ERROR_;
  }

  return {Commands::ERROR_, {nullptr, nullptr}};
}

