#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/optional.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

/*
Operators.

Description             PEG     Spirit Qi
Sequence                a b     a >> b
Alternative             a | b   a | b
Zero of more (Kleene)   a*      *a
One or more (Plus)      a+      +a
Optional                a?      -a

// and&not predicate can provide basic look-ahead
And-predicate           &a      &a          it matchs a without consuming it
Not-predicate           !a      !a          if a does match the parser is succesful without consuming a

Difference                      a - b       e.g. '"' >> *(char_ - '"') >> '"' matches literal string "string"
Expectation                     a > b       a MUST be followed by b; no backtracking allowed. a nonmatch throws exception_failure<iter>
List                            a % b       shortcut for a >> *( b >> a); int_ % ',' matches any sequence of integers sparated by ',' eg: "9,2,4,...,-2"
Permutation                     a ^ b       parse a,b in any order, each element can occur 0:1 times, e.g. *(char_('A') ^ 'C' ^ 'T' ^ 'G') matches "ACTGGCTAGACT"
Sequential Or                   a || b      shortcut for: a >> -b | b  , e.g. int_ || ('.' >> int_)  matches any of "123.12", ".456", "123"
*/

/*
antlr

DEFINE "define"
FROM "from"
WHERE "where"
CONSUMING "consuming"
END ";"

DEFINE complex_event_definition FROM trigger_events_pattern (WHERE fields_definition)? (CONSUMING consuming_events)? END

*/

//--

struct tesla_rule{
  std::string complex_event_definition_;
};

//--

BOOST_FUSION_ADAPT_STRUCT(
  tesla_rule,
  (boost::optional<std::string>, complex_event_definition_)
)

//--

std::ostream& operator<<(std::ostream& os, tesla_rule const& r){ return os; }

//--

template<typename Iterator>
struct tesla_grammar : qi::grammar<Iterator, tesla_rule(), ascii::space_type>{
  tesla_grammar() : tesla_grammar::base_type(expression){
    using namespace qi;

    expression;
  }

  qi::rule<Iterator, tesla_rule(), ascii::space_type> expression;
};

//g++ file.cpp -std=c++11

int main( /* ... */ ){
  std::cout << "\n";

  std::string line;
  while (std::getline(std::cin, line)){
    if (line.empty()) break;

    auto iter = line.begin();
    auto end = line.end();

    ascii::space_type ws;
    tesla_grammar<std::string::iterator> gram;

    tesla_rule r;
    if (phrase_parse(iter, end, gram, ws, r) && iter == end){
      std::cout << "Parsing succeeded - result: " << r << "\n";
    }else{
      std::string rest(iter, end);
      std::cout << "Parsing failed - stopped at: \" " << rest << "\"\n";
    }
  }

  std::cout << "Bye... :-) \n";
  return 0;
}
