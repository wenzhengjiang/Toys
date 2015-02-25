## The Sparse Matrix Library

A Sparse matrix Class with resource control support and normal operations

## Examples

```
// n*n all-zero matrix
SMatrix a(100,100);

// list initializer
SMatrix a = {
        {7,7,15},
        {0,0,1},{0,3,2},{0,6,3},
        {1,0,4},{1,1,5},
        {2,1,6},{2,2,7},{2,5,8},
        {3,0,9},{3,3,10},{3,4,11},{3,5,12},
        {4,1,13},{4,4,14},
        {6,6,15},
        };

// construct from istream
ifstream in ("data");
SMatrix b (in);

// copy constructor
SMatrix c = a;

// move constructor
SMatrix d = std::move(b);

// operations on SMatrix
a = b + c;
a = b - c;
a = b * c;
b == c;
cout << a(i,j) << endl;
cout << a << endl;

// identity Matrix
auto e = SMatrix::identity(5);
e.setValue(3,3,10); // update
e.setValue(3,4,2); // insert
e.setValue(1,1,0); // remove

auto d = transpose(e); // tranpose matrix

```
