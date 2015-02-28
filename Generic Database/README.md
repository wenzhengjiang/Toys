## Generic Database

C++ template practice

## Example

```
string data1 = "{\nAlcatel = 66\nApple = 48\nLilly Eli = 108\nOracle = 77\n}\n";
string data2 = "{\nAlcatel = -66\nApple = -48\nLilly Eli = -108\nOracle = -77\n}\n";
string data3 = "{\nAlcatel = 660\nApple = 480\nLilly Eli = 108\nOracle = 770\n}\n";
string data = data1+data2+data3;

// Create an integer database
Database<int> db;
istringstream is (data);
ostringstream os ;

// read from istream
db.read(is);

// select operation
db.selectAll();
db.deseleteAll();
db.select(Refine, "Apple", LessThan, 0);

// write to ostream
db.write(os, SelectedRecords);

```