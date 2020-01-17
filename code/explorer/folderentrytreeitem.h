#pragma once

#include <QVariant>
#include <QVector>

#include "asset/findhandle.h"

namespace Iridium
{
    struct FolderEntry;

    class FileDevice;

    class FolderEntryTreeItem
    {
    public:
        explicit FolderEntryTreeItem(String path, FolderEntry entry, FolderEntryTreeItem* parentItem = nullptr);
        ~FolderEntryTreeItem();

        void loadFiles(FileDevice& device);

        void appendChild(FolderEntryTreeItem* child);

        FolderEntryTreeItem* child(int row);
        int childCount() const;
        QVariant data(int column) const;
        int row() const;
        FolderEntryTreeItem* parentItem();

        String Path;
        FolderEntry Entry;
        bool Loaded {false};

    private:
        QVector<FolderEntryTreeItem*> m_childItems;

        FolderEntryTreeItem* m_parentItem;
    };
} // namespace Iridium
