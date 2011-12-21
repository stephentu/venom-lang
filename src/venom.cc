#include <iostream>
#include <fstream>

#include "driver.h"

using namespace std;
using namespace venom;

int main(int argc, char **argv) {
  ParseContext pctx;
  Driver driver(pctx);
  bool readfile = false;

  for(int ai = 1; ai < argc; ++ai) {
    if (argv[ai] == string ("-p")) {
      driver.trace_parsing = true;
    } else if (argv[ai] == string ("-s")) {
      driver.trace_scanning = true;
    } else {
      // read a file with expressions

      fstream infile(argv[ai]);
      if (!infile.good()) {
        cerr << "Could not open file: " << argv[ai] << endl;
        return 0;
      }

      bool result = driver.parse_stream(infile, argv[ai]);
      readfile = true;
    }
  }

  if (readfile) return 0;

  string line;
  while (cout << "input: " && getline(cin, line) && !line.empty()) {
    bool result = driver.parse_string(line, "input");
  }
}
