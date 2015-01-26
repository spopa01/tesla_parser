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

/*
Just as a note: here (at the moment) I am only trying to match as much as possible the original grammar,
but I believe that the original grammar requires refactoring / improving.
*/

typedef boost::variant<std::string, int, float, bool> static_value;

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

BOOST_FUSION_ADAPT_STRUCT(
  attribute_declaration,

  (attribute_name, attribute_name_)
  (attribute_declaration::attribute_type, attribute_type_)
)

// event_definition = event_name >> '(' >> -( attribute_declaration % ',' ) >> ')';
struct event_definition{
  event_name event_name_;
  boost::optional<std::vector<attribute_declaration> > attributes_declaration_;
};

BOOST_FUSION_ADAPT_STRUCT(
  event_definition,

  (event_name, event_name_)
  (boost::optional<std::vector<attribute_declaration> >, attributes_declaration_)
)

// attribute_reference = event_name >> '.' >> attribute_name;
struct attribute_reference{
  event_name event_name_;
  attribute_name attribute_name_;
};

BOOST_FUSION_ADAPT_STRUCT(
  attribute_reference,

  (event_name, event_name_)
  (attribute_name, attribute_name_)
)

// parameter_mapping = attribute_name >> "=>" >> parameter_name;
struct parameter_mapping{
  attribute_name attribute_name_;
  parameter_name parameter_name_;
};

BOOST_FUSION_ADAPT_STRUCT(
  parameter_mapping,

  (attribute_name, attribute_name_)
  (parameter_name, parameter_name_)
)

// parameter_atom = ( attribute_reference | parameter_name | static_value )
typedef boost::variant< attribute_reference, parameter_name, static_value  > parameter_atom;

struct expression;
struct term;
struct factor;

struct aggregate_atom;
typedef boost::variant< parameter_atom, boost::recursive_wrapper<aggregate_atom>, 
                        boost::recursive_wrapper<factor>, boost::recursive_wrapper<expression> > atom;

struct factor{
  enum sign { pos_sgn, neg_sgn };

  sign sign_; // +,-
  atom atom_;
};

BOOST_FUSION_ADAPT_STRUCT(
  factor,

  (factor::sign, sign_)
  (atom, atom_)
)

struct term{
  enum op_type { mul_op, div_op, add_op, sub_op };

  op_type op_; // *, /, +, -
  atom atom_;
};

BOOST_FUSION_ADAPT_STRUCT(
  term,

  (term::op_type, op_)
  (atom, atom_)
)

struct expression{
  atom first_;
  std::vector<term> rest_;
};

BOOST_FUSION_ADAPT_STRUCT(
  expression,

  (atom, first_)
  (std::vector<term>, rest_)
)

// within_reference = ( "within" >> uint_ >> "from" >> event_name )
struct within_reference{
  unsigned int delta_;
  event_name event_name_;
};

BOOST_FUSION_ADAPT_STRUCT(
  within_reference,

  (unsigned int, delta_)
  (event_name, event_name_)
)

// between_reference = ( "between" >> event_name >> "and" >> event_name )
struct between_reference{
  event_name fst_event_name_;
  event_name snd_event_name_;
};

BOOST_FUSION_ADAPT_STRUCT(
  between_reference,

  (event_name, fst_event_name_)
  (event_name, snd_event_name_)
)

// predicate_reference = ( within_reference | between_reference )
typedef boost::variant<within_reference, between_reference> predicate_reference;

// TRIGGER PATTERN DEFINITION SECTION

// op_type = ( "==", ">", "<", "!=", "&", "|" )     !?
// simple_attribute_constraint = ( atrribute_name >> op_type >> static_value );
struct simple_attribute_constraint{
  enum op_type{ eq_op, gt_op, lt_op, neq_op, and_op, or_op };

  attribute_name attribute_name_;
  op_type op_;
  static_value static_value_;
};

BOOST_FUSION_ADAPT_STRUCT(
  simple_attribute_constraint,

  (attribute_name, attribute_name_)
  (simple_attribute_constraint::op_type, op_)
  (static_value, static_value_)
)

// complex_attribute_constraint = ( '[' >> attribute_type >> ']' >> attribute_name >> op_type >> expression )
struct complex_attribute_constraint{
  attribute_declaration::attribute_type attribute_type_;
  attribute_name attribute_name_;
  simple_attribute_constraint::op_type op_type_;
  expression expression_;
};

BOOST_FUSION_ADAPT_STRUCT(
  complex_attribute_constraint,

  (attribute_declaration::attribute_type, attribute_type_)
  (attribute_name, attribute_name_)
  (simple_attribute_constraint::op_type, op_type_)
  (expression, expression_)
)

// attribute_constraint = simple_attribute_constraint | complex_attribute_constraint;
typedef boost::variant< simple_attribute_constraint, complex_attribute_constraint > attribute_constraint;

// aggregate_atom = aggregation_type >> '(' >> attribute_reference >> '(' >> -(attribute_constraint % ',')  >> ')' >> ')' >> predicate_reference;
struct aggregate_atom{
  enum aggregation_type{ avg_agg, sum_agg, min_agg, max_agg, count_agg };

  aggregation_type aggregation_type_;
  attribute_reference attribute_reference_;
  boost::optional<std::vector<attribute_constraint> > attribute_constraints_;
  predicate_reference reference_;
};

BOOST_FUSION_ADAPT_STRUCT(
  aggregate_atom,

  (aggregate_atom::aggregation_type, aggregation_type_)
  (attribute_reference, attribute_reference_)
  (boost::optional<std::vector<attribute_constraint> >, attribute_constraints_)
  (predicate_reference, reference_)
)

// predicate_parameter = ( parameter_mapping, simple_attribute_constraint | complex_attribute_constraint );
typedef boost::variant<parameter_mapping, simple_attribute_constraint, complex_attribute_constraint> predicate_parameter;

// predicate = ( event_name >> -( predicate_parameter % ',' ) );
struct predicate{
  event_name event_name_;
  boost::optional<std::vector<predicate_parameter> > predicate_parameters_;
};

BOOST_FUSION_ADAPT_STRUCT(
  predicate,

  (event_name, event_name_)
  (boost::optional<std::vector<predicate_parameter> >, predicate_parameters_)
)

// selection_policy = ( "each" | "first" | "last" );
// positive_predicate = ( "and" >> selection_policy >> predicate >> within_reference );
struct positive_predicate{
  enum selection_policy { each_policy, first_policy, last_policy };
  selection_policy selection_policy_;

  predicate predicate_;
  within_reference reference_;
};

BOOST_FUSION_ADAPT_STRUCT(
  positive_predicate,

  (positive_predicate::selection_policy, selection_policy_)
  (predicate, predicate_)
  (within_reference, reference_)
)

// negatine_predicate = ( "and not" >> predicate >> predicate_reference );
struct negative_predicate{
  predicate predicate_;
  predicate_reference reference_;
};

BOOST_FUSION_ADAPT_STRUCT(
  negative_predicate,

  (predicate, predicate_)
  (predicate_reference, reference_)
)

// predicate_pattern = ( negative_predicate | positive_predicate )
typedef boost::variant<positive_predicate, negative_predicate> pattern_predicate;

// trigger_pattern_definition = ( predicate >> -( pattern_predicate % ',' ) );
struct trigger_pattern_definition{
  predicate trigger_predicate_;
  boost::optional<std::vector<pattern_predicate> > pattern_predicates_;
};

BOOST_FUSION_ADAPT_STRUCT(
  trigger_pattern_definition,

  (predicate, trigger_predicate_)
  (boost::optional<std::vector<pattern_predicate> >, pattern_predicates_)
)

// ATTRIBUTES DEFINITION SECTION

// simple_attribute_definition = ( attribute_name >> "::=" >> static_value )
struct simple_attribute_definition{
  attribute_name attribute_name_;
  static_value static_value_;
};

BOOST_FUSION_ADAPT_STRUCT(
  simple_attribute_definition,

  (attribute_name, attribute_name_)
  (static_value, static_value_)
)

// complex_attribute_definition = ( attribute_name >> ":=" >> expression )
struct complex_attribute_definition{
  attribute_name attribute_name_;
  expression expression_;
};

BOOST_FUSION_ADAPT_STRUCT(
  complex_attribute_definition,

  (attribute_name, attribute_name_)
  (expression, expression_)
)

// attribute_definition = ( simple_attribute_definition | complex_atrribute_definition )
typedef boost::variant<simple_attribute_definition, complex_attribute_definition> attribute_definition;

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
  boost::optional<std::vector<attribute_definition> > attributes_definition_;
  boost::optional<std::vector<event_name> > events_to_consume_;
};

BOOST_FUSION_ADAPT_STRUCT(
  tesla_rule,

  (event_definition, event_definition_)
  (trigger_pattern_definition, trigger_pattern_)
  (boost::optional<std::vector<attribute_definition> >, attributes_definition_)
  (boost::optional<std::vector<event_name> >, events_to_consume_)
)

//-- I should really use (boost::spirit::)karma...........

//1

std::ostream& operator<<(std::ostream& os, attribute_declaration::attribute_type const& attr_type){
  switch( attr_type ){
    case attribute_declaration::string_type: os << "string"; break;
    case attribute_declaration::int_type: os << "int"; break;
    case attribute_declaration::float_type: os << "float"; break;
    case attribute_declaration::bool_type: os << "bool"; break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, attribute_declaration const& attr_dclr){
  return os << attr_dclr.attribute_name_ << ": " << attr_dclr.attribute_type_;
}

std::ostream& operator<<(std::ostream& os, event_definition const& def){
  os << def.event_name_ << "( ";
  if( def.attributes_declaration_ ){
    std::vector<attribute_declaration> attributes_declaration = *(def.attributes_declaration_);
    for( int i=0; i < attributes_declaration.size(); ++i )
      os << attributes_declaration[i] << (i < attributes_declaration.size()-1 ? ", " : "");
  }
  return os << " )";
}

//2

std::ostream& operator<<(std::ostream& os, parameter_mapping const& mp){
  return os << mp.attribute_name_ << "=>" << mp.parameter_name_;
}

std::ostream& operator<<(std::ostream& os, simple_attribute_constraint::op_type const& op){
  switch( op ){
    case simple_attribute_constraint::eq_op: os << "=="; break;
    case simple_attribute_constraint::gt_op: os << ">"; break;
    case simple_attribute_constraint::lt_op: os << "<"; break;
    case simple_attribute_constraint::neq_op: os << "!="; break;
    case simple_attribute_constraint::and_op: os << "|"; break;
    case simple_attribute_constraint::or_op: os << "&"; break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, simple_attribute_constraint const& cnstr){
  return os << cnstr.attribute_name_ << " " << cnstr.op_ << " " << cnstr.static_value_;
}

std::ostream& operator<<(std::ostream& os, complex_attribute_constraint const& cnstr){
  return os << "[***]";
}

std::ostream& operator<<(std::ostream& os, predicate const& pred){
  os << pred.event_name_ << "( ";
  if( pred.predicate_parameters_ ){
    std::vector< predicate_parameter > predicate_parameters = *(pred.predicate_parameters_);
    for( int i=0; i < predicate_parameters.size(); ++i ){
      os << predicate_parameters[i] << (i < predicate_parameters.size()-1 ? ", " : "");
    }
  }
  return os << " )";
}

std::ostream& operator<<(std::ostream& os, trigger_pattern_definition const& def){
  os << def.trigger_predicate_;
  return os;
}

//

std::ostream& operator<<(std::ostream& os, tesla_rule const& rule){
  os  << "define " << rule.event_definition_ << "\n"
      << "where " << rule.trigger_pattern_ << "\n"
      << ";";

  return os;
}

//--

template<typename It>
struct tesla_grammar : qi::grammar<It, tesla_rule(), ascii::space_type>{
  tesla_grammar() : tesla_grammar::base_type(tesla_rule_){
    using namespace qi;

    //-- 1

    strlit_ =  ('"' >> *~char_('"') >> '"');

    event_name_ = (char_("A-Z") >> *char_("0-9a-zA-Z"));
    attribute_name_ = (char_("a-z") >> *char_("0-9a-zA-Z"));
    parameter_name_ = (char_('$') >> attribute_name_);

    static_value_ = ( strlit_ | int_ | float_ | bool_ );

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
    
    attribute_reference_ = (event_name_ >> '.' >> attribute_name_);

    parameter_atom_ = ( attribute_reference_ | parameter_name_ | static_value_ );
    
    factor_sign_token_.add
      ("-", factor::neg_sgn)
      ("+", factor::pos_sgn);
   factor_sign_ = factor_sign_token_;

    term_mul_div_op_type_token_.add
      ("*", term::mul_op)
      ("/", term::div_op);
    term_mul_div_op_type_ = term_mul_div_op_type_token_;
    term_add_sub_op_type_token_.add
      ("+", term::add_op)
      ("-", term::sub_op);
    term_add_sub_op_type_ = term_add_sub_op_type_token_;

    expression_ = term_ >> *( term_add_sub_op_type_ >> term_ );
    term_ = factor_ >> *( term_mul_div_op_type_ >> factor_ );
    factor_ = parameter_atom_ | aggregate_atom_ | '(' >> expression_ >> ')' | (factor_sign_ >> factor_);
    
    within_reference_ = lexeme["within"] >> uint_ >> lexeme["from"] >> event_name_;
    between_reference_ = lexeme["between"] >> event_name_ >> lexeme["and"] >> event_name_;

    predicate_reference_ = ( within_reference_ | between_reference_ );

    constr_op_type_token_.add
      ("==", simple_attribute_constraint::eq_op)
      (">", simple_attribute_constraint::gt_op)
      ("<", simple_attribute_constraint::lt_op)
      ("!=", simple_attribute_constraint::neq_op)
      ("&", simple_attribute_constraint::and_op)
      ("|", simple_attribute_constraint::or_op);
    constr_op_type_ = constr_op_type_token_;
    simple_attribute_constraint_ = ( attribute_name_ >> constr_op_type_ >> static_value_ );
    complex_attribute_constraint_ = ( '[' >> attribute_type_ >> ']' >> attribute_name_ >> constr_op_type_ >> expression_ );
    attribute_constraint_ = ( simple_attribute_constraint_ | complex_attribute_constraint_ );

    agg_type_token_.add
      ("AVG", aggregate_atom::avg_agg)
      ("SUM", aggregate_atom::sum_agg)
      ("MIN", aggregate_atom::min_agg)
      ("MAX", aggregate_atom::max_agg)
      ("COUNT", aggregate_atom::count_agg);
    agg_type_ = agg_type_token_;
    aggregate_atom_ = (agg_type_ >> '(' >> attribute_reference_ >> '(' >> -( attribute_constraint_ % ',' )  >> ')'  >> ')' >> predicate_reference_ );

    predicate_parameter_ = (parameter_mapping_ | simple_attribute_constraint_ | complex_attribute_constraint_);
    predicate_ = event_name_ >> '(' >> -(predicate_parameter_ % ',') >> ')';

    selection_policy_token_.add
      ("each", positive_predicate::each_policy)
      ("first", positive_predicate::first_policy)
      ("last", positive_predicate::last_policy);
    selection_policy_ = selection_policy_token_;

    positive_predicate_ = lexeme ["and"] >> selection_policy_ >> predicate_ >> within_reference_;
    negative_predicate_ = lexeme ["and not"] >> predicate_ >> predicate_reference_;

    trigger_pattern_definition_ = predicate_ >> *( positive_predicate_ | negative_predicate_ );

    //-- 3

    simple_attribute_definition_ = ( attribute_name_ >> "::=" >> static_value_ );
    complex_attribute_definition_ = ( attribute_name_ >> ":=" >> expression_ );

    attribute_definition_ = ( simple_attribute_definition_ | complex_attribute_definition_ );

    attributes_definition_ = ( attribute_definition_ % ',' );

    //-- 4

    events_to_consume_ = (event_name_ % ',');

    //-- Rule

    tesla_rule_ %=    lexeme ["define"]   >> event_definition_
                >>    lexeme ["from"]     >> trigger_pattern_definition_
                >> -( lexeme["where"]     >> attributes_definition_ )
                >> -( lexeme["consuming"] >> events_to_consume_  )
                >> ';';
  }

  qi::rule<It, event_name()> event_name_;
  qi::rule<It, attribute_name()> attribute_name_;
  qi::rule<It, parameter_name()> parameter_name_;

  qi::rule<It, std::string()> strlit_;
  qi::rule<It, static_value()> static_value_;

  qi::symbols<char, attribute_declaration::attribute_type> type_token_;
  qi::rule<It, attribute_declaration::attribute_type()> attribute_type_;
  qi::rule<It, attribute_declaration(), ascii::space_type> attribute_declaration_;

  qi::rule<It, event_definition(), ascii::space_type> event_definition_;
  
  qi::rule<It, attribute_reference(), ascii::space_type> attribute_reference_;
  
  qi::rule<It, parameter_mapping(), ascii::space_type> parameter_mapping_;
  
  qi::rule<It, parameter_atom(), ascii::space_type> parameter_atom_;

  qi::symbols<char, factor::sign> factor_sign_token_;
  qi::rule<It, factor::sign(), ascii::space_type> factor_sign_;
  qi::symbols<char, term::op_type> term_mul_div_op_type_token_;
  qi::symbols<char, term::op_type> term_add_sub_op_type_token_;
  qi::rule<It, term::op_type(), ascii::space_type> term_mul_div_op_type_;
  qi::rule<It, term::op_type(), ascii::space_type> term_add_sub_op_type_;
  qi::rule<It, expression(), ascii::space_type> expression_;
  qi::rule<It, expression(), ascii::space_type> term_;
  qi::rule<It, atom(), ascii::space_type> factor_;

  qi::symbols<char, simple_attribute_constraint::op_type> constr_op_type_token_;
  qi::rule<It, simple_attribute_constraint::op_type() > constr_op_type_;
  qi::rule<It, simple_attribute_constraint(), ascii::space_type> simple_attribute_constraint_;
  qi::rule<It, complex_attribute_constraint(), ascii::space_type> complex_attribute_constraint_;
  qi::rule<It, attribute_constraint(), ascii::space_type> attribute_constraint_;
  
  qi::symbols<char, aggregate_atom::aggregation_type> agg_type_token_;
  qi::rule<It, aggregate_atom::aggregation_type()> agg_type_;
  qi::rule<It, aggregate_atom(), ascii::space_type> aggregate_atom_;

  qi::rule<It, within_reference(), ascii::space_type> within_reference_;
  qi::rule<It, between_reference(), ascii::space_type> between_reference_;
  qi::rule<It, predicate_reference(), ascii::space_type> predicate_reference_;

  qi::symbols<char, positive_predicate::selection_policy> selection_policy_token_;
  qi::rule<It, positive_predicate::selection_policy() > selection_policy_;

  qi::rule<It, predicate_parameter(), ascii::space_type> predicate_parameter_;

  qi::rule<It, predicate(), ascii::space_type> predicate_;
  qi::rule<It, positive_predicate(), ascii::space_type> positive_predicate_;
  qi::rule<It, negative_predicate(), ascii::space_type> negative_predicate_;

  qi::rule<It, trigger_pattern_definition(), ascii::space_type> trigger_pattern_definition_;
  
  qi::rule<It, simple_attribute_definition(), ascii::space_type> simple_attribute_definition_;
  qi::rule<It, complex_attribute_definition(), ascii::space_type> complex_attribute_definition_;
  qi::rule<It, attribute_definition(), ascii::space_type> attribute_definition_;

  qi::rule<It, std::vector<attribute_definition>(), ascii::space_type> attributes_definition_;

  qi::rule<It, std::vector<event_name>(), ascii::space_type> events_to_consume_;

  qi::rule<It, tesla_rule(), ascii::space_type> tesla_rule_;
};

// As a requirement you have to have boost installed...
//Compile: g++ file_name.cpp or using scons
//Run: binary < rules_file.txt

typedef std::string::const_iterator iter;

void validate( std::string const& phrase ){
  std::cout << phrase;

  iter curr = phrase.begin();
  iter end = phrase.end();

  ascii::space_type ws;
  tesla_grammar<iter> gram;

  tesla_rule rule;
  if (phrase_parse(curr, end, gram, ws, rule) && curr == end){
    std::cout << " - parsing succeeded: \n\n" << rule << "\n\n";
    //std::cout << " - parsing succeeded\n\n";
  }else{
    //std::string rest(curr, end);
    //std::cout << " - parsing failed - stopped at: \" " << rest << "\"\n";
    std::cout << " - parsing failed\n\n";
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

int main( int argc, char **argv ){
  std::cout << "\n";
  interactive();
  std::cout << "\n";
  return 0;
}
