#include "channels.hpp"
#include <climits>
#include <cstdlib>
#include <cstring>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <sstream>

void device::add_channel(channel* c) {
  m_chans.push_back(c);
}

bool device::process() {

  static bool firsttime = true;
  if (firsttime) {
    for (auto& c : m_chans){
      auto dat = c->get_data();
      m_dq.push(dat);
    }
    firsttime = false;
  }

  //use priority queue to get data based on time order
  auto data = m_dq.top();
  m_dq.pop();
  m_dq.push(data.chan->get_data());

  if (data.data == nullptr || data.len ==0)
    return false;

  m_cb(data.data, data.len, m_ud);
  free(data.data);
  return true;
}

device::~device() {
    for (auto& c : m_chans)
      delete c;

    while(!m_dq.empty()) {
      auto data = m_dq.top();
      m_dq.pop();
      free(data.data);
    }
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
  auto data = (char*) malloc(len+1);
  memcpy(data, line.c_str(), len);
  *(data+len) = 0;
  long long time = 0;

  char a = 1;
  boost::char_separator<char> sep{&a};
  boost::tokenizer<boost::char_separator<char>> tok(line, sep);
  for(auto beg=tok.begin(); beg!=tok.end(); ++beg){
    // 60= is the msg timestamp
    if (boost::starts_with(*beg,"60=")) {
      //extract timestamp of day, hour ... to millisecond
      auto substring = std::string(*beg).substr(10,10);
      std::stringstream(substring) >> time;
      break;
    }
  }

  return {data, len, time, this};
}

  
    
