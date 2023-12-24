#ifndef LIGHTKV_EXCEPTION_H
#define LIGHTKV_EXCEPTION_H

#include <exception>

class MiniKVLogicErr : public std::exception {
    const char* what() const throw() {
        return "logic error";
    }
};

#endif