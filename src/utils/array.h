#ifndef SERF_ARRAY_H
#define SERF_ARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// IAR适配：使用malloc/free，确保正确的内存管理
#define MAX_ARRAY_SIZE 512

template<typename T>
class Array {
 public:
  Array<T>() : length_(0), data_(NULL) {
  }

  explicit Array<T>(uint16_t length) : length_(length), data_(NULL) {
    if (length > 0 && length <= MAX_ARRAY_SIZE) {
      data_ = (T*)malloc(length * sizeof(T));
      if (data_ != NULL) {
        memset(data_, 0, length * sizeof(T));
      } else {
        length_ = 0;
      }
    } else {
      length_ = 0;
    }
  }

  // 复制构造函数
  Array<T>(const Array<T> &other) : length_(other.length_), data_(NULL) {
    if (other.data_ != NULL && length_ > 0 && length_ <= MAX_ARRAY_SIZE) {
      data_ = (T*)malloc(length_ * sizeof(T));
      if (data_ != NULL) {
        memcpy(data_, other.data_, length_ * sizeof(T));
      } else {
        length_ = 0;
      }
    } else {
      length_ = 0;
    }
  }

  // 赋值操作符
  Array<T> &operator=(const Array<T> &right) {
    if (this != &right) {
      // 释放旧内存
      if (data_ != NULL) {
        free(data_);
        data_ = NULL;
      }
      
      length_ = right.length_;
      if (right.data_ != NULL && length_ > 0 && length_ <= MAX_ARRAY_SIZE) {
        data_ = (T*)malloc(length_ * sizeof(T));
        if (data_ != NULL) {
          memcpy(data_, right.data_, length_ * sizeof(T));
        } else {
          length_ = 0;
        }
      } else {
        length_ = 0;
      }
    }
    return *this;
  }

  // 析构函数
  ~Array<T>() {
    if (data_ != NULL) {
      free(data_);
    }
  }

  T &operator[](uint16_t index) const {
    if (data_ != NULL && index < length_) {
      return data_[index];
    }
    static T error_value = {0};
    return error_value;
  }

  T *begin() const {
    return data_;
  }

  T *end() const {
    return (data_ != NULL) ? (data_ + length_) : NULL;
  }

  uint16_t length() const {
    return length_;
  }

  bool is_valid() const {
    return data_ != NULL && length_ > 0;
  }
  
  void swap(Array<T> &other) {
    T* temp_data = data_;
    uint16_t temp_length = length_;
    
    data_ = other.data_;
    length_ = other.length_;
    
    other.data_ = temp_data;
    other.length_ = temp_length;
  }

 private:
  uint16_t length_;
  T* data_;
};

#endif  // SERF_ARRAY_H
