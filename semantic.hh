#ifndef SEMANTIC_HH
#define SEMANTIC_HH

#include <vector>
#include <array>

template<typename Source, typename Dest, int array_size>
class sarray {
 public:
  using array_type = std::array<Dest, array_size>;
  using size_type = array_type::size_type;
  explicit sarray(const Dest& value) {
    a.fill(value);
  }
  sarray() {
    a.fill(Dest(0));
  }
  Dest& operator[](const Source& index) {
    return a[index];
  }
  const Dest& operator[](const Source& index) const {
    return a[index];
  }
  size_type size() {
    return a.size();
  }
 private:
  array_type a;
};

template<typename Source, typename Dest>
class svector {
 public:
  using size_type = std::vector<Dest>::size_type;
  using iterator = std::vector<Dest>::iterator;
  explicit svector(size_type size) : v(size) {
  }
  svector(size_type size, const Dest& value) : v(size, value) {
  }
  svector() {
  }
  Dest& operator[](const Source& index) {
    return v[index];
  }
  const Dest& operator[](const Source& index) const {
    return v[index];
  }
  size_type size() {
    return v.size();
  }
  void push_back(const Dest& value) {
    v.push_back(value);
  }
  iterator begin() {
    return begin(v);
  }
  iterator end() {
    return end(v);
  }
  iterator begin() const {
    return begin(v);
  }
  iterator end() const {
    return end(v);
  }
 private:
  std::vector<Dest> v;
};

template<typename T>
class Index {
 public:
  constexpr explicit Index(int index) : index(index) {
  }
  constexpr operator int() {
    return index;
  }
  constexpr operator int() const {
    return index;
  }
  constexpr Index& operator^=(int i) {
    index ^= i;
    return *this;
  }
  constexpr Index& operator/=(int i) {
    index /= i;
    return *this;
  }
  constexpr Index& operator+=(int i) {
    index += i;
    return *this;
  }
  constexpr Index& operator--() {
    index--;
    return *this;
  }
  constexpr Index operator--(int) {
    Index tmp(*this);
    operator--();
    return tmp;
  }
  constexpr Index& operator++() {
    index++;
    return *this;
  }
  constexpr Index operator++(int) {
    Index tmp(*this);
    operator++();
    return tmp;
  }
 private:
  int index;
};

#define SEMANTIC_INDEX(name, prefix) \
class name : public Index<name> { \
 public: \
  constexpr explicit name(int index) : Index(index) { \
  } \
  constexpr name() : Index(0) { \
  } \
}; \
constexpr name operator"" _##prefix(unsigned long long int index) { \
  return name{static_cast<int>(index)}; \
}

#endif
