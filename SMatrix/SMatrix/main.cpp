//
//  main.cpp
//  SMatrix
//
//  Created by Wenzheng Jiang on 23/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "SMatrix.h"

using std::cout; using std::endl;

void constructorTest()
{
    // list constructor
    SMatrix a = {
        {8,7,15},
        {0,0,1},{0,3,2},{0,6,3},
        {1,0,4},{1,1,5},
        {2,1,6},{2,2,7},{2,5,8},
        {3,0,9},{3,3,10},{3,4,11},{3,5,12},
        {4,1,13},{4,4,14},
        {7,6,15},
    };
    // construt from istream
    std::ifstream in ("data");
    SMatrix b (in);
    if (a != b) {
        cout << "List constructor and istream construction" << endl;
        cout << a << "and\n" << b
        << "are not equal" << endl;
    }
    cout << a << b;
    // default constructor
    SMatrix c(10,10);
    SMatrix d = {{10,10,0}};
    if (c != d) {
        cout << "List constructor and normal constructor " << endl;
        cout << a << "and\n" << b
        << "are not equal" << endl;
    }
    cout << "constructor test succeed !!" << endl;
}

void memoryTest()
{

}
void computationTest()
{
    
}

int main(int argc, const char * argv[]) {
    std::ofstream log ("log");
    std::clog.rdbuf(log.rdbuf());
    
    constructorTest();
    memoryTest();
    computationTest();
    return 0;
}
