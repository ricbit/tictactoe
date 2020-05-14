#ifndef SEMANTIC_HH
#define SEMANTIC_HH

template<typename T>
class Index {
 public:
  constexpr explicit Index(int index) : index(index) {
  }
  operator int() {
    return index;
  }
  operator int() const {
    return index;
  }
  Index& operator/=(int i) {
    index /= i;
    return *this;
  }
  Index& operator+=(int i) {
    index += i;
    return *this;
  }
  Index& operator++() {
    index++;
    return *this;
  }
  Index operator++(int) {
    Index tmp(*this);
    operator++();
    return tmp;
  }
 private:
  int index;
};

class Position : public Index<Position> {
 public:
  constexpr explicit Position(int index) : Index(index) {
  }
  constexpr Position() : Index(0) {
  }
};

constexpr Position operator"" _pos(unsigned long long int pos) {
  return Position{static_cast<int>(pos)};
}

class Side : public Index<Side> {
 public:
  constexpr explicit Side(int index) : Index(index) {
  }
  constexpr Side() : Index(0) {
  }
};

constexpr Side operator"" _side(unsigned long long int index) {
  return Side{static_cast<int>(index)};
}

#endif
