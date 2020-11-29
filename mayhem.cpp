#include "mayhem.hpp"

// "I want to cause havoc. I want to cause mayhem - and I mean the worst mayhem you can see." -- Tony Bellew
int main(int argc, char **argv) {
  mayhem::Init();
  if (argc == 2 && std::string(argv[1]) == "--version") {mayhem::PrintVersion();}
  else if (argc == 2 && std::string(argv[1]) == "--bench") {mayhem::Bench();}
  else if (argc == 3 && std::string(argv[1]) == "-list") {mayhem::PrintList(std::string(argv[2]));}
  else if (argc == 3 && std::string(argv[1]) == "-eval") {mayhem::PrintEval(std::string(argv[2]));}
  else {mayhem::UciLoop();}
  return EXIT_SUCCESS;
}
