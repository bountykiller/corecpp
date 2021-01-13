! C++ Core Components
!!Goal

The goal of this repository is to share some usefull c++ classes which aren't present in the STL.
Those libraries are designed so that they fit well with the STL.
Another thing is that this lib is standalone, meaning that it don't have any dependency.

!!Available classes/libraries :
!!!Serialisation
serialization/deserialization framework

!!!Diagnostics
logging framework.

!!!variant
A variant implementation among other.

!!!ref_ptr
a ptr class which doesn't take any ownership and can only be construct from a smart pointer (usefull for passing parameters for example)

!!!flags
class for manipulating enums as a list of flags

!!!command_line
Classes intended for making parameters managment more easy

!!!deferred
a class for easing lazy function evaluation (not available yet)

!!!event
simple class to ease event-oriented programming

!!Incomplete
!!!uuid
lib for manipulating universally unique identifier (not available yet)

!!!shared_vector
a vector whose list of elements can be efficiently shared

