#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse
{

bool operator==(const Token& lhs, const Token& rhs)
{
    using namespace token_type;

    if (lhs.index() != rhs.index())
    {
        return false;
    }
    if (lhs.Is<Char>())
    {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>())
    {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>())
    {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>())
    {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs)
{
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input)
    : input_(input)
{
    ParseNextToken();
    while (current_token_ == token_type::Newline())
    {
        ParseNextToken();
    }
}

const Token& Lexer::CurrentToken() const
{
    return current_token_;
}

Token Lexer::NextToken()
{
    ParseNextToken();
    return current_token_;
}

void Lexer::ParseNextToken()
{
    char c = input_.peek();
    if (!input_ || c == -1)
    {
        ParseEnd();
    }
    else if (c == '\n')
    {
        ParseLineEnd();
    }
    else if (c == ' ')
    {
        ParseSpaces();
    }
    else if (c == '#')
    {
        SkipComment();
        ParseNextToken();
    }
    else
    {
        ParseToken();
    }
}

bool Lexer::IsIndentChanged()
{
    if ((current_token_ == token_type::Newline() || current_token_ == token_type::Indent())
        && line_indent_ > current_indent_)
    {
        ++current_indent_;
        current_token_ = token_type::Indent();
        return true;
    }
    else if ((current_token_ == token_type::Newline() || current_token_ == token_type::Dedent())
             && line_indent_ < current_indent_)
    {
        --current_indent_;
        current_token_ = token_type::Dedent();
        return true;
    }
    return false;
}

void Lexer::ParseEnd()
{
    if (current_token_ == token_type::Eof())
    {
        current_token_ = token_type::Eof();
    }
    else if (!is_line_start_ && current_token_ != token_type::Newline())
    {
        current_token_ = token_type::Newline();
    }
    else
    {
        if (current_indent_ > 0)
        {
            line_indent_ = --current_indent_;
            current_token_ = token_type::Dedent();
        }
        else
        {
            current_token_ = token_type::Eof();
        }
    }
}

void Lexer::ParseLineEnd()
{
    line_indent_ = 0;
    is_line_start_ = true;
    input_.get();
    if (current_token_ != token_type::Newline())
    {
        current_token_ = token_type::Newline();
    }
    else
    {
        ParseNextToken();
    }
}

void Lexer::ParseSpaces()
{
    int spaces_count = 0;
    while (input_.peek() == ' ')
    {
        ++spaces_count;
        input_.get();
    }
    if (input_.peek() == '#')
    {
        SkipComment();
        ParseNextToken();
    }
    else
    {
        if (input_.peek() == '\n')
        {
            ParseLineEnd();
        }
        else
        {
            if (!input_ || input_.peek() == -1)
            {
                ParseEnd();
            }
            else if (is_line_start_)
            {
                line_indent_ = spaces_count / 2;
                if (!IsIndentChanged())
                {
                    ParseNextToken();
                }
            }
            else
            {
                ParseNextToken();
            }
        }
    }
}

void Lexer::ParseToken()
{
    is_line_start_ = false;
    char c = input_.peek();
    if (isdigit(c))
    {
        ParseNumber();
    }
    else if (c == '"' || c == '\'')
    {
        ParseString();
    }
    else if (isalpha(c) || c == '_')
    {
        ParseName();
    }
    else
    {
        ParseChar();
    }
}

void Lexer::ParseNumber()
{
    current_token_ = ReadNumber(input_);
}

void Lexer::ParseString()
{
    current_token_ = ReadString(input_);
}

void Lexer::ParseName()
{
    if (!IsIndentChanged())
    {
        token_type::Id id = ReadId(input_);
        string str = id.value;
        if (str == "class"s)
        {
            current_token_ = token_type::Class();
        }
        else if (str == "return"s)
        {
            current_token_ = token_type::Return();
        }
        else if (str == "if"s)
        {
            current_token_ = token_type::If();
        }
        else if (str == "else"s)
        {
            current_token_ = token_type::Else();
        }
        else if (str == "def"s)
        {
            current_token_ = token_type::Def();
        }
        else if (str == "print"s)
        {
            current_token_ = token_type::Print();
        }
        else if (str == "and"s)
        {
            current_token_ = token_type::And();
        }
        else if (str == "or"s)
        {
            current_token_ = token_type::Or();
        }
        else if (str == "not"s)
        {
            current_token_ = token_type::Not();
        }
        else if (str == "None"s)
        {
            current_token_ = token_type::None();
        }
        else if (str == "True"s)
        {
            current_token_ = token_type::True();
        }
        else if (str == "False"s)
        {
            current_token_ = token_type::False();
        }
        else
        {
            current_token_ = id;
        }
    }
}

void Lexer::ParseChar()
{
    char ch = input_.get();
    string compare_symbol(1, ch);
    compare_symbol += input_.peek();
    if (compare_symbol == "=="s)
    {
        input_.get();
        current_token_ = token_type::Eq();

    }
    else if (compare_symbol == "!="s)
    {
        input_.get();
        current_token_ = token_type::NotEq();
    }
    else if (compare_symbol == "<="s)
    {
        input_.get();
        current_token_ = token_type::LessOrEq();
    }
    else if (compare_symbol == ">="s)
    {
        input_.get();
        current_token_ = token_type::GreaterOrEq();
    }
    else
    {
        current_token_ = token_type::Char{ ch };
    }
}

void Lexer::SkipComment()
{
    while (input_ && input_.peek() != -1 && input_.peek() != '\n')
    {
        input_.get();
    }
    if (input_.peek() == '\n' && is_line_start_)
    {
        input_.get();
    }
}

token_type::Number ReadNumber(std::istream &input)
{
    int result = 0;
    while (isdigit(input.peek()))
    {
        result *= 10;
        result += input.get() - '0';
    }
    return token_type::Number{result};
}

token_type::String ReadString(std::istream &input)
{
    string str;
    char string_end = input.get();
    char c = input.get();
    while (c != string_end)
    {
        if (c != '\\')
        {
            str += c;
        }
        else
        {
            c = input.get();
            switch (c)
            {
            case 'n':
                str += '\n';
                break;
            case '"':
                str += '\"';
                break;
            case '\'':
                str += '\'';
                break;
            case 't':
                str += '\t';
                break;
            case '\\':
                str += '\\';
                break;
            default:
                break;
            }
        }
        if (!input.get(c))
        {
//			throw
        }
    }
    return token_type::String{move(str)};
}

token_type::Id ReadId(std::istream &input)
{
    string str;
    char c = input.peek();
    while (c == '_' || isalpha(c) || isdigit(c))
    {
        str += input.get();
        c = input.peek();
    }
    return token_type::Id{move(str)};
}

}  // namespace parse
