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
    if(data.empty() && first_index == output_.writer().bytes_pushed()) //这个情况是最后子串，但是是空流，
    {
      output_.writer().close();
    }
  }

  uint64_t pushed = output_.writer().bytes_pushed();
  uint64_t end_unassembled_index = pushed + output_.writer().available_capacity();

  if( first_index + data.size() <= pushed || data.empty() ) {
    return;
  }

  // 剪掉已经被写入的
  if (first_index < output_.writer().bytes_pushed()) {
    size_t skip = output_.writer().bytes_pushed() - first_index;
    if (skip >= data.size()) {
        return;
    }
    data = data.substr(skip);
    first_index = output_.writer().bytes_pushed();
  }

  // 剪掉超出容量的
  size_t available = output_.writer().available_capacity();
  if (first_index >= output_.writer().bytes_pushed() + available) {
      return;
  }
  if (first_index + data.size() > output_.writer().bytes_pushed() + available) {
      data = data.substr(0, output_.writer().bytes_pushed() + available - first_index);
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
    //如果前一个子串的末尾 超过 当前子串的首索引
    if(prev_end >= first_index){
      if(end_index > prev_end){ //非包含关系
        first_index = prev_start;
        data = prev_iter->second + data.substr(end_index - prev_end);
        R_buffer_size_ -= prev_iter->second.size();
        Reassembler_buffer_.erase(prev_iter); 
      }
      else{
        first_index = prev_start;
        data = prev_iter->second;
        R_buffer_size_ -= prev_iter->second.size();
        Reassembler_buffer_.erase(prev_iter);
      } 
    }
  }
  //向后合并
  while(iter != Reassembler_buffer_.end() && iter->first <= end_index) { //条件：R_buffer中存在比当前index要大的index
    uint64_t next_start = iter->first;
    uint64_t next_end = next_start + iter->second.size();
    if(next_start > end_index) break; //没有重叠，下面的情况是有重叠，需要剪裁合并
    if(iter->first + iter->second.size() <= end_index){
      R_buffer_size_ -= iter->second.size();
      iter = Reassembler_buffer_.erase(iter);
    }
    else{
      data += iter->second.substr(end_index - next_start);
      end_index = next_end;
      R_buffer_size_ -= iter->second.size();
      iter = Reassembler_buffer_.erase(iter);
    }
  }

  //插入重组器缓存区
  Reassembler_buffer_[first_index] = data;
  R_buffer_size_ += data.size();

  //超容量丢弃
  if(first_index + data.size() > end_unassembled_index){
    if(end_unassembled_index > first_index)
    {
      data = data.substr(0, end_unassembled_index - first_index);
    }
    else{
      data = "";
    }
  }
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
