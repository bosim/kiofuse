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

#include "kiofuseops.h"
#include "jobhelpers.h"

#include <QThread>

#include <QDebug>
extern "C" {
#include <unistd.h>
}

int kioFuseGetAttr(const char *relPath, struct stat *stbuf)
{
    qDebug()<<"relPath"<<relPath<<endl;

    int res = 0;
    StatJobHelper* helper;  // Helps retrieve the directory descriptors or file descriptors
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath)); // The remote URL of the directory that is being read

    helper = new StatJobHelper(url, eventLoop);  // Get the directory or file descriptor (entry)
    qDebug()<<"helper"<<helper<<endl;
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so entry is now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
        qDebug()<<"relPath"<<relPath<<"error"<<error<<"helper"<<helper<<endl;
    } else {
        qDebug()<<"relPath"<<relPath<<"helper"<<helper<<endl;
        KIO::UDSEntry entry = helper->entry();
        KFileItem* item = new KFileItem(entry, url,
                                        true /*delayedMimeTypes*/,
                                        false /*urlIsDirectory*/);
        fillStatBufFromFileItem(stbuf, item);

        delete item;
	item = NULL;
    }

    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseReadLink(const char *relPath, char *buf, size_t size)
{
    qDebug()<<"relPath"<<relPath<<endl;
    int res = 0;
    StatJobHelper* helper;
    QString destRelPath;
    QString localPath;
    bool properLink;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));

    helper = new StatJobHelper(url, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so entry is now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
    } else {
        KIO::UDSEntry entry = helper->entry();
        KFileItem* item = new KFileItem(entry, url,
                                        true /*delayedMimeTypes*/,
                                        false /*urlIsDirectory*/);

        qDebug()<<"item->isLink()"<<item->isLink()<<"item->linkDest()"<<item->linkDest()<<"kioFuseApp->baseUrl().path()"<<kioFuseApp->baseUrl().path()<<endl;

        if(!item->isLink()){
            properLink = false;
        } else if (item->linkDest().startsWith(kioFuseApp->baseUrl().path())){
            // Fully-qualified path that is a child of baseUrl is specified
            destRelPath = item->linkDest().section(kioFuseApp->baseUrl().path(), 1, -1);
            localPath = kioFuseApp->buildLocalUrl(destRelPath).path();
            properLink = true;
        } else if (item->linkDest().startsWith("/")){
            // Fully-qualified path outside of baseUrl is specified
            properLink = false;
        } else {
            // Relative path is specified
            localPath = item->linkDest();
            properLink = true;
        }

        if (properLink){
            fillLinkBufFromFileItem(buf, size, localPath);
        } else {
            res = -ENOENT;
        }

        delete item;
        item = NULL;
    }
    delete helper;
    helper = NULL;

    eventLoop->exit();

    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseMkNod(const char *relPath, mode_t mode, dev_t /*rdev*/)
{
    qDebug()<<"relPath"<<relPath<<endl;

    MkNodHelper* helper;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));
    int res = 0;

    helper = new MkNodHelper(url, mode, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so job is now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
    }

    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseMkDir(const char *relPath, mode_t mode)
{
    qDebug()<<"relPath"<<relPath<<endl;

    MkDirHelper* helper;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));
    int res = 0;

    helper = new MkDirHelper(url, mode, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so job is now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
    }

    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseUnLink(const char *relPath)
{
    qDebug()<<"relPath"<<relPath<<endl;

    UnLinkHelper* helper;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));
    int res = 0;

    helper = new UnLinkHelper(url, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so job is now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
    }

    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseRmDir(const char *relPath)
{
    return kioFuseUnLink(relPath);
}

int kioFuseSymLink(const char *from, const char *to)
{
    qDebug()<<"from"<<from<<"to"<<to<<endl;

    SymLinkHelper* helper;
    int res = 0;
    QUrl source;
    QUrl dest;
    bool properSource;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QString sourceStr = QString(from);

    if (sourceStr.startsWith(kioFuseApp->mountPoint().path())){
        // Fully-specified path that is a child of mountPoint is provided
        QString relPath = sourceStr.section(kioFuseApp->mountPoint().path(), 1,-1);
        source = kioFuseApp->buildRemoteUrl(relPath);
        properSource = true;
    } else if (!sourceStr.startsWith("/")) {
        // Relative path is provided
        source = QUrl(from);
        properSource = true;
    } else {
        // Fully-specified path outside of mountPoint is provided
        res = -EIO;
        qDebug()<<"Source doesn't start with the mountPoint path."<<endl;
        properSource = false;
    }

    if (properSource){
        dest = kioFuseApp->buildRemoteUrl(QString(to));
        qDebug()<<"source"<<source<<"dest"<<dest<<endl;
        helper = new SymLinkHelper(source, dest, eventLoop);
        eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

        //eventLoop has finished, so job is now available
        int error = helper->error();
        if (error){
            res = -kioFuseApp->sysErrFromKioErr(error);
        }

        delete helper;
        helper = NULL;
    }

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseReName(const char *from, const char *to)
{
    qDebug()<<"from"<<from<<"to"<<to<<endl;

    ReNameHelper* helper;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl source = kioFuseApp->buildRemoteUrl(QString(from)); // The remote source of the file being created
    QUrl dest = kioFuseApp->buildRemoteUrl(QString(to)); // The remote dest of the file being created
    int res = 0;

    helper = new ReNameHelper(source, dest, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so job is now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
    }

    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseLink(const char *from, const char *to)
{
    qDebug()<<"EOPNOTSUPP: from="<<from<<" to="<<to;
    return -EOPNOTSUPP;
}

int kioFuseChMod(const char *relPath, mode_t mode)
{
    qDebug()<<"relPath"<<relPath<<endl;

    ChModHelper* helper;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));
    int res = 0;

    helper = new ChModHelper(url, mode, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so job is now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
    }

    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseChOwn(const char *path, uid_t uid, gid_t gid)
{
    qDebug()<<"EOPNOTSUPP: path="<<path<<" uid="<<uid<<" gid="<<gid;
    return -EOPNOTSUPP;
}

int kioFuseTruncate(const char *relPath, off_t size)
{
    qDebug()<<"relPath"<<relPath<<"size"<<size<<endl;
    //int read;
    int chunkSize = 4096;
    char buf[4096] = {0};
    int repeatTimes;
    int leftover;
    off_t pos;
    struct stat stbuf;
    struct fuse_file_info* fi = new struct fuse_file_info;
    struct fuse_file_info* fiTemp = new struct fuse_file_info;

    kioFuseGetAttr(relPath, &stbuf);
    int origSize = stbuf.st_size;

    if (size > origSize){
        int bytesToWrite = size - origSize;
        repeatTimes = bytesToWrite / chunkSize;
        leftover = bytesToWrite % chunkSize;

        fi->flags = O_WRONLY | O_APPEND;
        kioFuseOpen(relPath, fi);

        if (origSize == 0){
            pos = 0;
        } else {
            pos = origSize -1;
        }

        for (int i = 0; i < repeatTimes; i++){
            kioFuseWrite(relPath, buf, chunkSize, pos, fi);
            pos += chunkSize;
        }
        kioFuseWrite(relPath, buf, leftover, pos, fi);
        kioFuseRelease(relPath, fi);
    } else if (size < origSize) {
        repeatTimes = size / chunkSize;
        leftover = size % chunkSize;
        fi->flags = O_RDONLY;
        kioFuseOpen(relPath, fi);

        QString relPathTempQString(relPath);
        qDebug()<<"relPathTempQString"<<relPathTempQString<<endl;
        char tmpSuffix[] = "kiofusetmp";
        qDebug()<<"tmpSuffix"<<tmpSuffix<<endl;
        relPathTempQString.append(tmpSuffix);
        qDebug()<<"relPathTempQString"<<relPathTempQString<<endl;
        QByteArray relPathArray = relPathTempQString.toLocal8Bit();
        char* relPathTemp = relPathArray.data();

        
        
       //BAD
        
        // If a file by that name already exists, fail the truncate
        /*if (kioFuseGetAttr(relPathTemp, &stbuf) == 0){
            kioFuseRelease(relPath, fi);
            delete fi;
            fi = NULL;

            delete fiTemp;
            fiTemp = NULL;

            return 0;
        }*/




//Good

        kioFuseMkNod(relPathTemp, stbuf.st_mode, 0);
        fiTemp->flags = O_WRONLY | O_TRUNC;
        kioFuseOpen(relPathTemp, fiTemp);

        pos = 0;
        for (int i = 0; i < repeatTimes; i++){
            kioFuseRead(relPath, buf, chunkSize, pos, fi);
            kioFuseWrite(relPathTemp, buf, chunkSize, pos, fiTemp);
            pos += chunkSize;
        }
        kioFuseRead(relPath, buf, leftover, pos, fi);
        kioFuseWrite(relPathTemp, buf, leftover, pos, fiTemp);

        kioFuseRelease(relPath, fi);
        kioFuseRelease(relPathTemp, fiTemp);

        kioFuseUnLink(relPath);
        kioFuseReName(relPathTemp, relPath);
    }

    delete fi;
    fi = NULL;

    delete fiTemp;
    fiTemp = NULL;

    return 0;
    










//BAD
    /*//off_t size2 = (off_t) 729608192;
    //int iSize = int(size2);
    char buf[xxxxx];

    // Read contents up to size
    fuse_file_info* fi = new fuse_file_info;
    qDebug()<<"relPath"<<relPath<<"size"<<size<<endl;
    fi->flags = O_RDONLY;
    kioFuseOpen(relPath, fi);
    kioFuseRead(relPath, buf, size, 0, fi);
    kioFuseRelease(relPath, fi);

    // Write shortened file
    fi->flags = O_WRONLY | O_TRUNC;
    kioFuseOpen(relPath, fi);
    kioFuseWrite(relPath, buf, size, 0, fi);
    kioFuseRelease(relPath, fi);

    delete fi;
    fi = NULL;

    // FIXME covert KIO errors
    return 0;*/
}

int kioFuseOpen(const char *relPath, struct fuse_file_info *fi)
{
    qDebug()<<"relPath"<<relPath<<endl;

    QIODevice::OpenMode qtMode = modeFromPosix(fi->flags);
    int res = 0;
    OpenJobHelper* helper;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));

    qDebug()<<"Waiting on fhIdtoFileJobDataMutex.lock()"<<relPath<<endl;
    kioFuseApp->fhIdtoFileJobDataMutex.lock();

    helper = new OpenJobHelper(url, qtMode, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so job is now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
        qDebug() << "Got error" << res << " kioerror " << error;
    } else {
        fi->fh = helper->fileHandleId();
        qDebug()<<"fi->fh"<<fi->fh<<endl;
    }

    qDebug()<<"fhIdtoFileJobDataMutex.unlock()"<<relPath<<fi->fh<<endl;
    kioFuseApp->fhIdtoFileJobDataMutex.unlock();

    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

int kioFuseRead(const char *relPath, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi)
{
    qDebug()<<"relPath"<<relPath<<"fi->fh"<<fi->fh<<endl;

    ReadJobHelper* readJobhelper;
    QEventLoop* lockEventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QEventLoop* readEventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));
    uint64_t fileHandleId = fi->fh;  // fi->fh is of type uint64_t
    QMutex* jobMutex;
    int res = 0;

    qDebug()<<"Waiting on fhIdtoFileJobDataMutex.lock()"<<relPath<<"fi->fh"<<fi->fh<<endl;
    kioFuseApp->fhIdtoFileJobDataMutex.lock();
    LockHelper* lockHelper = new LockHelper(fileHandleId, lockEventLoop);
    lockEventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished
    qDebug()<<"fhIdtoFileJobDataMutex.unlock()"<<relPath<<fi->fh<<endl;
    kioFuseApp->fhIdtoFileJobDataMutex.unlock();
    int error = lockHelper->error();
    if (error){
        qDebug()<<"Failed to lock job for reading. fileHandleId"<<fileHandleId<<endl;
        res = -kioFuseApp->sysErrFromKioErr(error);
    } else {
        jobMutex = lockHelper->jobMutex();
        VERIFY(jobMutex);
        jobMutex->lock();
        //qDebug()<<"lock jobMutex"<<jobMutex<<"fi->fh"<<fi->fh<<endl;

        if (kioFuseApp->isAnnulled(fi->fh))
        {
            jobMutex->unlock();
            res = -kioFuseApp->sysErrFromKioErr(KIO::ERR_COULD_NOT_READ);
        } else {
            readJobhelper = new ReadJobHelper(fileHandleId, url, size, offset, readEventLoop);
            readEventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

            //eventLoop has finished, so job is now available
            error = readJobhelper->error();
            if (error){
                qDebug()<<"Waiting on fhIdtoFileJobDataMutex.lock()"<<relPath<<"fi->fh"<<fi->fh<<endl;
                kioFuseApp->fhIdtoFileJobDataMutex.lock();

                // Tell other jobs that may be waiting to access this fh
                // that it has died.
                kioFuseApp->addAnnulledFh(fi->fh);
                jobMutex->unlock();
                //qDebug()<<"unlock jobMutex"<<jobMutex<<"fi->fh"<<fi->fh<<endl;
                kioFuseRelease(relPath, fi);
                qDebug()<<"fhIdtoFileJobDataMutex.unlock()"<<relPath<<fi->fh<<endl;
                kioFuseApp->removeAnnulledFh(fi->fh);
                kioFuseApp->fhIdtoFileJobDataMutex.unlock();
                res = -kioFuseApp->sysErrFromKioErr(error);
            } else {
                // Copy data to buffer
                QByteArray data = readJobhelper->data();
                res = data.size();
                VERIFY(static_cast<size_t>(res) <= size);
                memcpy(buf, data.data(), res);
                jobMutex->unlock();
                //qDebug()<<"unlock jobMutex"<<jobMutex<<"fi->fh"<<fi->fh<<endl;
            }

            delete readJobhelper;
            readJobhelper = NULL;
        }
    }

    delete lockHelper;
    lockHelper = NULL;

    delete lockEventLoop;
    lockEventLoop = NULL;

    delete readEventLoop;
    readEventLoop = NULL;

    return res;
}

int kioFuseWrite(const char *relPath, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi)
{
    qDebug()<<"relPath"<<relPath<<"buf"<<buf<<endl;

    WriteJobHelper* writeJobHelper;
    QEventLoop* lockEventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QEventLoop* writeEventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));
    uint64_t fileHandleId = fi->fh;  // fi->fh is of type uint64_t
    QMutex* jobMutex;
    int res = 0;

    qDebug()<<"Waiting on fhIdtoFileJobDataMutex.lock()"<<relPath<<"fi->fh"<<fi->fh<<endl;
    kioFuseApp->fhIdtoFileJobDataMutex.lock();
    LockHelper* lockHelper = new LockHelper(fileHandleId, lockEventLoop);
    lockEventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished
    qDebug()<<"fhIdtoFileJobDataMutex.unlock()"<<relPath<<fi->fh<<endl;
    kioFuseApp->fhIdtoFileJobDataMutex.unlock();
    int error = lockHelper->error();
    if (error){
        qDebug()<<"Failed to lock job for writing. fileHandleId"<<fileHandleId<<endl;
        res = -kioFuseApp->sysErrFromKioErr(error);
    } else {
        jobMutex = lockHelper->jobMutex();
        VERIFY(jobMutex);
        jobMutex->lock();
        //qDebug()<<"lock jobMutex"<<jobMutex<<"fi->fh"<<fi->fh<<endl;

        if (kioFuseApp->isAnnulled(fi->fh))
        {
            jobMutex->unlock();
            res = -kioFuseApp->sysErrFromKioErr(KIO::ERR_COULD_NOT_WRITE);
        } else {
            QByteArray data(buf, size);
            writeJobHelper = new WriteJobHelper(fileHandleId, url, data, offset, writeEventLoop);
            writeEventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

            //eventLoop has finished, so job is now available
            error = writeJobHelper->error();
            if (error){
                qDebug()<<"Waiting on fhIdtoFileJobDataMutex.lock()"<<relPath<<"fi->fh"<<fi->fh<<endl;
                kioFuseApp->fhIdtoFileJobDataMutex.lock();

                // Tell other jobs that may be waiting to access this fh
                // that it has died.
                kioFuseApp->addAnnulledFh(fi->fh);
                jobMutex->unlock();
                //qDebug()<<"unlock jobMutex"<<jobMutex<<"fi->fh"<<fi->fh<<endl;
                kioFuseRelease(relPath, fi);
                qDebug()<<"fhIdtoFileJobDataMutex.unlock()"<<relPath<<fi->fh<<endl;
                kioFuseApp->removeAnnulledFh(fi->fh);
                kioFuseApp->fhIdtoFileJobDataMutex.unlock();
                res = -kioFuseApp->sysErrFromKioErr(error);
            } else {
                VERIFY(writeJobHelper->written() == size);
                res = writeJobHelper->written();
                jobMutex->unlock();
                //qDebug()<<"unlock jobMutex"<<jobMutex<<"fi->fh"<<fi->fh<<endl;
            }

            delete writeJobHelper;
            writeJobHelper = NULL;
        }
    }

    delete lockHelper;
    lockHelper = NULL;

    delete lockEventLoop;
    lockEventLoop = NULL;

    delete writeEventLoop;
    writeEventLoop = NULL;

    return res;
}

int kioFuseStatFs(const char *path, struct statvfs *stbuf)
{
    qDebug()<<"path="<<path;

    // TODO is there a way to probe those values through KIO?
    stbuf->f_bsize = 0;
    stbuf->f_frsize = 0;
    stbuf->f_blocks = 0;
    stbuf->f_bfree = 0;
    stbuf->f_bavail = 0;
    stbuf->f_files = 0;
    stbuf->f_ffree = 0;
    stbuf->f_favail = 0;
    stbuf->f_fsid = 0;
    stbuf->f_flag = 0;
    stbuf->f_namemax = 0;

    return 0;
}

int kioFuseRelease(const char* relPath, struct fuse_file_info *fi)
{
    qDebug()<<"relPath"<<relPath<<endl;

    ReleaseJobHelper* releaseJobHelper;
    QEventLoop* lockEventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QEventLoop* releaseEventLoop = new QEventLoop();  // Returns control to this function after helper releases FileJob
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));
    uint64_t fileHandleId = fi->fh;  // fi->fh is of type uint64_t
    QMutex* jobMutex;
    int res = 0;
    bool jobIsAnnulled;

    qDebug()<<"Waiting on fhIdtoFileJobDataMutex.lock()"<<relPath<<"fi->fh"<<fi->fh<<endl;
    kioFuseApp->fhIdtoFileJobDataMutex.lock();

    LockHelper* lockHelper = new LockHelper(fileHandleId, lockEventLoop);
    lockEventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished
    int error = lockHelper->error();
    if (error){
        qDebug()<<"Failed to lock job for releasing. fileHandleId"<<fileHandleId<<endl;
        res = -kioFuseApp->sysErrFromKioErr(error);
    } else {
        jobMutex = lockHelper->jobMutex();

        // After locking and unlocking jobMutex, we can be sure that no one else
        // is wating to lock the jobMutex because we've already locked
        // fhIdtoFileJobDataMutex. We are guaranteed to be the last thread to
        // request the lock on jobMutex, and we assumes that threads are given
        // the lock in the order that they request it.
        VERIFY(jobMutex);
        jobMutex->lock();
        jobMutex->unlock();

        jobIsAnnulled = kioFuseApp->isAnnulled(fileHandleId);
        releaseJobHelper = new ReleaseJobHelper(url, fileHandleId, jobIsAnnulled, releaseEventLoop);
        releaseEventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

        //eventLoop has finished, so job is now available
        int error = releaseJobHelper->error();
        if (error){
            res = -kioFuseApp->sysErrFromKioErr(error);
        }

        delete releaseJobHelper;
        releaseJobHelper = NULL;
    }

    qDebug()<<"fhIdtoFileJobDataMutex.unlock()"<<relPath<<fi->fh<<endl;
    kioFuseApp->fhIdtoFileJobDataMutex.unlock();

    delete lockHelper;
    lockHelper = NULL;

    delete lockEventLoop;
    lockEventLoop = NULL;

    delete releaseEventLoop;
    releaseEventLoop = NULL;

    return res;
}

int kioFuseFSync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
    qDebug()<<"EOPNOTSUPP: path="<<path;
    Q_UNUSED(isdatasync)
    Q_UNUSED(fi)
    return -EOPNOTSUPP;
}

// TODO #ifdef HAVE_SETXATTR
int kioFuseSetXAttr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    qDebug()<<"EOPNOTSUPP: path="<<path<<"  name="<<name<<"  value="<<value<<"  size="<<size<<"  flags="<<flags;
    return -EOPNOTSUPP;
}

int kioFuseGetXAttr(const char *path, const char *name, char *value, size_t size)
{
    qDebug()<<"EOPNOTSUPP: path="<<path<<"  name="<<name;
    Q_UNUSED(value)
    Q_UNUSED(size)
    return -EOPNOTSUPP;
}

int kioFuseListXAttr(const char *path, char *list, size_t size)
{
    qDebug()<<"EOPNOTSUPP: path="<<path;
    Q_UNUSED(list)
    Q_UNUSED(size)
    return -EOPNOTSUPP;
}

int kioFuseRemoveXAttr(const char *path, const char *name)
{
    qDebug()<<"EOPNOTSUPP: path="<<path<<"  name="<<name;
    return -EOPNOTSUPP;
}
// TODO #endif // HAVE_SETXATTR

int kioFuseReadDir(const char *relPath, void *buf, fuse_fill_dir_t filler,
                    off_t /*offset*/, struct fuse_file_info* /*fi*/)
{
    int res = 0;
    ListJobHelper* helper;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));

    qDebug()<<"kioFuseReadDir relPath: "<<relPath<<"eventLoop->thread()"<<eventLoop->thread()<<endl;
    
    helper = new ListJobHelper(url, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so entries are now available
    int error = helper->error();
    if (error){
        res = -kioFuseApp->sysErrFromKioErr(error);
        qDebug()<<"errro"<<error<<endl;
    } else {
        KIO::UDSEntryList entries = helper->entries();
	qDebug() << "got " << entries.count();
        for(KIO::UDSEntryList::ConstIterator it = entries.begin();
            it!=entries.end(); ++it){
            KIO::UDSEntry entry = *it;
            struct stat st;

            /* The parent (..) directory doesn't belong to us. We need to
               get its local permissions. */
            if (entry.stringValue(KIO::UDSEntry::UDS_NAME) == ".."){
                qDebug()<<"Parent dir"<<endl;
                memset(&st, 0, sizeof(st));
                lstat("..", &st);
                filler(buf, "..", &st, 0);
            } else {
                KFileItem* item = new KFileItem(entry, url,
                        true /*delayedMimeTypes*/,
                        true /*urlIsDirectory*/);
                fillStatBufFromFileItem(&st, item);
                filler(buf, item->name().toLatin1(), &st, 0);  // Tell the name of this item to FUSE

                delete item;
                item = NULL;
            }
        }
    }
    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

// TODO when KIO can check permissions
/*int kioFuseAccess(const char *relPath, int mask)
{
    qDebug()<<"relPath"<<relPath<<endl;
    return 0;
}*/

int kioFuseUTimeNS(const char *relPath, const struct timespec ts[2])
{
    qDebug()<<"kiofuseUTimeNS relPath"<<relPath<<endl;

    ChTimeHelper* helper;
    QEventLoop* eventLoop = new QEventLoop();  // Returns control to this function after helper gets the data
    QUrl url = kioFuseApp->buildRemoteUrl(QString(relPath));

    // ts[1] contains the modification time
    // ts[0] contains the access time, but KIO can't set it, so we ignore it
    QDateTime dt = QDateTime::fromTime_t(ts[1].tv_sec);
    int res = 0;

    helper = new ChTimeHelper(url, dt, eventLoop);
    eventLoop->exec(QEventLoop::ExcludeUserInputEvents);  // eventLoop->quit() is called in BaseJobHelper::jobDone() of helper

    //eventLoop has finished, so job is now available
    int error = helper->error();
    if (error){
        qDebug() << "Got error" << error;
        res = -kioFuseApp->sysErrFromKioErr(error);
    }

    delete helper;
    helper = NULL;

    eventLoop->exit();
    
    delete eventLoop;
    eventLoop = NULL;

    return res;
}

// TODO #ifdef HAVE_POSIX_FALLOCATE
int kioFuseFAllocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi)
{
    qDebug()<<"EOPNOTSUPP: path="<<path<<"  mode="<<mode<<"  offset="<<offset<<"  length="<<length;
    Q_UNUSED(fi)
    return -EOPNOTSUPP;
}
// TODO #endif // HAVE_POSIX_FALLOCATE

void fillStatBufFromFileItem(struct stat *stbuf, KFileItem *item)
{
    //qDebug()<<" entry.numberValue(KIO::UDSEntry::UDS_ACCESS)"<<entry.numberValue(KIO::UDSEntry::UDS_ACCESS)<<endl;
    
    /* We should be finding out the effective permissions by taking into
    consideration 1) the group affiliation of the username we are connecting
    as and 2) the remote permission of file/directory we are accessing.
    This requires making KFileItem::isWritable() et. al. reliable for
    network slaves. For example, KFileItem::isWritable() should check the
       /etc/group file on the remote server to ensure that the user we are
    connecting with is a member of a group that has write permission. This
    will allow KioFuse to display the effective permission on the client
    side (currently KioFuse just copies the remote stat, which is not
    applicable to the local system).
    */
    qDebug() << "fillStatBufFromFileItem";
    memset(stbuf, 0, sizeof(struct stat));
    stbuf->st_dev = 0;
    stbuf->st_ino = 0;
    stbuf->st_mode = item->permissions()|(item->isLink()?S_IFLNK:item->mode());
    stbuf->st_nlink = 1;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_rdev = 0;
    stbuf->st_size = item->size();
    /*stbuf->st_mtime = item->time(KFileItem::FileTimes::ModificationTime).toSecsSinceEpoch();
    stbuf->st_atime = item->time(KFileItem::FileTimes::AccessTime).toSecsSinceEpoch(); 
    stbuf->st_ctime = item->time(KFileItem::FileTimes::CreationTime).toSecsSinceEpoch();*/
    stbuf->st_blksize = 0;
    stbuf->st_blocks = 0;
    qDebug() << "returned!";
}

void fillLinkBufFromFileItem(char *buf, size_t size, const QString& dest)
{
    qDebug() << "fillLinkBufFromFileItem";
    const char *data = dest.toLatin1();

    VERIFY(size > 0);
    size_t len = size-1;

    if (static_cast<size_t>(dest.length()) < len)
    {
        len = static_cast<size_t>(dest.length());
    }

    for(size_t i=0; i<len; i++)
    {
        buf[i] = data[i];
    }

    buf[len] = '\0';
}

QIODevice::OpenMode modeFromPosix(int flags)
{
    QIODevice::OpenMode qtMode;
    if ((flags & O_ACCMODE) == O_RDONLY){
        qtMode |= QIODevice::ReadOnly;
    }
    if ((flags & O_ACCMODE) == O_WRONLY){
        qtMode |= QIODevice::WriteOnly;
    }
    if ((flags & O_ACCMODE) == O_RDWR){
        qtMode |= QIODevice::ReadWrite;
    }
    if (flags & O_APPEND){
        qtMode |= QIODevice::Append;
    }
    if (flags & O_TRUNC){
        qtMode |= QIODevice::Truncate;
    }
    return qtMode;
}
