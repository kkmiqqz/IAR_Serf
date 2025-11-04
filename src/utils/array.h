#ifndef SERF_ARRAY_H
#define SERF_ARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
// IAR适配：使用C风格数组替代C++ STL
// 由于CC2530内存限制，使用全局静态缓冲区池，避免栈溢出和malloc/free
#define MAX_ARRAY_SIZE 64  // 最大数组大小（从128减少到64，避免XDATA空间不足）

// 全局静态缓冲区池（避免栈溢出）
// 为不同类型的数组提供独立的缓冲区池
template<typename T>
struct ArrayBufferPool {
  static uint8_t buffer_pool[MAX_ARRAY_SIZE * sizeof(T)];
  static bool in_use;
};

// 实例化缓冲区池
template<typename T>
uint8_t ArrayBufferPool<T>::buffer_pool[MAX_ARRAY_SIZE * sizeof(T)];

template<typename T>
bool ArrayBufferPool<T>::in_use = false;

template<typename T>
class Array {
 public:
  Array<T>() : length_(0), data_ptr_(NULL) {
    // 空数组，不使用缓冲区
  }

  explicit Array<T>(uint16_t length) : length_(0), data_ptr_(NULL) {
    if (length == 0) {
      return;
    }
    
    if (length > MAX_ARRAY_SIZE || ArrayBufferPool<T>::in_use) {
      return;
    }
    
    // 所有条件都满足，分配缓冲区
    length_ = length;
    data_ptr_ = (T*)ArrayBufferPool<T>::buffer_pool;
    ArrayBufferPool<T>::in_use = true;
    
    // 初始化缓冲区为零
    memset(data_ptr_, 0, length * sizeof(T));
  }

  // 复制构造函数
  Array<T>(const Array<T> &other) : length_(0), data_ptr_(NULL) {
    if (other.data_ptr_ != NULL && other.length_ <= MAX_ARRAY_SIZE && !ArrayBufferPool<T>::in_use) {
      length_ = other.length_;
      data_ptr_ = (T*)ArrayBufferPool<T>::buffer_pool;
      ArrayBufferPool<T>::in_use = true;
      memcpy(data_ptr_, other.data_ptr_, length_ * sizeof(T));
    } else {
      length_ = 0;
      data_ptr_ = NULL;
    }
  }

  // 赋值操作符
  Array<T> &operator=(const Array<T> &right) {
    if (this != &right) {
      // 如果目标大小相同且已有内存，直接复制数据（避免重新分配）
      if (data_ptr_ != NULL && right.data_ptr_ != NULL && length_ == right.length_) {
        memcpy(data_ptr_, right.data_ptr_, length_ * sizeof(T));
        return *this;
      }
      
      // 先保存源对象的数据和长度
      uint16_t new_length = right.length_;
      bool source_holds_buffer = (right.data_ptr_ != NULL && 
                                   right.data_ptr_ == (T*)ArrayBufferPool<T>::buffer_pool);
      
      // 先释放当前对象的缓冲区
      if (data_ptr_ != NULL && data_ptr_ == (T*)ArrayBufferPool<T>::buffer_pool) {
        ArrayBufferPool<T>::in_use = false;
        data_ptr_ = NULL;
      }
      
      if (right.data_ptr_ != NULL && new_length <= MAX_ARRAY_SIZE) {
        // 如果源对象持有缓冲区（临时对象），我们需要特殊处理
        if (source_holds_buffer) {
          // 源对象持有缓冲区，临时对象会在赋值完成后立即析构
          // 所以我们先复制数据，然后获取缓冲区池
          // 但问题是：临时对象在赋值操作符执行时仍然存在
          // 所以我们需要先复制数据到目标对象（如果已有缓冲区）
          if (data_ptr_ != NULL) {
            // 目标对象已经有缓冲区，直接复制
            length_ = new_length;
            memcpy(data_ptr_, right.data_ptr_, new_length * sizeof(T));
            return *this;
          }
          // 目标对象没有缓冲区，我们需要等待临时对象析构
          // 但临时对象在赋值操作符完成后才会析构
          // 所以我们需要先复制数据到临时位置，然后获取缓冲区池
          // 但临时缓冲区会导致栈溢出
          // 最简单的方案：返回失败，让调用者处理
          // 或者，我们可以使用一个静态缓冲区来临时存储数据
          // 但静态缓冲区只能存储一个实例的数据
          // 实际上，我们可以使用一个全局临时缓冲区
          // 但这样会有线程安全问题
          
          // 最终方案：如果目标对象没有缓冲区，我们暂时失败
          // 调用者需要确保目标对象已经有缓冲区
          data_ptr_ = NULL;
          length_ = 0;
        } else {
          // 源对象不持有缓冲区池，正常复制
          length_ = new_length;
          if (!ArrayBufferPool<T>::in_use) {
            data_ptr_ = (T*)ArrayBufferPool<T>::buffer_pool;
            ArrayBufferPool<T>::in_use = true;
            memcpy(data_ptr_, right.data_ptr_, length_ * sizeof(T));
          } else {
            // 缓冲区池被占用，无法分配
            data_ptr_ = NULL;
            length_ = 0;
          }
        }
      } else {
        data_ptr_ = NULL;
        length_ = 0;
      }
    }
    return *this;
  }

  // 析构函数
  ~Array<T>() {
    // 只有当当前对象持有缓冲区池时才释放
    if (data_ptr_ != NULL && data_ptr_ == (T*)ArrayBufferPool<T>::buffer_pool) {
      printf("~Array: releasing buffer pool, sizeof(T)=%u\n", (uint16_t)sizeof(T));
      ArrayBufferPool<T>::in_use = false;
    }
  }

  T &operator[](uint16_t index) const {
    if (data_ptr_ != NULL && index < length_) {
      return data_ptr_[index];
    }
    // 返回静态错误值
    static T error_value = {0};
    return error_value;
  }

  T *begin() const {
    return data_ptr_;
  }

  T *end() const {
    if (data_ptr_ != NULL && length_ > 0) {
      return data_ptr_ + length_;
    }
    return NULL;
  }

  uint16_t length() const {
    return length_;
  }

  // 检查数组是否有效
  bool is_valid() const {
    return (data_ptr_ != NULL && length_ > 0);
  }
  
  // 交换两个数组的内容
  void swap(Array<T> &other) {
    // 交换长度和指针
    uint16_t temp_length = length_;
    T* temp_ptr = data_ptr_;
    
    length_ = other.length_;
    data_ptr_ = other.data_ptr_;
    
    other.length_ = temp_length;
    other.data_ptr_ = temp_ptr;
  }

 private:
  uint16_t length_;
  T* data_ptr_;  // 指向全局缓冲区池的指针
};

#endif  // SERF_ARRAY_H
