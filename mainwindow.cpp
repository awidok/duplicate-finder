#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "duplicates_finder.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDirIterator>
#include <QtConcurrent>

main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry(this)));
    qRegisterMetaType<QFileInfoList>("QFileInfoList");
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    QCommonStyle style;
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionDelete_Duplicates->setIcon(style.standardIcon(QCommonStyle::SP_TrashIcon));
    ui->actionCancel->setIcon(style.standardIcon(QCommonStyle::SP_DialogCancelButton));

    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::scan_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionDelete_Duplicates, &QAction::triggered, this, &main_window::delete_duplicates);

    connect(&fnd, &duplicates_finder::progress_changed, this, &main_window::set_progress_bar);
    connect(&fnd, &duplicates_finder::group_added, this, &main_window::add_duplicate_group);
    connect(&fnd, &duplicates_finder::search_finished, this, &main_window::finish_search);
    connect(ui->actionCancel, &QAction::triggered, &fnd, &duplicates_finder::kill);
    connect(this, &main_window::killed, &fnd, &duplicates_finder::kill);
    connect(ui->actionCancel, &QAction::triggered, this, &main_window::finish_search);
}

main_window::~main_window() {
   emit killed();
   future.waitForFinished();
}
namespace  {
    QString get_readable_file_size(qint64 size) {
        if (size == 0) {
            return "0";
        }
        const static QString names[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"};
        int cnt = 0;
        double d_size = size;
        while (d_size >= 1024) {
            d_size /= 1024;
            cnt++;
        }
        return QString::number(d_size, 'f', 2) + " " + names[cnt];
    }
}

void main_window::scan_directory() {
    current_dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning");
    if (current_dir.size() == 0) {
        return;
    }
    ui->treeWidget->clear();
    ui->actionScan_Directory->setEnabled(false);
    ui->actionCancel->setEnabled(true);
    ui->progressBar->setValue(0);
    setWindowTitle(QString("Scanning Directory - %1").arg(current_dir));
    future = QtConcurrent::run(&fnd, &duplicates_finder::find_duplciates, current_dir);
}

void main_window::delete_duplicates() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Delete confirmation", "Delete selected files?\n", QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++) {
            int left = ui->treeWidget->topLevelItem(i)->childCount();
            for (int j = 0; j < ui->treeWidget->topLevelItem(i)->childCount(); j++) {
                if (ui->treeWidget->topLevelItem(i)->child(j)->checkState(2)) {
                    QFile file(current_dir + "/" + ui->treeWidget->topLevelItem(i)->child(j)->text(0));
                    if (file.remove()) {
                        left--;
                        ui->treeWidget->topLevelItem(i)->removeChild(ui->treeWidget->topLevelItem(i)->child(j));
                        j--;
                    } else {
                        QMessageBox Msgbox;
                        Msgbox.setText("File " + file.fileName() + " can not be deleted");
                        Msgbox.exec();
                    }
                }
            }
            if (left < 2) {
                delete ui->treeWidget->topLevelItem(i);
                i--;
            }
        }
    } else {
       return;
    }
}

void main_window::set_progress_bar(int progress) {
    ui->progressBar->setValue(progress);
}

void main_window::add_duplicate_group(QFileInfoList const& duplicate_group) {
    QDir d(current_dir);
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
    item->setExpanded(true);
    if (duplicate_group.first().size() == 0) {
        item->setText(0, QString::number(duplicate_group.size()) + " empty files");
    } else {
        item->setText(0, QString::number(duplicate_group.size()) + " duplicates");
    }
    item->setText(1, get_readable_file_size(duplicate_group.first().size()));
    for (auto const& file_info : duplicate_group) {
        QTreeWidgetItem* child_item = new QTreeWidgetItem(item);
        child_item->setText(0, d.relativeFilePath(file_info.filePath()));
        if (file_info.isSymLink()) {
            child_item->setIcon(0, QCommonStyle().standardIcon(QCommonStyle::SP_FileLinkIcon));
        }
        child_item->setCheckState(2, Qt::CheckState::Unchecked);
        item->addChild(child_item);
    }
    ui->treeWidget->addTopLevelItem(item);
}

void main_window::finish_search() {
    if (ui->treeWidget->topLevelItemCount() == 0) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, "Nothing was found");
        ui->treeWidget->addTopLevelItem(item);
    }
    setWindowTitle(QString("Duplicates in %1").arg(current_dir));
    ui->actionScan_Directory->setEnabled(true);
    ui->actionCancel->setEnabled(false);
}

