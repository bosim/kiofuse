/****************************************************************************
 *   Copyright (c) 2003-2004 by Alexander Neundorf & Kevin 'ervin' Ottens   *
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

#ifndef KIO_FUSE_APP_H
#define KIO_FUSE_APP_H

#include <errno.h>
extern "C" {
#include <stdint.h>
}

#include "cache.h"
#include "jobhelpers.h"

#include <QApplication>
#include <QTemporaryFile>
#include <QMutex>

#define VERIFY(expression) if(expression) {} else { kioFuseApp->quitGracefully(#expression, __FILE__, __LINE__, __FUNCTION__); }

//#desdfdsfine VERIFY(expression) if(!expression) { kioFuseApp->quitGracefully(#expression, __FILE__, __LINE__, __FUNCTION__); }

class ListJobHelper;
class StatJobHelper;
class OpenJobHelper;
class ReadJobHelper;
class WriteJobHelper;
class MkNodHelper;
class SymLinkHelper;
class ReNameHelper;
class ChModHelper;
class ReleaseJobHelper;
class MkDirHelper;
class UnLinkHelper;
class ChTimeHelper;
class LockHelper;

class KioFuseApp : public QCoreApplication {
    Q_OBJECT

    public:
        KioFuseApp(int &argc, char* argv[]);
        ~KioFuseApp();
	void setBaseUrl(QUrl& url) { m_baseUrl = url; }
	void setMountPoint(QUrl& url) { m_mountPoint = url; }
        const QUrl& baseUrl();  // Getter method for the remote base URL
        const QUrl& mountPoint();  // Getter method for the mountpoint URL
        QUrl buildRemoteUrl(const QString& path);  // Create a full URL containing both the remote base and the relative path
        QUrl buildLocalUrl(const QString& path);  // Create a full URL containing both the local mountpoint and the relative path
        void storeOpenHandle(KIO::FileJob* fileJob, OpenJobHelper* openJobHelper);
        //void lockJob(const uint64_t& fileHandleId);
        //void unLockJob(const uint64_t& fileHandleId);
        KIO::FileJob* checkOutJob(/*const QUrl& url,*/ const uint64_t& fileHandleId);  // Find the job using its ID, 
                                                                                   // and prevent other threads from using it.
       // void checkInJob(const QUrl& url, const uint64_t& fileHandleId);  // Allow other threads to use the
                                                                         // FileJob specified by this fileHandleId.
        void removeJob(const uint64_t& fileHandleId, FileJobData* fileJobData,
                       const bool& jobIsAnnulled);
        void setUpTypes();
        //void setUpErrorTranslator();
        int sysErrFromKioErr(const int& kioErr);
        void addAnnulledFh(const uint64_t& fh);
        void removeAnnulledFh(const uint64_t& fh);
        bool isAnnulled(const uint64_t& fh);
	
        void quitGracefully(const char* expression, const char* file,
                            const int& line, const char* function);

        QMutex fhIdtoFileJobDataMutex;

    public slots:
        void listJobMainThread(const QUrl& url, ListJobHelper* listJobHelper);
        void slotResult(KJob* job);
        void statJobMainThread(const QUrl& url, StatJobHelper* statJobHelper);
        void slotStatJobResult(KJob* job);
        void openJobMainThread(const QUrl& url, const QIODevice::OpenMode& qtMode, OpenJobHelper* openJobHelper);
        void fileJobOpened(KIO::Job* job);
        //void jobErrorOpen(KJob* job);
        void jobErrorReadWrite(KJob* job);
        void slotMimetype(KIO::Job* job, const QString& type);
        void findMutexMainThread(const uint64_t& fileHandleId, LockHelper* lockHelper);
        void seekReadMainThread(const uint64_t& fileHandleId, const off_t& offset, ReadJobHelper* readJobHelper);
        void slotReadPosition(KIO::Job* job, KIO::filesize_t pos);
        void readMainThread(KIO::FileJob* fileJob, const size_t& size, ReadJobHelper* readJobHelper);
        void slotData(KIO::Job* job, const QByteArray& data);
        void seekWriteMainThread(const uint64_t& fileHandleId, const off_t& offset, WriteJobHelper* writeJobHelper);
        void slotWritePosition(KIO::Job* job, KIO::filesize_t pos);
        void writeMainThread(KIO::FileJob* fileJob, const QByteArray& data, WriteJobHelper* writeJobHelper);
        void slotWritten(KIO::Job* job, const KIO::filesize_t& written);
        void mkDirMainThread(const QUrl& url, const mode_t& mode, MkDirHelper* mkDirHelper);
        void mkNodMainThread(const QUrl& url, const mode_t& mode, MkNodHelper* mkNodHelper);
        void slotMkNodResult(KJob* job);
        void chModMainThread(const QUrl& url, const mode_t& mode,
                             ChModHelper* chModHelper);
        void symLinkMainThread(const QUrl& source, const QUrl& dest,
                               SymLinkHelper* symLinkHelper);
        void reNameMainThread(const QUrl& source, const QUrl& dest,
                              ReNameHelper* reNameHelper);
        void unLinkMainThread(const QUrl& url, UnLinkHelper* unLinkHelper);
        void releaseJobMainThread(/*const QUrl& url,*/ const uint64_t& fileHandleId,
                                  const bool& jobIsAnnulled,
                                  ReleaseJobHelper* releaseJobHelper);
        void chTimeMainThread(const QUrl& url, const QDateTime& dt,
                              ChTimeHelper* chTimeHelper);
/*

    signals:
        void sendJobDone(const int&);
        void sendJobMutex(QMutex*, const int&);
        void sendEntry(const KIO::UDSEntry&);
        void sendFileHandleId(const uint64_t&);
        void sendPosition(const off_t&, const int&, KIO::FileJob*);
        void sendData(const QByteArray&, const int&);
        void sendWritten(const size_t&, const int&);
*/
    private:
        QUrl m_baseUrl;  // Remote base URL
        QUrl m_mountPoint; // Local mountpoint
        QMutex m_baseUrlMutex;  // Allows only one thread to access the remote
                                // base URL at a time.
        QMutex m_mountPointMutex;  // Allows only one thread to access the
                                   // local mountpoint URL at a time.
        QMutex m_terminatedFhListMutex;  // Allows only one thread to access
                                         // the list of terminated jobs at a
                                         // time.
        //QMutex m_errorTransMutex;  // Controls error translator

        //Cache* m_cacheRoot;  // Root node of cache
        //int m_numCached;  // Number of files cached
        //int m_numLeafStubsCached;  // Leaf stubs are for opened files that have no stat data
        //QMutex m_cacheMutex;  // Allows only one thread to access the cache at a time

        QMap<KJob *, BaseJobHelper *> m_jobToJobHelper;  // Correlate listJob with the ListJobHelper that needs it
        QMap<KJob *, QTemporaryFile *> m_jobToTempFile;  // Correlate FileCopyJob with the MkNodHelper that needs it
        QMap<uint64_t, FileJobData*> fhIdtoFileJobData;  // Correlate opened file
                                                     // handles with the FileJob
        //QMap<int, int> KIOErrToSysError;  // Translates between KIO:Error and
        //                                  // the errors defined in errno.h
        QList<uint64_t> m_terminatedFhList;
};

extern KioFuseApp *kioFuseApp;  // Make the kioFuseApp variable known to everyone who includes this file

#endif /* FUSE_KIO_FUSE_APP_H */
