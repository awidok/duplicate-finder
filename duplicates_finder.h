#ifndef FINDER_H
#define FINDER_H
#include <QHash>
#include <QFileInfoList>

class duplicates_finder : public QObject {
    Q_OBJECT

signals:
    void progress_changed(int progress);
    void group_added(QFileInfoList const duplicate_group);
    void search_finished();

public slots:
    void kill();

private:
    QHash<qint64, QFileInfoList> size_collisions;
    bool alive;
    void scan_directory(QString dir);
    void add_file_info(QFileInfo const& file_info);
public:
    void find_duplciates(QString const& dir);

};

#endif // FINDER_H
