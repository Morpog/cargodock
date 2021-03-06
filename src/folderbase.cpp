#include "folderbase.h"
#include "copyaction.h"

#include <QDebug>

FolderBase::FolderBase(QObject* parent)
    : QAbstractListModel(parent)
    , myMinDepth(INT_MAX)
{
    myRolenames.insert(NameRole, "name");
    myRolenames.insert(PathRole, "path");
    myRolenames.insert(UriRole, "uri");
    myRolenames.insert(TypeRole, "type");
    myRolenames.insert(MimeTypeRole, "mimeType");
    myRolenames.insert(IconRole, "icon");
    myRolenames.insert(SizeRole, "size");
    myRolenames.insert(MtimeRole, "mtime");
    myRolenames.insert(OwnerRole, "owner");
    myRolenames.insert(GroupRole, "group");
    myRolenames.insert(PermissionsRole, "permissions");
    myRolenames.insert(LinkTargetRole, "linkTarget");
    myRolenames.insert(ModelTargetRole, "modelTarget");
    myRolenames.insert(SelectedRole, "selected");
}

void FolderBase::setPath(const QString& path)
{
    unselectAll();
    loadDirectory(path);
    mySelection.clear();

    int depth = 0;
    QString p = path;
    QString pp = parentPath(p);
    while (pp != p)
    {
        qDebug() << pp << p;
        p = pp;
        pp = parentPath(p);
        ++depth;
    }
    if (depth < myMinDepth)
    {
        myMinDepth = depth;
        qDebug() << "new mindepth" << depth;
    }

    myPath = path;
    emit pathChanged();
}

QStringList FolderBase::breadcrumbs() const
{
    QStringList crumbs;

    QString path = myPath;
    QString parent = parentPath(path);

    crumbs << userBasename(path);

    while (parent != path)
    {
        path = parent;
        crumbs.prepend(userBasename(path));
        parent = parentPath(path);
    }

    return crumbs.mid(myMinDepth);
}

QStringList FolderBase::selection() const
{
    QStringList items;
    foreach (int idx, mySelection)
    {
        items << joinPath(QStringList() << myPath << itemName(idx));
    }
    return items;
}

void FolderBase::open(const QString& name)
{
    QString path = joinPath(QStringList() << myPath << name);
    ItemType itemType = type(path);
    if (itemType == Folder || itemType == FolderLink)
    {
        setPath(path);
    }
    else
    {
        runFile(path);
    }
}

void FolderBase::copySelected(FolderBase* dest)
{
    QStringList paths;
    foreach (int idx, mySelection)
    {
        paths << joinPath(QStringList() << myPath << itemName(idx));
    }

    CopyAction* action = new CopyAction(this, dest, paths, dest->path());
    connect(action, SIGNAL(finished()),
            this, SIGNAL(finished()));
    connect(action, SIGNAL(error(QString)),
            this, SIGNAL(error(QString)));
    action->start();
}

void FolderBase::deleteSelected()
{
    foreach (int idx, mySelection)
    {
        QString path = joinPath(QStringList() << myPath << itemName(idx));
        if (! deleteFile(path))
        {
            emit error(QString("Could not delete file: %1").arg(itemName(idx)));
        }
    }
    unselectAll();
    emit finished();
}

void FolderBase::deleteItems(const QStringList& items)
{
    foreach (const QString& path, items)
    {
        if (! deleteFile(path))
        {
            emit error(QString("Could not delete file: %1").arg(basename(path)));
        }
    }
    unselectAll();
    emit finished();
}

void FolderBase::linkSelected(FolderBase* dest)
{
    foreach (int idx, mySelection)
    {
        const QString endpoint = joinPath(QStringList() << myPath << itemName(idx));
        const QString destPath = dest->joinPath(QStringList() << dest->path() << itemName(idx));

        if (! linkFile(destPath, endpoint))
        {
            emit error(QString("Could not create link: %1 -> %2")
                       .arg(destPath)
                       .arg(endpoint));
        }
    }
    emit finished();
}

void FolderBase::newFolder(const QString& name)
{
    if (makeDirectory(joinPath(QStringList() << myPath << name)))
    {
        emit finished();
    }
    else
    {
        emit error("could not create folder");
    }
}

void FolderBase::setPermissions(const QString&, int)
{
    emit error("changing permissions not supported");
}

void FolderBase::rename(const QString&, const QString&)
{
    emit error("renaming not supported");
}

void FolderBase::refresh()
{
    loadDirectory(myPath);
}

void FolderBase::cdUp(int amount)
{
    QString path = myPath;
    for (int i = 0; i < amount; ++i)
    {
        path = parentPath(path);
    }
    setPath(path);
}

void FolderBase::setSelected(int idx, bool value)
{
    if (idx < rowCount(QModelIndex()))
    {
        if (value)
        {
            mySelection << idx;
        }
        else
        {
            mySelection.remove(idx);
        }
        emit selectionChanged();

        QVector<int> roles;
        roles << SelectedRole;
        emit dataChanged(index(idx), index(idx), roles);
    }
}

void FolderBase::selectAll()
{
    int size = rowCount(QModelIndex());
    for (int i = 0; i < size; ++i)
    {
        if (! mySelection.contains(i))
        {
            setSelected(i, true);
        }
    }
}

void FolderBase::unselectAll()
{
    int size = rowCount(QModelIndex());
    for (int i = 0; i < size; ++i)
    {
        if (mySelection.contains(i))
        {
            setSelected(i, false);
        }
    }
}

void FolderBase::invertSelection()
{
    int size = rowCount(QModelIndex());
    for (int i = 0; i < size; ++i)
    {
        setSelected(i, ! mySelection.contains(i));
    }
}

bool FolderBase::isSelected(int idx) const
{
    return mySelection.contains(idx);
}

QStringList FolderBase::list(const QString&) const
{
    return QStringList();
}

FolderBase::ItemType FolderBase::type(const QString&) const
{
    return Unsupported;
}

QIODevice* FolderBase::openFile(const QString&, QIODevice::OpenModeFlag)
{
    return 0;
}

bool FolderBase::makeDirectory(const QString&)
{
    return false;
}

bool FolderBase::linkFile(const QString&, const QString&)
{
    return false;
}

bool FolderBase::deleteFile(const QString&)
{
    return false;
}

void FolderBase::runFile(const QString& path)
{

}
