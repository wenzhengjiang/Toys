//
//  MatrixError.h
//  SMatrix
//
//  Created by Wenzheng Jiang on 23/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#ifndef SMatrix_MatrixError_h
#define SMatrix_MatrixError_h

#include <string>
#include <exception>

class MatrixError : public std::exception {
public:
    MatrixError(const std::string& what_arg) : _what(what_arg) { }
    virtual const char* what() const throw() { return _what.c_str (); }
    virtual ~MatrixError() throw() { }
private:
    std::string _what;
};

#endif
