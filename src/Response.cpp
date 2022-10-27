/* ************************************************************************ */
/*                                                                          */
/*                              Class: Response                             */
/*                                                                          */
/* ************************************************************************ */

#include "../include/Response.hpp"

namespace ws {
namespace http {

const char* Response::ResponseException::what() const throw() {
    return ("Response error");
}

Response::Response(const Request& request, const config_data& config, const Tokens& tokens):
    _request(request), _config(config), _tokens(tokens), _status(request.status()),
    _is_persistent(request._is_persistent) {
    
    // std::cout << GREEN << "Response constr is persistent ?" << _is_persistent << NC << std::endl;
    build_response();
}

Response::~Response() {}

bool Response::is_persistent() const { return (_is_persistent); }

void Response::send(const int fd) { // more error handeling here too [ + ]
    // if (DEBUG)
        // std::cout << "SENDING RESPONSE:\n" << _response_str;
        // std::cout << "SENDING RESPONSE:\n" << _response_str.substr(0 , _response_str.size() - _body.str().size());
    int error = ::send(fd, _response_str.c_str(), _response_str.length(), 0);
    if (error < 0)
        throw_error_status(WS_500_INTERNAL_SERVER_ERROR, "Error sending data");
    if (error == 0)
        throw_error_status(WS_500_INTERNAL_SERVER_ERROR, "Error sending data");
    std::cout << "Response class: Server sent " << _body.str().size() << " bytes to fd " << fd \
        << " (" << _resource.file << ")" << std::endl;
}

int Response::throw_error_status(int status, const char* msg) {
    error_msg = msg;
    if (DEBUG) {
        if (msg)
            std::cout << RED << msg << ": " << NC;
        std::cout << RED << "Error: " << _tokens.status_phrases[status] << NC << std::endl;
    }
    _status = status;
    throw Response::ResponseException();
    return (status);
}

void Response::append_slash(std::string& path) {
    if (!path.empty())
        if (path.rfind('/') != path.npos)
            path = path + "/";
}

void Response::remove_leading_slash(std::string& path) {
    if (!path.empty())
        if (path[0] == '/')
            path.erase(0, 1);
}

// - - - - - - - - - - - PRIVATE - - - - - - - - - - - - - 

std::string Response::generate_status_line() const {
    std::stringstream stream_status_line;
    stream_status_line << WS_HTTP_VERSION << SP << _tokens.status_phrases[_status];
    return (stream_status_line.str());
}

// adds a string formatted as <'field name': 'value'CRLF> to the header stream buffer
void Response::add_field(const std::string& field_name, const std::string& value) {
    _fields_stream << field_name << ": " << value << CRLF;
}

// field format example: "date: Mon, 26 Sep 2022 09:14:21 GMT"
void Response::add_formatted_timestamp() {
    std::stringstream s;
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);
    s << std::put_time(now, "%a, %d %b %Y %T %Z");
    add_field("Date", s.str());
}

// main blocks - - - - - - - - -

void Response::response_to_string() {
    std::stringstream response;
    if (!_body.str().empty())
        add_field("Content-length", std::to_string(_body.str().size()));
    // if (this->is_persistent()) // [ + ] condition for chunked requests ?
    //     add_field("Connection", "keep-alive");
    response << generate_status_line() << CRLF;
    response << _fields_stream.str() << CRLF;
    response << _body.str() << CRLF << CRLF;
    _response_str = response.str();
}

bool Response::getValid(const std::string & nameof)
{
    std::vector<std::string> temp = _config.http_methods;
    std::vector<std::string>::iterator it = temp.begin();
    std::vector<std::string>::iterator eit = temp.end();
    while (it != eit)
    {
        if (!((*it).compare(nameof)))
            return (false);
        it++;
    }
    return (true);
}

void Response::build_response() {
    if (_status != WS_200_OK) {
        respond_to_error();
        return ;
    }
    add_field("Server", "ZHero serv/1.0");
    add_formatted_timestamp();
    try {
        identify_resource();
        // map function pointers to avoid if else statements [ ? ]
        std::cout <<_request.header.method << "------------------------\n";

            //  std::list<std::string>::iterator it;
        // std::vector<std::string>::iterator it = (_config.http_methods).begin();


        if (_request.header.method == "GET")
        {
            if (getValid("GET"))
                throw_error_status(WS_405_METHOD_NOT_ALLOWED, "Method forbidden by config file");
            if (_config.isCgiOn && _config.cgi.compare(".php") == 0 && (_resource.extension == "php" || _resource.extension == "html"))
            {
                // std::cout << "HERE 1 GET -------- CGI --------\n" <<std::endl;
                respond_cgi_get();
            }
			else
			{
                //  std::cout << "HERE 2 GET ----- no ----- CGI\n" <<std::endl;
                add_field("Content-type", _resource.subtype.empty() ? _resource.type : (_resource.type + "/" + _resource.subtype));
                std::cout << CYAN << "Content-type: " <<  (_resource.subtype.empty() ? _resource.type : (_resource.type + "/" + _resource.subtype));
                std::cout << NC << std::endl;
	            respond_get();
			}
        }
        else if (_request.header.method == "POST")
        {
            if (getValid("POST"))
                throw_error_status(WS_405_METHOD_NOT_ALLOWED, "Method forbidden by config file");
            if (_config.isCgiOn && _config.cgi.compare(".php") == 0 && (_resource.extension == "php" || _resource.extension == "html"))
            {
                // std::cout << "HERE 3 POST ----- CGI ------\n" <<std::endl;
                respond_cgi_post();
            }
			else
			{
                // std::cout << "HERE 4 POST  --no-- CGI\n" <<std::endl;
                add_field("Content-type", _resource.subtype.empty() ? _resource.type : (_resource.type + "/" + _resource.subtype));
	            respond_get();
			}
        }
        else if (_request.header.method == "DELETE")
        {
            if (getValid("DELETE"))
                throw_error_status(WS_405_METHOD_NOT_ALLOWED, "Method forbidden by config file");
            // HTTP/1.1 204 OK
            // Content-Length: 0
            // set status or something?
            // _status = 204; // no body
            // _status = 202; // accepted may be completed. 

            if (std::remove(_resource.abs_path.c_str()) == 0)
            {
                
                // if (true)
                // {

                // }
                respond_to_delete();
            }
            else
            {
                // _status = 404; // maybe?

                respond_to_error();
            }
        }
        else
        {
            // error_status(_request, WS_501_NOT_IMPLEMENTED, "HTML Method not implemented");

            throw_error_status(WS_501_NOT_IMPLEMENTED, "Sadly this HTTP method is not implemented.");            // something like this?
            return ;
        }
    }
    catch (ResponseException& e) {
        respond_to_error(); // will build error response
    }
    // anything else like stringstream errors etc: MEANS STATUS STILL OK and has to be set to not okay
    catch (std::exception& e) {
        if (DEBUG) { std::cout << "unforeseen exception in response: " << e.what() << std::endl; }
        _status = WS_500_INTERNAL_SERVER_ERROR;
        respond_to_error(); // will build error response
    }
}

// - - - - - - METHODS - - - - - - - -

// implement custom error pages fetching
//     root = "./default_pages/errors";
// assuming any other thing besides 200 ok is wrong for now (rdr?)
void Response::respond_to_delete() {
    _body.str(std::string());;
    _fields_stream.str(std::string());
    _response_str = std::string();
    add_field("Server", "ZHero serv/1.0");
    add_formatted_timestamp();
    // decide_persistency();
    _body 
        << "The file was deleted!\r\n";
    response_to_string();
}

void Response::respond_to_error() {
    _body.str(std::string());;
    _fields_stream.str(std::string());
    _response_str = std::string();
    add_field("Server", "ZHero serv/1.0");
    add_formatted_timestamp();
    _body << "<!DOCTYPE html>\n<html lang=\"en\">\n"
        << "<head><title>Error " << _status << "</title></head>\n"
        << "<body body style=\"background-color:black;"
        << "font-family: 'Courier New', Courier, monospace; color:rgb(209, 209, 209)\">"
        << "<h3> Zhero serv 1.0: Error</h3>\n"
        << "<h1>" << _tokens.status_phrases[_status] << "</h1>"
        << "<h3>" << _request.error_msg << "</h3>\n"
        << "<h3>" << error_msg << "</h3>\n"
        << "</body></html>\r\n";
    response_to_string();
}

void Response::respond_get() {
    // do we need this?
    add_field("Accept-Ranges", "bytes");
    // chunked request: ?
    // Transfer-Encoding: chunked ...
    // handle_type(); // TODO: map function pointers ? have a decision tree system.
    upload_file();
    response_to_string();
    // status 200
}

void Response::respond_cgi_get()
{
	add_field("accept-ranges", "bytes");
	add_field("Cache-Control", "no-cache");
	int templength;
	std::stringstream response;
	Cgi test;
	std::string phpresp;
	phpresp +=  cgiRespCreator();
    // std::cout << "here ---- 1 ------\n";
	std::string::size_type shitindex;
    // std::cout << "here ---- 2 ------\n";
    if (phpresp.empty())
        return ;
	shitindex = phpresp.find("\r\n\r\n");
    if (shitindex == std::string::npos)
        return ;
    // std::cout << "here ---- 3 ------\n";

	_body << phpresp;
	std::string temp = phpresp.substr(shitindex + 4);
	templength = temp.length();

	// if (!_body.str().empty())
	add_field("Content-length", std::to_string(templength));
	response << generate_status_line() << CRLF;
	response << _fields_stream.str();
	response << _body.str();
	_response_str = response.str();

    // std::cout << "------------------ ------ -- - - -respons:\n" << response.str() << std::endl;
	return ;
}

std::string Response::cgiRespCreator()
 {
    // std::string temp;
    	char ** env;
		env = new char*[7];

        int i = 0;
		env[i++] = &(*((new std::string("REQUEST_METHOD=" + _request.header.method)))->begin()); // need to be newd othervised funny things happen
		env[i++] = &(*((new std::string("PATH_TRANSLATED=" + _resource.abs_path                ))->begin()));
        env[i++] = &(*((new std::string("REDIRECT_STATUS=200")))->begin());
        // env[i++] = &(*((new std::string("CONTENT_TYPE=" + _resource.type + "/" + _resource.subtype )))->begin()); // only POST PUT
    	// env[i++] = &(*((new std::string("CONTENT_LENGTH=" + std::to_string(_request._body.str().length() )))->begin())); //only post put
        env[i++] = &(*((new std::string("QUERY_STRING=" + _resource.query)))->begin());
		env[i++] = NULL;

        Cgi test;
        std::string phpresp;
        phpresp += test.executeCgiNew(env);
        if (phpresp.empty())
            std::cout << "unfortunetly this shit has nothing inside you mother fucker!\n";
        delete [] env;


    return (phpresp);
}

std::string Response::cgiRespCreator_post()
 {
    // std::string temp;
    	char ** env;
		env = new char*[14];
        std::list<std::string> konttype = _request.get_field_value("content-type");
        std::list<std::string>::iterator it;
         std::string tmp;

        if ( !(konttype.empty()) )
        {
            it = konttype.begin();
            while (it != konttype.end())
            {
                tmp += *it;
                std::cout << "type:\n" << tmp << std::endl;
                it++;
            }
        }
        konttype.clear();
        konttype = _request.get_field_value("content-length");
        std::string tempLength;

        if ( !(konttype.empty()) )
        {
            it = konttype.begin();
            while (it != konttype.end())
            {
                tempLength += *it;
                std::cout << "length:\n" << tempLength << std::endl;
                it++;
            }
        }

        int i = 0;
        // env[i++] = &(*((new std::string("\r\n\r\n" + _request._body.str() + "\r\n\r\n" )))->begin());
        env[i++] = &(*((new std::string(_request._body.str())))->begin());
		env[i++] = &(*((new std::string("CONTENT_LENGTH=" + tempLength))->begin()));
        env[i++] = &(*((new std::string("REQUEST_METHOD=" + _request.header.method)))->begin());
		env[i++] = &(*((new std::string("PATH_TRANSLATED=" + _resource.abs_path                ))->begin()));
		
        env[i++] = &(*((new std::string("PATH_INFO=" + _resource.abs_path                ))->begin()));
        env[i++] = &(*((new std::string("REMOTE_HOST=localhost:8400")))->begin());
        env[i++] = &(*((new std::string("SERVER_NAME=localhost")))->begin());
        env[i++] = &(*((new std::string("SERVER_PORT=8400")))->begin());
        env[i++] = &(*((new std::string("SERVER_PROTOCOL=HTTP/1.1")))->begin());
        env[i++] = &(*((new std::string("GATEWAY_INTERFACE=CGI/1.1")))->begin());


        env[i++] = &(*((new std::string("REDIRECT_STATUS=200")))->begin());
        // env[i++] = &(*((new std::string("CONTENT_TYPE=application/x-www-form-urlencoded")))->begin());
		env[i++] = &(*((new std::string("CONTENT_TYPE=" +   tmp      ))->begin()));
		// env[i++] = &(*((new std::string("CONTENT_LENGTH=" + std::to_string(_request._body.str().length()) ))->begin()));
        env[i++] = &(*((new std::string("QUERY_STRING=" + _resource.query)))->begin());
		env[i++] = NULL;

        Cgi test;
        std::string phpresp;
        phpresp += test.executeCgiNew(env);
        delete [] env;

    return (phpresp);
}

void Response::respond_cgi_post()
{
    add_field("accept-ranges", "bytes");
	add_field("Cache-Control", "no-cache");
	int templength;
	std::stringstream response;
	Cgi test;
	std::string phpresp;
	phpresp +=  cgiRespCreator_post();
	std::string::size_type shitindex;
    if (phpresp.empty())
        return ;
	shitindex = phpresp.find("\r\n\r\n");
    if (shitindex == std::string::npos)
        return ;
	_body << phpresp;
	std::string temp = phpresp.substr(shitindex + 4);
	templength = temp.length();
	add_field("Content-length", std::to_string(templength));
	response << generate_status_line() << CRLF;
	response << _fields_stream.str();
	response << _body.str();
	_response_str = response.str();

    // std::cout << "------------------ POST ------ -- - - -respons:\n" << response.str() << std::endl;
	return ;
}
// The data that you send in a POST request must adhere to specific formatting requirements.
// You can send only the following content types in a POST request to Media Server:
// application/x-www-form-urlencoded
// multipart/form-data
// https://httpwg.org/specs/rfc9110.html#POST

void Response::respond_post() {
    // read content-type field
    // read content-length field
    // send 201 created
    // create location header with resource
    
    // for now:
    // handle_type(); // TODO: map function pointers ? have a decision tree system.
    upload_file();
    response_to_string();

}

// - - - - - Subfunctions - - - - 

// identifies target path and type and adds content-type field to header
void Response::identify_resource() {
    interpret_target();
    validate_target_abs_path(); // maybe only in get ?
    extract_resource_extension();
    identify_resource_type();
}

// separate uri components, decoding done in request parser
// -> root always ends in '/' and file never starts with '/'
void Response::interpret_target() {
    std::string uri = _request.header.target;
    try {
        size_t uri_end = uri.npos;
        size_t query_pos = uri.find('?');
        _resource.path = uri.substr(0, query_pos);
        if (query_pos != uri_end)
            _resource.query = uri.substr(query_pos + 1);
    }
    catch (std::exception& e) {
        throw_error_status(WS_500_INTERNAL_SERVER_ERROR, "Uri could not be parsed, format error");
    }
    if (DEBUG) {
        std::cout << "Separated URI components:" << std::endl;
        std::cout << "path: " << _resource.path << std::endl;
        std::cout << "query: " << _resource.query << std::endl;
    }
    _resource.root = _config.root; // always ?
    _resource.file = (_resource.path == "/") ? _config.index : _resource.path;
    append_slash(_resource.root);
    remove_leading_slash(_resource.file);
    _resource.abs_path = _resource.root + _resource.file;
    if (DEBUG)
        std::cout << "PATH: " << _resource.abs_path << std::endl;
}

void Response::validate_target_abs_path() {
    // check also for W in POST ?
    // [ + ] system to send error pages accordingly
    if (!(_config.http_redirects.compare("non")) )
    {
        int tmp_fd;
        if ((tmp_fd = open(_resource.abs_path.c_str(), O_RDONLY)) < 0) {
            if (errno == ENOENT)
                throw_error_status(WS_404_NOT_FOUND, strerror(errno));
            else if (errno == EACCES)
                throw_error_status(WS_403_FORBIDDEN, strerror(errno));
            else
                throw_error_status(WS_500_INTERNAL_SERVER_ERROR, strerror(errno));
        }
        close(tmp_fd);
    }
    else
    {
        int tmp_fd;
        std::string temp = _config.http_redirects + "/" + _config.index;
        if ((tmp_fd = open(temp.c_str(), O_RDONLY)) < 0) {
            if (errno == ENOENT)
                throw_error_status(WS_404_NOT_FOUND, strerror(errno));
            else if (errno == EACCES)
                throw_error_status(WS_403_FORBIDDEN, strerror(errno));
            else
                throw_error_status(WS_500_INTERNAL_SERVER_ERROR, strerror(errno));
        }
        close(tmp_fd);
    }
}

void Response::extract_resource_extension() {
    size_t pos = _resource.abs_path.rfind('.');
    if (pos != _resource.abs_path.npos)
        _resource.extension = _resource.abs_path.substr(pos + 1);
    else
    _resource.extension = std::string();
}

// only if the extesion is mapped in 'tokens' content-type field is set.
// if not found type is set to extension.
void Response::identify_resource_type() {
    std::map<std::string, std::string>::const_iterator it \
        = _tokens.extensions.typemap.find(_resource.extension);
    if (it == _tokens.extensions.typemap.end()) {
        _resource.type = _resource.extension; //    [ ? ]
        return ;
    }
    size_t separator_pos = it->second.find('/');
    _resource.type = it->second.substr(0, separator_pos);
    if (separator_pos != it->second.npos)
        _resource.subtype = it->second.substr(separator_pos + 1);
}

// temporarily handles every type, later have a decision tree or similar
void Response::handle_type() {

    upload_file();
}

void Response::upload_file() { // + error handeling & target check here !
    // if (DEBUG)
        std::cout << "BUFFERING BODY FROM TARGET: " << _resource.abs_path << std::endl;
        std::cout << "redirect: " << _config.http_redirects << std::endl;
    try {

        if (!(_config.http_redirects.compare("non")) )
        {
            std::ifstream fin(_resource.abs_path, std::ios::in);
            _body << fin.rdbuf();
        }
        else
        {
            std::string deside = (_resource.path == "/") ? _config.index : _resource.path;
            std::string temp = _config.http_redirects + "/" + deside;
            std::ifstream fin(temp, std::ios::in);
            _body << fin.rdbuf();
        }
        // if (!page_file.is_open()) ...
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        throw_error_status(WS_500_INTERNAL_SERVER_ERROR, strerror(errno));
    }
}

} // NAMESPACE http
} // NAMESPACE ws
