/****************************************************************************
 *   Copyright (c) 2007-2008 Vlad Codrea                                    *
 *   Copyright (c) 2015 Thomas Fischer                                      *
 *                                                                          *
 *   This program is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation, either version 3 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License.     *
 *                                                                          *
 ****************************************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <QString>
#include <QList>
#include <QMutex>

#include <QUrl>
#include <kio/filejob.h>
#include <kfileitem.h>

class FileJobData
{
    public:
        FileJobData(KIO::FileJob* aFileJob);
        ~FileJobData();
        KIO::FileJob* fileJob;
        QTime qTime;
        //bool inUse;
        QMutex jobMutex;
        bool jobIsAnnulled;
};

/*class Cache : public QObject
{
    Q_OBJECT

    public:
        enum NodeType {regularType, innerStubType, leafStubType};
        Cache(KFileItem* item);
        Cache(const QString& rootOfRelPath, NodeType nodeType);
        ~Cache();
        KFileItem* item() const {return m_item;}
        void insert(KFileItem* newItem);
        bool setExtraData(const KUrl& url, const uint64_t& key,
                          KIO::FileJob* fileJob);
        bool releaseJob(const uint64_t& fileHandleId);
        Cache* find(const KUrl &url);
        QMap<uint64_t, FileJobData*> jobsMap() const {return fhIdtoFileJob;}
        void removeExpired();
        bool cachedChildren;
        QTime* lastUpdated;

    private:
        int findIdxOfChildFromFileName(const QString& fileName);
        int setItem(KFileItem* newItem);
        QString stripBegSlashes(const QString& path);

        KFileItem* m_item;
        QList<Cache*> children;
        NodeType m_nodeType;
        QMap<uint64_t, FileJobData*> fhIdtoFileJob;
};*/

#endif /* CACHE_H */
