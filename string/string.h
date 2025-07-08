#include <iostream>
#include <cstring>

class String {
  size_t cap_;
  size_t len_;
  char* str_;

  void increase_cap(size_t new_cap) {
    if (new_cap <= cap_) return;
    cap_ = std::max(new_cap, cap_ * 2);
    char* new_str = new char[cap_ + 1];
    std::copy(str_, str_ + len_, new_str);
    new_str[len_] = '\0';
    delete[] str_;
    str_ = new_str;
  }


public:
  String(const char c)
  : cap_(1)
  , len_(1)
  , str_(new char[cap_ + 1])
  {
    str_[0] = c;
    str_[len_] = '\0';
  }

  String(size_t len_, char c)
  : cap_(len_)
  , len_(len_)
  , str_(new char[cap_ + 1])
  {
    std::fill(str_, str_ + len_, c);
    str_[len_] = '\0';
  }

  String(const char* string)
  : cap_(strlen(string))
  , len_(strlen(string))
  , str_(new char[cap_ + 1])
  {
    strcpy(str_, string);
    str_[len_] = '\0';
  }

  String(const String& other)
  : cap_(other.cap_)
  , len_(other.len_)
  , str_(new char[cap_ + 1])
  {
    std::copy(other.str_, other.str_ + other.len_, str_);
    str_[len_] = '\0';
  }

  String() : cap_(0), len_(0), str_(new char[1]) {
    str_[0] = '\0';
  }

  ~String() {
    delete[] str_;
  }

  String& operator=(const String& other) {
    if (this != &other) {
      delete[] str_;
      cap_ = other.len_;
      len_ = other.len_;
      str_ = new char[other.len_ + 1];
      if (other.str_) {
        std::copy(other.str_, other.str_ + other.len_, str_);
      }
      str_[len_] = '\0';
    }
    return *this;
  }

  char& operator[](size_t ind) { return str_[ind]; }

  const char& operator[](size_t ind) const { return str_[ind]; }

  size_t length() const { return len_; }

  size_t size() const { return length(); }

  size_t capacity() const { return cap_; }

  void push_back(char c) {
    increase_cap(len_ + 1);
    str_[len_] = c;
    ++len_;
    str_[len_] = '\0';
  }

  void pop_back() {
    if (len_ > 0) {
      --len_;
      str_[len_] = '\0';
    }
  }

  const char& front() const { return str_[0]; }

  char& front() { return str_[0]; }

  const char& back() const { return str_[len_-1]; }

  char& back() { return str_[len_-1]; }

  String& operator+=(const String& other) {
    increase_cap(len_ + other.len_);
    std::copy(other.str_, other.str_ + other.len_, str_ + len_);
    len_ += other.len_;
    str_[len_] = '\0';
    return *this;
  }

  String& operator+=(char c) {
    push_back(c);
    return *this;
  }

  long long comp(const String& substr, size_t i) const {
    if (str_[i] == substr.str_[0]) {
      long long j = 1;
      bool is_cmp = true;
      while (j < static_cast<long long>(substr.len_)) {
        if (str_[i + j] != substr.str_[j]) {
          is_cmp = false;
          break;
        }
        ++j;
      }
      if (is_cmp) return i;
    }
    return -1;
  }

  size_t find(const String& substr) const {
    if (substr.len_ > len_) return len_;
    for (size_t i = 0; i <= len_ - substr.len_; ++i) {
      long long j = comp(substr, i);
      if (j != -1) {
        return j;
      }
    }
    return len_;
  }

  size_t rfind(const String& substr) const {
    if (substr.len_ > len_) return len_;
    for (long long i = len_ - substr.len_; i >= 0; --i) {
      long long j = comp(substr, i);
      if (j != -1) {
        return j;
      }
    }
    return len_;
  }

  String substr(size_t start, size_t count) const {
    if (start > len_) start = len_;
    count = (start + count > len_) ? len_ - start : count;
    String new_str(count, '\0');
    std::copy(str_ + start, str_ + start + count, new_str.str_);
    new_str.str_[count] = '\0';
    new_str.len_ = count;
    return new_str;
  }

  bool empty() const { return len_ == 0; }

  void clear() {
    len_ = 0; 
    str_[len_] = '\0';
  }

  void shrink_to_fit() {
    if (len_ < cap_) {
      char* new_str = new char[len_ + 1];
      std::copy(str_, str_ + len_, new_str);
      new_str[len_] = '\0';
      delete[] str_;
      str_ = new_str;
      cap_ = len_;
    }
  }

  const char* data() const { return str_; }

  char* data() { return str_; }
};

bool operator==(const String& first, const String& second) {
  return strcmp(first.data(), second.data()) == 0;
}

bool operator!=(const String& first, const String& second) {
  return !(first == second);
}

bool operator<(const String& first, const String& second) {
  return strcmp(first.data(), second.data()) < 0;
}

bool operator>(const String& first, const String& second) {
  return second < first;
}

bool operator>=(const String& first, const String& second) {
  return !(first < second);
}

bool operator<=(const String& first, const String& second) {
  return second >= first;
}

String operator+(const String& first, const String& second) {
  String new_str(first);
  new_str += second;
  return new_str;
}

std::ostream& operator<<(std::ostream& os, const String& string) {
  for (size_t i = 0; i < string.length(); ++i) {
    os << string[i];
  }
  return os;
}

std::istream& operator>>(std::istream& is, String& string) {
  string.clear();
  char c;
  while (is.get(c) && c == ' ') {}
  if (c == '\n') return is;
  if (is) {
    do {
      string.push_back(c);
    } while (is.get(c) && c != ' ' && c != '\n');
  }
  return is;
}
