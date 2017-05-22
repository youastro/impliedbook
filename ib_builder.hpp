#ifndef _IB_BUILDER_HPP
#define _IB_BUILDER_HPP

#include "channels.hpp"
#include <unordered_map>
#include <functional>
#include <boost/functional/hash.hpp>
#include <vector>

enum class side {
	BID,
	ASK,
	NONE
};

enum class operation {
  NEW = 0,
  MOD,
  DEL,
  NONE
};

enum class imply_out_type{
	SAME,       //E1 and E2-E1
	OPPOSITE    //E1 and E1-E2
};

struct spread_sym {
	std::string long_;
	std::string short_;
};

inline bool operator==(const spread_sym& ss1, const spread_sym& ss2) {
	return ss1.long_ == ss2.long_ && ss1.short_ == ss2.short_;
}

struct spread_sym_hash{
	std::size_t operator()(const spread_sym& ss) const {
		std::size_t seed = 0;
		boost::hash_combine(seed, ss.long_);
		boost::hash_combine(seed, ss.short_);
		return seed;
	}
};

struct level {
	double prc;
	int sz;
	level* pre;
	level* next;

	level(double p, int s) : prc(p), sz(s), 
		pre(nullptr), next(nullptr) {}
	level(const level& l) : prc(l.prc), sz(l.sz),
		pre(nullptr), next(nullptr) {}
	bool betterthan(const level& l, side s);
	bool betterequal(const level& l, side s);

};

struct greater_by_prc {
  bool operator()(const level* lhs, const level* rhs) {
    return lhs->prc > rhs.prc;
  }
};

struct less_by_prc {
  bool operator()(const level* lhs, const level* rhs) {
    return lhs->prc < rhs.prc;
  }
};


struct book {
	level* atop;
	level* btop;

	book();
	~book() = default;
	void clear(level* top);
	void clear();
	void update_trade(const level& l, side s);
	void insert(const level& l, side s, int lvl);
	void modify(const level& l, side s, int lvl);
	void del(int lvl, side s);
	void print();
};

class book_manager;
class imp_book {
  std::string m_sym;
  std::unordered_map<std::string, book> dpdt_books;
  book_manager* ibb;

  // book* build_imp_book(book* ob, book* sb, unsigned short lvl, imply_out_type t);
  level* build_imp_levels(level*, level*, unsigned short, std::function<double(double,double)>);

public:
  imp_book(const std::string& s, book_manager* ib) : m_sym(s), ibb(ib) {}
  void update_spread(const spread_sym&, side);
  void update_outright(const std::string&, side);
  void print();
  void print_combined();
};

class book_manager 
{
  friend imp_book;

  device m_d;
  std::unordered_map<std::string, book> m_outright_book;
  std::unordered_map<spread_sym,  book, spread_sym_hash> m_spread_book;
  std::unordered_map<std::string, imp_book> m_imp_book;
  std::unordered_map<std::string, std::vector<std::string>> m_outright2implied;
  std::unordered_map<std::string, book> m_imp_cme_book;
  //void build_ib(const std::string& outright, const std::string& spread);

public:
  book_manager(const std::vector<std::string>& files, 
	       const std::unordered_map<std::string, std::vector<std::string>>& symmap);

  void update_spread(const spread_sym& sym, 
		     const level& l, side s, int lvl, operation op);
  void update_outright(const std::string& sym, 
		       const level& l, side s, int lvl, operation op);
  void update_cme_imp_book(const std::string& sym,
			   const level& l, side s, int lvl, operation op);
  void start();
  void print();
};


#endif
