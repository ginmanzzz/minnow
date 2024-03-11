#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  // cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  // cerr << "Warning: get_URL() has not been implemented yet.\n";
  Address serv_addr( host, "http" );
  TCPSocket tcpSock = TCPSocket();
  tcpSock.connect(serv_addr);
  // cout << "connected to server" << endl;
  string tx_msg = "GET " + path;
  tx_msg += " HTTP/1.1\r\n";
  tx_msg += "Host: ";
  tx_msg += host;
  tx_msg += "\r\n";
  tx_msg += "Connection: close\r\n";
  tx_msg += "\r\n";
  // cout << "sent:" << endl;
  // cout << tx_msg << endl;
  if ( tcpSock.write(tx_msg) == 0 ) {
	  cerr << "Client send failed" << endl;
  }
  // Tips: HTTP/1.1 host request header is essential. If there isn't the header, server should return 400
  // Tips: Can't write: tcpSock.shutdown(SHUT_WR);
  // Tips: You can only print rx_buf
  while ( !tcpSock.eof() ) {
  	string rx_buf;
  	tcpSock.read(rx_buf);
	cout << rx_buf;
  }
  	tcpSock.close();
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
