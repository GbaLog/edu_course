#include <iostream>
#include "application.h"

int main(int argc, char ** argv)
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " [server ip] [server port]" << std::endl;
    return EXIT_FAILURE;
  }

  application app;
  return app.run(argv[1], std::stoi(argv[2]));
}
