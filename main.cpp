#include "ib_builder.hpp"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/tokenizer.hpp>

// void handle_data(char* data, size_t len) {
//   long long time;
//   char a = 1;
//   boost::char_separator<char> sep{&a};
//   boost::tokenizer<boost::char_separator<char>> tok(std::string(data), sep);
//   for(auto beg=tok.begin(); beg!=tok.end(); ++beg){
//     if (boost::starts_with(*beg, "60=")) {
//       time = boost::lexical_cast<long long>(std::string(*beg).substr(11));
//       break;
//     }
//   }
//   std::cout << time << std::endl;
//   delete data;
// }

int main() {

  boost::property_tree::ptree pt;
  boost::property_tree::ini_parser::read_ini("config", pt);

  std::vector<std::string> files;
  auto fileinput = pt.get<std::string>("files.input");
  boost::char_separator<char> sep{"|"};
  boost::tokenizer<boost::char_separator<char>> tok(fileinput, sep);
  for (auto f = tok.begin(); f != tok.end(); ++f) {
    std::cout << *f <<std::endl;
    files.push_back(*f);
  }

  std::unordered_map<std::string, std::vector<std::string>> symmap;
  auto children = pt.get_child("implyout");
  for (const auto& kv : children) {
    std::string nodestr = children.get<std::string>(kv.first); 
    auto& v = symmap[kv.first];
    boost::tokenizer<boost::char_separator<char>> token(nodestr, sep);
    std::cout << kv.first <<std::endl;
    for (auto f = token.begin(); f != token.end(); ++f) {
      std::cout << *f <<std::endl;
      v.push_back(*f);
    }
  }

  book_manager bm(files, symmap);
  bm.start();
  return 0;
}
