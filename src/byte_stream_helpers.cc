#include "byte_stream.hh"

#include <cstdint>
#include <stdexcept>

using namespace std;

/*
 * read: A helper function thats peeks and pops up to `max_len` bytes
 * from a ByteStream Reader into a string;
 */
void read( Reader& reader, uint64_t max_len, string& out )
{
  out.clear();

  while ( reader.bytes_buffered() and out.size() < max_len ) {
    auto view = reader.peek();

    if ( view.empty() ) {
      throw runtime_error( "Reader::peek() returned empty string_view" );
    }

    view = view.substr( 0, max_len - out.size() ); // Don't return more bytes than desired.
    out += view;
    reader.pop( view.size() );
  }
}

Reader& ByteStream::reader()
{
  static_assert( sizeof( Reader ) == sizeof( ByteStream ),
                 "Please add member variables to the ByteStream base, not the ByteStream Reader." );

  return static_cast<Reader&>( *this ); // NOLINT(*-downcast)
}

const Reader& ByteStream::reader() const
{
  static_assert( sizeof( Reader ) == sizeof( ByteStream ),
                 "Please add member variables to the ByteStream base, not the ByteStream Reader." );

  return static_cast<const Reader&>( *this ); // NOLINT(*-downcast)
}

Writer& ByteStream::writer()
{
  static_assert( sizeof( Writer ) == sizeof( ByteStream ),
                 "Please add member variables to the ByteStream base, not the ByteStream Writer." );

  return static_cast<Writer&>( *this ); // NOLINT(*-downcast)
}

//writer()返回一个常引用，实际上就是ByteStream自己的一部分
const Writer& ByteStream::writer() const
{
  static_assert( sizeof( Writer ) == sizeof( ByteStream ),
                 "Please add member variables to the ByteStream base, not the ByteStream Writer." );

  return static_cast<const Writer&>( *this ); // NOLINT(*-downcast)
}
//static_assert 作用：确保 Writer 和 ByteStream 结构相同，不允许 Writer 有额外成员变量。
//static_cast<const Writer&>(*this) 让 ByteStream 以 Writer 视角存在。
//static_cast 不会改变对象的实际类型，只是告诉编译器用 Writer 视角看待 ByteStream。