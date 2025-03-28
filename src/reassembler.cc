#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  //! \name Data preprocessing
  //!@{
  if(is_last_substring)
  {
    last_index_ = max(first_index + data.size(), last_index_);            //如果为最后一部分的子字符串，将其记录
  }

  first_unassembled_index = output_.writer().bytes_pushed();
  end_unassembled_index = first_unassembled_index + output_.writer().available_capacity();

  //discard unuseful data
  if(first_index + data.size() - 1 < first_unassembled_index || first_index >= end_unassembled_index) return; 
  //reserve useful data
  if(first_index <= first_unassembled_index)                           
  {
    uint64_t cut_size = first_unassembled_index - first_index;
    if(cut_size > data.size())
    {
      return;
    }
    else
    {
      data = data.substr(cut_size);                           //更新data
      first_index = first_unassembled_index;                  //修改first_index，也就是修改成未重组的首位
    }
  }
  //!@}

  //first_index, data, first_un , end_un , last_index

  //! \name 对缓冲区的操作
  //!@{
  //数据的存储以及状态量的更新,需要对数据取并集
  uint64_t key1 {}, key2 {}, left = 0, right = 0, is_merge = 0;

  auto now_lower = Reassembler_buffer_.lower_bound(first_index);
  if(now_lower != Reassembler_buffer_.begin())
  {
    key1 = Reassembler_buffer_.lower_bound(first_index)->first;
    if((key1 + Reassembler_buffer_[key1].size()) > first_index)    left = 1;  //判断有前重叠
  }

  auto now_upper = Reassembler_buffer_.lower_bound(first_index + data.size());
  if(now_upper != Reassembler_buffer_.begin())
  {
    key2 = Reassembler_buffer_.lower_bound(first_index + data.size())->first;
    if(key2 > first_index) right = 1;                                         //判定有后重叠或者是当前数据包含了已有的数据
  }


  //仅有左重叠的情况
  if(right == 0 && left == 1 )
  {
    is_merge = 1;
    string temp = data.substr((key1 + Reassembler_buffer_[key1].size()) - first_index);
    Reassembler_buffer_[key1].append(temp);
    R_buffer_size_ += temp.size();                                            //更新重组器缓存区的数据量
    first_index = key1;
  }
  else if(right == 1 && left == 0) //仅有右重叠/包含的情况
  {
    is_merge = 1;
    if( (key2 + Reassembler_buffer_[key2].size()) > (first_index + data.size()) )//此情况为后重叠
    {
      string temp = Reassembler_buffer_[key2];
      temp.substr(data.size() - (key2 - first_index));
      data.append(temp);
      R_buffer_size_ += key2 - first_index;
      Reassembler_buffer_[first_index] = data;
      Reassembler_buffer_.erase(key2);
    }
    else //此情况为包含，直接插入新的数据，删除原有数据
    {
      is_merge = 1;
      R_buffer_size_ += (key2 - first_index);
      R_buffer_size_ += (key2 + Reassembler_buffer_[key2].size() - first_index - data.size()); //更新重组器缓冲区大小
      Reassembler_buffer_.erase(key2); 
      Reassembler_buffer_[first_index] = data;
    }
  }
  else if(right == 0 && left == 0) //与任何部分都没有重叠
  {
    is_merge = 0;
    R_buffer_size_ += data.size();
    Reassembler_buffer_[first_index] = data;
  }
  else //既有左侧重叠又有右侧重叠
  {
    is_merge = 1;
    string temp = data.substr((key1 + Reassembler_buffer_[key1].size()) - first_index);
    Reassembler_buffer_[key1].append(temp);
    first_index = key1;
    if( (key2 + Reassembler_buffer_[key2].size()) <= (first_index + data.size()) )  //此情况为后侧包含：删除key2部分，
    {
      R_buffer_size_ += temp.size(); //更新1，拼接temp
      R_buffer_size_ -= Reassembler_buffer_[key2].size(); //更新2，删除key2
      Reassembler_buffer_.erase(key2); //删除
    }
    else //此情况为后侧重叠
    {
      string temp2 = Reassembler_buffer_[key2]; 
      temp2.substr(key1 + Reassembler_buffer_[key1].size() - key2); //取字符串
      Reassembler_buffer_[key1].append(temp2); 
      R_buffer_size_ += temp.size(); // 更新1，拼接temp
      R_buffer_size_ += temp2.size(); //更新2，拼接temp2
      R_buffer_size_ -= key1 + Reassembler_buffer_[key1].size() - key2; //更新3，删除key2
    }
  }

  //写到此处 2025.3.27


  //写入操作
  while(!Reassembler_buffer_.empty() && Reassembler_buffer_.count(first_unassembled_index))
  {
    const string& temp =  Reassembler_buffer_[first_unassembled_index]; 
    output_.writer().push(temp);                                //插入相邻的子字符串
    R_buffer_size_ -=  Reassembler_buffer_[first_unassembled_index].size();//更新重组器缓存区的数据量
    first_unassembled_index += Reassembler_buffer_[first_unassembled_index].size();   //更新流索引
    Reassembler_buffer_.erase(first_index);                     //更新重组器缓存区,删除刚刚插入的数据
  }
  if((first_unassembled_index >= last_index_) && (output_.reader().bytes_buffered()==0))
  {
    output_.writer().close();                                 //表示写入结束，已经全部放入到了buffer中
  }
}


// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return R_buffer_size_;
}
