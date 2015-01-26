# tesla_parser
This is a C++ (boost::spirit) parser for TESLA "grammar"...

TESLA is a formally defined event specification language which provides high expressiveness and increased flexibility in defining complex event processing rules.  <br />
These rules offer content and temporal filters, negations, aggregates and fully customizable policies for event selection and consumption.

Example:

define  Fire( area: string ) <br />
from  TemperatureReading( area => $a, temperature > 50 ) <br />
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;and not RainReading( [string] area == $a ) within 1 hour from TemperatureReading <br />
where area = TemperatureReading.area

The original project lacks a C++ parser for the rules, so here I aim to build one.

You can find more about it form here: https://github.com/deib-polimi/TRex
