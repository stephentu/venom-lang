/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the PostgreSQL Global Development Group nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/** driver.h Declaration of the venom::Driver class. */

#ifndef VENOM_DRIVER_H
#define VENOM_DRIVER_H

#include <fstream>
#include <string>
#include <vector>

namespace venom {

namespace analysis {
  class SemanticContext;
}

namespace ast {
  class ASTStatementNode;
}

class ParseContext {
public:
  ast::ASTStatementNode* stmts;
};

/** The Driver class brings together all components. It creates an instance of
 * the Parser and Scanner classes and connects them. Then the input stream is
 * fed into the scanner object and the parser gets it's token
 * sequence. Furthermore the driver object is available in the grammar rules as
 * a parameter. Therefore the driver class contains a reference to the
 * structure into which the parsed data is saved. */
class Driver
{
public:
    /// construct a new parser driver context
    Driver(class ParseContext& ctx);

    /// enable debug output in the flex scanner
    bool trace_scanning;

    /// enable debug output in the bison parser
    bool trace_parsing;

    /// stream name (file or input stream) used for error messages.
    std::string streamname;

    /** Invoke the scanner and parser for a stream.
     * @param in  input stream
     * @param sname  stream name for error messages
     * @return    true if successfully parsed
     */
    bool parse_stream(std::istream& in,
          const std::string& sname = "stream input");

    /** Invoke the scanner and parser on an input string.
     * @param input  input string
     * @param sname  stream name for error messages
     * @return    true if successfully parsed
     */
    bool parse_string(const std::string& input,
          const std::string& sname = "string stream");

    /** Invoke the scanner and parser on a file. Use parse_stream with a
     * std::ifstream if detection of file reading errors is required.
     * @param filename  input file name
     * @return    true if successfully parsed
     */
    bool parse_file(const std::string& filename);

    // To demonstrate pure handling of parse errors, instead of
    // simply dumping them on the standard error output, we will pass
    // them to the driver using the following two member functions.

    /** Error handling with associated line number. This can be modified to
     * output the error e.g. to a dialog box. */
    void error(const class location& l, const std::string& m);

    /** General error handling. This can be modified to output the error
     * e.g. to a dialog box. */
    void error(const std::string& m);

    /** Pointer to the current lexer instance, this is used to connect the
     * parser to the scanner. It is used in the yylex macro. */
    class Scanner* lexer;

    /** Reference to the calculator context filled during parsing of the
     * expressions. */
    ParseContext& ctx;
};

struct compile_opts {
  compile_opts()
    : trace_lex(false), trace_parse(false),
      print_ast(false), print_bytecode(false),
      semantic_check_only(false), venom_import_path(".") {}
  bool trace_lex;
  bool trace_parse;
  bool print_ast;
  bool print_bytecode;
  bool semantic_check_only;
  std::string venom_import_path;
};
extern compile_opts global_compile_opts;

struct compile_result {
  enum type {
    Success,
    InvalidSyntax,
    SemanticError,
    TypeError,
    UnknownError,
  };
  type result;
  std::string message;
};

/** Used internally */
void
unsafe_compile_module(
    const std::string& fname, std::fstream& infile,
    analysis::SemanticContext& ctx);

void
unsafe_compile(
    const std::string& fname, std::fstream& infile,
    analysis::SemanticContext& ctx);

/**
 * Main entry point into the venom compiler/vm.
 * Reads from global_compile_opts
 */
bool compile_and_exec(
    const std::string& fname, compile_result& result);

} // namespace venom

#endif // VENOM_DRIVER_H
