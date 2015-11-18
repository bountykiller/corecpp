#ifndef CORE_CPP_VISIBILITY_H
#define CORE_CPP_VISIBILITY_H


//GCC specific part. TODO: look for other compiler
#define _internal __attribute__ ((visibility ("hidden")))
#define _external __attribute__ ((visibility ("default")))

#endif