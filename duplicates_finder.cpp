#include "duplicates_finder.h"
#include <QtDebug>
#include <QCryptographicHash>
#include <QDirIterator>

void duplicates_finder::add_file_info(QFileInfo const& file_info) {
    size_collisions[file_info.size()].push_back(file_info);
}


void duplicates_finder::find_duplciates(QString const& dir) {
    size_collisions.clear();
    alive = true;
    scan_directory(dir);
    if (!alive) {
        return;
    }
    qint64 full_size = 0;
    qint64 current_size = 0;
    QList<QFileInfoList> possible_duplciates;
    for (auto file_group : size_collisions) {
        if (!alive) {
            return;
        }
        if (file_group.size() > 1) {
            full_size += file_group.first().size() * file_group.size();
             possible_duplciates.append(file_group);
        }
    }
    if (full_size == 0) {
        emit search_finished();
        emit progress_changed(100);
        return;
    }
    std::sort(possible_duplciates.begin(), possible_duplciates.end(), [](QFileInfoList const& a, QFileInfoList const& b) {return a.first().size() > b.first().size();});
    for (auto const& file_group : possible_duplciates) {
        if (!alive) {
            return;
        }
        QHash<QByteArray, QFileInfoList> hash_collisions;
        for (auto const& file_info : file_group) {
            if (!alive) {
                return;
            }
            QFile file(file_info.absoluteFilePath());
            if (file.open(QFile::ReadOnly)) {
                QCryptographicHash hash(QCryptographicHash::Sha3_512);
                if (hash.addData(&file)) {
                    hash_collisions[hash.result()].push_back(file_info);
                }
            }
            if (!alive) {
                return;
            }
            current_size += file.size();
            emit progress_changed(static_cast<int>(100 * current_size / full_size));
        }
        for (auto const& duplicate_group : hash_collisions) {
            if (!alive) {
                return;
            }
            if (duplicate_group.size() > 1) {
                emit group_added(duplicate_group);
            }
        }
    }
    emit search_finished();
}

void duplicates_finder::scan_directory(QString dir) {
    QDirIterator it(dir, QStringList() << "*", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (!alive) {
            break;
        }
        add_file_info(it.next());
    }
}

void duplicates_finder::kill() {
    alive = false;
}
