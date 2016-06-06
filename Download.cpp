//
// Created by warnelso on 6/6/16.
//

#include "Download.h"


void Download::add_data(const Data &data, uint16_t block_number) {
    if (!complete() && _blocks_completed + 1 == block_number) {
        _data.insert(_data.end(), data.begin(), data.end());
        _blocks_completed++;
    } else throw block_does_not_fit();
}

bool Download::complete() const {
    return (uint32_t) _blocks_completed * (uint32_t) _block_size >= _total_size;
}
