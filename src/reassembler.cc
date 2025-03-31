#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if(is_last_substring) {
    last_index_ = first_index + data.size();
    if(data.empty() && first_index == output_.writer().bytes_pushed()) {
      output_.writer().close();
    }
  }

  uint64_t pushed = output_.writer().bytes_pushed();

  if( first_index + data.size() <= pushed || data.empty() ) {
    return;
  }

  if (first_index < pushed) {
    size_t skip = pushed - first_index;
    if (skip >= data.size()) {
      return;
    }
    data = data.substr(skip);
    first_index = pushed;
  }

  size_t available = output_.writer().available_capacity();
  if (first_index >= pushed + available) {
    return;
  }
  if (first_index + data.size() > pushed + available) {
    data = data.substr(0, pushed + available - first_index);
  }
  uint64_t end_index = first_index + data.size();
  auto iter = Reassembler_buffer_.lower_bound(first_index);
  // 向前合并
  if(iter != Reassembler_buffer_.begin()){
    auto prev_iter = prev(iter);
    uint64_t prev_start = prev_iter->first;
    uint64_t prev_end = prev_start + prev_iter->second.size();
    if(prev_end >= first_index){
      size_t overlap = prev_end - first_index;
      if (overlap < data.size()) {
        data = prev_iter->second + data.substr(overlap);
      } else {
        data = prev_iter->second; // 当前data完全被覆盖
      }
      first_index = prev_start;
      R_buffer_size_ -= prev_iter->second.size();
      Reassembler_buffer_.erase(prev_iter);
    }
  }
  //向后合并
  iter = Reassembler_buffer_.lower_bound(first_index);
  while(iter != Reassembler_buffer_.end() && iter->first <= end_index) {
    uint64_t next_start = iter->first;
    uint64_t next_end = next_start + iter->second.size();
    if(next_start > end_index) break; //现在只剩 next 与 当前子字符串有相连或者重叠
    if(next_end <= end_index) {
      R_buffer_size_ -= iter->second.size();
      iter = Reassembler_buffer_.erase(iter);
    }else { //也就是next_end > end_index
      data += iter->second.substr(end_index - next_start);
      end_index = next_end;
      R_buffer_size_ -= iter->second.size();
      iter = Reassembler_buffer_.erase(iter);
    }
    iter = Reassembler_buffer_.lower_bound(first_index);
  }

  Reassembler_buffer_[first_index] = data;
  R_buffer_size_ += data.size();

  // 写入操作
  while(!Reassembler_buffer_.empty() && Reassembler_buffer_.begin()->first == output_.writer().bytes_pushed()) {
    output_.writer().push(Reassembler_buffer_.begin()->second);
    R_buffer_size_ -= Reassembler_buffer_.begin()->second.size();
    Reassembler_buffer_.erase(Reassembler_buffer_.begin());
  }

  if(output_.writer().bytes_pushed() == last_index_) {
    output_.writer().close();
  }
}

uint64_t Reassembler::count_bytes_pending() const
{
  return R_buffer_size_;
}
