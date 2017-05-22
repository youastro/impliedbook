#include "ib_builder.hpp"
#include <iostream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

namespace {

const int BOOKDEPTH = 10;


spread_sym parse_spread_sym(const std::string& sym) {
  boost::char_separator<char> sep{"-"};
  boost::tokenizer<boost::char_separator<char>> tok(sym, sep);
  auto beg=tok.begin();
  return {*beg, *(++beg)};
}

double add(double a, double b)  {
  return a+b;
}

double subtract(double a, double b) {
  return a-b;
}

void handle_data(char* data, size_t len, void* userdata) {
  auto p = reinterpret_cast<book_manager*>(userdata);
  char a = 1;
  boost::char_separator<char> sep{&a};
  boost::tokenizer<boost::char_separator<char>> tok(std::string(data), sep);
  auto beg=tok.begin();

  operation op = operation::NONE;
  side s = side::NONE;
  double prc = 0;
  int sz = 0;
  std::string sym;
  spread_sym ssym;
  int lvl;
  bool ready = false;
  while( beg!=tok.end()){
    if(ready) {
      level l(prc, sz);
      //at most one of them will have effects
      if(!sym.empty())
        p->update_outright(sym, l, s, lvl, op);
      else
        p->update_spread(ssym, l, s, lvl, op);
      ready = false;
      op = operation::NONE;
      s = side::NONE;
      prc = 0;
      sz = 0;
      sym = "";
      ssym.long_ = "";
      ssym.short_ = "";
    }

    if (boost::starts_with(*beg,"35=")) {
      if (*beg != "35=X")
        return;
    }
    if (boost::starts_with(*beg,"279=")) {
      int tmp;
      auto substring = std::string(*beg).substr(4);
      std::stringstream(substring) >> tmp;
      //auto tmp  = boost::lexical_cast<int>(std::string(*beg).substr(4,1));
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
    } else if (boost::starts_with(*beg,"269=")) {
      auto tmp  = boost::lexical_cast<std::string>(std::string(*beg).substr(4));
      if(tmp == "0") s = side::BID;
      else if(tmp == "1") s = side::ASK;
      else if (tmp == "E") {}
      else if (tmp == "F") {}
      else {
        while (beg != tok.end()) {
          if (boost::starts_with(*beg,"1023="))
            break;
          ++beg;
        }
        if (beg == tok.end()) return;
      }
    } else if (boost::starts_with(*beg,"55=")) {
      auto tmp  = boost::lexical_cast<std::string>(std::string(*beg).substr(3));
      auto pos = tmp.find("-");
      if (pos != std::string::npos && pos != tmp.size()-1)
        ssym = parse_spread_sym(tmp);
      else
        sym = tmp;
    } else if (boost::starts_with(*beg,"270=")) {
      auto substring = std::string(*beg).substr(4);
      std::stringstream(substring) >> prc;
      //prc  = boost::lexical_cast<double>(std::string(*beg).substr(4));
    } else if (boost::starts_with(*beg,"271=")) {
      auto substring = std::string(*beg).substr(4);
      std::stringstream(substring) >> sz;
      //sz  = boost::lexical_cast<int>(std::string(*beg).substr(4));
    } else if (boost::starts_with(*beg,"1023=")) {
      auto substring = std::string(*beg).substr(5);
      std::stringstream(substring) >> lvl;
      //lvl  = boost::lexical_cast<int>(std::string(*beg).substr(5));
      ready = true;
    }

    ++beg;
  }

}

} //end of anonymous namespace

bool level::betterthan(const level& l, side s) {
  return s == side::BID ? prc > l.prc : prc < l.prc;
}

bool level::betterequal(const level& l, side s) {
  return s == side::BID ? prc >= l.prc : prc <= l.prc;
}

book::book() : atop(nullptr), btop(nullptr)
{}

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

void book::clear() {

  auto atop_cpy = atop;
  while(atop_cpy) {
    auto tmp = atop_cpy;
    atop_cpy = atop_cpy->next;
    delete tmp;
  }
  atop = nullptr;

  auto btop_cpy = btop;
  while(btop_cpy) {
    auto tmp = btop_cpy;
    btop_cpy = btop_cpy->next;
    delete tmp;
  }
  btop=nullptr;
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
  std::cout << "book new: " << lvl << " " << static_cast<int>(s) <<std::endl;
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
  std::cout << "book modify: " << lvl << " " <<static_cast<int>(s) <<std::endl;
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
  std::cout << "book del: " << lvl << " " <<static_cast<int>(s)<<std::endl;
  auto& top = (s == side::BID ? btop : atop);
  if (!top){
    std::cout << "book empty, nothing to be deleted, lvl=" << lvl << std::endl;
    return;
  }

  auto curlvl = top;
  if(lvl == 1 ) {
    top = top->next;
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

// book imp_book::build_imp_book(book* ob, book* sb, unsigned short lvl, imply_out_type t)
// {
//   book ib;
//   if (t == imply_out_type.OPPOSITE) {
//     ib.atop = build_imp_levels(ob->atop, sb->btop, lvl, subtract);
//     ib.btop = build_imp_levels(ob->btop, sb->atop, lvl, subtract);
//   } else {
//     ib.atop = build_imp_levels(ob->atop, sb->atop, lvl, add);
//     ib.btop = build_imp_levels(ob->btop, sb->btop, lvl, add);
//   }

//   return ib;
// }

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
    auto& ibtop = (s == side::BID ? ob.btop : ob.atop);
    ib.clear(ibtop);

    ibtop = build_imp_levels(obtop, sbtop, BOOKDEPTH, add);

  } else {

    auto& ob = ibb->m_outright_book[ssym.long_];
    auto obtop = (s == side::BID ? ob.atop : ob.btop);

    auto& ib = dpdt_books[ssym.long_];
    auto& ibtop = (s == side::BID ? ib.btop : ib.atop);
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

void imp_book::print() {
  std::cout << "start of implied book: " << m_sym <<std::endl;
  for (auto db : dpdt_books) {
    std::cout << "dependent sym" << db.first <<std::endl;
    db.second.print();
  }
  std::cout << "end of implied book: " << m_sym <<std::endl;
}

book_manager::book_manager(const std::vector<std::string>& files, 
  const std::unordered_map<std::string, std::vector<std::string>>& symmap) 
{
  for (auto& s : files) {
    auto p = new file_channel(s);
    m_d.add_channel(p);
  }

  m_d.set_cb(handle_data, this);

  std::cout << "inside book_manager" <<std::endl;
  for (auto& s: symmap) {
    std::cout << s.first << std::endl;
    m_imp_book.insert(std::make_pair(s.first, imp_book{s.first, this}));

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
  std::cout << "booksize" << m_outright_book.size() <<" " << m_spread_book.size() << " " 
    << m_imp_book.size() << std::endl;
}

void book_manager::start() {
  for (int i = 0; i < 10000 ; ++i) {
    m_d.process();
    print();
  }
  //print();
}

void book_manager::update_spread(const spread_sym& sym, 
    const level& l, side s, int lvl, operation op ) {

  std::cout << sym.long_ <<"-"<< sym.short_ << " " << l.prc <<" " <<l.sz << " "
    << static_cast<int>(s) << " " << lvl << " " << static_cast<int>(op) <<std::endl;

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
  //m_spread_book[sym].update(l, s);

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

  std::cout << sym <<" " << l.prc <<" " <<l.sz << " "
    << static_cast<int>(s) << " " << lvl << " " << static_cast<int>(op) <<std::endl;

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

  //m_outright_book[sym].update(l, s);

  auto implied_syms = m_outright2implied[sym];
  for (auto& is : implied_syms) {
    auto it = m_imp_book.find(is);
    if (it != m_imp_book.end())
      it->second.update_outright(sym, s);
  }

}

void book_manager::print() {
  std::cout <<"***********************************"<<std::endl;
  for(auto b : m_outright_book) {
    
    std::cout << "start of outright book: " << b.first <<std::endl;
    b.second.print();
    std::cout << "end of outright book: " << b.first <<std::endl;
  }

  for(auto b : m_spread_book) {
    std::cout << "start of spread book: " << b.first.long_ <<"-"<<b.first.short_ <<std::endl;
    b.second.print();
    std::cout << "end of spread book: " << b.first.long_ <<"-"<<b.first.short_  <<std::endl;
  }

  for(auto b : m_imp_book) {
    b.second.print();
  }
  std::cout <<"***********************************"<<std::endl;
}