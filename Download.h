//
// Created by warnelso on 6/6/16.
//

#ifndef PXESIM_DOWNLOAD_H
#define PXESIM_DOWNLOAD_H


#include <string>
#include <vector>

typedef std::string Filename;
typedef std::vector<uint8_t> Data;

struct block_does_not_fit : public std::exception {
    const char *what() const throw() {
        return "Block does not belong to this download";
    }
};

class Download {
public:
    Download(const Filename &filename, uint32_t total_size = 0, uint16_t block_size = 4092) :
            _total_size(total_size), _block_size(block_size), _filename(filename), _blocks_completed(0) { }

    void add_data(const Data &data, uint16_t block_number);

    void set_total_size(uint32_t total_size) { _total_size = total_size; }

    void set_block_size(uint16_t block_size) { _block_size = block_size; }

    bool complete() const;

private:
    Filename _filename;
    Data _data;

    uint16_t _blocks_completed, _block_size;
    uint32_t _total_size;

};


#endif //PXESIM_DOWNLOAD_H
