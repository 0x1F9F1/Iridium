#include "filedevicetreemodel.h"
#include "folderentrytreeitem.h"

#include <QStringList>

#include "asset/findhandle.h"
#include "asset/local.h"

namespace Iridium
{
    FileDeviceTreeModel::FileDeviceTreeModel(String path, Rc<FileDevice> device, QObject* parent)
        : QAbstractItemModel(parent)
        , device_(std::move(device))
    {
        FolderEntry root;

        root.Reset();
        root.IsFolder = true;

        root_item_ = new FolderEntryTreeItem(std::move(path), root);
    }

    FileDeviceTreeModel::~FileDeviceTreeModel()
    {
        delete root_item_;
    }

    int FileDeviceTreeModel::columnCount(const QModelIndex& /*parent*/) const
    {
        return 4;
    }

    bool FileDeviceTreeModel::hasChildren(const QModelIndex& parent) const
    {
        FolderEntryTreeItem* parentItem = getTreeItem(parent);

        return parentItem->Entry.IsFolder || parentItem->childCount() > 0;
    }

    bool FileDeviceTreeModel::canFetchMore(const QModelIndex& parent) const
    {
        FolderEntryTreeItem* parentItem = getTreeItem(parent);

        return !parentItem->Loaded && parentItem->Entry.IsFolder;
    }

    void FileDeviceTreeModel::fetchMore(const QModelIndex& parent)
    {
        getTreeItem(parent)->loadFiles(*device_);
    }

    FolderEntryTreeItem* FileDeviceTreeModel::getTreeItem(const QModelIndex& index) const
    {
        if (index.isValid())
            return static_cast<FolderEntryTreeItem*>(index.internalPointer());

        return root_item_;
    }

    QVariant FileDeviceTreeModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        if (role != Qt::DisplayRole)
            return QVariant();

        return getTreeItem(index)->data(index.column());
    }

    Qt::ItemFlags FileDeviceTreeModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        return QAbstractItemModel::flags(index);
    }

    QVariant FileDeviceTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section)
            {
                case 0: return "Name";
                case 1: return "Type";
                case 2: return "Size";
                case 3: return "Attributes";
            }
        }

        return QVariant();
    }

    QModelIndex FileDeviceTreeModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        FolderEntryTreeItem* parentItem = getTreeItem(parent);
        FolderEntryTreeItem* childItem = parentItem->child(row);

        if (childItem)
        {
            return createIndex(row, column, childItem);
        }

        return QModelIndex();
    }

    QModelIndex FileDeviceTreeModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
            return QModelIndex();

        FolderEntryTreeItem* childItem = getTreeItem(index);
        FolderEntryTreeItem* parentItem = childItem->parentItem();

        if (parentItem == root_item_)
            return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int FileDeviceTreeModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.column() > 0)
            return 0;

        return getTreeItem(parent)->childCount();
    }
} // namespace Iridium