/*
Mayhem. Linux UCI Chess960 engine. Written in C++17 language
Copyright (C) 2020-2021 Toni Helminen <GPLv3>
*/

#include "mayhem.hpp"

// "The louder the dogs bark the less a lion feels threatened."
int main(int argc, char **argv) {
  mayhem::Init();
  mayhem::PrintVersion();

  argc == 2 && std::string(argv[1]) == "bench" ? mayhem::Bench() : mayhem::UciLoop();

  return EXIT_SUCCESS;
}
