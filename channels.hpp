#ifndef _CHANNELS_HPP
#define _CHANNELS_HPP

#include <fstream>
#include <vector>
#include <memory>
#include <queue>

class channel;
struct data_agg {
char* data;
size_t len;
long long time;
channel* chan;
data_agg(char* d, size_t l, long long t, channel* c) : data(d), len(l), time(t), chan(c) {}
data_agg(const data_agg& da) : data(da.data), len(da.len), 
	time(da.time),chan(da.chan) {}
};

struct greater_by_time {
	bool operator()(const data_agg& lhs, const data_agg& rhs) {
		return lhs.time > rhs.time;
	}
};

typedef void (*cb_t)(char* data, size_t len, void* userdata);

class channel;
class device
{
  cb_t m_cb;
  void* m_ud;
  std::vector<channel*> m_chans;
  std::priority_queue<data_agg,std::vector<data_agg>, greater_by_time> m_dq;
public:
	device() = default;
	~device();
  bool process();
  void add_channel(channel* c);
  void set_cb(cb_t cb, void* userdata) {
  	m_cb = cb;
  	m_ud = userdata;
  }
};

class channel
{
public:
  virtual data_agg get_data() = 0;
  virtual ~channel() = default;
};

class file_channel : public channel
{
std::ifstream m_ifs;

public:
  file_channel(const std::string& file);
  ~file_channel(){m_ifs.close();}
  data_agg get_data();
};

#endif
