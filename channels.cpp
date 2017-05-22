#include "channels.hpp"
#include <limits>
#include <cstdlib>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

void device::add_channel(channel* c) {
  m_chans.push_back(c);
}

bool device::process() {

  static bool firsttime = true;
  if (firsttime) {
    for (auto& c : m_chans){
      auto data = c->get_data();
      m_dq.push(data);
    }
    firsttime = false;
  }

  auto data = m_dq.top();
  m_dq.pop();
  m_dq.push(data.chan->get_data());

  if (data.data == nullptr || data.len ==0)
    return false;
  //std::cout << "time: " << data.time << " chan " << data.chan << std::endl;  
  m_cb(data.data, data.len, m_ud);
  return true;
}

device::~device() {
    for (auto& c : m_chans)
      delete c;
}


file_channel::file_channel(const std::string& file) :
  m_ifs(file)
{
  if (m_ifs.bad()) {
    std::cerr << "failed to open file " << file <<std::endl;
    std::exit(EXIT_FAILURE);
  }
}

data_agg file_channel::get_data() {
  std::string line;
  getline (m_ifs,line);
  if (line.empty())
    return {nullptr, 0, LLONG_MAX, this};

  auto len = line.size();
  auto data = (char*) malloc(len);
  memcpy(data, line.c_str(), len);
  long long time = 0;

  char a = 1;
  boost::char_separator<char> sep{&a};
  boost::tokenizer<boost::char_separator<char>> tok(line, sep);
  for(auto beg=tok.begin(); beg!=tok.end(); ++beg){
    if (boost::starts_with(*beg,"52=")) {
      //time = boost::lexical_cast<long long>(std::string(*beg).substr(11));
      auto substring = std::string(*beg).substr(11);
      std::stringstream(substring) >> time;
      break;
    }
  }

  return {data, len, time, this};
}

  
    
