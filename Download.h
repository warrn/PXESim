//
// Created by warnelso on 6/8/16.
//

#ifndef PXESIM_DOWNLOAD_H
#define PXESIM_DOWNLOAD_H

#include <exception>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include "constants.h"

typedef std::string Filename;
typedef std::vector<uint8_t> Data;

struct block_does_not_fit : public std::exception {
    const char *what() const throw() {
        return "Block does not belong to this download";
    }
};

class Download {
public:

    Download() : _blocks_completed(0), _total_size(0) { }

    ~Download() { }

    void total_size(uint32_t total_size) { _total_size = total_size; }

    void filename(const Filename &filename) { _filename = filename; }

    const Filename &filename() const { return _filename; }

    virtual void add_data(const Data &data, uint16_t block_number) { };

    virtual const uint8_t *data() const { return nullptr; }

    virtual uint32_t size() const { return 0; }

    virtual void finalize() { }

    bool complete() const { return (uint32_t) _blocks_completed * BLOCK_SIZE >= _total_size; }

protected:
    uint32_t _total_size;
    uint16_t _blocks_completed;
    Filename _filename;

};


#endif //PXESIM_DOWNLOAD_H
