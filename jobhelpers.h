/****************************************************************************
 *    Copyright (c) 2007 Vlad Codrea                                        *
 *    Copyright (c) 2003-2004 by Alexander Neundorf & Kevin 'ervin' Ottens  *
 *                                                                          *
 *   This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 2 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program; if not, write to the Free Software            *
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston,                 *
 *   MA 02111-1307, USA.                                                    *
 ****************************************************************************/

#ifndef JOB_HELPERS_H
#define JOB_HELPERS_H

#include "basejobhelper.h"
#include "kiofuseapp.h"

#include <QByteArray>

#include <kio/job.h>
#include <kio/filejob.h>
#include <kio/udsentry.h>

class ListJobHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        ListJobHelper(const KUrl& url, QEventLoop* eventLoop);
        ~ListJobHelper();
        KIO::UDSEntryList entries();  // Sends file and directory info to the FUSE op that started the job
/*    
    signals:
        void reqListJob(const KUrl&, ListJobHelper*);
*/
    public slots:
        void receiveEntries(KIO::Job*, const KIO::UDSEntryList &entries);  // Store entries so that the FUSE op can get them
    
    protected:
        KIO::UDSEntryList m_entries;  // file and directory info gathered by m_job and given to the FUSE ops that started the job
};

class StatJobHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        StatJobHelper(const KUrl& url, QEventLoop* eventLoop);
        ~StatJobHelper();
        KIO::UDSEntry entry();  // Sends file and directory info to the FUSE op that started the job
/*    
    signals:
        void reqStatJob(const KUrl&, StatJobHelper*);
*/
    public slots:
        void receiveEntry(const KIO::UDSEntry &entry);  // Store entry so that the FUSE op can get it
        
    protected:
        KIO::UDSEntry m_entry;
};

class OpenJobHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        OpenJobHelper(const KUrl& url, const QIODevice::OpenMode& qtMode,
                      QEventLoop* eventLoop);
        ~OpenJobHelper();
        uint64_t fileHandleId() {return m_fileHandleId;}

    public slots:
        //void setFileHandleId(const uint64_t& aFileHandleId) {m_fileHandleId = aFileHandleId;}
        void setFileHandleId(const uint64_t& aFileHandleId);
/*
    signals:
        void reqFileJob(const KUrl&, const QIODevice::OpenMode&, OpenJobHelper*);
*/
    protected:
        uint64_t m_fileHandleId;
};

class LockHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        LockHelper(const uint64_t& fileHandleId, QEventLoop* eventLoop);
        ~LockHelper();
        QMutex* jobMutex() {return m_jobMutex;}

    public slots:
        void setJobMutex(QMutex* mutex, const int& error);
/*
    signals:
        void reqFindMutex(const uint64_t&, LockHelper*);
        void sendJobDone(const int&);
*/
    protected:
        uint64_t m_fileHandleId;
        QMutex* m_jobMutex;
};

class ReadJobHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        ReadJobHelper(const uint64_t& fileHandleId, const KUrl& url, const size_t& size,
                      const off_t& offset, QEventLoop* eventLoop);
        ~ReadJobHelper();
        QByteArray data() {return m_data;}  // Sends data to the FUSE op
        uint64_t fileHandleId() {return m_fileHandleId;}
/*
    signals:
        void reqSeek(const uint64_t&, const off_t&, ReadJobHelper*);
        void reqRead(KIO::FileJob*, const size_t&, ReadJobHelper*);
        void sendJobDone(const int&);
*/
    public slots:
        void receivePosition(const off_t& pos, const int& error, KIO::FileJob* fileJob);
        void receiveData(const QByteArray& data, const int& error);

    protected:
        uint64_t m_fileHandleId;
        KIO::FileJob* m_fileJob;  // FIXME Needs to be deleted by close() or the cache cleaner
        QByteArray m_data;
        size_t m_size;
        off_t m_offset;
};

class WriteJobHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        WriteJobHelper(const uint64_t& fileHandleId, const KUrl& url, const QByteArray& data,
                       const off_t& offset, QEventLoop* eventLoop);
        ~WriteJobHelper();
        size_t written() {return m_written;}  // Sends number of
                                              // bytes written to the FUSE op
        uint64_t fileHandleId() {return m_fileHandleId;}
/*
    signals:
        void reqSeek(const uint64_t&, const off_t&, WriteJobHelper*);
        void reqWrite(KIO::FileJob*, const QByteArray&, WriteJobHelper*);
        void sendJobDone(const int&);
*/
    public slots:
        void receivePosition(const off_t& pos, const int& error, KIO::FileJob* fileJob);
        void receiveWritten(const size_t& written, const int& error);

    protected:
        uint64_t m_fileHandleId;
        KIO::FileJob* m_fileJob;  // FIXME Needs to be deleted by close() or the cache cleaner
        QByteArray m_data;
        size_t m_written;
        off_t m_offset;
};

class MkDirHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        MkDirHelper(const KUrl& url, const mode_t& mode, QEventLoop* eventLoop);
        ~MkDirHelper();
/*    
    signals:
        void reqMkDir(const KUrl&, const mode_t&, MkDirHelper*);
*/
};

class UnLinkHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        UnLinkHelper(const KUrl& url, QEventLoop* eventLoop);
        ~UnLinkHelper();
/*    
    signals:
        void reqUnLink(const KUrl&, UnLinkHelper*);
*/
};

class MkNodHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        MkNodHelper(const KUrl& url, const mode_t& mode, QEventLoop* eventLoop);
        ~MkNodHelper();
/*    
    signals:
        void reqMkNod(const KUrl&, const mode_t&, MkNodHelper*);
*/
};

class SymLinkHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        SymLinkHelper(const KUrl& source, const KUrl& dest, QEventLoop* eventLoop);
        ~SymLinkHelper();
/*    
    signals:
        void reqSymLink(const KUrl&, const KUrl&, SymLinkHelper*);
*/
};

class ReNameHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        ReNameHelper(const KUrl& source, const KUrl& dest, QEventLoop* eventLoop);
        ~ReNameHelper();
/*    
    signals:
        void reqReName(const KUrl&, const KUrl&, ReNameHelper*);
*/
};

class ChModHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        ChModHelper(const KUrl& url, const mode_t& mode, QEventLoop* eventLoop);
        ~ChModHelper();
/*    
    signals:
        void reqChMod(const KUrl&, const mode_t&, ChModHelper*);
*/
};

class ReleaseJobHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        ReleaseJobHelper(const KUrl& url, const uint64_t& fileHandleId,
                         const bool& jobIsAnnulled, QEventLoop* eventLoop);
        ~ReleaseJobHelper();
/*    
    signals:
        void reqReleaseJob(const KUrl&, const uint64_t&, ReleaseJobHelper*);
*/
};

class ChTimeHelper : public BaseJobHelper
{
    Q_OBJECT

    public:
        ChTimeHelper(const KUrl& url, const QDateTime& dt, QEventLoop* eventLoop);
        ~ChTimeHelper();
/*    
    signals:
        void reqChTime(const KUrl&, const QDateTime&, ChTimeHelper*);
*/
};

#endif /* JOB_HELPERS_H */
