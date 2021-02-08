# C++ Core Components
## Goal

The goal of this repository is to share some usefull c++ classes which aren't present in the STL.\
Those libraries are designed so that they fit well with the STL.\
Another thing is that this lib is standalone, meaning that it don't have any dependency.

## Available classes/libraries :

### Serialisation
Serialization/deserialization framework\
Allows to serialize/deserialize raw structures/class by just listing their properties.\
Right now only JSON is supported, but the API is designed so that other format may be added in the future.

### Diagnostics
Logging framework, suppport up to 8 levels from fatal to debug.\
Supports having multiple appenders.\
Each log is written by the appenders associated to their channel.

### variant
A variant implementation among other.\
A variation of the stl-provided ones :)

### ref_ptr
A ptr class which doesn't take any ownership and can only be construct from a smart pointer (usefull for passing parameters for example)

### flags
Class for manipulating enums types as a list of flags

### command_line
Classes intended for making parameters managment more easy.\
Can handle having multiple sub-commands.

### deferred
A class for easing lazy function evaluation

### event
Simple class to ease event-oriented programming

## In-progress
### uuid
Lib for manipulating universally unique identifier

### shared_vector
A vector whose list of elements can be efficiently shared

