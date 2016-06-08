//
// Created by warnelso on 6/8/16.
//

#include "DownloadHash.h"

void DownloadHash::add_data(const Data &data, uint16_t block_number) {
    if (!complete() && _blocks_completed + 1 == block_number) {
        _blocks_completed++;
        if (!complete()) MD5_Update(&_md5_ctx, data.data(), BLOCK_SIZE);
        else MD5_Update(&_md5_ctx, data.data(), _total_size % BLOCK_SIZE);
    } else throw block_does_not_fit();
}

void DownloadHash::finalize() {
    uint8_t hash[MD5_DIGEST_LENGTH];
    MD5_Final((unsigned char *) hash, &_md5_ctx);
    for (uint8_t iter = 0; iter < MD5_DIGEST_LENGTH; iter++) {
        _data.push_back("0123456789abcdef"[hash[iter] / 16]);
        _data.push_back("0123456789abcdef"[hash[iter] % 16]);
    }
}