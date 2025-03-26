#pragma once

#include <cstddef>
#include <cstdint>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <utility>

//! Wrapper around [IPv4 addresses](@ref man7::ip) and DNS operations.
class Address
{
public:
  //! \brief Wrapper around [sockaddr_storage](@ref man7::socket).
  //! \details A `sockaddr_storage` is enough space to store any socket address (IPv4 or IPv6).
  class Raw
  {
  public:
    sockaddr_storage storage {}; //!< The wrapped struct itself             //sockaddr_storage是一个通用的地址储存结构，能够存储Ipv4和IPv6的地址
    // NOLINTBEGIN (*-explicit-*)
    operator sockaddr*();                         //可以将raw对象转换成sockaddr指针类型，类型转换运算符
    operator const sockaddr*() const;             //常量转化为常量。
    // NOLINTEND (*-explicit-*)
  };

private:
  socklen_t _size; //!< Size of the wrapped address.
  Raw _address {}; //!< A wrapped [sockaddr_storage](@ref man7::socket) containing the address.

  //! Constructor from ip/host, service/port, and hints to the resolver.
  Address( const std::string& node, const std::string& service, const addrinfo& hints );    //构造函数，主机ip，服务器端口，提示信息

public:
  //! Construct by resolving a hostname and servicename.
  Address( const std::string& hostname, const std::string& service );     //构造函数，主机名称，服务器名称

  //! Construct from dotted-quad string ("18.243.0.1") and numeric port.
  explicit Address( const std::string& ip, std::uint16_t port = 0 );     //explicit防止隐式转换，构造函数，ip地址（如 "192.168.1.1"），端口号

  //! Construct from a [sockaddr *](@ref man7::socket).
  Address( const sockaddr* addr, std::size_t size );              //通过sockaddr地址和地址大小，直接通过套接字构造

  //! Equality comparison.
  bool operator==( const Address& other ) const;
  bool operator!=( const Address& other ) const { return not operator==( other ); }



  //! \name Conversions
  //!@{

  //! Dotted-quad IP address string ("18.243.0.1") and numeric port.
  std::pair<std::string, uint16_t> ip_port() const;
  //! Dotted-quad IP address string ("18.243.0.1").
  std::string ip() const { return ip_port().first; }
  //! Numeric port (host byte order).
  uint16_t port() const { return ip_port().second; }
  //! Numeric IP address as an integer (i.e., in [host byte order](\ref man3::byteorder)).
  uint32_t ipv4_numeric() const;
  //! Create an Address from a 32-bit raw numeric IP address
  static Address from_ipv4_numeric( uint32_t ip_address );
  //! Human-readable string, e.g., "8.8.8.8:53".
  std::string to_string() const;
  //!@}



  //! \name Low-level operations
  //!@{

  //! Size of the underlying address storage.
  socklen_t size() const { return _size; }
  //! Const pointer to the underlying socket address storage.
  const sockaddr* raw() const { return static_cast<const sockaddr*>( _address ); } //将调用raw()变为输出sockaddr指针类型的_address地址
  //! Safely convert to underlying sockaddr type
  template<typename sockaddr_type>
  const sockaddr_type* as() const;

  //!@}
};


//! \name learn note
//!@{
/*
首先是socklen_t类型，此类型在socket.h中定义，通过宏定义得知，其就是32位无符号整数类型(unsigned int) ，用于存储套接字地址的长度
uint_16 无符号16位整数类型 0~65535
uint_32 无符号32位整数类型 0 ~ 4294967295
sockaddr 是一个通用的socket地址结构，用于描述ip地址，端口等信息。
*/
//!@}