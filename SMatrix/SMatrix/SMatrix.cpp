//
//  SMatrix.cpp
//  SMatrix
//
//  Created by Wenzheng Jiang on 23/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#include "SMatrix.h"
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

using std::string;
using std::vector;
using std::endl;
using std::clog;

std::allocator<int> SMatrix::alloc;
std::allocator<SMatrix::size_type> SMatrix::alloc_sz;
SMatrix::size_type SMatrix::max_init_size = 1000;

static SMatrix::size_type min (SMatrix::size_type x, SMatrix::size_type y)
{
    return x < y ? x : y;
}

SMatrix::SMatrix(size_type nr, size_type nc)
:cidx(nullptr),ncol(nc),nrow(nr)
{
    size_type sz = min(nrow*ncol/5,max_init_size);
    val = alloc.allocate(sz);
    cidx = alloc_sz.allocate(sz);
    first_free = val;
    cap = val + sz;
    iter = val;
}

SMatrix::SMatrix(std::istream & in)
{
    std::string str;
    vector<vector<int>> vvi;
    while(in >> str) {
        if(check_format(str)) {
            vvi.push_back(deserialize(str));
        }
        else
            throw MatrixError("Format Error : " + str);
    }
    if (vvi.empty()) {
        throw MatrixError("Input empty matrix");
    }
    init_with_vvi(vvi);
}

SMatrix::SMatrix(const std::initializer_list
            <std::initializer_list<int>> &initl)
{
    vector<vector<int>> vvi;
    for (auto l : initl) {
        vvi.push_back(l);
    }
    init_with_vvi(vvi);
}

// compare val, cidx and ridx
bool operator==(const SMatrix &lhs, const SMatrix &rhs)
{
    if (lhs.size() != rhs.size()
        || memcmp(lhs.val, rhs.val, lhs.size())
        || memcmp(lhs.cidx, rhs.cidx, lhs.size())
        || lhs.ridx != rhs.ridx ) {
        
        return false;
    }

    return true;
}

bool operator!=(const SMatrix &lhs, const SMatrix &rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream &out, const SMatrix &sm)
{
    using size_type = SMatrix::size_type;
    for (auto i = 0; i < sm.size(); ++i) {
        clog << sm.val[i] << " ";
    }
    clog << endl;
    for (auto i = 0; i < sm.size(); ++i) {
        clog << sm.cidx[i] << " ";
    }
    clog << endl;
    for (auto r : sm.ridx) {
        clog << r.first << ' ' << r.second.first << ' ' << r.second.second << endl;
    }
    out << SMatrix::serialize({static_cast<int>(sm.rows()), static_cast<int>(sm.cols()), static_cast<int>(sm.size())}) << endl;
    
    for (auto e : sm.ridx) {
        auto r = e.first;
        for (auto i = 0; i < e.second.second; ++i) {
            auto idx = e.second.first + i;
            out << SMatrix::serialize({static_cast<int>(r), static_cast<int>(sm.cidx[idx]),sm.val[idx]}) << ' ';
        }
        out << endl;
    }
    return out;
}

void SMatrix::free()
{
    if (val) {
        delete val;
    }
    if (cidx) {
        delete cidx;
    }
    val = first_free = cap = iter = nullptr;
    cidx = nullptr;
    ridx.clear();
}

void SMatrix::init_with_vvi(const vector<vector<int>>& vvi)
{
    nrow = vvi[0][0];
    ncol = vvi[0][1];
    int elem = vvi[0][2];
    val = alloc.allocate(elem*2);
    cidx = alloc_sz.allocate(elem*2);
    for (auto i = 0; i < elem; ++i) {
        const vector<int> &vi = vvi[i+1];
        val[i] = vi[2];
        cidx[i] = vi[1];
        if (ridx.find(vi[0]) == ridx.end()) {
            ridx[vi[0]] = {i, 0};
        } else
            ridx[vi[1]].second++;
    }
    first_free = val + elem;
    cap = val + 2 * elem;
    iter = val;
}

// check str with (s1,s2,s3) format
bool SMatrix::check_format(std::string str)
{
    if (str.empty())
        return false;
    if(str[0] != '(' || str[str.size()-1] != ')')
        return false;
    if (std::count(str.begin(), str.end(),',') != 2)
        return false;
    return true;
}

string SMatrix::serialize(vector<int> v)
{
    return "(" + std::to_string(v[0]) + ","
    + std::to_string(v[1]) + "," + std::to_string(v[2])
    + ")";
}

std::vector<int> SMatrix::deserialize(string str)
{
    vector<int> ret;
    auto iter1 = std::find(str.begin(), str.end(), ',');
    auto iter2 = std::find(str.rbegin(), str.rend(), ',').base()-1;
    ret.push_back(std::stoi(string(str.begin()+1,iter1)));
    ret.push_back(std::stoi(string(iter1+1,iter2)));
    ret.push_back(std::stoi(string(iter2+1,str.end()-1)));
    
    return ret;
}



