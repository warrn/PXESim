//
// Created by warnelso on 6/6/16.
//

#ifndef PXESIM_DOWNLOADFILE_H
#define PXESIM_DOWNLOADFILE_H

#include <string>
#include <vector>

#include "Download.h"

class DownloadFile : public Download {
public:
    DownloadFile(const Filename &filename) { _filename = filename; }

    void add_data(const Data &data, uint16_t block_number);

    const uint8_t *data() const { return _data.data(); }

    uint32_t size() const { return (uint32_t) _data.size(); }

private:
    Data _data;
};


#endif //PXESIM_DOWNLOAD_H
