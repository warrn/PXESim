//
// Created by warnelso on 6/6/16.
//

#ifndef PXESIM_DOWNLOADHANDLER_H
#define PXESIM_DOWNLOADHANDLER_H

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <algorithm>
#include <list>
#include <exception>


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
    typedef std::string Filename;

    DownloadHandler() { };

    DownloadHandler(Filename filename);

    DownloadHandler(std::list<Filename> files);

    ~DownloadHandler();

    bool add_download(const Filename &filename);

    void append_data(const Data &data);

    void finalize_current_download();

    const Filename start_new_download();

    void delete_current_download();

    bool downloaded(const Filename &filename) const;

    bool in_download_queue(const Filename &filename) const;

    bool currently_downloading(const Filename &filename) const;

    bool currently_downloading() const;

    bool complete() const;

private:
    std::pair<Filename, Data *> *_current_download;
    std::deque<Filename> _download_queue;
    std::map<Filename, Data *> _completed_downloads;
};


#endif //PXESIM_DOWNLOADHANDLER_H
