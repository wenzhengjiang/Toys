//
//  grammer.h
//  RSG
//
//  Created by Wenzheng Jiang on 20/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#ifndef RSG_grammer_h
#define RSG_grammer_h

#include <fstream>
#include <string>
#include <vector>
#include <map>


class Grammer {
public:
    typedef std::vector<std::string>  production;
    
    static const std::string start;
    
    Grammer () = default;
    Grammer (std::ifstream &);
    std::vector<production> prod_list (std::string nont);
    bool is_nont(std::string s) {
        return s.size() >= 2 && s[0] == '<' && s[s.size()-1] == '>';
    }
    
private:
    std::multimap<std::string, production> gram;
    bool read_nont(std::ifstream &);
    bool read_prod(std::ifstream &,production &);
    
};

std::ostream &operator<< (std::ostream &os, const Grammer::production &item);

#endif
