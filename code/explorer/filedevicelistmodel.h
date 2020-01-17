#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

namespace Iridium
{
    class FileDevice;
    struct FolderEntry;

    class FileDeviceListModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        explicit FileDeviceListModel(String path, Rc<FileDevice> device, QObject* parent = nullptr);
        ~FileDeviceListModel();

        QVariant data(const QModelIndex& index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

        QModelIndex parent(const QModelIndex& child) const override;

    public slots:
        void setPath(const QString& path);
        void setFilter(const QString& filter);
        void goToParent();

        void refreshFiles();

    private:
        String path_;
        String filter_;
        Rc<FileDevice> device_;

        Vec<FolderEntry> entries_;
    };
} // namespace Iridium
