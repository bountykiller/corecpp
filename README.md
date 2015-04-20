# experimental_pp
c++ experiments

The goal of this repository is to share some usefull c++ classes which aren't present in the STL.
Those libraries are designed so that they fit well with the STL. Also, they don't have any other dependencies than the STL.
Note that the classes are organised into separated libraries so that you can take one or another without taking them as a whole.
Here is the list of libraries : 
- ref_ptr : a ptr class which doesn't take any ownership and can only be construct from a smart pointer (usefull for assing parameters for example)
- flags : class for manipulating enums as a list of flags
- uuid : lib for manipulating universally unique identifier (not available yet)
- lazy : a class for easing lazy function evaluation (not available yet)
- more to come...
