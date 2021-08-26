/*
Mayhem. Linux UCI Chess960 engine. Written in C++17 language
Copyright (C) 2020-2021 Toni Helminen <GPLv3>
*/

#include "mayhem.hpp"

// "The louder the dogs bark the less a lion feels threatened."
int main() {
  mayhem::PrintVersion();
  mayhem::Init();
  mayhem::UciLoop();

  return EXIT_SUCCESS;
}
