#include "mayhem.hpp"

// "I want to cause havoc. I want to cause mayhem - and I mean the worst mayhem you can see." -- Tony Bellew
int main(int argc, char **argv) {
  mayhem::Init();
  mayhem::Args(argc, argv);
  return EXIT_SUCCESS;
}
