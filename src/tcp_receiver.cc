#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )   //SYN FIN seqno RST payload
{   
    if(message.SYN == true) { 
        zero_point = message.seqno + 1;                 //这是传输的起始点, 丢弃SYN
        is_FIN = false;
        is_SYN = true;
    }
    uint64_t first_index = message.seqno.unwrap(message.seqno, message.seqno.unwrap(*zero_point, 0));
    reassembler_.insert(first_index, message.payload, message.FIN); //插入进重组器-
    
    if (message.FIN){
        is_FIN = message.FIN;
        if(reassembler_.writer().bytes_pushed() == reassembler_.count_bytes_pending()){
            const_cast<Writer&>(reassembler_.writer()).close(); 
        }
    } 
    receiveRST = message.RST; //复位标志
}

TCPReceiverMessage TCPReceiver::send() const //ackno, window_size, RST
{
    TCPReceiverMessage msg;
    if(zero_point.has_value()) {
        msg.ackno = Wrap32::wrap(reassembler_.writer().bytes_pushed() + reassembler_.count_bytes_pending() , zero_point.value());
        if(is_FIN){
            msg.ackno = *msg.ackno + 1;
        }
    }else {
        msg.ackno = nullopt;
    }

    // if(is_SYN)  //window size at max+1
    // {
    //     msg.window_size = reassembler_.writer(). available_capacity();
    // } else{
    //     msg.window_size = reassembler_.writer(). available_capacity() - 1;
    // }
    msg.window_size = reassembler_.writer(). available_capacity();
    msg.RST = receiveRST;
    return msg;
}
