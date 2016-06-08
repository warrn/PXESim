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
        MD5_Init(&md5_ctx);
    }

    const uint8_t *data() const {
        return hash;
    }

    void add_data(const Data &data, uint16_t block_number);

    void finalize() { MD5_Final((unsigned char *) hash, &md5_ctx); }

    uint32_t size() const { return MD5_DIGEST_LENGTH; }

private:
    uint8_t hash[MD5_DIGEST_LENGTH];
    MD5_CTX md5_ctx;
};


#endif //PXESIM_DOWNLOADHASH_H
