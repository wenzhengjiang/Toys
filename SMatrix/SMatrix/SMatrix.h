//
//  SMatrix.h
//  SMatrix
//
//  Created by Wenzheng Jiang on 23/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#ifndef __SMatrix__SMatrix__
#define __SMatrix__SMatrix__

#include "MatrixError.h"
#include <map>
#include <vector>

class SMatrix {
public:
    using size_type = size_t;
    
    // Constructors
    
    //    Given SMatrix a(3, 5), a is constructed as a 3 × 5
    //    matrix (3 rows, 5 columns), with its entries being zero
    SMatrix(size_type, size_type);
    SMatrix(size_type size = 1) {
        SMatrix(size, size);
    }
    // same format as the one produced by ouput operator
    SMatrix(std::istream &);
    // list constructor
    // {{1,2,3}, {4,5,6}}
    SMatrix(const std::initializer_list
            <std::initializer_list<int>> &);
    SMatrix(const std::vector<std::vector<int>> &vvi) {
        init_with_vvi(vvi);
    }
    // copy constructor
    SMatrix(const SMatrix &);
    // move constructor
    // leave move-from object in a valid state. its size must be 0*0
    SMatrix(SMatrix &&) noexcept;
    
    // Destructor
    ~SMatrix() { free(); }
    
    
    // Operators on SMatrix
    
    // copy-assignment operator. (copy semantic)
    SMatrix& operator=(const SMatrix &);
    // move-assignment operator. (move semantic)
    SMatrix& operator=(SMatrix &&) noexcept;
    SMatrix& operator+=(const SMatrix &);
    SMatrix& operator-=(const SMatrix &);
    SMatrix& operator*=(const SMatrix &);
    // value of (i,j)th element
    int operator()(const size_type, const size_type) const;
    // number of rows
    size_type rows() const { return nrow; }
    // number of cols
    size_type cols() const {return ncol; }
    // non-zero elements
    size_type size() const {return first_free - val; }
    // capacity of SMatrix
    size_type capacity() const {return cap - val; }
    // set the value of (i,j)th element
    // true if additional memory is allocated
    // false otherwise.
    bool setVal(size_type, size_type, int);
    
    // A fake Itertor
    
    // set internal pointer to the first element.
    void begin() const {
        iter = val;
    }
    // true if internal point goes past the last element
    bool end() const {
        return iter == first_free;
    }
    // move internal pointer to the next element.
    void next() const {
        ++iter;
    }
    // value of element pointed by th internal point.
    int value() const {
        return *iter;
    }
    
    int row() const {
        for (auto r : ridx) {
            if (r.second.first >= (iter-val)) {
                return static_cast<int>(r.first);
            }
        }
        return static_cast<int>(rows());
    }
    int col() const {
        return static_cast<int>(*(cidx + (iter - val)));
    }
    // A new n*n identity matrix
    static SMatrix identity(int);
    // other operators
    friend bool operator==(const SMatrix&, const SMatrix&);
    friend bool operator!=(const SMatrix&, const SMatrix&);
    friend SMatrix operator+(const SMatrix&, const SMatrix&);
    friend SMatrix operator-(const SMatrix&, const SMatrix&);
    friend SMatrix operator*(const SMatrix&, const SMatrix&);
    friend SMatrix transpose(const SMatrix&);
    friend std::ostream& operator<<(std::ostream&, const SMatrix&);
    
private:
    size_type nrow, ncol;
    int *val;                   // pointer to the first element in the smatrix
    size_type *cidx;            // pointer to the column index of elements in smatrix
    // map from row index to first element index in smatrix at this row
    // and number of elements in this row
    std::map<size_type, std::pair<size_t, unsigned int>> ridx;
    mutable int *iter;          // pointer to one element
    int *cap;                   // pointer to the one past the end of the smatrix
    int *first_free;            // pointer to the first free element in the smatrix
    
    
    void free();
    void reallocate();
    void chk_n_alloc();
    void init_with_vvi(const std::vector<std::vector<int>>&);
    // refill val, cidx and ridx and reajust first_free
    // Only ridx is reallocated
    void fill_with_vvi(const std::vector<std::vector<int>>&);
    void insert(size_type, size_type, int);
    void remove(size_type, size_type);
    void update(size_type, size_type, int);
    
    static size_type max_init_size;
    static std::allocator<int> alloc;
    static std::allocator<size_type> alloc_sz;
    
    static int *alloc_n_copy(const int*, size_t);
    static size_type *alloc_n_copy(const size_type*, size_t);
    static bool check_format(std::string);
    static std::string serialize(std::vector<int> v);
    static std::vector<int> deserialize(std::string str);
    static std::vector<std::vector<int>> to_vector(const SMatrix&);
};

#endif /* defined(__SMatrix__SMatrix__) */
