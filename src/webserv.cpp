#include "../include/Server.hpp"
#include "../include/utility.hpp"
#include "../include/Config.hpp"

#define PORT 8001
#define BACKLOG 100


int main(int argc, char **argv) {

	if (argc != 2)
	{
		std::cout << "Wrong number of argument!.. try: ./webserv [configuration file]\n";
		return (0);
	}
	try
	{
		ws::Config configData(argv[1]);
		#if DEBUG
			std::cout << configData.getNumberConfigData(0).cgi << std::endl;
			std::cout << std::boolalpha << "direcotry listing: " << configData.getNumberConfigData(0).directory_listing << std::endl;
			std::cout << configData.getNumberConfigData(0).download << std::endl;
			std::cout << configData.getNumberConfigData(0).error << std::endl;
			std::cout << configData.getNumberConfigData(0).host << std::endl;
			std::cout << configData.getNumberConfigData(0).http_methods[0] << std::endl;
			std::cout << configData.getNumberConfigData(0).http_redirects << std::endl;
			std::cout << configData.getNumberConfigData(0).index << std::endl;
			std::cout << std::boolalpha << configData.getNumberConfigData(0).isCgiOn << std::endl;
			std::cout << configData.getNumberConfigData(0).limit_body << std::endl;
			std::cout << configData.getNumberConfigData(0).ports[0] << std::endl;
		#endif
		
		ws::Server server(configData.getAllConfigData());

		server.listen(BACKLOG);
        server.run();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return (-1);
	}
	#if DEBUG
	system("leaks webserv | tail - 3");
	#endif
	return (0);
}
