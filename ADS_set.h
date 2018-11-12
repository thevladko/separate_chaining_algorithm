#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 7>
class ADS_set {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = key_type&;
  using const_reference = const key_type&;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = Iterator;
  using const_iterator = Iterator;
  using key_compare = std::less<key_type>;   // B+-Tree
  using key_equal = std::equal_to<key_type>; // Hashing
  using hasher = std::hash<key_type>;        // Hashing

private:
  struct Node {
    value_type key; //the key_type that will be used in the hash function
    Node* next; //next element that is linked to the current node
    Node(const value_type& element) : key{element}, next{nullptr} {}
    Node() : next{nullptr} {}
  };

  struct Head {
    Node* first_node;
    Head() : first_node{nullptr} {}
  };

  Head* table = nullptr; //initial table
  size_type sz{0}, table_size = N; //variable to keep track of the size and the maximum size of the table.
  float max_allowed {0.7};

  value_type last_erased;
  value_type last_unsuccessful;

  bool was_unsuccessful = false;

  size_type hash_idx(const key_type& k) const {
     return hasher{}(k) % table_size;
  }

  Node* find_pos(const key_type& k) const {
    size_type idx{hash_idx(k)};
    if (table[idx].first_node == nullptr) {
      return nullptr;
    }
    else {
      for(Node* node = table[idx].first_node; node != nullptr; node = node->next) {
        if (key_equal{}((node->key), k)) {
          return node;
        }
      }
    }
    return nullptr;
  }

  void insert_elem(const key_type& k) {
    check_capacity();
    size_type current_idx = hash_idx(k);
    if (find_pos(k) != nullptr) {
      return;
    } else {
      if (table[current_idx].first_node != nullptr) {
        Node* buffer = table[current_idx].first_node;
        table[current_idx].first_node = new Node(k);
        table[current_idx].first_node->next = buffer;
        ++sz;
      } else {
        table[current_idx].first_node = new Node(k);
        ++sz;
      }
    }
  }

  void check_capacity() {
    if((float(sz)/table_size) > max_allowed) {
      rehash(table_size * 2 + 1);
    }
  }

  void rehash(const size_type& new_sz) {
    size_type old_table_size{table_size};
    Head* new_table = new Head[new_sz]; //here...+ 1
    Head* old_table{table};
    table = new_table;
    sz = 0;
    table_size = new_sz;
    for(size_type i{0}; i < old_table_size; ++i){
      Node* buff = old_table[i].first_node;
      Node* help{nullptr};
      while(buff != nullptr){
        help = buff;
        insert_elem(buff->key);
        buff = buff->next;
        delete help;
      }
    }
    delete [] old_table;
  }

  void reserve(const size_type& n) {
    table = new Head[n]; //here...+ 1
  }

  void delete_head(const key_type& key) {
    Node* buff = table[hash_idx(key)].first_node;
    Node* help{nullptr};
    help = buff;
    buff = buff->next;
    delete help;
    table[hash_idx(key)].first_node = buff;
  }

  void delete_in_position(const key_type& key) {
    Node* buff = table[hash_idx(key)].first_node;
    Node* help{nullptr};
    while (buff != nullptr) {
      help = buff;
      buff = buff->next;
      if(key_equal{}((buff->key), key)){
        help->next = buff->next;
        delete buff;
        break;
      }
    }
  }

public:
  ADS_set() {
    reserve(table_size);
  }

  ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} {
    insert(ilist);
  }

  template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{} {
    insert(first, last);
  }

  ADS_set(const ADS_set& other) {
    reserve(other.table_size);
    for(const auto& elem : other) {
      insert_elem(elem);
    }
  }

   ~ADS_set() {
     for(size_type i{0}; i < table_size; ++i){
       Node* buff = table[i].first_node;
       Node* help{nullptr};
       while(buff != nullptr){
         help = buff;
         buff = buff->next;
         delete help;
       }
     }
     delete[] table;
   }

  ADS_set& operator=(const ADS_set& other) {
    if (this == &other) { return *this; }
    clear();
    rehash(other.table_size);
    for(const auto& elem : other) {
      insert_elem(elem);
    }
    return *this;
  }

  ADS_set& operator=(std::initializer_list<key_type> ilist) {
    clear();
    insert(ilist);
    return *this;
  }

  size_type size() const {
    return sz;
  }

  bool empty() const {
    return sz == 0;
  }

  size_type count(const key_type& key) const {
    return !(!find_pos(key));
  }

  iterator find(const key_type& key) const {
    if(Node* node{find_pos(key)}){
      return iterator{&(table[hash_idx(key)]), this, node};
    }
    return end();
  }

  void clear() {
    ADS_set buff;
    swap(buff);
  }

  void swap(ADS_set& other) {
    using std::swap;
    swap(sz, other.sz);
    swap(table, other.table);
    swap(table_size, other.table_size);
    swap(max_allowed, other.max_allowed);
  }

  void insert(std::initializer_list<key_type> ilist) {
    if(ilist.size()){
      for(const auto& elem : ilist) {
        insert_elem(elem);
      }
    }
  }

  std::pair<iterator, bool> insert(const key_type& key) {
    if(Node* node{find_pos(key)}) {
      return std::make_pair(iterator{&(table[hash_idx(key)]), this, node}, false);
    }
    insert_elem(key);
    return std::make_pair(iterator{&(table[hash_idx(key)]), this, find_pos(key)}, true);
  }

  template<typename InputIt> void insert(InputIt first, InputIt last) {
    for(auto it = first; it != last; ++it){
      insert_elem(*it);
    }
  }

  size_type erase(const key_type& key) {
    if (count(key) == 1) {
      if(key_equal{}((table[hash_idx(key)].first_node->key), key)) {
        delete_head(key);
      } else {
        delete_in_position(key);
      }
      last_erased = key;
      --sz;
      return 1;
    }
    was_unsuccessful = true;
    last_unsuccessful = key;
    return 0;
  }

  const_iterator begin() const {
    if(!sz){
      return end();
    }
    return const_iterator{table, this};
  }

  const_iterator end() const {
    return const_iterator{table + table_size, this};
  }

  void dump(std::ostream& o = std::cerr) const {
    for (size_type i = 0; i < table_size; ++i) {
      o << i << ": ";
      Head* head = &(table[i]);
      for (Node* node = head->first_node; node != nullptr; node = node->next) {
        o << node->key;
        if(node->next != nullptr) {
          o << " -> ";
        }
      }
      o << std::endl;
    }
  }

  friend bool operator==(const ADS_set& lhs, const ADS_set& rhs) {
    if(lhs.sz != rhs.sz){return false;}
    for(auto it = lhs.begin(); it != lhs.end(); ++it){
      if(rhs.count(*it) == 0){
        return false;
      }
    }
    return true;
  }

  friend bool operator!=(const ADS_set& lhs, const ADS_set& rhs) {
    return !(lhs == rhs);
  }

  value_type get_last_erased(){
    return last_erased;
  }

  value_type get_last_unsuccessful() {
    return last_unsuccessful;
  }

  size_type z() const {
    if(last_erased == -1){
      throw std::runtime_error("no elements have been erased yet!");
    }
    int counter = 0;
    for(int i = 0; i < table_size; i++) {
      Head* head = &(table[i]);
      for(Node* node = head->first_node; node != nullptr; node = node->next){
        if(std::less<key_type>{}(node->key, last_erased)){
          counter++;
        }
      }
    }
    return counter;
  }

  key_type y() const {
    value_type min;
    for(int i = 0; i < table_size; i++) {
      Head* head = &(table[i]);
      for(Node* node = head->first_node; node != nullptr; node = node->next){
        if(std::less<key_type>{}(last_unsuccessful, node->key)){
          min = node->key;
        }
      }
    }
    for(int i = 0; i < table_size; i++) {
      Head* head = &(table[i]);
      for(Node* node = head->first_node; node != nullptr; node = node->next){
        if(std::less<key_type>{}(node->key, min) && std::less<key_type>{}(last_unsuccessful, node->key)){
          min = node->key;
        }
      }
    }
    return min;
  }

  const_iterator x(const key_type& k) const {
    value_type max;
    for(int i = 0; i < table_size; i++){
      Head* head = &(table[i]);
      for(Node* node = head->first_node; node != nullptr; node = node->next){
        if(std::less<key_type>{}(node->key, k)){
          max = node->key;
        }
      }
    }

    for(int i = 0; i < table_size; i++){
      Head* head = &(table[i]);
      for(Node* node = head->first_node; node != nullptr; node = node->next){
        if(std::less<key_type>{}(node->key, k) && !std::less<key_type>{}(node->key, max)){
          max = node->key;
        }
      }
    }
    return const_iterator{&(table[hash_idx(max)]), this, find_pos(max)};
  }


  friend bool func(const ADS_set& a, const ADS_set& b){
    if(a.size() == 0 && b.size() == 0){
      return true;
    }
    int counter = 0;
    int counter2 = 0;
    for(int i = 0; i < a.table_size; i++){
      Head* head = &(a.table[i]);
      for(Node* node = head->first_node; node != nullptr; node = node->next){
        if(b.count(node->key) == 1){
          counter++;
        }
      }
    }

    for(int i = 0; i < b.table_size; i++){
      Head* head = &(b.table[i]);
      for(Node* node = head->first_node; node != nullptr; node = node->next){
        if(a.count(node->key) == 1){
          counter2++;
        }
      }
    }

    if((counter * 100 / a.size()) >= 90 && (counter2 * 100 / b.size()) >= 90){
      return true;
    }
    return false;
  }
};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type&;
  using pointer = const value_type*;
  using iterator_category = std::forward_iterator_tag;

private:
    Head* first;
    Head* last;
    Node* pos;
    const ADS_set* set;

    Node* find_pos_iter(){
      if(first == last){
        return nullptr;
      }
      for(size_t i = 0; i < set->table_size; ++i){
        if(set->table[i].first_node != nullptr){
          first = set->table + i;
          return set->table[i].first_node;
        }
      }
      return nullptr;
    }

public:
  explicit Iterator(){}
  explicit Iterator(Head* head, const ADS_set* s) {
    this->first = head;
    this->set = s;
    this->last = s->table + s->table_size;
    pos = find_pos_iter();
  }
  explicit Iterator(Head* head, const ADS_set* s, Node* node) {
    this->first = head;
    this->pos = node;
    this->set = s;
    this->last = s->table + s->table_size;
  }
  reference operator*() const {
    return pos->key;
  }
  pointer operator->() const {
    return &(pos->key);
  }
  Iterator& operator++() {
    if(pos->next != nullptr){
      pos = pos->next;
    } else {
        do {
          ++first;
        } while(first != last && first->first_node == nullptr);
      if(first == last){ pos = nullptr;}
      else if (first->first_node != nullptr) { pos = first->first_node;}
    }
    return *this;
  }
  Iterator operator++(int) {
    auto old = *this;
    ++*this;
    return old;
  }
  friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
    if(lhs.pos != nullptr && rhs.pos != nullptr){
      return (lhs.pos == rhs.pos);
    }
    return lhs.first == rhs.first;
  }
  friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
    if(lhs.pos != nullptr && rhs.pos != nullptr){
      return (lhs.pos != rhs.pos);
    }
    return lhs.first != rhs.first;
  }
};

template <typename Key, size_t N> void swap(ADS_set<Key,N>& lhs, ADS_set<Key,N>& rhs) { lhs.swap(rhs); }

#endif // ADS_SET_H
