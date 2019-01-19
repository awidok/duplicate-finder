#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileInfoList>
#include <QMainWindow>
#include <memory>
#include <duplicates_finder.h>
#include <QFuture>

namespace Ui {
class MainWindow;
}

class main_window : public QMainWindow {
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();

signals:
    void killed();

private slots:
    void scan_directory();
    void delete_duplicates();
    void set_progress_bar(int progress);
    void add_duplicate_group(QFileInfoList const& duplicate_group);
    void finish_search();
private:
    std::unique_ptr<Ui::MainWindow> ui;
    QString current_dir;
    duplicates_finder fnd;
    QFuture<void> future;
    QString get_readable_file_size(qint64 size);
};

#endif // MAINWINDOW_H
