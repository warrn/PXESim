//
// Created by warnelso on 6/6/16.
//

#ifndef PXESIM_DOWNLOADHANDLER_H
#define PXESIM_DOWNLOADHANDLER_H

#include <deque>
#include <map>
#include <algorithm>
#include <list>
#include <exception>

#include "Download.h"

struct not_found_exception : public std::exception {
    const char *what() const throw() {
        return "Element could not be found";
    }
};

struct overwrite_exception : public std::exception {
    const char *what() const throw() {
        return "Download is already completed and saved from a previous transfer";
    }
};


class DownloadHandler {
public:

    typedef std::vector<uint8_t> Data;

    DownloadHandler() { };

    DownloadHandler(Filename filename);

    DownloadHandler(std::list<Filename> files);

    ~DownloadHandler();

    bool add_download(const Filename &filename);

    void append_data(const Data &data, uint16_t block_number);

    void set_current_download_sizes(uint16_t block_size, uint32_t total_size);

    void finalize_current_download();

    const Filename start_new_download();

    void delete_current_download();

    void clear_queue();

    bool downloaded(const Filename &filename) const;

    bool in_download_queue(const Filename &filename) const;

    bool currently_downloading(const Filename &filename) const;

    bool currently_downloading() const;

    bool complete() const;

    bool current_download_complete() const;

private:
    std::pair<Filename, Download *> *_current_download;
    std::deque<Filename> _download_queue;
    std::map<Filename, Download *> _completed_downloads;
};


#endif //PXESIM_DOWNLOADHANDLER_H
