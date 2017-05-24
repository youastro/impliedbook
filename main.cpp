#include "ib_builder.hpp"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/tokenizer.hpp>

int main(int argc, char *argv[]) {

  if (argc < 2) {
    std::cerr << "no input config file" <<std::endl;
    std::exit(EXIT_FAILURE);
  }
  boost::property_tree::ptree pt;
  boost::property_tree::ini_parser::read_ini(argv[1], pt);

  std::vector<std::string> files;
  auto fileinput = pt.get<std::string>("files.input");
  boost::char_separator<char> sep{"|"};
  boost::tokenizer<boost::char_separator<char>> tok(fileinput, sep);
  for (auto f = tok.begin(); f != tok.end(); ++f)
    files.push_back(*f);

  std::unordered_map<std::string, std::vector<std::string>> symmap;
  auto children = pt.get_child("implyout");
  for (const auto& kv : children) {
    std::string nodestr = children.get<std::string>(kv.first); 
    auto& v = symmap[kv.first];
    boost::tokenizer<boost::char_separator<char>> token(nodestr, sep);
    for (auto f = token.begin(); f != token.end(); ++f)
      v.push_back(*f);
  }

  book_manager bm(files, symmap);
  bm.start();
  return 0;
}
