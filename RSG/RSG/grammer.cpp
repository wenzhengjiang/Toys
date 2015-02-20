//
//  grammer.cpp
//  RSG
//
//  Created by Wenzheng Jiang on 20/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#include "grammer.h"

#include <iostream>

using std::string; using std::vector;
using std::clog; using std::endl;

const string Grammer::start = "<start>";

Grammer::Grammer (std::ifstream & in) {
    if (!in.good()) {
        throw std::runtime_error("Bad ifstream");
    }
    while (read_nont(in)) ;
    in.close();
}

/*
 Read a non-terminal grammer in gram.
 Return false when meet EOF
 */
bool Grammer::read_nont(std::ifstream &in) {
    string text;
    while (in >> text && text != "{") ;
    if (in.eof()) {
        return false;
    }
    string nont;
    production prod;
    // read non-terminal and all productions
    in >> nont;
    while (read_prod(in, prod)) {
        gram.insert({nont, prod});
    }
    return true;
}
/*
 
 */
bool Grammer::read_prod(std::ifstream &in, production &prod) {
    prod.clear();
    string word;
    while (in >> word && word != ";") {
        if (word == "}") {
            return false;
        }
        prod.push_back(word);
    }
    return true;
}

vector<Grammer::production> Grammer::prod_list (string nont) {
    auto range = gram.equal_range(nont);
    vector<Grammer::production> prods;
    while (range.first != range.second) {
        prods.push_back(range.first->second);
        ++range.first;
    }
    return prods;
}

std::ostream &operator<< (std::ostream &os, const Grammer::production &item)
{
    for (auto i : item)
        os << i << " ";
    return os;
}