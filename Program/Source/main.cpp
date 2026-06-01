#include "App.h"
#include "Helpers/Config.h"

int main()
{
	std::cout << "Build Test - v1.2.3\n";
	Config::Get().Load("config.toml");
	App application;
	application.Run();
	return 0;
}