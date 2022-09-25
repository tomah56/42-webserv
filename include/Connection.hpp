/* ************************************************************************ */
/*                                                                          */
/*                              Class: Connection                           */
/*                                                                          */
/* ************************************************************************ */

#ifndef __Connection_HPP__
# define __Connection_HPP__

#include "utility.hpp"
#include "Socket.hpp"
#include "http_type_traits.hpp"
#include "Request.hpp"
#include "Response.hpp"

#include <string>
#include <iostream>

namespace ws {
namespace http {

class Connection {

    public:

        // class : public ws::exception {
        //         virtual const char* what() const throw();
        // };

        Connection(const int fd, const http::Tokens& tokens);
        Connection(const http::Tokens& tokens);

        Connection(const Connection& other);
        Connection& operator=(const Connection& other);
        ~Connection();

        int fd() const;
        bool is_good() const;
        bool is_persistent() const;
        int status() const;

        void establish(const int fd);
        void handle();

    private:

        int _fd;
        int _status;
        struct sockaddr_in6 _address; // ??
        bool _is_persistent;
        Request _request;
        Tokens _tokens;

}; // CLASS Connection

} // NAMESPACE http
} // NAMESPACE ws
#endif // __Connection_HPP__
