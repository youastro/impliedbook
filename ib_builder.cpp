#include "ib_builder.hpp"
#include <iostream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace {

const int BOOKDEPTH = 10;

spread_sym parse_spread_sym(const std::string& sym) {
  boost::char_separator<char> sep{"-"};
  boost::tokenizer<boost::char_separator<char>> tok(sym, sep);
  auto beg=tok.begin();
  auto l = *beg;
  std::string s = "";
  if (++beg != tok.end())
    s = *beg;
  return {l, s};
}

double add(double a, double b)  {
  return a+b;
}

double subtract(double a, double b) {
  return a-b;
}

void handle_data(char* data, size_t len, void* userdata) {
  auto p = reinterpret_cast<book_manager*>(userdata);
  char a=1;
  // boost::char_separator<char> sep(&a);
  // boost::tokenizer<boost::char_separator<char>> tok(std::string(data, len), sep);
  // auto beg=tok.begin();


  std::stringstream ss(data);
  std::string item;

  operation op = operation::NONE;
  side s = side::NONE;
  double prc = 0;
  int sz = 0;
  std::string sym;
  spread_sym ssym;
  int lvl;
  bool ready = false;
  bool implied = false;
  long long last_time = 0;

  while(getline(ss, item, a)) {
    if(ready) {
      level l(prc, sz);

      if (implied) {
         p->update_cme_imp_book(sym, l, s, lvl, op);
      } else {
        if(!sym.empty())
          p->update_outright(sym, l, s, lvl, op);
        else
          p->update_spread(ssym, l, s, lvl, op);
      }

      ready = false;
      op = operation::NONE;
      s = side::NONE;
      prc = 0;
      sz = 0;
      sym = "";
      ssym.long_ = "";
      ssym.short_ = "";
      implied = false;
    }

    // only care about incremental refresh
    if (boost::starts_with(item,"35=")) {
      if (item != "35=X")
        return;
    }

    if (boost::starts_with(item,"60=")) {
      long long time;
      auto substring = item.substr(10,10);
      std::stringstream(substring) >> time;
      // 1e5 corresponds to 1 minite
      if ((time - last_time) > 1e5) {
        p->print();
        last_time = time;
      }
    }

    if (boost::starts_with(item,"279=")) {
      int tmp;
      auto substring = item.substr(4);
      std::stringstream(substring) >> tmp;
      switch(tmp) {
        case 0: 
          op = operation::NEW;
          break;
        case 1:
          op = operation::MOD;
          break;
        case 2:
          op = operation::DEL;
          break;
        default:
          std::cerr << "unknown operation " << tmp <<std::endl;
          return;
      }
    } else if (boost::starts_with(item,"269=")) {
      auto tmp  = item.substr(4);
      if(tmp == "0") s = side::BID;
      else if(tmp == "1") s = side::ASK;
      else if (tmp == "E") {
        s = side::BID;
        implied = true;
      }
      else if (tmp == "F") {
        s = side::ASK;
        implied = true;
      }
      else {
        while (getline(ss, item, a)) {
          if (boost::starts_with(item,"1023="))
            break;
        }
        // if (beg == tok.end()) return;
      }
    } else if (boost::starts_with(item,"55=")) {
      auto tmp  = item.substr(3);
      auto pos = tmp.find("-");
      if (pos != std::string::npos && pos != tmp.size()-1)
        ssym = parse_spread_sym(tmp);
      else
        sym = tmp;
    } else if (boost::starts_with(item,"270=")) {
      auto substring = item.substr(4);
      std::stringstream(substring) >> prc;
    } else if (boost::starts_with(item,"271=")) {
      auto substring = item.substr(4);
      std::stringstream(substring) >> sz;
    } else if (boost::starts_with(item,"1023=")) {
      auto substring = item.substr(5);
      std::stringstream(substring) >> lvl;
      ready = true;
    }
  }


  // while( beg!=tok.end()){
  //   if(ready) {
  //     level l(prc, sz);

  //     if (implied) {
	 //       p->update_cme_imp_book(sym, l, s, lvl, op);
  //     } else {
  //     	if(!sym.empty())
  //     	  p->update_outright(sym, l, s, lvl, op);
  //     	else
  //     	  p->update_spread(ssym, l, s, lvl, op);
  //     }

  //     ready = false;
  //     op = operation::NONE;
  //     s = side::NONE;
  //     prc = 0;
  //     sz = 0;
  //     sym = "";
  //     ssym.long_ = "";
  //     ssym.short_ = "";
  //     implied = false;
  //   }

  //   // only care about incremental refresh
  //   if (boost::starts_with(*beg,"35=")) {
  //     if (*beg != "35=X")
  //       return;
  //   }

  //   if (boost::starts_with(*beg,"60=")) {
  //     long long time;
  //     if (std::string(*beg).size() < 20) {
  //       ++beg;
  //       continue;
  //     }
  //     auto substring = std::string(*beg).substr(10,10);
  //     std::stringstream(substring) >> time;
  //     // 1e5 corresponds to 1 minite
  //     if ((time - last_time) > 1e5) {
	 //      p->print();
	 //      last_time = time;
  //     }
  //   }

  //   if (boost::starts_with(*beg,"279=")) {
  //     int tmp;
  //     auto substring = std::string(*beg).substr(4);
  //     std::stringstream(substring) >> tmp;
  //     switch(tmp) {
  //       case 0: 
  //         op = operation::NEW;
  //         break;
  //       case 1:
  //         op = operation::MOD;
  //         break;
  //       case 2:
  //         op = operation::DEL;
  //         break;
  //       default:
  //         std::cerr << "unknown operation " << tmp <<std::endl;
  //         return;
  //     }
  //   } else if (boost::starts_with(*beg,"269=")) {
  //     auto tmp  = std::string(*beg).substr(4);
  //     if(tmp == "0") s = side::BID;
  //     else if(tmp == "1") s = side::ASK;
  //     else if (tmp == "E") {
  //     	s = side::BID;
  //     	implied = true;
  //     }
  //     else if (tmp == "F") {
  //     	s = side::ASK;
  //     	implied = true;
  //     }
  //     else {
  //       while (beg != tok.end()) {
  //         if (boost::starts_with(*beg,"1023="))
  //           break;
  //         ++beg;
  //       }
  //       if (beg == tok.end()) return;
  //     }
  //   } else if (boost::starts_with(*beg,"55=")) {
  //     auto tmp  = std::string(*beg).substr(3);
  //     auto pos = tmp.find("-");
  //     if (pos != std::string::npos && pos != tmp.size()-1)
  //       ssym = parse_spread_sym(tmp);
  //     else
  //       sym = tmp;
  //   } else if (boost::starts_with(*beg,"270=")) {
  //     auto substring = std::string(*beg).substr(4);
  //     std::stringstream(substring) >> prc;
  //   } else if (boost::starts_with(*beg,"271=")) {
  //     auto substring = std::string(*beg).substr(4);
  //     std::stringstream(substring) >> sz;
  //   } else if (boost::starts_with(*beg,"1023=")) {
  //     auto substring = std::string(*beg).substr(5);
  //     std::stringstream(substring) >> lvl;
  //     ready = true;
  //   }

  //   ++beg;
  // }
}

} //end of anonymous namespace

bool level::betterthan(const level& l, side s) {
  return s == side::BID ? prc > l.prc : prc < l.prc;
}

book::book() : atop(nullptr), btop(nullptr)
{}

book::~book() {
  while(atop) {
    auto p = atop;
    atop = atop->next;
    delete p;
  }

  while(btop) {
    auto p = btop;
    btop = btop->next;
    delete p;
  }
}

void book::clear(level* top) {

  if(top != nullptr) {
    auto top_cpy = top;
    while(top_cpy) {
      auto tmp = top_cpy;
      top_cpy = top_cpy->next;
      delete tmp;
    }
    top = nullptr;
  }
}

void book::update_trade(const level& l, side s) {
  auto& top = (s == side::BID ? btop : atop);
  auto b = top;
  if (!b) {
    auto newl = new level(l);
    top = newl;
    return;
  } 

  while (b->betterthan(l, s) && b->next)
    b = b->next;

  if (b->betterthan(l,s)) {
    auto newl = new level(l);
    newl->pre = b;
    b->next = newl;
    return;
  } 

  if (b->prc == l.prc) {
    b->sz += l.sz;
    if (b->sz < 0) {
      std::cerr << " has negetive contract size";
    } else if (b->sz == 0) {
      if (b == top) {
        top = b->next;
        top->pre = nullptr;
        delete b;
      } else {
        b->pre->next = b->next;
        b->next->pre = b->pre;
        delete b;
      }
    }
  } else {
    auto newl = new level(l);
    if (b == top) {
      top = newl;
      newl->next = b;
      b->pre = newl;
    } else {
      newl->pre = b->pre;
      newl->next = b;
      newl->pre->next = newl;
      b->pre = newl;
    }
  }
}

void book::insert(const level& l, side s, int lvl) {
  auto& top = (s == side::BID ? btop : atop);
  auto curlvl = top;
  if (lvl ==1) {
    top = new level(l);
    if (curlvl) {
      top->next = curlvl;
      curlvl->pre = top;
    }
    return;
  }

  int count = 1;
  while (count < lvl -1 && curlvl != nullptr) {
    curlvl = curlvl->next;
    ++count;
  }

  if (curlvl == nullptr) {
    std::cout << "book not ready to take this new level, prc " << l.prc 
      << " sz: " << l.sz << " level " << lvl <<std::endl;
      return;
  }

  auto curlvl_next = curlvl->next;
  auto newl = new level(l);
  curlvl->next = newl;
  newl->pre = curlvl;

  if (curlvl_next) {
    newl->next = curlvl_next;
    curlvl_next->pre = newl;
  }
}

void book::modify(const level& l, side s, int lvl) {
  auto& top = (s == side::BID ? btop : atop);
  auto curlvl = top;

  int count = 1;
  while (count < lvl && curlvl != nullptr) {
    curlvl = curlvl->next;
    ++count;
  }
  if (curlvl == nullptr) {
    if (count != lvl) {
      std::cout << "book not ready to take modified level, prc " << l.prc 
        << " sz: " << l.sz << " level " << lvl <<std::endl;
    } else {
      auto newl = new level(l);
      if (lvl ==1) {
        top = newl;
        return;
      }
      curlvl = top;
      while (curlvl->next != nullptr) 
        curlvl = curlvl->next;
      curlvl->next = newl;
      newl->pre = curlvl;
    }
    return;
  }

  curlvl->prc = l.prc;
  curlvl->sz = l.sz;
}

void book::del(int lvl, side s) {
  auto& top = (s == side::BID ? btop : atop);
  if (!top){
    std::cout << "book empty, nothing to be deleted, lvl=" << lvl << std::endl;
    return;
  }

  auto curlvl = top;
  if(lvl == 1 ) {
    top = top->next;
    if (top)
      top->pre = nullptr;
    delete curlvl;
    return;
  }
  
  int count = 1;
  while (count < lvl && curlvl != nullptr) {
    curlvl = curlvl->next;
    ++count;
  }

  if (curlvl == nullptr) {
    std::cout << "book not ready to take delete level, lvl=" << lvl << std::endl;
      return;
  }

  curlvl->pre->next = curlvl->next;
  if(curlvl->next != nullptr) 
    curlvl->next->pre = curlvl->pre;

  delete curlvl;
}


void book::print() {
  std::cout << " bid" <<std::endl;
  auto b = btop;
  while (b) {
    std::cout << "(" << b->prc << " , " << b->sz <<  ") ";
    b = b->next;
  }
  std::cout << std::endl;

  std::cout << " ask" <<std::endl;
  b = atop;
  while (b) {
    std::cout << "(" << b->prc << " , " << b->sz <<  ") ";
    b = b->next;
  }
  std::cout << std::endl;
}

level* imp_book::build_imp_levels(level* obtop, level* sbtop, unsigned short lvl,
  std::function<double(double,double)> func) 
{
  if (!obtop || !sbtop || !lvl)
    return nullptr;

  level* toplevel = new level(0,0);  //create a fake level
  level* curlevel = toplevel;
  unsigned short lvl_built = 0;

  int ob_sz_left = obtop->sz;
  int sb_sz_left = sbtop->sz;

  while (obtop && sbtop && lvl_built < lvl) {
    level* l = nullptr;
    if (ob_sz_left < sb_sz_left) {
      auto prc = func(obtop->prc, sbtop->prc);
      l = new level(prc, ob_sz_left);
      sb_sz_left -= ob_sz_left;
      obtop = obtop->next;
      if (obtop)
        ob_sz_left = obtop->sz;
    } else if(ob_sz_left > sb_sz_left) {
      auto prc = func(obtop->prc, sbtop->prc);
      l = new level(prc, sb_sz_left);
      ob_sz_left -= sb_sz_left;
      sbtop = sbtop->next;
      if (sbtop)
        sb_sz_left = sbtop->sz;
    } else {
      auto prc = func(obtop->prc, sbtop->prc);
      l = new level(prc, ob_sz_left);
      sbtop = sbtop->next;
      if (sbtop)
        sb_sz_left = sbtop->sz;

      obtop = obtop->next;
      if (obtop)
        ob_sz_left = obtop->sz;
    }

    curlevel->next = l;
    l->pre = curlevel;
    curlevel = l;
    ++lvl_built;
  }

  auto l = toplevel->next;
  l->pre = nullptr;
  delete toplevel;
  return l;
}

void imp_book::update_spread(const spread_sym& ssym, side s) {

  auto& sb = ibb->m_spread_book[ssym];
  auto sbtop = (s == side::BID ? sb.btop : sb.atop);

  if (ssym.long_ == m_sym) {
    
    auto& ob = ibb->m_outright_book[ssym.short_];
    auto obtop = (s == side::BID ? ob.btop : ob.atop);

    auto& ib = dpdt_books[ssym.short_];
    auto& ibtop = (s == side::BID ? ib.btop : ib.atop);
    ib.clear(ibtop);

    ibtop = build_imp_levels(obtop, sbtop, BOOKDEPTH, add);

  } else {

    auto& ob = ibb->m_outright_book[ssym.long_];
    auto obtop = (s == side::BID ? ob.atop : ob.btop);

    auto& ib = dpdt_books[ssym.long_];
    auto& ibtop = (s == side::BID ? ib.atop : ib.btop);
    ib.clear(ibtop);

    ibtop = build_imp_levels(obtop, sbtop, BOOKDEPTH, subtract);

  }
}

void imp_book::update_outright(const std::string& osym, side s) {

  auto& ob = ibb->m_outright_book[osym];
  auto obtop = (s == side::BID ? ob.btop : ob.atop);

  spread_sym ssym{m_sym,osym};
  auto it = ibb->m_spread_book.find(ssym);
  imply_out_type t = imply_out_type::SAME;
  if (it == ibb->m_spread_book.end()) {
    swap(ssym.long_, ssym.short_);
    it = ibb->m_spread_book.find(ssym);
    t = imply_out_type::OPPOSITE;
  }
  auto& sb = it->second;
  auto sbtop = ((s == side::BID)^(t == imply_out_type::SAME) 
    ? sb.atop : sb.btop);

  auto& ib = dpdt_books[osym];
  auto& ibtop = (s == side::BID ? ib.btop : ib.atop);
  ib.clear(ibtop);

  ibtop = build_imp_levels(obtop, sbtop, BOOKDEPTH, t == imply_out_type::SAME ? add : subtract);
}

void imp_book::print_combined() {
  std::priority_queue<level*,std::vector<level *>, less_by_prc> bid_pq;
  for (auto& db : dpdt_books) {
    if (db.second.btop != nullptr)
      bid_pq.push(db.second.btop);
  }

  std::cout <<"Constructed implied Bid book of " << m_sym << std::endl;
  int count = 0;
  while(count < BOOKDEPTH && !bid_pq.empty()) {
    auto l = bid_pq.top();
    bid_pq.pop();
    if (l->next)
      bid_pq.push(l->next);

    level combined(*l);
    while (bid_pq.top()->prc == l->prc && !bid_pq.empty()) {
      auto ll = bid_pq.top();
      bid_pq.pop();
      if (ll->next)
        bid_pq.push(ll->next);
      combined.sz += ll->sz;
    }
    ++count;
    std::cout << "( " << combined.prc <<" , " << combined.sz << " )";
  }

  std::cout <<std::endl;

  std::cout << "Constructed implied Ask book of " << m_sym << std::endl;
  std::priority_queue<level*,std::vector<level *>, greater_by_prc> ask_pq;
  for (auto& db : dpdt_books) {
    if (db.second.atop != nullptr)
      ask_pq.push(db.second.atop);
  }

  count = 0;
  while(count < BOOKDEPTH && !ask_pq.empty()) {
    auto l = ask_pq.top();
    ask_pq.pop();
    if (l->next)
      ask_pq.push(l->next);

    level combined(*l);
    while (ask_pq.top()->prc == l->prc && !ask_pq.empty()) {
      auto ll = ask_pq.top();
      ask_pq.pop();
      if (ll->next)
        ask_pq.push(ll->next);
      combined.sz += ll->sz;
    }
    ++count;
    std::cout << "( " << combined.prc <<" , " << combined.sz << " )";
  }

  std::cout <<std::endl;
}

book_manager::book_manager(const std::vector<std::string>& files, 
  const std::unordered_map<std::string, std::vector<std::string>>& symmap) 
{
  for (auto& s : files) {
    auto p = new file_channel(s);
    m_d.add_channel(p);
  }

  m_d.set_cb(handle_data, this);

  for (auto& s: symmap) {
    std::cout << s.first << std::endl;
    m_imp_book.insert(std::make_pair(s.first, imp_book{s.first, this}));
    m_imp_cme_book[s.first] = book();

    for (auto& spread : s.second) {
      std::cout << spread << std::endl;
      auto ss = parse_spread_sym(spread);
      if (m_spread_book.find(ss) == m_spread_book.end())
        m_spread_book[ss] = book();

      if (ss.long_ == s.first) {
        if (m_outright_book.find(ss.short_) == m_outright_book.end())
          m_outright_book[ss.short_] = book();
        auto& m = m_outright2implied[ss.short_];
        m.push_back(s.first);
      } else {
        if (m_outright_book.find(ss.long_) == m_outright_book.end())
          m_outright_book[ss.long_] = book();
        auto& m = m_outright2implied[ss.long_];
        m.push_back(s.first);
      }
    }
  }
}

void book_manager::start() {

    // for(int i = 0; i < 1000 ; ++i) {
    //   m_d.process();
    // }
    while (m_d.process()){}
}

void book_manager::update_spread(const spread_sym& sym, 
    const level& l, side s, int lvl, operation op ) {

  if (m_spread_book.find(sym) == m_spread_book.end())
    return;
    
  switch (op) {
    case operation::NEW: 
      m_spread_book[sym].insert(l, s, lvl);
      break;
    case operation::MOD:
      m_spread_book[sym].modify(l, s, lvl);
      break;
    case operation::DEL:
      m_spread_book[sym].del(lvl, s);
      break;
    default:
      std::cerr <<"unknown operation" <<std::endl;
      return;
  }

  auto it = m_imp_book.find(sym.long_);
  if (it != m_imp_book.end()) {
    it->second.update_spread(sym, s);
  }

  it = m_imp_book.find(sym.short_);
  if (it != m_imp_book.end()) {
    it->second.update_spread(sym, s);
  }
}

void book_manager::update_outright(const std::string& sym, 
    const level& l, side s, int lvl, operation op) {

  if (m_outright_book.find(sym) == m_outright_book.end())
    return;

  switch (op) {
    case operation::NEW: 
      m_outright_book[sym].insert(l, s, lvl);
      break;
    case operation::MOD:
      m_outright_book[sym].modify(l, s, lvl);
      break;
    case operation::DEL:
      m_outright_book[sym].del(lvl, s);
      break;
    default:
      std::cerr <<"unknown operation" <<std::endl;
      return;
  }

  auto implied_syms = m_outright2implied[sym];
  for (auto& is : implied_syms) {
    auto it = m_imp_book.find(is);
    if (it != m_imp_book.end())
      it->second.update_outright(sym, s);
  }

}

void book_manager::update_cme_imp_book(const std::string& sym,
			 const level& l, side s, int lvl, operation op) 
{
  auto it = m_imp_cme_book.find(sym);
  if (it == m_imp_cme_book.end())
    return;

  if (lvl >2)
    return;

  auto& b  = it->second;
  switch(op) {
  case operation::NEW:
    b.insert(l,s,lvl);
    b.del(3,s);
    break;
  case operation::MOD:
    b.modify(l,s,lvl);
    break;
  case operation::DEL:
    b.del(lvl,s);
    break;
  default:
    std::cerr <<"unknown operation" <<std::endl;
    return;
  }
}


void book_manager::print() {
  std::cout <<"***********************************"<<std::endl;

  for(auto& b : m_imp_book)
    b.second.print_combined();

  std::cout <<std::endl;
  for(auto& b : m_imp_cme_book) {
    std::cout << "start of implied book from CME: " << b.first <<std::endl;
    b.second.print();
    std::cout << "end of implied book from CME: " << b.first <<std::endl;
  }
}
