//
// Created by warnelso on 6/6/16.
//

#include "DownloadHandler.h"

DownloadHandler::DownloadHandler(Filename filename) {
    add_download(filename);
}

DownloadHandler::DownloadHandler(std::list<Filename> files) {
    for (auto &filename : files) {
        add_download(filename);
    }
}

DownloadHandler::~DownloadHandler() {
    for (auto &element: _completed_downloads) { // delete all completed downloads
        delete element.second;
    }
    if (currently_downloading()) {
        delete_current_download(); // delete incomplete download
    }
}

bool DownloadHandler::add_download(const Filename &filename) {
    if (downloaded(filename)  // if downloaded
        || in_download_queue(filename) // or in queue
        || currently_downloading(filename))
        return false; // or currently being downloaded
    else {
        _download_queue.push_back(filename);
        return true;
    }
}

void DownloadHandler::append_data(const Data &data) {
    if (currently_downloading()) {
        _current_download->second->insert(_current_download->second->end(), data.begin(), data.end());
    }
}

void DownloadHandler::finalize_current_download() {
    if (!currently_downloading()) throw not_found_exception();
    else if (!downloaded(_current_download->first)) {
        _completed_downloads[_current_download->first] = _current_download->second;
        _current_download->second = nullptr;
        delete _current_download;
        _current_download = nullptr;
    } else throw overwrite_exception();
}

void DownloadHandler::delete_current_download() {
    if (!currently_downloading()) throw not_found_exception();
    else {
        delete _current_download->second;
        _current_download->second = nullptr;
        delete _current_download;
        _current_download = nullptr;
    }
}

const DownloadHandler::Filename DownloadHandler::start_new_download() {
    if (!currently_downloading()) {
        auto filename = _download_queue.front();
        _download_queue.pop_front();
        _current_download = new std::pair<Filename, Data *>(filename, new Data);
    } else throw not_found_exception();
}

bool DownloadHandler::downloaded(const Filename &filename) const {
    return (bool) _completed_downloads.count(filename);
}

bool DownloadHandler::in_download_queue(const Filename &filename) const {
    return std::find(_download_queue.begin(), _download_queue.end(), filename) != _download_queue.end();
}

bool DownloadHandler::currently_downloading(const Filename &filename) const {
    return _current_download && _current_download->first == filename;
}

bool DownloadHandler::currently_downloading() const {
    return (bool) _current_download;
}

bool DownloadHandler::complete() const {
    return (!_current_download && _download_queue.empty());
}
