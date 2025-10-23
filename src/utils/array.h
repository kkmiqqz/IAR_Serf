#ifndef SERF_ARRAY_H
#define SERF_ARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// IAR适配：使用C风格数组替代C++ STL
// 由于CC2530内存限制，使用固定大小数组和XDATA存储
#define MAX_ARRAY_SIZE 512  // 硬编码，避免宏定义问题

template<typename T>
class Array {
 public:
  Array<T>() : length_(0) {
    // 使用XDATA存储大型数组以节省内部RAM
    data_ = NULL;
  }

  explicit Array<T>(uint16_t length) : length_(length) {
    if (length <= MAX_ARRAY_SIZE) {
      // 使用XDATA存储大型数组
      data_ = (T*)malloc(length * sizeof(T));
      if (data_ == NULL) {
        length_ = 0; // 分配失败
      } else {
        // 初始化分配的内存为零
        memset(data_, 0, length * sizeof(T));
      }
    } else {
      data_ = NULL;
      length_ = 0;
    }
  }

  // 复制构造函数
  Array<T>(const Array<T> &other) : length_(other.length_) {
    if (other.data_ != NULL && length_ <= MAX_ARRAY_SIZE) {
      data_ = (T*)malloc(length_ * sizeof(T));
      if (data_ != NULL) {
        memcpy(data_, other.data_, length_ * sizeof(T));
      } else {
        length_ = 0;
      }
    } else {
      data_ = NULL;
      length_ = 0;
    }
  }

  // 赋值操作符
  Array<T> &operator=(const Array<T> &right) {
    if (this != &right) {
      // 释放原有内存
      if (data_ != NULL) {
        free(data_);
      }
      
      length_ = right.length_;
      if (right.data_ != NULL && length_ <= MAX_ARRAY_SIZE) {
        data_ = (T*)malloc(length_ * sizeof(T));
        if (data_ != NULL) {
          memcpy(data_, right.data_, length_ * sizeof(T));
        } else {
          length_ = 0;
        }
      } else {
        data_ = NULL;
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
    // 返回静态错误值（需要根据类型T定义）
    static T error_value = {0};
    return error_value;
  }

  T *begin() const {
    return data_;
  }

  T *end() const {
    if (data_ != NULL) {
      return data_ + length_;
    }
    return NULL;
  }

  uint16_t length() const {
    return length_;
  }

  // 检查数组是否有效
  bool is_valid() const {
    return data_ != NULL && length_ > 0;
  }

 private:
  uint16_t length_;
  T* data_; // 使用C风格指针替代std::unique_ptr
};

#endif  // SERF_ARRAY_H
