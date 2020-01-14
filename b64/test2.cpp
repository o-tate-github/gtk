#include "base64.h"
#include <iostream>

int main() {

  std::string decoded = base64_decode("=E6=9C=9D=EF=BC=9A=E3=82=B5=E3=83=B3=E3=83=89=E3=82=A4=E3=83=83=E3=83=81");

  std::cout << "decoded: " << decoded << std::endl;

  return 0;
}
