#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  //! \name Data preprocessing
  //!@{
  if(is_last_substring) {
    last_index_ = first_index + data.size();        //如果为最后一部分的子字符串，将其记录
  }

  uint64_t pushed = output_.writer().bytes_pushed();
  uint64_t end_unassembled_index = pushed + output_.writer().available_capacity();

  if(first_index + data.size() <= pushed || first_index >= end_unassembled_index) {
    return;
  }

  if(first_index < pushed) {
    data = data.substr(pushed - first_index);
    first_index = pushed;
  }
  //!@}


  //! \name 对缓冲区的操作
  //!@{
  //向前合并
  uint64_t end_index = first_index + data.size();
  auto iter = Reassembler_buffer_.lower_bound(first_index);
  if(iter != Reassembler_buffer_.begin()){
    auto prev_iter = prev(iter);
    uint64_t prev_start = prev_iter->first;
    uint64_t prev_end = prev_iter->first + prev_iter->second.size();
    if(prev_end > first_index) {
      first_index = prev_start;
      data = prev_iter->second + data.substr(prev_end - first_index);
      R_buffer_size_ -= prev_iter->second.size();
      Reassembler_buffer_.erase(prev_iter);
    }
  }
  //向后合并
  while(iter != Reassembler_buffer_.end() && iter->first <= end_index) {
    uint64_t next_start = iter->first;
    uint64_t next_end = next_start + iter->second.size();
    data += iter->second.substr(end_index - next_start);
    end_index = next_end;
    R_buffer_size_ -= iter->second.size();
    iter = Reassembler_buffer_.erase(iter);
  }

  //插入重组器缓存区
  Reassembler_buffer_[first_index] = data;
  R_buffer_size_ += data.size();


  //写入操作
  while(!Reassembler_buffer_.empty() && Reassembler_buffer_.begin()->first == output_.writer().bytes_pushed()) {
    output_.writer().push(Reassembler_buffer_.begin()->second);                       //插入相邻的子字符串
    R_buffer_size_ -=  Reassembler_buffer_.begin()->second.size();              //更新重组器缓存区的数据量
    Reassembler_buffer_.erase(Reassembler_buffer_.begin());                     //更新重组器缓存区,删除刚刚插入的数据
  }

  if(output_.writer().bytes_pushed() == last_index_) {
    output_.writer().close();                                 //表示写入结束，已经全部放入到了buffer中
  }
}


// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return R_buffer_size_;
}
