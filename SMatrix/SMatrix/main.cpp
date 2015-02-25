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
        {7,7,15},
        {0,0,1},{0,3,2},{0,6,3},
        {1,0,4},{1,1,5},
        {2,1,6},{2,2,7},{2,5,8},
        {3,0,9},{3,3,10},{3,4,11},{3,5,12},
        {4,1,13},{4,4,14},
        {6,6,15},
    };
    // construt from istream
    std::ifstream in ("data");
    SMatrix b (in);
    if (a != b) {
        cout << "List constructor and istream construction" << endl;
        cout << a << "and\n" << b
        << "are not equal" << endl;
        return;
    }
    // default constructor
    SMatrix c(10,10);
    SMatrix d = {{10,10,0}};
    if (c != d) {
        cout << "Normal constructor and list constructor " << endl;
        cout << a << "and\n" << b
        << "are not equal" << endl;
        return;
    }
    cout << "constructor test succeed !!" << endl;
}

void memoryTest()
{
    std::ifstream in ("data");
    SMatrix a (in);
    SMatrix b = a;
    if (a != b) {
        cout << "Copy constructor is wrong" << endl;
        cout << a << endl << b;
        return;
    }
    SMatrix c = std::move(a);
    if (a.size() != 0 || b != c) {
        cout << "Move constructor is wrong" << endl;
        return;
    }
    a = b;
    if (a != b) {
        cout << "copy assignment" << endl;
        return;
    }
    b = std::move(c);
    if (b != a || c.size() != 0) {
        cout << "move assignment" << endl;
        return;
    }
    std::vector<SMatrix> v;
    v.push_back(a);
    
    for (auto i = 1; i < 1000; ++i) {
        v.push_back(std::move(v[i-1]));
    }
    cout << "resource control test succeed!!" << endl;
}

void computationTest()
{
    std::ifstream in ("data");
    SMatrix a (in);
    SMatrix id7 = SMatrix::identity(7);
    SMatrix b;
//    cout << b.capacity() << endl;
    b = a + id7;
    SMatrix c = {
        {7,7,16},
        {0,0,2},{0,3,2},{0,6,3},
        {1,0,4},{1,1,6},
        {2,1,6},{2,2,8},{2,5,8},
        {3,0,9},{3,3,11},{3,4,11},{3,5,12},
        {4,1,13},{4,4,15},
        {5,5,1},
        {6,6,16},
    };
    if (b != c) {
        cout << "addition" << endl;
        cout << b << c;
    }
    b -= id7;
    if (a != b) {
        cout << "substraction" << endl;
    }
    if (transpose(id7) != id7) {
        cout << "transpose" << endl;
    }
    a *= id7;
    if (a != b) {
        cout << "multiplication" << endl;
        cout << a << b << endl;
    }
    if (c(0,0) != 2 || c(3,4) != 11) {
        cout << "() operator" << endl;
    }
    SMatrix e(5);
    e.setVal(1, 1, 4);
    e.setVal(4, 2, 9);
    e.setVal(1, 3, 8);
    e.setVal(1, 1, 0);
    e.setVal(4, 2, 0);
    e.setVal(1, 3, 9);
    if (e.size() != 1 || e(1,3) != 9) {
        cout << "setVal" << endl;
    }
    cout << "Operations tests suceed!" << endl;
}

int main(int argc, const char * argv[]) {
    std::ofstream log ("log");
    std::clog.rdbuf(log.rdbuf());
    
    constructorTest();
    memoryTest();
    computationTest();
    return 0;
}
