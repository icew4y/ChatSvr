#pragma once
#include <condition_variable>
#include <vector>
#include <mutex>
#include <atomic>
#include <common.hpp>
//静止拷贝
#define NOCOPYABLE(_CLASS_)\
    _CLASS_ operator = (const _CLASS_&) = delete;\
    _CLASS_(const _CLASS_&) = delete;\
    _CLASS_(const _CLASS_&&) = delete;\
    _CLASS_ &operator == (const _CLASS_ &&) = delete;
//单例模板
template <typename T>
class Singleton
{
    NOCOPYABLE(Singleton)
  public:
    static T *instance()
    {
        static T instance;
        return &instance;
    }

  private:
    Singleton() {}
    ~Singleton() {}
};

