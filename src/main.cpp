#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "ast/tree.hpp"

extern int yydebug;  // 0: disable debug mode, 1: enable debug mode
extern int yyparse();
extern int yylex();
extern int yylineno;  // line number
extern FILE *yyin;
AST::NodePtr root;

class Argument {
 public:
  std::string input_file;
  std::string output_file;
  bool output_ir = false;
  bool use_venus = false;

  Argument(int argc, char **argv) {
    if (argc < 2) {
      throw std::runtime_error("Usage: " + std::string(argv[0]) +
                               " <input file> [output file] [--ir] [--venus]");
    }
    int pos = 1;
    for (int i = 1; i < argc; i++) {
      if (std::string(argv[i]) == "--ir") {
        output_ir = true;
      } else if (std::string(argv[i]) == "--venus") {
        use_venus = true;
      } else if (pos == 1) {
        input_file = argv[i];
        pos++;
      } else if (pos == 2) {
        output_file = argv[i];
        pos++;
      } else {
        throw std::runtime_error("No matching argument: " +
                                 std::string(argv[i]));
      }
    }
    if (output_ir && use_venus) {
      throw std::runtime_error(
          "Cannot output IR and Venus assembly at the same time");
    }
  }
};

int main(int argc, char **argv) {
  try {
    yylineno = 1;  // initialize line number

    Argument args(argc, argv);

    yyin = fopen(args.input_file.c_str(), "r");
    if (!yyin) {
      throw std::runtime_error("Cannot open file: " + args.input_file);
    }

    // 输出 flex/bison 的调试信息
    // yydebug = 1;

    if (int parse_status = yyparse()) {
      throw std::runtime_error("Parse failed with status " +
                               std::to_string(parse_status));
    }
    fclose(yyin);

    if (root) {
      root->print_tree();
      std::cout << "Parse succeeded" << std::endl;
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
