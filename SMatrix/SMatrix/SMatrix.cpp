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
#include <cassert>

using std::string;
using std::vector;
using std::endl;
using std::clog;

std::allocator<int> SMatrix::alloc;
std::allocator<SMatrix::size_type> SMatrix::alloc_sz;
SMatrix::size_type SMatrix::max_init_size = 1000;

bool same (const vector<int> &a, const vector<int> &b)
{
    return (a[0] == b[0]) && (a[1] == b[1]);
}

static SMatrix::size_type min (SMatrix::size_type x, SMatrix::size_type y)
{
    return x < y ? x : y;
}

SMatrix::SMatrix(size_type nr, size_type nc)
:val(nullptr),cidx(nullptr),ncol(nc),nrow(nr)
{
    size_type sz = min(nrow*ncol/5,max_init_size);
    if (sz) {
        val = alloc.allocate(sz);
        cidx = alloc_sz.allocate(sz);
    }
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

SMatrix::SMatrix(const SMatrix & sm)
:nrow(sm.nrow),ncol(sm.ncol),ridx(sm.ridx)
{
    val = alloc_n_copy(sm.val, sm.size());
    cidx = alloc_n_copy(sm.cidx, sm.size());
    first_free = val + (sm.first_free - sm.val);
    cap = val + (sm.cap - sm.val);
    iter = val + (sm.iter - sm.val);
}

SMatrix::SMatrix(SMatrix && sm) noexcept
:ncol(sm.ncol),nrow(sm.nrow),
val(sm.val),cidx(sm.cidx),ridx(std::move(sm.ridx)),
first_free(sm.first_free),iter(sm.iter),cap(sm.cap)
{
    sm.nrow = sm.ncol = 0;
    sm.val = sm.first_free = sm.iter = sm.cap = nullptr;
    sm.cidx = nullptr;
    sm.ridx.clear();
}

SMatrix& SMatrix::operator=(const SMatrix &rhs)
{
    free();
    nrow = rhs.nrow;
    ncol = rhs.ncol;
    val = alloc_n_copy(rhs.val, rhs.size());
    cidx = alloc_n_copy(rhs.cidx, rhs.size());
    ridx = rhs.ridx;
    first_free = val + (rhs.first_free - rhs.val);
    cap = val + (rhs.cap - rhs.val);
    return  *this;
}

SMatrix& SMatrix::operator=(SMatrix &&rhs) noexcept
{
    if (this != &rhs) {
        free();
        ncol = rhs.ncol;
        nrow = rhs.nrow;
        val = rhs.val;
        cidx = rhs.cidx;
        ridx = std::move(rhs.ridx);
        first_free = rhs.first_free;
        cap = rhs.cap;
        iter = rhs.iter;
        
        rhs.nrow = rhs.ncol = 0;
        rhs.val = rhs.first_free = rhs.iter = rhs.cap = nullptr;
        rhs.cidx = nullptr;
        rhs.ridx.clear();
    }
    return *this;
}

SMatrix& SMatrix::operator+=(const SMatrix & rhs)
{
    if (cols() != rhs.cols() || rows() != rhs.rows()) {
        throw MatrixError("operands are not the same size");
    }
    vector<vector<int>> ret = {{static_cast<int>(rows()),
        static_cast<int>(cols()),0}};
    auto lvvi = to_vector(*this), rvvi = to_vector(rhs);
    lvvi.insert(lvvi.end(), rvvi.begin()+1, rvvi.end());
    std::sort(lvvi.begin()+1, lvvi.end());
    for (auto i = 1; i < lvvi.size()-1; ++i) {
        if (same(lvvi[i], lvvi[i+1])) {
            lvvi[i][2] += lvvi[i+1][2];
            if (lvvi[i][2] != 0) {
                ret.push_back(lvvi[i]);
            }
            ++i;
        } else {
            ret.push_back(lvvi[i]);
        }
    }
    ret[0][2] = static_cast<int>(ret.size() - 1);
    fill_with_vvi(ret);
    return *this;
}

SMatrix& SMatrix::operator-=(const SMatrix &rhs)
{
    SMatrix minus_rhs(rhs);
    std::transform(minus_rhs.val, minus_rhs.val+rhs.size(), minus_rhs.val, [](int i){return -i;});
    return operator+=(minus_rhs);
}

SMatrix& SMatrix::operator*=(const SMatrix &rhs)
{
    vector<vector<int>> ret = {{static_cast<int>(rows()),
        static_cast<int>(rhs.cols()),0}};
    auto aux_rhs = transpose(rhs);
    for (auto r : ridx) {
        for (auto c : aux_rhs.ridx) {
            size_type lidx = r.second.first;
            size_type ridx = c.second.first;
            int sum = 0;
            while (lidx < r.second.first+r.second.second &&
                   ridx < c.second.first+c.second.second) {
                if (cidx[lidx] < aux_rhs.cidx[ridx]) {
                    ++lidx;
                } else if(cidx[lidx] > aux_rhs.cidx[ridx]) {
                    ++ridx;
                } else {
                    sum += val[lidx] * aux_rhs.val[ridx];
                    ++lidx;
                    ++ridx;
                }
            }
            if (sum) {
                ret.push_back({static_cast<int>(r.first),static_cast<int>(c.first),sum});
            }
        }
    }
    ret[0][2] = static_cast<int>(ret.size()-1);
    fill_with_vvi(ret);
    return *this;
}

int SMatrix::operator()(const size_type i, const size_type j) const {
    if (i > rows() || j > cols()) {
        throw MatrixError("Out of boundary");
    }
    auto it = ridx.begin();
    if ((it = ridx.find(i)) != ridx.end()) {
        for (size_type i = 0; i < it->second.second; ++i) {
            if (cidx[it->second.first+i] == j) {
                return val[it->second.first+i];
            }
        }
    }
    return 0;
}

bool SMatrix::setVal(size_type i, size_type j, int v)
{
    if (i > rows() || j > cols()) {
        throw MatrixError("Out of boundary");
    }
    auto vvi = to_vector(*this);
    if (!operator()(i,j) && v) {
        chk_n_alloc();
        vvi.push_back({static_cast<int>(i),static_cast<int>(j),v});
        std::sort(vvi.begin()+1, vvi.end());
    } else if (operator()(i,j)) {
        for (auto it = vvi.begin(); it != vvi.end(); ++it) {
            if ((*it)[0] == i && (*it)[1] == j) {
                if (v == 0) {
                    vvi.erase(it);
                } else {
                    (*it)[2] = v;
                }
                break;
            }
        }
    }
    fill_with_vvi(vvi);
    return true;
}

SMatrix SMatrix::identity(int sz)
{
    vector<vector<int>> vvi = {{sz, sz, sz}};
    for (auto i = 0; i < sz; ++i) {
        vvi.push_back({i,i,1});
    }
    return SMatrix(vvi);
}

// compare val, cidx and ridx
bool operator==(const SMatrix &lhs, const SMatrix &rhs)
{
    if (lhs.rows() != rhs.rows()
        || lhs.cols() != rhs.cols()
        || lhs.size() != rhs.size() ) {
        return false;
    }
    if (lhs.size() &&
        ( memcmp(lhs.val, rhs.val, lhs.size())
        || memcmp(lhs.cidx, rhs.cidx, lhs.size())
        || lhs.ridx != rhs.ridx)) {
            return false;
    }
    return true;
}

bool operator!=(const SMatrix &lhs, const SMatrix &rhs)
{
    return !(lhs == rhs);
}
SMatrix operator+(const SMatrix &lhs, const SMatrix &rhs)
{
    SMatrix ret (lhs);
    ret += rhs;
    return ret;
}

SMatrix operator-(const SMatrix &lhs, const SMatrix &rhs)
{
    SMatrix ret (lhs);
    ret -= rhs;
    return ret;   
}

SMatrix operator*(const SMatrix &lhs, const SMatrix &rhs)
{
    SMatrix ret (lhs);
    ret *= rhs;
    return ret;
}

SMatrix transpose(const SMatrix& sm)
{
    vector<vector<int>> ret = {{static_cast<int>(sm.cols()),
        static_cast<int>(sm.rows()),static_cast<int>(sm.size())}};
    for (sm.begin(); !sm.end(); sm.next()) {
        ret.push_back({sm.col(), sm.row(), sm.value()});
    }
    
    std::sort(ret.begin()+1, ret.end());
    return SMatrix(ret);
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
        assert(capacity());
        alloc.deallocate(val, capacity());
    }
    if (cidx) {
        alloc_sz.deallocate(cidx, capacity());
    }
    val = first_free = cap = iter = nullptr;
    cidx = nullptr;
    ridx.clear();
}

void SMatrix::reallocate()
{
    auto newcapacity = size() ? 2 * size() : 1;
    auto newval = alloc.allocate(newcapacity);
    auto newcidx = alloc_sz.allocate(newcapacity);
    auto it_val = val;
    auto it_cidx = cidx;
    auto dest_val = newval;
    auto dest_cidx = newcidx;
    auto offset = iter - val;
    for (size_t i = 0; i != size(); ++i) {
        alloc.construct(dest_val++, std::move(*it_val++));
        alloc.construct(dest_cidx++, std::move(*it_cidx++));
    }
    free();
    val = newval;
    cidx = newcidx;
    first_free = dest_val;
    cap = val + newcapacity;
    iter = val + offset;
}

void SMatrix::chk_n_alloc()
{
    if (size() == capacity()) {
        reallocate();
    }
}

void SMatrix::init_with_vvi(const vector<vector<int>>& vvi)
{
    nrow = vvi[0][0];
    ncol = vvi[0][1];
    int elem = vvi[0][2];
    if (elem) {
        val = alloc.allocate(elem);
        cidx = alloc_sz.allocate(elem);
        fill_with_vvi(vvi);
    } else {
        val = nullptr;
        cidx = nullptr;
        first_free = val;
    }
    cap = val + elem;
    iter = val;
}

void SMatrix::fill_with_vvi(const vector<vector<int>>& vvi)
{
    ridx.clear();
    for (auto i = 0; i < vvi.size()-1; ++i) {
        const vector<int> &vi = vvi[i+1];
        val[i] = vi[2];
        cidx[i] = vi[1];
        if (ridx.find(vi[0]) == ridx.end()) {
            ridx[vi[0]] = {i, 1};
        } else
            ridx[vi[0]].second++;
    }
    first_free = val + (vvi.size()-1);
}

/***************** static methods *******************/

int* SMatrix::alloc_n_copy(const int *b, size_t sz)
{
    auto data = alloc.allocate(sz);
    std::uninitialized_copy(b, b+sz, data);
    return data;
}

SMatrix::size_type* SMatrix::alloc_n_copy(const SMatrix::size_type *b, size_t sz)
{
    auto data = alloc_sz.allocate(sz);
    std::uninitialized_copy(b, b+sz, data);
    return data;
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

vector<vector<int>> SMatrix::to_vector(const SMatrix &sm)
{
    vector<vector<int>> vvi = {{static_cast<int>(sm.rows()), static_cast<int>(sm.cols()), static_cast<int>(sm.size())}};
    
    for (auto e : sm.ridx) {
        auto r = e.first;
        for (auto i = 0; i < e.second.second; ++i) {
            auto idx = e.second.first + i;
            vvi.push_back({static_cast<int>(r), static_cast<int>(sm.cidx[idx]),sm.val[idx]});
        }
    }
    assert(vvi.size() == sm.size()+1);
    return vvi;
}