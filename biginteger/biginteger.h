#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <stdlib.h>

using std::vector;

class BigInteger {
  vector<int64_t> digits_;
  static const int64_t n_digits_ = 8;
  static const int64_t base_ = 100000000;
  bool is_negative_ = false;

  void CheckForZero() {
    if (digits_.empty() or (digits_.size() == 1 && digits_.front() == 0)) {
      is_negative_ = false;
    }
  }

  void removeLeadingZeros() {
    while (digits_.size() > 1 && digits_.back() == 0) {
      digits_.pop_back();
    }
    CheckForZero();
  }

  BigInteger Add(const BigInteger& first, const BigInteger& second) {
    const vector<int64_t>& num1 = first.digits_;
    const vector<int64_t>& num2 = second.digits_;
    size_t max_size = std::max(num1.size(), num2.size()) + 1;
    BigInteger result(first);
 
    if (max_size > result.digits_.size()) {
      result.digits_.resize(max_size + 1);
    }

    int64_t temp = 0;
    for (size_t i = 0; i < max_size || temp; ++i) {
      result.digits_[i] += temp;
      if (i < num2.size()) {
        result.digits_[i] += num2[i];
      }
      temp = result.digits_[i] / result.base_;
      result.digits_[i] %= result.base_;
    }
    result.removeLeadingZeros();
    return result;
  }

  BigInteger Difference(const BigInteger& first, const BigInteger& second) {
    const vector<int64_t>& num1 = first.digits_;
    const vector<int64_t>& num2 = second.digits_;
    BigInteger result(first);

    int64_t temp = 0;
    for (size_t i = 0; i < std::min(num1.size(), num2.size()) || temp; ++i) {
      if (i >= num2.size()) {
        result.digits_[i] -= temp;
      } else {
        result.digits_[i] -= (temp + num2[i]);
      }
      if (result.digits_[i] < 0) {
        result.digits_[i] += result.base_;
        temp = 1;
      } else {
        temp = 0;
      }
    }
    result.removeLeadingZeros();
    return result;
  }

  BigInteger Multiply(const BigInteger& first, const BigInteger& second) {
    if (second == 0 || first == 0) {
      return 0;
    }
    BigInteger ans(first);
    const vector<int64_t>& num1 = ans.digits_;
    const vector<int64_t>& num2 = second.digits_;
    vector<int64_t> result(num1.size() + num2.size() + 1, 0);

    if (first.is_negative_ == second.is_negative_) {
      ans.is_negative_ = false;
    } else {
      ans.is_negative_ = true;
    }

    for (size_t i = 0; i < num1.size(); ++i) {
      int64_t temp = 0;
      for (size_t j = 0; j < num2.size(); ++j) {
        result[i + j] += num1[i] * num2[j] + temp;
        temp = result[i + j] / ans.base_;
        result[i + j] %= ans.base_;
      }
      result[i + num2.size()] += temp;
    }
    
    ans.digits_ = std::move(result);
    ans.removeLeadingZeros();
    return ans;
  }

  BigInteger Divide(const BigInteger& first, const BigInteger& second) {
    if (second == 0) {
      throw std::runtime_error("Division by zero");
    }
    if (first == 0) {
      return 0;
    }
    if (first.abs() < second.abs()) return 0;

    BigInteger ans;
    if (first.is_negative_ == second.is_negative_) {
      ans.is_negative_ = false;
    } else {
      ans.is_negative_ = true;
    }
    BigInteger temp;

    BigInteger divised = first.abs();
    BigInteger divisior = second.abs();

    for (int64_t i = divised.digits_.size() - 1; i >= 0; --i) {
      temp.digits_.insert(temp.digits_.begin(), divised.digits_[i]);
      temp.removeLeadingZeros();
      int64_t l = 0;
      int64_t r = temp.base_;
      int64_t x = 0;
      
      while (l <= r) {
        int64_t m = (l + r) / 2;
        BigInteger t = Multiply(divisior, m);
        if (t < temp || t == temp) {
          x = m;
          l = m + 1;
        } else {
          r = m - 1;
        }
      }
      ans.digits_.push_back(x);
      temp -= Multiply(divisior, x);
    }
    std::reverse(ans.digits_.begin(), ans.digits_.end());
    ans.removeLeadingZeros();
    return ans;
  }

 public:
  BigInteger(int64_t x) {
    if (x == 0) {
      digits_.push_back(0);
    }
    if (x < 0) {
      is_negative_ = true;
      x = std::abs(x);
    }
    while(x > 0) {
      digits_.push_back(x % base_);
      x /= base_;
    }
  }

  BigInteger(const BigInteger& other) = default;

  BigInteger(const std::string s, size_t size) {
    int64_t temp = 0;
    int64_t start = 0;
    if (size == 0) {
      is_negative_ = false;
    } else {
      if (s[start] == '-') {
        ++start;
        is_negative_ = true;
        if (s[start] == '0' && size - start == 1) {
          is_negative_ = false;
        }
      } else {
        is_negative_ = false;
      }
      int64_t len = 1;
      for (int64_t i = size - 1; i >= start; --i) {
        if (len == base_) {
          len = 1;
          digits_.push_back(temp);
          temp = 0;
        }
        temp += (s[i] - '0') * len;
        len *= 10;
      }
      digits_.push_back(temp);
      removeLeadingZeros();
    }
  }

  BigInteger(const std::string s) : BigInteger(s, s.length()) {}

  BigInteger() : is_negative_(false) {};

  ~BigInteger() = default;

  BigInteger& operator=(const BigInteger &other) = default;

  BigInteger operator-() const {
    BigInteger big_int(*this);
    if (big_int == 0) return big_int;
    big_int.is_negative_ = !is_negative_;
    return big_int;
  }

  BigInteger& operator++() {
    *this += 1;
    return *this;
  }

  BigInteger operator++(int) {
    BigInteger temp(*this);
    ++(*this);
    return temp;
  }

  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }

  BigInteger operator--(int) {
    BigInteger temp(*this);
    --(*this);
    return temp;
  }

  explicit operator bool() const {
    return !(digits_.size() == 1 && digits_[0] == 0);
  }

  std::string toString() const {
    if (digits_.empty()) {
      return "0";
    }
    std::ostringstream oss;
    if (is_negative_)
      oss << '-';
    oss << digits_.back();
    for (int64_t i = static_cast<int64_t>(digits_.size()) - 2; i >= 0; --i)
      oss << std::setw(n_digits_) << std::setfill('0') << digits_[i];
    return oss.str();
  }

  BigInteger& operator+=(const BigInteger& other) {
    BigInteger result;
    if (is_negative_ == other.is_negative_) {
      result = Add(*this, other);
      result.is_negative_ = is_negative_;
    } else {
      if (CompareByModul(*this, other)) {
        result = Difference(other, *this);
        result.is_negative_ = other.is_negative_;
      } else {
        result = Difference(*this, other);
        result.is_negative_ = is_negative_;
      }
    }
    result.removeLeadingZeros();
    *this = result;
    return *this;
  }

  BigInteger& operator-=(const BigInteger& other) {
    return (*this) += (-other);
  }

  BigInteger& operator*=(const BigInteger& other) {
    *this = Multiply(*this, other);
    return *this;
  }

  BigInteger& operator/=(const BigInteger& other) {
    *this = Divide(*this, other);
    return *this;
  }

  BigInteger& operator%=(const BigInteger& other) {
    *this -= Multiply(Divide(*this, other), other);
    return *this;
  }

  bool IsNegative() const { return is_negative_; }

  BigInteger abs() const {
    if (!this->is_negative_) return *this;
    BigInteger temp(*this);
    temp.is_negative_ = false;
    return temp;
  }

  static bool CompareByModul(const BigInteger& first, const BigInteger& second) {
    if (first.digits_.size() < second.digits_.size()) {
      return true;
    }
    if (first.digits_.size() > second.digits_.size()) {
      return false;
    }
    for (int64_t i = first.digits_.size() - 1; i >= 0; --i) {
      int64_t x = first.digits_[i];
      int64_t y = second.digits_[i];
      if (x < y) {
        return true;
      } else if (x > y){
        return false;
      }
    }
    return false;
  }
  friend bool operator<(const BigInteger& first, const BigInteger& second);
  friend bool operator==(const BigInteger& first, const BigInteger& second);
};


BigInteger operator+(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result += second;
  return result;
}

BigInteger operator-(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result -= second;
  return result;
}

BigInteger operator*(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result *= second;
  return result;
}

BigInteger operator/(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result /= second;
  return result;
}

BigInteger operator%(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result %= second;
  return result;
}

bool operator<(const BigInteger& first, const BigInteger& second) {
  if (first.IsNegative() == second.IsNegative()) {
    if (!first.IsNegative()) {
      return BigInteger::CompareByModul(first, second);
    } else {
      return -(second) < -(first);
    }
  } else {
    return first.IsNegative();
  }
};

bool operator>(const BigInteger& first, const BigInteger& second) {
  return second < first;
}

bool operator>=(const BigInteger& first, const BigInteger& second) {
  return !(first < second);
}

bool operator<=(const BigInteger& first, const BigInteger& second) {
  return !(first > second);
}

bool operator==(const BigInteger& first, const BigInteger& second) {
  return (first <= second) && (first >= second);
}

bool operator!=(const BigInteger& first, const BigInteger& second) {
  return !(first == second);
}

std::ostream& operator<<(std::ostream& os, const BigInteger& big_int) {
  os << big_int.toString();
  return os;
}

BigInteger operator "" _bi(unsigned long long arg) {
  return BigInteger(arg);
}

BigInteger operator "" _bi(const char* str, size_t size) {
  return BigInteger(std::string(str), size);
}

std::istream& operator>>(std::istream& is, BigInteger& big_int) {
  std::string s;
  is >> s;
  big_int = BigInteger(s);
  return is;
}

BigInteger gcd(const BigInteger& first, const BigInteger& second) {
  BigInteger a;
  BigInteger b;
  if (first > second) {
    a = first.abs();
    b = second.abs();
  } else {
    a = second.abs();
    b = first.abs();
  }
  BigInteger r;
  do {
    r = a % b;
    a = b;
    b = r;
  } while (r != 0);
  return a;
}

class Rational {
  BigInteger numerator;
  BigInteger denominator;

  void norm() {
    if (denominator == 0) throw std::runtime_error("Denominator cannot be zero");
    if (numerator == 0) {
      denominator = 1;
      return;
    }
    if (denominator < 0) {
      numerator = -numerator;
      denominator = -denominator;
    }
    BigInteger k = gcd(numerator, denominator);
    numerator /= k;
    denominator /= k;
  }

 public:
  Rational(const BigInteger& numerator, const BigInteger& denominator = 1)
           : numerator(numerator),
             denominator(denominator)
  {
    norm();
  }

  Rational(int64_t numerator, int64_t denominator = 1)
          : numerator(numerator),
            denominator(denominator) 
  {
    norm();
  }

  Rational() : numerator(0), denominator(1) {}

  ~Rational() = default;

  Rational &operator=(const Rational &other) = default;

  Rational operator-() const {
    return Rational(-numerator, denominator);
  }

  std::string toString() const {
    if (numerator == 0) return "0";
    if (denominator == 1) return numerator.toString();
    return numerator.toString() + '/' + denominator.toString();
  }

  Rational& operator+=(const Rational& other) {
    *this = Rational(this->numerator * other.denominator + this->denominator * other.numerator,
                     this->denominator * other.denominator);;
    return *this;
  }

  Rational& operator-=(const Rational& other) {
    *this = *this += (-other);
    return *this;
  }

  Rational& operator*=(const Rational& other) {
    *this = Rational(this->numerator * other.numerator,
                     this->denominator * other.denominator);
    return *this;
  }

  Rational& operator/=(const Rational& other) {
    if (other.numerator == 0) throw std::runtime_error("Denominator cannot be zero");
    *this = Rational(this->numerator * other.denominator,
                     this->denominator * other.numerator);
    return *this;
  }

  std::string asDecimal(int64_t precision = 0) const {
    BigInteger numerator_new = numerator.abs();
    BigInteger integer = numerator_new / denominator;
    BigInteger temp = numerator_new % denominator;
    std::string result;
    if (numerator < BigInteger(0))
      result += '-';
    result += integer.toString();
    if (precision > 0) {
      result += '.';
      for (int64_t i = 0; i < precision; ++i) {
        temp *= BigInteger(10);
        BigInteger digit = temp / denominator;
        result += digit.toString();
        temp %= denominator;
      }
    }
    return result;
  }

  explicit operator double() const {
    double numerator_new = std::stod(numerator.toString());
    double denominator_new = std::stod(denominator.toString());
    return numerator_new / denominator_new;
  }

  friend bool operator==(const Rational& first, const Rational& second);
  friend bool operator<(const Rational& first, const Rational& second);
};

bool operator<(const Rational& first, const Rational& second) {
  return (first.numerator * second.denominator < first.denominator * second.numerator);
}

bool operator>(const Rational& first, const Rational& second) {
  return second < first;
}

bool operator<=(const Rational& first, const Rational& second) {
  return !(first > second);
}

bool operator>=(const Rational& first, const Rational& second) {
  return second <= first;
}

bool operator==(const Rational& first, const Rational& second) {
  return (first <= second) && (first >= second);
}

bool operator!=(const Rational& first, const Rational& second) {
  return !(first == second);
}

Rational operator+(const Rational& first, const Rational& second) {
  Rational result = first;
  result += second;
  return result;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational result = first;
  result -= second;
  return result;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational result = first;
  result *= second;
  return result;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational result = first;
  result /= second;
  return result;
}