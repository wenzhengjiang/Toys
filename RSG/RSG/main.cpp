//
//  main.cpp
//  RSG
//  Created by Wenzheng Jiang on 20/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#include <iostream>
#include <string>
#include <cstdlib>
#include "grammer.h"


using std::cout ; using std::endl;
using std::string;

string random_expand(string nont, Grammer grm) {
    auto prods = grm.prod_list(nont);
    auto prod = prods[rand()%prods.size()];
    string ret;
    bool first = true;
    for (auto w : prod) {
        if (first) {
            first = false;
        } else {
            ret += " ";
        }
        if (grm.is_nont(w)) {
            ret += random_expand(w, grm);
        } else {
            ret += w;
        }
    }
    return ret;
}

string random_sentence(Grammer grm) {

    return random_expand(Grammer::start, grm);
}

int main(int argc, const char * argv[]) {
    srand(static_cast<unsigned>(time(NULL)));
    std::ifstream in ("poem.g");
    if (in.good()) {
        cout << "Open grammer file" << endl;
    } else {
        cout << "Failed to open grammer file" << endl;
        return 1;
    }
    Grammer grm (in);
    cout << random_sentence(grm) << endl;

    return 0;
}
