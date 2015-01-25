#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <utility>

using std::vector;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

using  boost::optional;
using boost::variant;
using std::string;

/*
Just as a note, here (at the moment) I am only trying to match as much as possible the original grammar,
but I believe that the grammar requires some refactoring.
*/

typedef variant<string, int, float, bool> static_value;

typedef std::string event_name;
typedef std::string attribute_name;
typedef std::string parameter_name;

// attribute_type = ( "string" | "int" | "float" | "bool" )
// attribute_declaration = attribute_name >> ':' attribute_type;
struct attribute_declaration{
  enum attribute_type { string_type, int_type, float_type, bool_type };

  attribute_name attribute_name_;
  attribute_type attribute_type_;
};

// event_definition = event_name >> '(' >> -( attribute_declaration % ',' ) >> ')';
struct event_definition{
  event_name event_name_;
  optional<vector<attribute_declaration> > attributes_declaration_;
};

// attribute_reference = event_name >> '.' >> attribute_name;
struct attribute_reference{
  event_name event_name_;
  attribute_name attribute_name_;
};

// parameter_mapping = attribute_name >> "=>" >> parameter_name;
struct parameter_mapping{
  attribute_name attribute_name_;
  parameter_name parameter_name_;
};

// parameter_atom = ( attribute_reference | parameter_name | static_value )
typedef variant< attribute_reference, parameter_name, static_value  > parameter_atom;

struct expr{

};

// within_reference = ( "within" >> uint_ >> "from" >> event_name )
struct within_reference{
  unsigned int delta_;
  event_name event_name_;
};

// between_reference = ( "between" >> event_name >> "and" >> event_name )
struct between_reference{
  event_name fst_event_name_;
  event_name snd_event_name_;
};

// predicate_reference = ( within_reference | between_reference )
typedef variant<within_reference, between_reference> predicate_reference;

// TRIGGER PATTERN DEFINITION SECTION

// op_type = ( "==", ">", "<", "!=", "&", "|" )     !?
// simple_attribute_constraint = ( atrribute_name >> op_type >> static_value );
struct simple_attribute_constraint{
  enum op_type{ eq_op, gt_op, lt_op, neq_op, and_op, or_op };

  attribute_name attribute_name_;
  op_type op_;
  static_value static_value_;
};

// complex_attribute_constraint = ( '[' >> attribute_type >> ']' >> attribute_name >> op_type >> expr )
struct complex_attribute_constraint{
  attribute_declaration::attribute_type attribute_type;
  attribute_name attribute_name_;
  simple_attribute_constraint::op_type op_type_;
  expr expr_;
};

// predicate_parameter = ( parameter_mapping, simple_attribute_constraint | complex_attribute_constraint );
typedef variant<parameter_mapping, simple_attribute_constraint, complex_attribute_constraint> predicate_parameter;

// predicate = ( event_name >> -( predicate_parameter % ',' ) );
struct predicate{
  event_name event_name_;
  optional<vector<predicate_parameter> > predicate_parameters_;
};

// selection_policy = ( "each" | "first" | "last" );
// positive_predicate = ( "and" >> selection_policy >> predicate >> within_reference );
struct positive_predicate{
  enum selection_policy { each_policy, first_policy, last_policy };
  selection_policy selection_policy_;

  predicate predicate_;
  within_reference reference_;
};

// negatine_predicate = ( "and not" >> predicate >> predicate_reference );
struct negative_predicate{
  predicate predicate_;
  predicate_reference reference_;
};

// predicate_pattern = ( negative_predicate | positive_predicate )
typedef boost::variant<positive_predicate, negative_predicate> pattern_predicate;

// trigger_pattern_definition = ( predicate >> -( pattern_predicate % ',' ) );
struct trigger_pattern_definition{
  predicate trigger_predicate_;
  optional<vector<pattern_predicate> > pattern_predicates_;
};

// ATTRIBUTES DEFINITION SECTION

// simple_attribute_definition = ( attribute_name >> ":=" >> static_value )
struct simple_attribute_definition{
  attribute_name attribute_name_;
  static_value static_value_;
};

// complex_attribute_definition = ( attribute_name >> ":=" >> expr )
struct complex_attribute_definition{
  attribute_name attribute_name_;
  expr expr_;
};

// attribute_definition = ( simple_attribute_definition | complex_atrribute_definition )
typedef variant<simple_attribute_definition, complex_attribute_definition> attribute_definition;

// RULE DEFINITION SECTION

/*
rule %=   ( "define" >> event_definition )
     >>   ( "from" >> trigger_pattern_definition )
     >> - ( "where" >> ( atrributes_definition % ',' ) )
     >> - ( "consuming" >> ( event_name % ',' ) )
     >> ';';
*/
struct tesla_rule{
  event_definition event_definition_; 
  trigger_pattern_definition trigger_pattern_;
  optional<vector<attribute_definition> > attributes_definition_;
  optional<vector<event_name> > events_to_consume_;
};

//--

// boost fusion stuff

//--

//to do...
std::ostream& operator<<(std::ostream& os, tesla_rule const& r){ return os << "..."; }

//--

template<typename It>
struct tesla_grammar : qi::grammar<It, tesla_rule(), ascii::space_type>{
  tesla_grammar() : tesla_grammar::base_type(tesla_rule_){
    using namespace qi;
/*
    strlit_ =  ('"' >> *~char_('"') >> '"');

    event_name_ = (char_("A-Z") >> *char_("0-9a-zA-Z"));
    attribute_name_ = (char_("a-z") >> *char_("0-9a-zA-Z"));
    parameter_name_ = (char_('$') >> attribute_name_);

    static_value_ = ( strlit_ | int_ | float_ | bool_ );

    //-- 1

    type_token_.add
      ("string", attribute_declaration::string_type)
      ("int", attribute_declaration::int_type)
      ("float", attribute_declaration::float_type)
      ("bool", attribute_declaration::bool_type);
    attribute_type_ = type_token_;

    attribute_declaration_ = attribute_name_ >> ':' >> attribute_type_;

    event_definition_ = event_name_ >> '(' >> -(attribute_declaration_ % ',') >> ')';

    //-- 2

    parameter_mapping_ = (attribute_name_ >> "=>" >> parameter_name_);

    op_type_token_.add
      ("==", attribute_constraint::eq_op)
      (">", attribute_constraint::gt_op)
      ("<", attribute_constraint::lt_op)
      ("!=", attribute_constraint::neq_op)
      ("&", attribute_constraint::and_op)
      ("|", attribute_constraint::or_op);
    op_type_ = op_type_token_;

    attribute_constraint_ = ( attribute_name_ >> op_type_ >> static_value_ );

    predicate_parameter_ = parameter_mapping_ | attribute_constraint_;
    predicate_ = event_name_ >> '(' >> -(predicate_parameter_ % ',') >> ')';

    within_reference_ = lexeme["within"] >> uint_ >> lexeme["from"] >> event_name_;
    between_reference_ = lexeme["between"] >> event_name_ >> lexeme["and"] >> event_name_;

    selection_policy_token_.add
      ("each", positive_predicate::each_policy)
      ("first", positive_predicate::first_policy)
      ("last", positive_predicate::last_policy);
    selection_policy_ = selection_policy_token_;

    positive_predicate_ = lexeme ["and"] >> selection_policy_ >> predicate_ >> within_reference_;
    negative_predicate_ = lexeme ["and not"] >> predicate_ >> (within_reference_ | between_reference_);

    trigger_pattern_ = predicate_ >> *( positive_predicate_ | negative_predicate_ );

    //-- 3

    //-- 4

    consuming_events_ = (event_name_ % ',');

    //-- Rule

    tesla_rule_ %=    lexeme ["define"]   >> event_definition_
                >>    lexeme ["from"]     >> trigger_pattern_
                >> -( lexeme["where"]     >> attribute_name_    )
                >> -( lexeme["consuming"] >> consuming_events_  )
                >> ';';
*/
  }
/*
  qi::rule<It, std::string()> event_name_;
  qi::rule<It, std::string()> attribute_name_;
  qi::rule<It, std::string()> parameter_name_;
  qi::rule<It, std::string()> strlit_;

  qi::rule<It, static_value()> static_value_;

  qi::symbols<char, attribute_declaration::attribute_type> type_token_;
  qi::rule<It, attribute_declaration::attribute_type()> attribute_type_;

  qi::rule<It, attribute_declaration(), ascii::space_type> attribute_declaration_;

  qi::rule<It, event_definition(), ascii::space_type> event_definition_;
  
  qi::rule<It, parameter_mapping(), ascii::space_type> parameter_mapping_;
  
  qi::symbols<char, attribute_constraint::op_type> op_type_token_;
  qi::rule<It, attribute_constraint::op_type() > op_type_;
  qi::rule<It, attribute_constraint(), ascii::space_type> attribute_constraint_;
 
  qi::rule<It, within_reference(), ascii::space_type> within_reference_;
  qi::rule<It, between_reference(), ascii::space_type> between_reference_;

  qi::symbols<char, positive_predicate::selection_policy> selection_policy_token_;
  qi::rule<It, positive_predicate::selection_policy() > selection_policy_;

  qi::rule<It, predicate_parameter(), ascii::space_type> predicate_parameter_;
  qi::rule<It, predicate(), ascii::space_type> predicate_;
  qi::rule<It, positive_predicate(), ascii::space_type> positive_predicate_;
  qi::rule<It, negative_predicate(), ascii::space_type> negative_predicate_;

  qi::rule<It, trigger_pattern(), ascii::space_type> trigger_pattern_;
  
  qi::rule<It, std::vector<std::string>(), ascii::space_type> consuming_events_;
*/
  qi::rule<It, tesla_rule(), ascii::space_type> tesla_rule_;
};

// As a requirement you have to have boost installed...
//Compile: g++ file_name.cpp

typedef std::string::const_iterator iter;

void validate( std::string const& phrase ){
  std::cout << phrase;

  iter curr = phrase.begin();
  iter end = phrase.end();

  ascii::space_type ws;
  tesla_grammar<iter> gram;

  tesla_rule rule;
  if (phrase_parse(curr, end, gram, ws, rule) && curr == end){
    //std::cout << " - parsing succeeded - result: " << rule << "\n";
    std::cout << " - parsing succeeded\n";
  }else{
    //std::string rest(beg, end);
    //std::cout << " - parsing failed - stopped at: \" " << rest << "\"\n";
    std::cout << " - parsing failed\n";
  }
}

void interactive(){
  std::string line;
  while (std::getline(std::cin, line)){
    if (line.empty()) break;
    validate(line);
  }

  std::cout << "Bye... :-) \n";
}

void tests(){
  validate("define A() from B();");
  validate("define A() from B() and each C() within 5000 from B;");
  validate("define A() from B() and last C() within 5000 from B;");
  validate("define A() from B() and first C() within 5000 from B;");

  validate("define A() from B() and each C() within 5000 from B and not D() within 2000 from B;");
  validate("define A() from B() and each C() within 5000 from B and not D() between B and C;");

  validate("define Fire( area: string ) from TemperatureReading( area => $a, temperature > 50 ) and not RainReading() within 500000 from TemperatureReading;");

  //need to finish the parser in order to parse this...
  //validate("define Fire( area: string ) from TemperatureReading( area => $a, temperature > 50 ) and not RainReading( area == $a ) within 500000 from TemperatureReading where Fire.area = Temperature.area");
}

int main( int argc, char **argv ){
  std::cout << "\n";

  if(argc == 2 && std::string(argv[1]) == "-interactive"){
    interactive();
  }else
    tests();

  std::cout << "\n";

  return 0;
}
