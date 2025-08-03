#pragma once

#include <array>
#include <type_traits>

// simple static vector for trivial types
template<typename T, std::size_t Capacity>
requires std::is_trivial_v<T>
class StaticVector {
public:
  StaticVector() = default; 
  StaticVector(const StaticVector&) = default;
  StaticVector& operator=(const StaticVector&) = default;

  template<typename... Args>
  requires (std::conjunction_v<std::is_convertible<Args, T>...> && sizeof...(Args) <= Capacity)
  StaticVector(Args... args) : data_{args...}, sz_(sizeof...(args)) {};
  // StaticVector(std::initializer_list<T> args) : data_(args.begin(), args.end()), sz_(args.size()) {};

  T* begin() { return data_; }
  const T* begin() const { return data_; }

  T* end() { return data_ + sz_; }
  const T* end() const { return data_ + sz_; }

  size_t size() const { return sz_; }
  T front() const { return data_[0]; }
  T back() const { return data_[sz_-1]; }

  void push_back(const T elt) { data_[sz_++] = elt; }
  void clear() { sz_ = 0; } 
 
private:
  T data_[Capacity];
  // std::array<T, Capacity> data_;
  size_t sz_ = 0;
};

