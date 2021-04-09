# C++ Core Components
## Goal

The goal of this repository is to share some usefull c++ classes which aren't present in the STL.\
Those libraries are designed so that they fit well with the STL.\
Another thing is that this lib is standalone, meaning that it don't have any dependency.

## Available classes/libraries :

### serialisation
Serialization/deserialization framework\
Allows to serialize/deserialize raw structures/class by just listing their properties.\
It is also possible to provide his own serialisation/deserialisation methods instead.\
\
Right now only JSON is supported, but the API is designed so that other format may be added in the future.

### diagnostics
Logging framework, suppport up to 8 levels from fatal to debug.\
Supports having multiple appenders.\
Each log is written by the appenders associated to their channel.

### variant
A variant implementation among others.  :)\
It is a small variation of the stl-provided ones, and are compatible with the serialisation framework.

### ref_ptr
A ptr class which doesn't take any ownership and can only be construct from a smart pointer (usefull for passing parameters for example)

### flags
Class for manipulating enums types as a list of flags

### command_line
Made from several classes intended for making parameters parsing and managment more easy.\
They can handle
 * having multiple options in both short or long format (the syntax of the value may depend on the format used)
 * having options made of several values
 * having multiple sub-commands.
 * having several parameters passed at the end of the command line.

Later it will also be able to validate the options using builtin or custom validators (not done yet)

### deferred
A class for easing lazy function evaluation.

### event
Simple class to ease event-oriented programming.

### TestU
The library feature a framework for implementing unit-tests. \
These tests are divised into unit tests, which are divised into tests fixtures, which in turn are subdivised into methods
implementing tests or several tests cases. \
 \
In the end, this model allows an easy, yet efficient, implementation of unit tests.


## In-progress
### uuid
Lib for manipulating universally unique identifier

### shared_vector
A vector whose list of elements can be efficiently shared

