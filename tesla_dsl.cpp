#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <utility>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

/*  "Pseudo" ... Spirit Qi

DEFINE = "define"
FROM = "from"
WHERE = "where"
CONSUMING = "consuming"

WITHIN = "within"
BETWEEN = "between"

AND = "and"
NOT = "not"

EACH = "each"
FIRST = "first"
LAST = "last"

EVENT_NAME = char_("A-Z") >> *char_("0-9a-zA-Z")
ATTRIBUTE_NAME = char_("a-z") >> *char_("0-9a-z")
ATTRIBUTE_TYPE = ( "int" | "float" | "string" | "bool" )

PARAMETER_NAME = ('$' >> ATTRIBUTE_NAME)

INTEGER_REF = uint_

INTEGER = int_
FLOAT = float_
STRING = ('"' >> *~char_('"') >> '"')
BOOL = bool_

STATIC_REFERENCE = ( INTEGER | FLOAT | STRING | BOOL )

ATTRIBUTE_DECLARATION = (ATTRIBUTE_NAME >> ':' >> ATTRIBUTE_TYPE)

EVENT_DEFINITION = EVENT_NAME >> "(" >> *ATTRIBUTE_DECLARATION >> ")"   /// 1

PARAMETER_MAPPING = ATTRIBUTE_NAME "=>" PARAMETER_NAME

ATTRIBUTE_CONSTRAINT = ATTRIBUTE_NAME >> OPERATOR >> STATIC_REFERENCE

PREDICATE_PARAMETER = PARAMETER_MAPPING | ATTRIBUTE_CONSTRAINT

PREDICATE = EVENT_NAME >> "(" >> -(PREDICATE_PARAMETER % ',')  >> ")"

TRIGGER_PREDICATE = PREDICATE

WITHIN_REFERENCE = WITHIN >> INTEGER_REF >> FROM >> EVENT_NAME
BETWEEN_REFERENCE = BETWEEN >> EVENT_NAME >> AND >> EVENT_NAME

SELECTION_POLICY = (EACH | FIRST | LAST)

POSITIVE_PREDICATE = AND >> SELECTION_POLICY >> PREDICATE >> WITHIN_REFERENCE
NEGATIVE_PREDCIATE = AND >> NOT >> PREDICATE >> ( WITHIN_REFERENCE | BETWEEN_REFERENCE )

PREDICATES_PATTERN = *(POSITIVE_PREDICATE | NEGATIVE_PREDICATE)


TRIGGER_PATTERNS = TRIGGER_PREDICATE >> -PREDICATES_PATTERN           /// 2


ATTRIBUTE_DEFINITIONS = ...                                           /// 3


CONSUMING_EVENTS = (EVENT_NAME % ',')                                 /// 4

/// FINAL
RULE = DEFINE >> EVENT_DEFINITION >> FROM >> TRIGGER_PATTERNS >> -(WHERE ATTRIBUTE_DEFININITIONS) >> -(CONSUMING CONSUMING_EVENTS) >> ';'

*/

//--

struct attribute_declaration{
  enum attribute_type { string_type, int_type, float_type, bool_type };

  std::string attribute_name_;
  attribute_type attribute_type_;
};

struct event_definition{
  std::string event_name_;
  boost::optional<std::vector<attribute_declaration> > attribute_declarations_;
};

struct parameter_mapping{
  std::string attribute_name_;
  std::string parameter_name_;
};

typedef boost::variant<std::string, int, float, bool> static_value;

struct attribute_constraint{
  enum op_type{ eq_op, gt_op, lt_op, neq_op, and_op, or_op };

  std::string attribute_name_;
  op_type op_;
  static_value static_value_;
};

typedef boost::variant<parameter_mapping, attribute_constraint> predicate_parameter;

struct predicate{
  std::string event_name_;
  boost::optional<std::vector<predicate_parameter> > predicate_parameters_;
};

struct within_reference{
  unsigned int delta_;
  std::string event_name_;
};

struct between_reference{
  std::string fst_event_name_;
  std::string snd_event_name_;
};

typedef boost::variant<within_reference, between_reference> reference;

struct positive_predicate{
  enum selection_policy { each_policy, first_policy, last_policy };
  selection_policy selection_policy_;

  predicate predicate_;
  within_reference reference_;
};

struct negative_predicate{
  predicate predicate_;
  reference reference_;
};

typedef boost::variant<positive_predicate, negative_predicate> pattern_predicate;

struct trigger_pattern{
  predicate trigger_predicate_;
  boost::optional<std::vector<pattern_predicate> > pattern_predicates_;
};

struct tesla_rule{
  event_definition event_definition_; 
  trigger_pattern trigger_pattern_;
  boost::optional<std::string> attribute_definitions_;
  boost::optional<std::vector<std::string> > consuming_events_;
};

//--

BOOST_FUSION_ADAPT_STRUCT(
  attribute_declaration,

  (std::string, attribute_name_)
  (attribute_declaration::attribute_type, attribute_type_)
)

BOOST_FUSION_ADAPT_STRUCT(
  event_definition,

  (std::string, event_name_)
  (boost::optional<std::vector<attribute_declaration> >, attribute_declarations_)
)

BOOST_FUSION_ADAPT_STRUCT(
  parameter_mapping,

  (std::string, attribute_name_)
  (std::string, parameter_name_)
)

BOOST_FUSION_ADAPT_STRUCT(
  attribute_constraint,

  (std::string, attribute_name_)
  (attribute_constraint::op_type, op_)
  (static_value, static_value_)
)

BOOST_FUSION_ADAPT_STRUCT(
  predicate,
  
  (std::string, event_name_)
  (boost::optional<std::vector<predicate_parameter> >, predicate_parameters_)
)

BOOST_FUSION_ADAPT_STRUCT(
  within_reference,

  (unsigned int, delta_)
  (std::string, event_name_)
)

BOOST_FUSION_ADAPT_STRUCT(
  between_reference,

  (std::string, fst_event_name_)
  (std::string, snd_event_name_)
)

BOOST_FUSION_ADAPT_STRUCT(
  positive_predicate,
  
  (positive_predicate::selection_policy, selection_policy_)

  (predicate, predicate_)
  (within_reference, reference_)
)

BOOST_FUSION_ADAPT_STRUCT(
  negative_predicate,

  (predicate, predicate_)
  (reference, reference_)
)

BOOST_FUSION_ADAPT_STRUCT(
  trigger_pattern,

  (predicate, trigger_predicate_)
  (boost::optional<std::vector<pattern_predicate> >, pattern_predicates_)
)

BOOST_FUSION_ADAPT_STRUCT(
  tesla_rule,

  (event_definition, event_definition_)
  (trigger_pattern, trigger_pattern_)
  (boost::optional<std::string>, attribute_definitions_)
  (boost::optional<std::vector<std::string> >, consuming_events_)
)

//--

//to do...
std::ostream& operator<<(std::ostream& os, tesla_rule const& r){ return os << "..."; }

//--

template<typename Iterator>
struct tesla_grammar : qi::grammar<Iterator, tesla_rule(), ascii::space_type>{
  tesla_grammar() : tesla_grammar::base_type(tesla_rule_){
    using namespace qi;

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

    //enum op_type{ eq_op, gt_op, lt_op, neq_op, and_op, or_op };
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
  }

  qi::rule<Iterator, std::string() > event_name_;
  qi::rule<Iterator, std::string() > attribute_name_;
  qi::rule<Iterator, std::string() > parameter_name_;
  qi::rule<Iterator, std::string() > strlit_;

  qi::rule<Iterator, static_value() > static_value_;

  qi::symbols<char, attribute_declaration::attribute_type> type_token_;
  qi::rule<Iterator, attribute_declaration::attribute_type() > attribute_type_;

  qi::rule<Iterator, attribute_declaration(), ascii::space_type> attribute_declaration_;

  qi::rule<Iterator, event_definition(), ascii::space_type> event_definition_;
  
  qi::rule<Iterator, parameter_mapping(), ascii::space_type> parameter_mapping_;
  
  qi::symbols<char, attribute_constraint::op_type> op_type_token_;
  qi::rule<Iterator, attribute_constraint::op_type() > op_type_;
  qi::rule<Iterator, attribute_constraint(), ascii::space_type> attribute_constraint_;
 
  qi::rule<Iterator, within_reference(), ascii::space_type> within_reference_;
  qi::rule<Iterator, between_reference(), ascii::space_type> between_reference_;

  qi::symbols<char, positive_predicate::selection_policy> selection_policy_token_;
  qi::rule<Iterator, positive_predicate::selection_policy() > selection_policy_;

  qi::rule<Iterator, predicate_parameter(), ascii::space_type> predicate_parameter_;
  qi::rule<Iterator, predicate(), ascii::space_type> predicate_;
  qi::rule<Iterator, positive_predicate(), ascii::space_type> positive_predicate_;
  qi::rule<Iterator, negative_predicate(), ascii::space_type> negative_predicate_;

  qi::rule<Iterator, trigger_pattern(), ascii::space_type> trigger_pattern_;
  
  qi::rule<Iterator, std::vector<std::string>(), ascii::space_type> consuming_events_;
  
  qi::rule<Iterator, tesla_rule(), ascii::space_type> tesla_rule_;
};

//g++ file.cpp

typedef std::string::const_iterator iter_t;

void validate( std::string const& phrase ){
  std::cout << phrase;

  iter_t iter = phrase.begin();
  iter_t end = phrase.end();

  ascii::space_type ws;
  tesla_grammar<iter_t> gram;

  tesla_rule rule;
  if (phrase_parse(iter, end, gram, ws, rule) && iter == end){
    std::cout << " - parsing succeeded - result: " << rule << "\n";
  }else{
    std::string rest(iter, end);
    std::cout << " - parsing failed - stopped at: \" " << rest << "\"\n";
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
  validate("define A() from B() and each A() within 5000 from B;");
  validate("define A() from B() and last A() within 5000 from B;");
  validate("define A() from B() and first A() within 5000 from B;");

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
