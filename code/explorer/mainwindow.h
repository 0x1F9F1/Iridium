#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui
{
    class MainWindow;
}

namespace Iridium
{
    class FileDeviceTreeModel;
    class FileDeviceListModel;
} // namespace Iridium

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void goToParent();

private:
    Ui::MainWindow* ui;
    Iridium::FileDeviceListModel* fs_model_;
};

#endif // MAINWINDOW_H
