//
//  main.cpp
//  Generic Database
//
//  Created by Wenzheng Jiang on 28/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <string>
#include "Database.h"

using namespace std;

void testRecord()
{
    string data1 = "{\nAlcatel = 66\nApple = 48\nLilly Eli = 108\nOracle = 77\n}\n";
    istringstream is(data1);
    ostringstream os;
    Record<int> record1;
    is >> record1;
    os << record1;
    if (os.str() != data1) {
        cout << "Record<int> in out" << endl;
        cout << data1 << record1 << endl;
    }
    if(!record1.matchesQuery("Apple", Equal, 48)) {
        cout << "Record<int> Apple equal 48" << endl;
    }
    string data2 = "{\nname = Charles Montgomery Burns\ne-mail = burns@snpp.com\nhome page = http://www.snpp.com\noccupation = director\naddress = 1000 Mammon Street, Springfield Heights\nage = 126 years\n}\n";
    is = istringstream(data2);
    Record<string> record2;
    is >> record2;
    os = ostringstream();
    os << record2;
    if (os.str() != data2) {
        cout << "Record<string> IO" << endl;
        cout << data2 << os.str() << endl;
    }
    cout << "testRecord succeed!" << endl;
}

void testDatabase()
{
    string data1 = "{\nAlcatel = 66\nApple = 48\nLilly Eli = 108\nOracle = 77\n}\n";
    string data2 = "{\nAlcatel = -66\nApple = -48\nLilly Eli = -108\nOracle = -77\n}\n";
    string data3 = "{\nAlcatel = 660\nApple = 480\nLilly Eli = 108\nOracle = 770\n}\n";
    string data = data1+data2+data3;
    Database<int> db;
    istringstream is (data);
    ostringstream os ;
    db.read(is);
    db.write(os, AllRecords);
    if (os.str() != data) {
        cout << "database IO" << endl;
    }
    db.selectAll();
    os = ostringstream();
    db.deseleteAll();
    db.write(os, SelectedRecords);
    if (!os.str().empty()) {
        cout << "deselect All" << endl;
    }
    db.selectAll();
    os = ostringstream();
    db.write(os, SelectedRecords);
    if (os.str() != data) {
        cout << "select All" << endl;
    }
    os = ostringstream();
    db.select(Refine, "Apple", LessThan, 0);
    db.write(os, SelectedRecords);
    if (os.str() != data2) {
        cout << "select" << endl;
        cout << os.str() << data2;
    }
    
    db.write(os, SelectedRecords);
    os = ostringstream();
    db.deleteRecord(SelectedRecords);
    db.write(os, AllRecords);
    if (os.str() != data1 + data3) {
        cout << "delete" << endl;
        cout << os.str();
    }
    cout << "testDatabase suceed!" << endl;
}

int main(int argc, const char * argv[]) {
    testRecord();
    testDatabase();
    return 0;
}
