#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :
    _output(capacity), _capacity(capacity), _cache(),
    _streamStartIndex(0), _firstUnReadIndex(0), _firstUnReassembledIndex(0),
    _firstUnAcceptableIndex(_firstUnReadIndex + capacity), _unReassembledSize(0)
 {
    auto cb = [this, capacity](const size_t &sz) -> void {
        this->_firstUnReadIndex += sz;
        this->_firstUnAcceptableIndex = this->_firstUnReadIndex + capacity;
    };
    _output.set_read_callback(cb);
 }

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, bool eof) {
    // eof已经发送了, 丢掉就行
    if (_output.eof()) {
        return;
    }

    // 无效信息, 直接返回
    if (data.empty() && !eof) {
        return;
    }

    // 如果有eof==true, 如何处理eof? 我们规定单独弄一个eof节点, 它占一个index, data为空, eof为true

    // 要查询字节是否在窗口内, 如果部分在窗口内需要截取

    // 左闭右开区间
    string d = data;
    size_t startIndex = index;
    size_t endIndex = index + d.size();

    // streamStartIndex = 0
    // firstUnReadIndex
    // firstUnReassembledIndex
    // firstUnAcceptableIndex

    // Already Read: [0, firstUnReadIndex)
    // In ByteStream: [firstUnReadIndex,firstUnReassembledIndex)
    // Window Range: [firstUnReadIndex,firstUnAcceptableIndex)

    // segment整体都不在区间内, 就直接丢弃
    if (startIndex >= _firstUnAcceptableIndex && endIndex <= _firstUnReassembledIndex) {
        return;
    }

    // startIndex = 300, endIndex=600
    // windowRange = [400, 500)

    // 3 5
    // 3 7

    // 部分超过窗口, 截取
    if (startIndex < _firstUnReassembledIndex) {
        d.erase(0, _firstUnReassembledIndex - startIndex);
        startIndex = _firstUnReassembledIndex;
    }
    if (endIndex > _firstUnAcceptableIndex) {
        d.erase(d.size() - (endIndex - _firstUnAcceptableIndex), endIndex - _firstUnAcceptableIndex);
        endIndex = _firstUnAcceptableIndex;
        eof = false;
    }

    // 尝试merge, 有两个阶段, 删除重复的, 然后插入新的

    // 阶段1, 删除重复的
    if (d.size()) {
        for (auto iter = _cache.begin(); iter != _cache.end();) {
            size_t iterStartIndex = iter->index;
            size_t iterEndIndex = iter->index + iter->data.size();
            
            // 头部重叠
            if (startIndex < iterStartIndex && endIndex >= iterStartIndex &&
                endIndex < iterEndIndex) {
                // 删除iter字符串overlap的部分
                size_t delete_size = endIndex - iterStartIndex;
                iter->data.erase(0, delete_size);
                iter->index += delete_size;
                _unReassembledSize -= delete_size;
                break;
            }

            // 全部重叠
            if (startIndex >= iterStartIndex && endIndex <= iterEndIndex) {
                d.clear();
                endIndex = startIndex;
                break;
            }

            // 尾部重叠
            if (startIndex >= iterStartIndex && startIndex <= iterEndIndex &&
                endIndex >= iterEndIndex) {
                size_t delete_size = iterEndIndex - startIndex;
                d.erase(0,delete_size);
                startIndex = iterEndIndex;
                iter++;
                continue;
            }

            // 覆盖
            if (startIndex <= iterStartIndex && endIndex >= iterEndIndex) {
                // delete this node
                _unReassembledSize -= iter->data.size();
                iter = _cache.erase(iter);
                continue;
            }

            // 小于
            if (endIndex <= iterStartIndex) {
                break;
            }
            iter++;
        }
    }

    // 阶段2, 追加这个
    if (d.size()) {
        for (auto iter = _cache.begin();; iter++) {
            if (startIndex <= iter->index || iter == _cache.end()) {
                Node insertNode = {};
                insertNode.data = d;
                insertNode.index = startIndex;
                insertNode.eof = false;
                _cache.insert(iter, insertNode);
                _unReassembledSize += d.size();
                break;
            }
        }
    }

    // 如果有eof, 那么追加
    if (eof) {
        Node insertNode = {};
        insertNode.eof = true;
        insertNode.index = endIndex;
        _cache.push_back(insertNode);
    }

    // 然后尝试把重组器里面的字节弄到_output里面去
    for (auto beg = _cache.begin(); beg != _cache.end();) {
        if (beg->index != _firstUnReassembledIndex) {
            return;
        }

        if (beg->eof) {
            _output.end_input();
            return;
        }

        _output.write(beg->data);
        _firstUnReassembledIndex += beg->data.size();
        _unReassembledSize -= beg->data.size();
        _cache.erase(beg);
        beg = _cache.begin();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return _unReassembledSize;
}

bool StreamReassembler::empty() const {
    if (_cache.size() == 0) {
        return true;
    }

    return _cache.size() == 1 && _cache.back().eof;
}
