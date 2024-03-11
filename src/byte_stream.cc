#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ),
	container(){
	}

void Writer::push( string data )
{
  // Your code here.
  // (void)data;    start up
  if ( !is_closed() && available_capacity() > 0 ) {
	  uint64_t real_len = min(data.size(), available_capacity());
	  container.push(data.substr(0, real_len));
	  pushedBytes_ += real_len;
  }
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

void Writer::set_error()
{
  // Your code here.
  error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - pushedBytes_ + popedBytes_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return pushedBytes_;
}

string_view Reader::peek() const
{
  // Your code here.
  queue<string> tmp = container;
  static string view;
  view = "";
  while ( !tmp.empty() ) {
	  view += tmp.front();
	  tmp.pop();
  }
  return view;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && pushedBytes_ == popedBytes_;
}

bool Reader::has_error() const
{
  // Your code here.
  return error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  // (void)len; 
  uint64_t real_len = min(len, bytes_buffered());
  uint64_t cnt = 0;
  while (cnt < real_len) {
	  if (container.front().size() > real_len - cnt) {
		  container.front() = container.front().substr(real_len - cnt);
		  cnt += real_len - cnt;
	  } else {
		  cnt += container.front().size();
		  container.pop();
	  }
  }
  popedBytes_ += real_len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return pushedBytes_ - popedBytes_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return popedBytes_;
}
