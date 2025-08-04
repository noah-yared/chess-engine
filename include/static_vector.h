#pragma once

#include <array>
#include <type_traits>

// simple static vector for trivial types
template<typename T, std::size_t Capacity>
requires std::is_trivial_v<T>
class StaticVector {
public:
  StaticVector() noexcept = default; 
  StaticVector(const StaticVector&) noexcept = default;
  StaticVector& operator=(const StaticVector&) noexcept = default;

  template<typename... Args>
  requires (std::conjunction_v<std::is_convertible<Args, T>...> && sizeof...(Args) <= Capacity)
  StaticVector(Args... args) noexcept : data_{args...}, sz_(sizeof...(args)) {};
  // StaticVector(std::initializer_list<T> args) : data_(args.begin(), args.end()), sz_(args.size()) {};

  T* begin() noexcept { return data_; }
  const T* begin() const noexcept { return data_; }

  T* end() noexcept { return data_ + sz_; }
  const T* end() const noexcept { return data_ + sz_; }

  size_t size() const noexcept { return sz_; }
  T front() const noexcept { return data_[0]; }
  T back() const noexcept { return data_[sz_-1]; }

  void push_back(const T elt) noexcept { data_[sz_++] = elt; }
  void clear() noexcept { sz_ = 0; } 
 
private:
  T data_[Capacity];
  // std::array<T, Capacity> data_;
  size_t sz_ = 0;
};

