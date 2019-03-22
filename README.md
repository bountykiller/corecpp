# corecpp
c++ core components

The goal of this repository is to share some usefull c++ classes which aren't present in the STL.
Those libraries are designed so that they fit well with the STL. Also, they don't have any other dependencies.
Here is some available class :
- variant : a variant implementation among other.
- ref_ptr : a ptr class which doesn't take any ownership and can only be construct from a smart pointer (usefull for passing parameters for example)
- flags : class for manipulating enums as a list of flags
- command_line : a class for making parameters managment more easy
- uuid : lib for manipulating universally unique identifier (not available yet)
- deferred : a class for easing lazy function evaluation (not available yet)
- event : simple class to ease event-oriented programming
- shared_vector : a vector whose list of elements can be efficiently shared

This library also feature both a serialization/deserialization framework and a logging framework.
