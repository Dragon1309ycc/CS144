#pragma once

#include <cstdint>
#include <string>
#include <string_view>      //C++ 17标准引入的一个类，只读的轻量级的字符串视图，用于引用而非复制
#include<queue>             //引入queue容器，来进行缓冲区buffer的定义
using namespace std;

class Reader;
class Writer;

class ByteStream
{
public:
  explicit ByteStream( uint64_t capacity );

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;

  void set_error() { error_ = true; };       // Signal that the stream suffered an error.
  bool has_error() const { return error_; }; // Has the stream had an error?

protected:
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  uint64_t capacity_;
  bool error_ {};
  queue<string> buffer_ {};       //缓冲区buffer定义，队列即可
  string_view buffer_view_ {};    //C++17结构，只读类
  unsigned char flag {};          //结束标识符,用位运算来维护，0001是已经结束了输出，0000是还没结束输出
  enum state { CLOSED , ERROR};   //枚举定义宏
  uint64_t total_pushed_ = 0;     //总共已经写入的数据量
  uint64_t total_poped_ = 0;      //总共已经弹出的数据量
};

class Writer : public ByteStream
{
public:
  void push( std::string data ); // Push data to stream, but only as much as available capacity allows.
  void close();                  // Signal that the stream has reached its ending. Nothing more will be written.

  bool is_closed() const;              // Has the stream been closed?
  uint64_t available_capacity() const; // How many bytes can be pushed to the stream right now?
  uint64_t bytes_pushed() const;       // Total number of bytes cumulatively pushed to the stream
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const; // Peek at the next bytes in the buffer
  void pop( uint64_t len );      // Remove `len` bytes from the buffer

  bool is_finished() const;        // Is the stream finished (closed and fully popped)?
  uint64_t bytes_buffered() const; // Number of bytes currently buffered (pushed and not popped)
  uint64_t bytes_popped() const;   // Total number of bytes cumulatively popped from stream
};

/*
 * read: A (provided) helper function thats peeks and pops up to `max_len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t max_len, std::string& out );
