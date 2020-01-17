#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "filedevicelistmodel.h"
#include "filedevicetreemodel.h"

#include "asset/local.h"

using namespace Iridium;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fs_model_(new FileDeviceListModel("", LocalFiles()))
{
    ui->setupUi(this);

    ui->fileList->setModel(fs_model_);
    ui->fileList->header()->resizeSection(0, 700);

    ui->splitter->setStretchFactor(0, 3);
    ui->splitter->setStretchFactor(1, 1);

    connect(ui->filePath, &QLineEdit::textChanged, fs_model_, &FileDeviceListModel::setPath);
    connect(ui->fileFilter, &QLineEdit::textChanged, fs_model_, &FileDeviceListModel::setFilter);
    connect(ui->parentPath, &QToolButton::pressed, this, &MainWindow::goToParent);

    ui->filePath->setText("X:/Games/Rockstar/Red Dead Redemption 2");
}

MainWindow::~MainWindow()
{
    delete fs_model_;
    delete ui;
}

void MainWindow::goToParent()
{
    QString path = ui->filePath->text();

    if (int idx = path.lastIndexOf('/'); idx != -1)
    {
        path.resize(idx);

        ui->filePath->setText(path);
    }
    else
    {
        ui->filePath->setText("");
    }
}
