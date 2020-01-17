#include "filedevicelistmodel.h"

#include <QStringList>

#include "asset/findhandle.h"
#include "asset/glob.h"
#include "asset/local.h"

namespace Iridium
{
    FileDeviceListModel::FileDeviceListModel(String path, Rc<FileDevice> device, QObject* parent)
        : QAbstractItemModel(parent)
        , device_(std::move(device))
    {
        setPath(QString::fromStdString(path));
    }

    FileDeviceListModel::~FileDeviceListModel()
    {}

    int FileDeviceListModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return 0;

        return (int) entries_.size();
    }

    int FileDeviceListModel::columnCount(const QModelIndex& /*parent*/) const
    {
        return 4;
    }

    QModelIndex FileDeviceListModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (parent.isValid())
            return QModelIndex();

        return createIndex(row, column, 1);
    }

    QModelIndex FileDeviceListModel::parent(const QModelIndex& /*child*/) const
    {
        return QModelIndex();
    }

    QVariant FileDeviceListModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        if (role == Qt::DisplayRole)
        {
            if (index.isValid() && index.row() < static_cast<int>(entries_.size()))
            {
                const auto& entry = entries_[index.row()];

                switch (index.column())
                {
                    case 0: return QString::fromStdString(entry.Name);
                    case 1: return entry.IsFolder ? "Folder" : "File";
                    case 2:
                    {
                        QString result;

                        if (!entry.IsFolder)
                        {
                            if (entry.Size < (10000 * 1024)) // 10000 KB
                                result.sprintf("%5zu KB", (entry.Size + 0x3FF) >> 10);
                            else
                                result.sprintf("%5zu MB", (entry.Size + 0xFFFFF) >> 20);
                        }

                        return result;
                    }
                }
            }
        }

        return QVariant();
    }

    Qt::ItemFlags FileDeviceListModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        return QAbstractItemModel::flags(index);
    }

    QVariant FileDeviceListModel::headerData(int section, Qt::Orientation orientation, int role) const
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

    void FileDeviceListModel::setPath(const QString& _path)
    {
        path_ = _path.toStdString();

        if (!path_.empty())
            path_ += "/";

        refreshFiles();
    }

    void FileDeviceListModel::setFilter(const QString& filter)
    {
        filter_ = filter.toStdString();

        refreshFiles();
    }

    void FileDeviceListModel::goToParent()
    {
        if (!path_.empty())
        {
            usize split = path_.rfind('/');

            if (split != String::npos)
                path_.resize(split);

            refreshFiles();
        }
    }

    void FileDeviceListModel::refreshFiles()
    {
        beginResetModel();

        entries_.clear();

        if (auto find = !filter_.empty() ? Glob(device_, path_, filter_) : device_->Find(path_))
        {
            FolderEntry entry;

            while (find->Next(entry))
            {
                entries_.emplace_back(std::move(entry));
            }
        }

        endResetModel();
    }
} // namespace Iridium