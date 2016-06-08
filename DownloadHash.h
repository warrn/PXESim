//
// Created by warnelso on 6/8/16.
//

#ifndef PXESIM_DOWNLOADHASH_H
#define PXESIM_DOWNLOADHASH_H

#include <openssl/md5.h>
#include <cstdint>
#include "Download.h"

class DownloadHash : public Download {
public:
    DownloadHash(const Filename &filename) {
        _filename = filename;
        MD5_Init(&_md5_ctx);
    }

    const uint8_t *data() const { return _data.data(); }

    void add_data(const Data &data, uint16_t block_number);

    void finalize();

    uint32_t size() const { return MD5_DIGEST_LENGTH * 2; }

private:
    Data _data;
    MD5_CTX _md5_ctx;
};


#endif //PXESIM_DOWNLOADHASH_H
