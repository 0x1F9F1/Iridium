#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

namespace Iridium
{
    class FolderEntryTreeItem;
    class FileDevice;

    class FileDeviceTreeModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        explicit FileDeviceTreeModel(String path, Rc<FileDevice> device, QObject* parent = nullptr);
        ~FileDeviceTreeModel();

        QVariant data(const QModelIndex& index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

        bool canFetchMore(const QModelIndex& parent) const override;

        void fetchMore(const QModelIndex& parent) override;

    private:
        Rc<FileDevice> device_;
        FolderEntryTreeItem* root_item_;

        FolderEntryTreeItem* getTreeItem(const QModelIndex& index) const;
    };
} // namespace Iridium
