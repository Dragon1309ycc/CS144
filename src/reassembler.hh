#pragma once

#include "byte_stream.hh"
#include <map>
class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  //防止隐式转换，接收右值引用的ByteStream类，初始化列表
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ) {} 

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring     当前子字符串的首字节索引
   *   `data`: the substring itself    当前收到的子字符串
   *   `is_last_substring`: this substring represents the end of the stream    布尔值：当前子字符串是否是最后一块的子字符串
   *   `output`: a mutable reference to the Writer     将正确排序的字节写入流中
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   * //根据下面的要求，声明定义了end_unassembled_index。即最大可容许字节索引
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;
  uint64_t last_index_ = INT_FAST64_MAX;                  //记录最后一部分子字符串的首字节索引
  map<uint64_t,string> Reassembler_buffer_ {} ;           //用来存储不连续的子字符串
  uint64_t R_buffer_size_ {};                             //用来记录在重组器缓存区中的字节数
  uint64_t first_unassembled_index {};                    //未重组的首字节索引
  uint64_t end_unassembled_index {};                      //最大容许的字节索引范围（开区间
  
};
 