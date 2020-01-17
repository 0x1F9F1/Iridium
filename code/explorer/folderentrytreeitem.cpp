#include "folderentrytreeitem.h"

#include "asset/filedevice.h"

namespace Iridium
{
    FolderEntryTreeItem::FolderEntryTreeItem(String path, FolderEntry entry, FolderEntryTreeItem* parent)
        : Path(std::move(path))
        , Entry(std::move(entry))
        , m_parentItem(parent)
    {
        if (!Path.empty())
            Path += "/";
    }

    FolderEntryTreeItem::~FolderEntryTreeItem()
    {
        qDeleteAll(m_childItems);
    }

    void FolderEntryTreeItem::loadFiles(FileDevice& device)
    {
        if (Loaded)
            return;

        m_childItems.clear();

        if (auto find = device.Find(Path))
        {
            FolderEntry entry;

            while (find->Next(entry))
            {
                appendChild(new FolderEntryTreeItem(Path + entry.Name, entry, this));
            }
        }

        Loaded = true;
    }

    void FolderEntryTreeItem::appendChild(FolderEntryTreeItem* item)
    {
#pragma warning(suppress : 4127)
        m_childItems.append(item);
    }

    FolderEntryTreeItem* FolderEntryTreeItem::child(int row)
    {
        if (row < 0 || row >= m_childItems.size())
            return nullptr;
        return m_childItems.at(row);
    }

    int FolderEntryTreeItem::childCount() const
    {
        return m_childItems.count();
    }

    QVariant FolderEntryTreeItem::data(int column) const
    {
        switch (column)
        {
            case 0: return QString::fromStdString(Entry.Name);
            case 1: return Entry.IsFolder ? "Folder" : "File";
            case 2:
            {
                QString result;

                if (!Entry.IsFolder)
                {
                    if (Entry.Size < (10000 * 1024)) // 10000 KB
                        result.sprintf("%5zu KB", (Entry.Size + 0x3FF) >> 10);
                    else
                        result.sprintf("%5zu MB", (Entry.Size + 0xFFFFF) >> 20);
                }

                return result;
            }
        }

        return QVariant();
    }

    FolderEntryTreeItem* FolderEntryTreeItem::parentItem()
    {
        return m_parentItem;
    }

    int FolderEntryTreeItem::row() const
    {
        if (m_parentItem)
            return m_parentItem->m_childItems.indexOf(const_cast<FolderEntryTreeItem*>(this));

        return 0;
    }
} // namespace Iridium