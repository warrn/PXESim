//
// Created by warnelso on 6/8/16.
//

#include "DownloadHash.h"

void DownloadHash::add_data(const Data &data, uint16_t block_number) {
    if (!complete() && _blocks_completed + 1 == block_number) {
        _blocks_completed++;
        if (!complete()) MD5_Update(&md5_ctx, data.data(), BLOCK_SIZE);
        else MD5_Update(&md5_ctx, data.data(), _total_size % BLOCK_SIZE);
    } else throw block_does_not_fit();
}