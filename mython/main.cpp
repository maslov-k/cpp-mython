#include "lexer.h"
#include "parse.h"
#include "runtime.h"
#include "statement.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace std;

void InterpretMythonProgram(istream& input, ostream& output)
{
    parse::Lexer lexer(input);
    unique_ptr<runtime::Executable> exec = ParseProgram(lexer);
    runtime::SimpleContext context{output};
    runtime::Closure closure;
    exec->Execute(closure, context);
}

int main(int argc, const char** argv) {
    if (argc != 3) {
            std::filesystem::path interpreter = argv[0];
            cerr << "Usage Mython interpreter: "sv << interpreter.filename() << " <file_in> <file_out>"sv << endl;
            return 1;
    }

    std::filesystem::path file_in = argv[1];
    std::filesystem::path file_out = argv[2];

    ifstream ifile(file_in);
    if (!ifile.is_open())
    {
        std::cerr << "Can't open file "s << file_in << endl;
    }
    ofstream ofile(file_out);
    if (!ofile.is_open())
    {
        std::cerr << "Can't open file "s << file_out << endl;
    }

    try
    {
        InterpretMythonProgram(ifile, ofile);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}


//#include "lexer.h"
//#include "parse.h"
//#include "runtime.h"
//#include "statement.h"
//#include "test_runner_p.h"

//#include <iostream>

//using namespace std;

//namespace parse {
//void RunOpenLexerTests(TestRunner& tr);
//}  // namespace parse

//namespace ast {
//void RunUnitTests(TestRunner& tr);
//}
//namespace runtime {
//void RunObjectHolderTests(TestRunner& tr);
//void RunObjectsTests(TestRunner& tr);
//}  // namespace runtime

//void TestParseProgram(TestRunner& tr);

//namespace {

//void RunMythonProgram(istream& input, ostream& output) {
//    parse::Lexer lexer(input);
//    auto program = ParseProgram(lexer);

//    runtime::SimpleContext context{output};
//    runtime::Closure closure;
//    program->Execute(closure, context);
//}

//void TestSimplePrints() {
//    istringstream input(R"(
//print 57
//print 10, 24, -8
//print 'hello'
//print "world"
//print True, False
//print
//print None
//)");

//    ostringstream output;
//    RunMythonProgram(input, output);

//    ASSERT_EQUAL(output.str(), "57\n10 24 -8\nhello\nworld\nTrue False\n\nNone\n");
//}

//void TestAssignments() {
//    istringstream input(R"(
//x = 57
//print x
//x = 'C++ black belt'
//print x
//y = False
//x = y
//print x
//x = None
//print x, y
//)");

//    ostringstream output;
//    RunMythonProgram(input, output);

//    ASSERT_EQUAL(output.str(), "57\nC++ black belt\nFalse\nNone False\n");
//}

//void TestArithmetics() {
//    istringstream input("print 1+2+3+4+5, 1*2*3*4*5, 1-2-3-4-5, 36/4/3, 2*5+10/2");

//    ostringstream output;
//    RunMythonProgram(input, output);

//    ASSERT_EQUAL(output.str(), "15 120 -13 3 15\n");
//}

//void TestVariablesArePointers() {
//    istringstream input(R"(
//class Counter:
//  def __init__():
//    self.value = 0

//  def add():
//    self.value = self.value + 1

//class Dummy:
//  def do_add(counter):
//    counter.add()

//x = Counter()
//y = x

//x.add()
//y.add()

//print x.value

//d = Dummy()
//d.do_add(x)

//print y.value
//)");

//    ostringstream output;
//    RunMythonProgram(input, output);

//    ASSERT_EQUAL(output.str(), "2\n3\n");
//}

//void TestAll() {
//    TestRunner tr;
//    parse::RunOpenLexerTests(tr);
//    runtime::RunObjectHolderTests(tr);
//    runtime::RunObjectsTests(tr);
//    ast::RunUnitTests(tr);
//    TestParseProgram(tr);

//    RUN_TEST(tr, TestSimplePrints);
//    RUN_TEST(tr, TestAssignments);
//    RUN_TEST(tr, TestArithmetics);
//    RUN_TEST(tr, TestVariablesArePointers);
//}

//}  // namespace

//int main() {
//    try {
//        TestAll();

//        RunMythonProgram(cin, cout);
//    } catch (const std::exception& e) {
//        std::cerr << e.what() << std::endl;
//        return 1;
//    }
//    return 0;
//}
