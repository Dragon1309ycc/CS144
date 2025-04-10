#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <functional>

using namespace std;

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ),
    isn_( isn ),
    send_SYN( false ),
    send_FIN( false ),
    is_tick_running( false ),
    next_seqno_( 0 ),
    receive_seqno_( 0 ),
    initial_RTO_ms_( initial_RTO_ms ),
    RTO_ms_( initial_RTO_ms ),
    passed_time_( 0 ),
    receive_window_size_( 1 ),
    wait_ack_msg(),
    numbers_in_flight_( 0 ),
    consecutive_retransmissions_( 0 ) {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?

  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }
  
  ByteStream input_;  //! 这是bytestream
  Wrap32 isn_;        //!这是初始序列号

  bool send_SYN;
  bool send_FIN;
  bool is_tick_running {};
  
  uint64_t next_seqno_ {};    // 期望的receive接收的序列号，也是在push函数中下一个字段将赋予的序列号，我们会用空报文检查它
  
  uint64_t receive_seqno_ {}; // 接收到的序列号

  uint64_t initial_RTO_ms_;   //!初始的RTO时间
  uint64_t RTO_ms_ {};        //!RTO时间
  uint64_t passed_time_ {};

  uint16_t receive_window_size_ {};

  queue<TCPSenderMessage> wait_ack_msg;     // 用来存放子串的容器
  
  uint64_t numbers_in_flight_ {};           // 记录有多少个子串未完成 
  
  uint64_t consecutive_retransmissions_ {}; // 记录有多少次连续重传 
};
