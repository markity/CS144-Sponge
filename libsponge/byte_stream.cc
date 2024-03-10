#include "byte_stream.hh"
#include <iostream>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t cap):
 capacity(cap), data(), read_cnt(0), write_cnt(0), _read_cb(){
}

size_t ByteStream::write(const string &data_) {
    if (end_intput_flag) {
        throw exception();
    }

    if (data_.size() == 0) {
        throw exception();
    }

    size_t reamin = this->remaining_capacity();
    for (size_t i = 0; i < reamin && i < data_.size(); i++) {
        this->data.push_back(data_[i]);
    }
    write_cnt += reamin - remaining_capacity();
    return {reamin - remaining_capacity()};
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string s(data.begin(), data.begin() + len);
    return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    for (size_t i = 0; i < len; i ++) {
        data.pop_front();
    }
    read_cnt += len;
    if (_read_cb) {
        _read_cb(len);
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string s(data.begin(), data.begin() + len);
    for (size_t i = 0; i < len; i++) {
        data.pop_front();
    }
    read_cnt += len;
    if (_read_cb) {
        _read_cb(len);
    }
    return s;
}

void ByteStream::end_input() {
    this->end_intput_flag = true;
}

bool ByteStream::input_ended() const { return this->end_intput_flag; }

size_t ByteStream::buffer_size() const {
    return data.size();
}

bool ByteStream::buffer_empty() const {
    return data.empty();
}

bool ByteStream::eof() const {
    return end_intput_flag && data.empty();
}

size_t ByteStream::bytes_written() const {
    return write_cnt;
}

size_t ByteStream::bytes_read() const {
    return read_cnt;
}

size_t ByteStream::remaining_capacity() const {
    return capacity - data.size();
}

void ByteStream::set_read_callback(function<void(const size_t&)> func) {
    this->_read_cb = func;
}
