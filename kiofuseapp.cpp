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

#include "kiofuseapp.h"

#include <signal.h>

//#include <QThread>
//#include <QMutexLocker>

#include <QDebug>
#include <KIO/DeleteJob>

KioFuseApp *kioFuseApp = NULL;
//const unsigned int MAX_TIME_OPENED = 8*3600*1000 /*4000*/;  // Time out for opened files in ms

KioFuseApp::KioFuseApp(int &argc, char* argv[])
    : QCoreApplication(argc, argv),  //No GUI
      fhIdtoFileJobDataMutex(QMutex::Recursive),
      m_baseUrlMutex(QMutex::Recursive),
      m_mountPointMutex(QMutex::Recursive),
      m_terminatedFhListMutex(QMutex::Recursive)
      //m_errorTransMutex(QMutex::Recursive)  // Controls error translator
      //m_cacheRoot(NULL),
      //m_numCached(1),  // One stub (the root) is already cached in the constructor, so start counter at 1
      //m_numLeafStubsCached(0),  // Leaf stubs are for opened files that have no stat data
      //m_cacheMutex(QMutex::Recursive)  // Allow the mutex to be locked several times within the same thread
{
    //QMutexLocker locker(&m_cacheMutex);
}

KioFuseApp::~KioFuseApp()
{
    //QMutexLocker locker(&m_cacheMutex);

    qDebug()<<"KioFuseApp dtor"<<endl;
    foreach (FileJobData* fileJobData, fhIdtoFileJobData){
        // FileJobData dtor will close the FileJob
        delete fileJobData;
    }
    //delete m_cacheRoot;  // Recursively delete the whole cache
    //m_cacheRoot = NULL;
}

const QUrl& KioFuseApp::baseUrl()  // Getter method for the remote base URL
{
    QMutexLocker locker(&m_baseUrlMutex);
    return m_baseUrl;
}

const QUrl& KioFuseApp::mountPoint()  // Getter method for the remote base URL
{
    QMutexLocker locker(&m_mountPointMutex);
    return m_mountPoint;
}

QUrl KioFuseApp::buildRemoteUrl(const QString& path)  // Create a full URL containing both the remote base and the relative path
{
    QMutexLocker locker(&m_baseUrlMutex);
    QUrl url = baseUrl();
    /*if (path == "/"){
        // Don't need to append only a "/"
        // Allows files to be baseUrls
        return url;
    }*/
    url.setPath(path);
    return url;
}

QUrl KioFuseApp::buildLocalUrl(const QString& path)  // Create a full URL containing both the remote base and the relative path
{
    QMutexLocker locker(&m_mountPointMutex);
    QUrl url = mountPoint();
    url.setPath(path);
    return url;
}

void KioFuseApp::storeOpenHandle(KIO::FileJob* fileJob, OpenJobHelper* openJobHelper)
{
    //QMutexLocker locker(&m_cacheMutex);
    FileJobData* fileJobData = new FileJobData(fileJob);  //FIXME delete at random intervals
    VERIFY(fileJobData->fileJob->thread() == this->thread());
    uint64_t fileHandleId;  // fi->fh is of type uint64_t

    // Make sure we haven't run out of candidate fileHandleIds
    VERIFY(fhIdtoFileJobData.size() < (RAND_MAX - 1));
    
    //FIXME FIXME remove
    //VERIFY(1==2);

    fileJobData->qTime.start();

    do {
        fileHandleId = static_cast<uint64_t>(rand());
    } while (fhIdtoFileJobData.contains(fileHandleId));

    //Don't need an assert here because we're out of the while loop
    fhIdtoFileJobData.insertMulti(fileHandleId, fileJobData);
    qDebug()<<"fileHandleId"<<fileHandleId<<"fileJob"<<fileJob<<"&(fileJobData->jobMutex)"<<&(fileJobData->jobMutex)<<endl;

    //qRegisterMetaType<uint64_t>("uint64_t");
    VERIFY(QMetaObject::invokeMethod(openJobHelper, "setFileHandleId",
                                       Q_ARG(uint64_t, fileHandleId)));
    //QTimer::singleShot(0, openJobHelper, SLOT(setFileHandleId(fileHandleId)));
    /*connect(this, SIGNAL(sendFileHandleId(const uint64_t&)),
            openJobHelper, SLOT(setFileHandleId(const uint64_t&)),
            Qt::QueuedConnection);
    emit sendFileHandleId(fileHandleId);*/

    /*bool addedLeafStub = m_cacheRoot->setExtraData(url, fileHandleId, fileJob);
    if (addedLeafStub){
        m_numLeafStubsCached++;
    }*/
}

// Find the job using its ID, and prevent other threads from
// using it. Because it locks a mutex, it must be called directly
// from the client thread.
/*void KioFuseApp::lockJob(const uint64_t& fileHandleId)
{
    qDebug()<<"Locking fileHandleId"<<fileHandleId<<endl;
    FileJobData* fileJobData = fhIdtoFileJobData.value(fileHandleId);
    if (fileJobData){
        fileJobData->jobMutex.lock();
    }
}

// Opposite of lockJob
void KioFuseApp::unLockJob(const uint64_t& fileHandleId)
{
    qDebug()<<"Unlocking fileHandleId"<<fileHandleId<<endl;
    FileJobData* fileJobData = fhIdtoFileJobData.value(fileHandleId);
    if (fileJobData){
        fileJobData->jobMutex.unlock();
    }
}*/

// Return an opened job and reset its timer.
KIO::FileJob* KioFuseApp::checkOutJob(/*const QUrl& url,*/
                                      const uint64_t& fileHandleId)
{
    //QMutexLocker cacheLocker(&m_cacheMutex);

    if (fhIdtoFileJobData.contains(fileHandleId)){
        FileJobData* fileJobData = fhIdtoFileJobData.value(fileHandleId);
        VERIFY(fhIdtoFileJobData.count(fileHandleId) == 1);
        VERIFY(fileJobData->fileJob->thread() == this->thread());

        //FIXME Fuse renames temporary files on the server. This is a FUSE bug.
        /*// The urls of the file being accessed and the opened file must match
        VERIFY(url.path(QUrl::RemoveTrailingSlash) == 
                fileJobData->fileJob->url().path(QUrl::RemoveTrailingSlash));*/

/*        if (fileJobData->qTime.elapsed() > MAX_TIME_OPENED){
            qDebug()<<"Found expired fileHandleId"<<fileHandleId<<endl;
            removeJob(fileHandleId, fileJobData);
            fileJobData = NULL;

            return NULL;
        }*/

        qDebug()<<"fileHandleId"<<fileHandleId<<"fileJobData"<<fileJobData<<"fileJobData->fileJob"<<fileJobData->fileJob<<endl;
        //cacheLocker.unlock();
        /*while (fileJobData->inUse){
            qDebug()<<"fileJobData is in use, wasting some time."<<endl;
        }
        fileJobData->inUse = true;*/
        //fileJobData->jobMutex.lock();
        //cacheLocker.relock();

        fileJobData->qTime.restart();

        return fileJobData->fileJob;
    } else {
        // Didn't find the job
        qDebug()<<"WARNING: Didn't find fileHandleId"<<fileHandleId<<endl;
        return NULL;
    }
}

// Allow other threads to use the FileJob specified by this fileHandleId
/*void KioFuseApp::checkInJob(const QUrl& url, const uint64_t& fileHandleId)
{
    //QMutexLocker locker(&m_cacheMutex);

    // The fileJob *must* be there if we're checking it back in
    if(!fhIdtoFileJobData.contains(fileHandleId)){
        qDebug()<<"WARNING: File expired: url"<<url<<endl;
        return;
    }
    VERIFY(fhIdtoFileJobData.count(fileHandleId) == 1);

    FileJobData* fileJobData = fhIdtoFileJobData.value(fileHandleId);
    VERIFY(fileJobData->fileJob->thread() == this->thread());

    fileJobData->qTime.restart();
    //fileJobData->inUse = false;
    //fileJobData->jobMutex.unlock();
}*/

void KioFuseApp::removeJob(const uint64_t& fileHandleId,
                           FileJobData* fileJobData, const bool& jobIsAnnulled)
{
    //QMutexLocker locker(&m_cacheMutex);
    //VERIFY(fileJobData->fileJob->thread() == this->thread());

    qDebug()<<"fhIdtoFileJobData.size()"<<fhIdtoFileJobData.size()<<endl;
    int removed = fhIdtoFileJobData.remove(fileHandleId);
    VERIFY(removed == 1);
    /*if (removed != 1){
        qDebug()<<"WARNING: File expired: fileHandleId"<<fileHandleId<<endl;
        fileJobData = NULL;  // Don't delete twice
        return;
    }*/

    fileJobData->jobIsAnnulled = jobIsAnnulled;
    delete fileJobData;
    fileJobData = NULL;
}

/*********** ListJob ***********/
void KioFuseApp::listJobMainThread(const QUrl& url, ListJobHelper* listJobHelper)
{
    qDebug()<<"this->thread()"<<this->thread()<<endl;

    KIO::ListJob* listJob = KIO::listDir(url, KIO::HideProgressInfo, true);
    VERIFY(listJob->thread() == this->thread());

    qDebug()<<"listJob"<<listJob<<endl;
    qDebug()<<"listJob->thread()"<<listJob->thread()<<endl;

    // Correlate listJob with the ListJobHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(listJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(listJob),
                            qobject_cast<BaseJobHelper*>(listJobHelper));

    // Needed to be able to use Qt::QueuedConnection
    //qRegisterMetaType<KIO::UDSEntryList>("KIO::UDSEntryList");

    // Send the entries to listJobHelper when they become available
    connect(listJob, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList &)),
            listJobHelper, SLOT(receiveEntries(KIO::Job*, const KIO::UDSEntryList &)));

    // Job will be deleted when finished
    connect(listJob, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));
}

void KioFuseApp::slotResult(KJob* job)
{
    qDebug()<<"this->thread()"<<this->thread()<<endl;
    qDebug()<<"job"<<job<<endl;
    int error = job->error();

    BaseJobHelper* jobHelper = m_jobToJobHelper.value(job);

    // Remove job and jobHelper from map
    int numJobsRemoved = m_jobToJobHelper.remove(job);
    VERIFY(numJobsRemoved == 1);

    VERIFY(job);
    job->kill();
    job = NULL;

    VERIFY(QMetaObject::invokeMethod(jobHelper, "jobDone",
                                       Q_ARG(int, error)));
    /*connect(this, SIGNAL(sendJobDone(const int&)),
            jobHelper, SLOT(jobDone(const int&)), Qt::QueuedConnection);
    emit sendJobDone(error);*/
}

/*********** StatJob ***********/
void KioFuseApp::statJobMainThread(const QUrl& url,
                                   StatJobHelper* statJobHelper)
{
    /*KIO::StatJob* statJob = KIO::stat(url, KIO::StatJob::SourceSide,
                                      2, KIO::HideProgressInfo);*/
    KIO::StatJob* statJob = KIO::stat(url, KIO::HideProgressInfo);
    qDebug()<<"url"<<url<<"statJobHelper"<<statJobHelper<<"statJob"<<statJob<<endl;
    VERIFY(statJob->thread() == this->thread());

    // Correlate listJob with the ListJobHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(statJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(statJob),
                            qobject_cast<BaseJobHelper*>(statJobHelper));

    // Job will be deleted when finished
    connect(statJob, SIGNAL(result(KJob*)),
            this, SLOT(slotStatJobResult(KJob*)));
}

void KioFuseApp::slotStatJobResult(KJob* job)
{
    int error = job->error();
    qDebug()<<"error"<<error<<endl;
    if (error != 0) qDebug()<<job->errorString()<<endl;

    BaseJobHelper* jobHelper = m_jobToJobHelper.value(job);
    StatJobHelper* statJobHelper = qobject_cast<StatJobHelper*>(jobHelper);

    if (job->error() == 0){
        KIO::StatJob* statJob = qobject_cast<KIO::StatJob*>(job);
        KIO::UDSEntry entry = statJob->statResult();
        qDebug()<<"statJobHelper"<<statJobHelper<<"statJob"<<statJob<<endl;

        // Needed to be able to use Qt::QueuedConnection
        //qRegisterMetaType<KIO::UDSEntry>("KIO::UDSEntry");

        // Send the entry to statJobHelper
        VERIFY(QMetaObject::invokeMethod(statJobHelper, "receiveEntry",
                                           Q_ARG(KIO::UDSEntry, entry)));
        /*connect(this, SIGNAL(sendEntry(const KIO::UDSEntry &)),
                statJobHelper, SLOT(receiveEntry(const KIO::UDSEntry &)),
                Qt::QueuedConnection);
        emit sendEntry(entry);*/
    }

    // Remove job and jobHelper from map
    int numJobsRemoved = m_jobToJobHelper.remove(job);
    //VERIFY(numJobsRemoved == 1);

    VERIFY(job);
    job->kill();
    job = NULL;

    VERIFY(QMetaObject::invokeMethod(jobHelper, "jobDone",
                                       Q_ARG(int, error)));
    /*connect(this, SIGNAL(sendJobDone(const int&)),
            jobHelper, SLOT(jobDone(const int&)), Qt::QueuedConnection);
    emit sendJobDone(error);*/
}

/*********** OpenJob ***********/
void KioFuseApp::openJobMainThread(const QUrl& url, const QIODevice::OpenMode& qtMode, OpenJobHelper* openJobHelper)
{
    // Will be cached and close()-ed upon POSIX release()
    KIO::FileJob* fileJob = KIO::open(url, qtMode);
    //fileJob->setAutoDelete(false);  // We'll kill() the fileJob in the 
                                    // destructor of FileJobData or in
                                    // slotResult.

    VERIFY(fileJob->thread() == this->thread());

    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(fileJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(fileJob),
                            qobject_cast<BaseJobHelper*>(openJobHelper));

    connect(fileJob, SIGNAL(open(KIO::Job*)),
            this, SLOT(fileJobOpened(KIO::Job*)));
    connect(fileJob, SIGNAL(canceled(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(finished(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(result(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    // FIXME for testing purposes, only
    connect(fileJob, SIGNAL(mimetype(KIO::Job*, const QString&)),
            this, SLOT(slotMimetype(KIO::Job*, const QString&)));
}

void KioFuseApp::fileJobOpened(KIO::Job* job)
{
    KIO::FileJob* fileJob = qobject_cast<KIO::FileJob*>(job);
    qDebug()<<"fileJob->thread()"<<fileJob->thread()<<"this->thread()"<<this->thread()<<endl;

    // Prevent fileJob calling jobErrorReadWrite() after the file is already opened
    disconnect(fileJob, 0, this, SLOT(jobErrorReadWrite(KJob*)));

    int error = job->error();
    BaseJobHelper* jobHelper = m_jobToJobHelper.value(qobject_cast<KJob*>(job));
    OpenJobHelper* openJobHelper = qobject_cast<OpenJobHelper*>(jobHelper);

    if (!error){
        // Store fh in cache and in the openJobHelper
        kioFuseApp->storeOpenHandle(fileJob, openJobHelper);
    } else
        qDebug()<<job->errorString();

    // Remove job and jobHelper from map
    int numJobsRemoved = m_jobToJobHelper.remove(qobject_cast<KJob*>(job));
    VERIFY(numJobsRemoved == 1);

    VERIFY(QMetaObject::invokeMethod(openJobHelper, "jobDone",
                                       Q_ARG(int, error)));
    /*connect(this, SIGNAL(sendJobDone(const int&)),
            openJobHelper, SLOT(jobDone(const int&)), Qt::QueuedConnection);
    emit sendJobDone(error);*/
}

// The difference between jobErrorReadWrite and jobErrorOpen is that 
// jobErrorOpen kills the job by calling slotResult while jobErrorReadWrite
// doesn't.
/*void KioFuseApp::jobErrorOpen(KJob* job)
{
    KIO::FileJob* fileJob = qobject_cast<KIO::FileJob*>(job);
    disconnect(fileJob, 0, this, SLOT(jobErrorOpen(KJob*)));

    // If there's no error, ignore and don't call slotResult.
    // fileJobOpened will handle it
    if (job->error()){
        qDebug()<<"Error opening fileJob. fileJob"<<fileJob<<"job->error()"<<job->error()<<endl;
        VERIFY(QMetaObject::invokeMethod(this, "slotResult",
                 Q_ARG(KJob*, job)));
    }
}*/

/*// The difference between jobErrorReadWrite and jobErrorOpen is that 
// jobErrorOpen kills the job by calling slotResult while jobErrorReadWrite
// doesn't.*/
// The difference between jobErrorReadWrite and slotResult is that slotResult
// kills the job, while jobErrorReadWrite does not. This is because a job
// is already finished after it gives an error, and attempting to kill() it
// can cause a crash.
void KioFuseApp::jobErrorReadWrite(KJob* job)
{
    KIO::FileJob* fileJob = qobject_cast<KIO::FileJob*>(job);
    disconnect(fileJob, 0, this, SLOT(jobErrorReadWrite(KJob*)));
    int error = job->error();

    // If there's no error, ignore and don't call slotResult.
    // fileJobOpened will handle it
    if (error){
        qDebug()<<"Error reading/writing to fileJob. error"<<error<<endl<<job->errorString();

        BaseJobHelper* jobHelper = m_jobToJobHelper.value(job);

        // Remove job and jobHelper from map but don't kill the job
        int numJobsRemoved = m_jobToJobHelper.remove(job);
        VERIFY(numJobsRemoved == 1);

        VERIFY(QMetaObject::invokeMethod(jobHelper, "jobDone",
                 Q_ARG(int, error)));
    }
}

// FIXME for testing purposes, only
void KioFuseApp::slotMimetype(KIO::Job* job, const QString& type)
{
    KIO::FileJob* fileJob = qobject_cast<KIO::FileJob*>(job);
    disconnect(fileJob, 0, this, SLOT(slotMimetype(KIO::Job*, const QString&)));

    qDebug()<<"type"<<type<<endl;
}

/*********** LockHelper ***********/
void KioFuseApp::findMutexMainThread(const uint64_t& fileHandleId, LockHelper* lockHelper)
{
    QMutex* jobMutex;
    int error = 0;

    FileJobData* fileJobData = fhIdtoFileJobData.value(fileHandleId);
    if (fileJobData){
        jobMutex = &(fileJobData->jobMutex);
    } else {
        jobMutex = NULL;
        error = KIO::ERR_DOES_NOT_EXIST;
    }

    VERIFY(QMetaObject::invokeMethod(lockHelper, "setJobMutex",
                                       Q_ARG(QMutex*, jobMutex),
                                       Q_ARG(int, error)));
    /*connect(this, SIGNAL(sendJobMutex(QMutex*, const int&)),
            lockHelper, SLOT(setJobMutex(QMutex*, const int&)), Qt::QueuedConnection);
    emit sendJobMutex(jobMutex, error);*/
}

/*********** Seek for Read ***********/
void KioFuseApp::seekReadMainThread(const uint64_t& fileHandleId, const off_t& offset, ReadJobHelper* readJobHelper)
{
    //FIXME trying out
    //FIXME don't need because of QMetaObject::invokeMethod
    //disconnect(readJobHelper, 0, this, 0);
    //readJobHelper->disconnect();

    KIO::FileJob* fileJob = checkOutJob(/*readJobHelper->url(),*/ fileHandleId);
    if (!fileJob){
        qDebug()<<"fileJob not found. fileHandleId"<<fileHandleId<<endl;
        VERIFY(QMetaObject::invokeMethod(readJobHelper, "jobDone",
                                           Q_ARG(int, KIO::ERR_COULD_NOT_READ)));
        /*connect(this, SIGNAL(sendJobDone(const int&)),
                readJobHelper, SLOT(jobDone(const int&)), Qt::QueuedConnection);
        emit sendJobDone(KIO::ERR_COULD_NOT_READ);*/
        return;
    }

    VERIFY(fileJob->thread() == this->thread());
    qDebug()<<"fileJob"<<fileJob<<"readJobHelper"<<readJobHelper<<endl;

    qDebug()<<"m_jobToJobHelper.count(qobject_cast<KJob*>(fileJob))"<<m_jobToJobHelper.count(qobject_cast<KJob*>(fileJob))<<endl;
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(fileJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(fileJob),
                                 qobject_cast<BaseJobHelper*>(readJobHelper));

    // Needed by Qt::QueuedConnection
    //qRegisterMetaType<KIO::filesize_t>("KIO::filesize_t");
    connect(fileJob, SIGNAL(position(KIO::Job*, KIO::filesize_t)),
            this, SLOT(slotReadPosition(KIO::Job*, KIO::filesize_t)));
    connect(fileJob, SIGNAL(canceled(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(finished(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(result(KJob*)), 
            this, SLOT(jobErrorReadWrite(KJob*)));

    fileJob->seek(static_cast<KIO::filesize_t>(offset));
}

void KioFuseApp::slotReadPosition(KIO::Job* job, KIO::filesize_t pos)
{
    KIO::FileJob* fileJob = qobject_cast<KIO::FileJob*>(job);

    // Prevent FileJob::position() signal from being triggered multiple times
    // upon subsequent seeks. Also prevents fileJob from calling
    // jobErrorReadWrite() after we've received the position.
    disconnect(fileJob, 0, this, 0);

    qDebug()<<"job"<<job<<"job->thread()"<<job->thread()<<"pos"<<pos<<endl;

    VERIFY(m_jobToJobHelper.contains(qobject_cast<KJob*>(job)));
    BaseJobHelper* jobHelper = m_jobToJobHelper.value(qobject_cast<KJob*>(job));
    ReadJobHelper* readJobHelper = qobject_cast<ReadJobHelper*>(jobHelper);

    qDebug()<<"readJobHelper"<<readJobHelper<<endl;

    // Remove job and jobHelper from map
    int numJobsRemoved = m_jobToJobHelper.remove(qobject_cast<KJob*>(job));
    VERIFY(numJobsRemoved == 1);

    // Needed by Qt::QueuedConnection
    //qRegisterMetaType<off_t>("off_t");
    // Send the position to readJobHelper
    VERIFY(QMetaObject::invokeMethod(readJobHelper, "receivePosition",
                                       Q_ARG(off_t, static_cast<off_t>(pos)),
                                       Q_ARG(int, job->error()),
                                       Q_ARG(KIO::FileJob*, fileJob)));
    /*connect(this, SIGNAL(sendPosition(const off_t&, const int&, KIO::FileJob*)),
            readJobHelper, SLOT(receivePosition(const off_t&, const int&, KIO::FileJob*)),
            Qt::QueuedConnection);
    emit sendPosition(static_cast<off_t>(pos), job->error(), fileJob);*/
}

void KioFuseApp::readMainThread(KIO::FileJob* fileJob, const size_t& size, ReadJobHelper* readJobHelper)
{
    qDebug()<<"size"<<size<<endl;
    //qDebug()<<"readJobHelper"<<readJobHelper<<endl;

    //FIXME trying out
    //Not needed because of QMetaObject::invokeMethod
    //disconnect(readJobHelper, 0, this, 0);
    //readJobHelper->disconnect();

    VERIFY(fileJob->thread() == this->thread());

    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(fileJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(fileJob),
                                 qobject_cast<BaseJobHelper*>(readJobHelper));

    connect(fileJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
            this, SLOT(slotData(KIO::Job*, const QByteArray&)));
    connect(fileJob, SIGNAL(canceled(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(finished(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(result(KJob*)), 
            this, SLOT(jobErrorReadWrite(KJob*)));

    fileJob->read(static_cast<KIO::filesize_t>(size));
}

void KioFuseApp::slotData(KIO::Job* job, const QByteArray& data)
{
    qDebug()<<"data"<<data<<"job"<<job<<endl;

    KIO::FileJob* fileJob = qobject_cast<KIO::FileJob*>(job);

    // Prevent FileJob::data() signal from being triggered multiple times
    // upon subsequent reads. Also prevents fileJob from calling
    // jobErrorReadWrite() after we've received the data.
    disconnect(fileJob, 0, this, 0);
    //fileJob->disconnect();

    BaseJobHelper* jobHelper = m_jobToJobHelper.value(qobject_cast<KJob*>(job));
    ReadJobHelper* readJobHelper = qobject_cast<ReadJobHelper*>(jobHelper);

    VERIFY(readJobHelper);
    checkOutJob(/*readJobHelper->url(),*/ readJobHelper->fileHandleId());

    // Remove job and jobHelper from map
    int numJobsRemoved = m_jobToJobHelper.remove(qobject_cast<KJob*>(job));
    /*if (numJobsRemoved != 1)
    {
    qDebug()<<"numJobsRemoved"<<numJobsRemoved<<"readJobHelper->fileHandleId()"<<readJobHelper->fileHandleId()<<endl;
}*/
    VERIFY(numJobsRemoved == 1);

    VERIFY(QMetaObject::invokeMethod(readJobHelper, "receiveData",
                                       Q_ARG(QByteArray, data),
                                       Q_ARG(int, job->error())));
    /*connect(this, SIGNAL(sendData(const QByteArray&, const int&)),
            readJobHelper, SLOT(receiveData(const QByteArray&, const int&)),
            Qt::QueuedConnection);
    emit sendData(data, job->error());*/
}

/*********** Seek for Write ***********/
void KioFuseApp::seekWriteMainThread(const uint64_t& fileHandleId, const off_t& offset, WriteJobHelper* writeJobHelper)
{
    KIO::FileJob* fileJob = checkOutJob(/*writeJobHelper->url(),*/ fileHandleId);
    if (!fileJob){
        qDebug()<<"fileJob not found. fileHandleId"<<fileHandleId<<endl;
        VERIFY(QMetaObject::invokeMethod(writeJobHelper, "jobDone",
                                           Q_ARG(int, KIO::ERR_COULD_NOT_READ)));
        /*connect(this, SIGNAL(sendJobDone(const int&)),
                writeJobHelper, SLOT(jobDone(const int&)), Qt::QueuedConnection);
        emit sendJobDone(KIO::ERR_COULD_NOT_READ);*/
        return;
    }

    VERIFY(fileJob->thread() == this->thread());
    qDebug()<<"fileJob"<<fileJob<<"fileJob->thread()"<<fileJob->thread()<<endl;

    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(fileJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(fileJob),
                                 qobject_cast<BaseJobHelper*>(writeJobHelper));

    // Needed by Qt::QueuedConnection
    //qRegisterMetaType<KIO::filesize_t>("KIO::filesize_t");
    connect(fileJob, SIGNAL(position(KIO::Job*, KIO::filesize_t)),
            this, SLOT(slotWritePosition(KIO::Job*, KIO::filesize_t)));
    connect(fileJob, SIGNAL(canceled(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(finished(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(result(KJob*)), 
            this, SLOT(jobErrorReadWrite(KJob*)));

    fileJob->seek(static_cast<KIO::filesize_t>(offset));
}

void KioFuseApp::slotWritePosition(KIO::Job* job, KIO::filesize_t pos)
{
    KIO::FileJob* fileJob = qobject_cast<KIO::FileJob*>(job);

    // Prevent FileJob::position() signal from being triggered multiple times
    // upon subsequent seeks. Also prevents fileJob from calling
    // jobErrorReadWrite() after we've received the position.
    disconnect(fileJob, 0, this, 0);
    //fileJob->disconnect();

    qDebug()<<"job"<<job<<"job->thread()"<<job->thread()<<endl;

    VERIFY(m_jobToJobHelper.contains(qobject_cast<KJob*>(job)));
    BaseJobHelper* jobHelper = m_jobToJobHelper.value(qobject_cast<KJob*>(job));
    WriteJobHelper* writeJobHelper = qobject_cast<WriteJobHelper*>(jobHelper);

    // Remove job and jobHelper from map
    int numJobsRemoved = m_jobToJobHelper.remove(qobject_cast<KJob*>(job));
    VERIFY(numJobsRemoved == 1);

    // Needed by Qt::QueuedConnection
    //qRegisterMetaType<off_t>("off_t");
    // Send the position to writeJobHelper
    VERIFY(QMetaObject::invokeMethod(writeJobHelper, "receivePosition",
                                       Q_ARG(off_t, static_cast<off_t>(pos)),
                                       Q_ARG(int, job->error()),
                                       Q_ARG(KIO::FileJob*, fileJob)));
    /*connect(this, SIGNAL(sendPosition(const off_t&, const int&, KIO::FileJob*)),
            writeJobHelper, SLOT(receivePosition(const off_t&, const int&, KIO::FileJob*)),
            Qt::QueuedConnection);
    emit sendPosition(static_cast<off_t>(pos), job->error(), fileJob);*/
}

void KioFuseApp::writeMainThread(KIO::FileJob* fileJob, const QByteArray& data, WriteJobHelper* writeJobHelper)
{
    qDebug()<<"writeJobHelper"<<writeJobHelper<<endl;

    VERIFY(fileJob->thread() == this->thread());

    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(fileJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(fileJob),
                                 qobject_cast<BaseJobHelper*>(writeJobHelper));

    // Needed by Qt::QueuedConnection
    //qRegisterMetaType<KIO::filesize_t>("KIO::filesize_t");
    connect(fileJob, SIGNAL(written(KIO::Job*, const KIO::filesize_t&)),
            this, SLOT(slotWritten(KIO::Job*, const KIO::filesize_t&)));
    connect(fileJob, SIGNAL(canceled(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(finished(KJob*)),
            this, SLOT(jobErrorReadWrite(KJob*)));
    connect(fileJob, SIGNAL(result(KJob*)), 
            this, SLOT(jobErrorReadWrite(KJob*)));

    fileJob->write(data);
}

void KioFuseApp::slotWritten(KIO::Job* job, const KIO::filesize_t& written)
{
    qDebug()<<"written"<<written<<endl;

    KIO::FileJob* fileJob = qobject_cast<KIO::FileJob*>(job);

    // Prevent FileJob::written() signal from being triggered multiple times
    // upon subsequent reads. Also prevents fileJob from calling
    // jobErrorReadWrite() after we've received the number of bytes written.
    disconnect(fileJob, 0, this, 0);
    //fileJob->disconnect();

    BaseJobHelper* jobHelper = m_jobToJobHelper.value(qobject_cast<KJob*>(job));
    WriteJobHelper* writeJobHelper = qobject_cast<WriteJobHelper*>(jobHelper);

    VERIFY(writeJobHelper);
    checkOutJob(/*writeJobHelper->url(),*/ writeJobHelper->fileHandleId());

    // Remove job and jobHelper from map
    int numJobsRemoved = m_jobToJobHelper.remove(qobject_cast<KJob*>(job));
    VERIFY(numJobsRemoved == 1);

    // Needed by Qt::QueuedConnection
    //qRegisterMetaType<size_t>("size_t");
    VERIFY(QMetaObject::invokeMethod(writeJobHelper, "receiveWritten",
                                       Q_ARG(size_t,
                                             static_cast<size_t>(written)),
                                       Q_ARG(int, job->error())));
    /*connect(this, SIGNAL(sendWritten(const size_t&, const int&)),
            writeJobHelper, SLOT(receiveWritten(const size_t&, const int&)),
            Qt::QueuedConnection);
    emit sendWritten(static_cast<size_t>(written), job->error());*/
}

/*********** MkDir ***********/
void KioFuseApp::mkDirMainThread(const QUrl& url, const mode_t& mode,
                                 MkDirHelper* mkDirHelper)
{
    KIO::SimpleJob* simpleJob = KIO::mkdir(url, mode);
    VERIFY(simpleJob->thread() == this->thread());

    // Correlate SimpleJob with the MkDirHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(simpleJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(simpleJob),
                            qobject_cast<BaseJobHelper*>(mkDirHelper));
    qDebug()<<"mode"<<mode<<url<<url<<endl;
    connect(simpleJob, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));
}

/*********** MkNod ***********/
void KioFuseApp::mkNodMainThread(const QUrl& url, const mode_t& mode,
                                 MkNodHelper* mkNodHelper)
{
    QTemporaryFile* temp = new QTemporaryFile();
    temp->setAutoRemove(false);  // Must remove manually in slotMkNodResult()
                                 // if move fails
    temp->open();

    // FIXME file_move ignores mode, BAD! file_copy seems to work OK.
    KIO::FileCopyJob* fileCopyJob = KIO::file_move(temp->fileName(), url, mode /*33188*/,
                      KIO::Overwrite | KIO::HideProgressInfo);
    VERIFY(fileCopyJob->thread() == this->thread());

    // Correlate temp file with job
    VERIFY(m_jobToTempFile.count(qobject_cast<KJob*>(fileCopyJob)) == 0);
    m_jobToTempFile.insertMulti(qobject_cast<KJob*>(fileCopyJob), temp);

    // Correlate FileCopyJob with the MkNodHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(fileCopyJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(fileCopyJob),
                            qobject_cast<BaseJobHelper*>(mkNodHelper));
    qDebug()<<"mode"<<mode<<"temp.fileName()"<<temp->fileName()<<url<<url<<endl;
    connect(fileCopyJob, SIGNAL(result(KJob*)),
            this, SLOT(slotMkNodResult(KJob*)));
}

void KioFuseApp::slotMkNodResult(KJob* job)
{
    int error = job->error();

    // Delete the temp file if there was an error
    QTemporaryFile* temp = m_jobToTempFile.value(job);
    VERIFY(temp);
    if (error){
        temp->setAutoRemove(true);
    }

    BaseJobHelper* jobHelper = m_jobToJobHelper.value(job);

    // Remove job and temp file from map
    int numTempFilesRemoved = m_jobToTempFile.remove(job);
    VERIFY(numTempFilesRemoved == 1);
    delete(temp);

    // Remove job and jobHelper from map
    int numJobsRemoved = m_jobToJobHelper.remove(job);
    VERIFY(numJobsRemoved == 1);

    VERIFY(job);
    job->kill();
    job = NULL;

    VERIFY(QMetaObject::invokeMethod(jobHelper, "jobDone",
                                       Q_ARG(int, error)));
    /*connect(this, SIGNAL(sendJobDone(const int&)),
            jobHelper, SLOT(jobDone(const int&)), Qt::QueuedConnection);
    emit sendJobDone(error);*/
}

/*********** SymLink ***********/
void KioFuseApp::symLinkMainThread(const QUrl& source, const QUrl& dest,
                                   SymLinkHelper* symLinkHelper)
{
    qDebug()<<"source.path()"<<source.path()<<"dest"<<dest<<endl;
    KIO::SimpleJob* simpleJob = KIO::symlink(source.path(), dest,
                                KIO::HideProgressInfo);
    VERIFY(simpleJob->thread() == this->thread());

    // Correlate SimpleJob with the SymLinkHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(simpleJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(simpleJob),
                            qobject_cast<BaseJobHelper*>(symLinkHelper));

    connect(simpleJob, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));
}

/*********** ReName ***********/
void KioFuseApp::reNameMainThread(const QUrl& source, const QUrl& dest,
                                 ReNameHelper* reNameHelper)
{
    KIO::FileCopyJob* fileCopyJob = KIO::file_move(source, dest, -1,
            KIO::Overwrite | KIO::HideProgressInfo);
    VERIFY(fileCopyJob->thread() == this->thread());

    // Correlate FileCopyJob with the ReNameHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(fileCopyJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(fileCopyJob),
                            qobject_cast<BaseJobHelper*>(reNameHelper));

    connect(fileCopyJob, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));
}

/*********** ChMod ***********/
void KioFuseApp::chModMainThread(const QUrl& url, const mode_t& mode,
                                 ChModHelper* chModHelper)
{
    KIO::SimpleJob* simpleJob = KIO::chmod(url, mode);
    VERIFY(simpleJob->thread() == this->thread());

    // Correlate SimpleJob with the ChModHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(simpleJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(simpleJob),
                            qobject_cast<BaseJobHelper*>(chModHelper));
    qDebug()<<"mode"<<mode<<endl;
    connect(simpleJob, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));
}

/*********** UnLink ***********/
void KioFuseApp::unLinkMainThread(const QUrl& url, UnLinkHelper* unLinkHelper)
{
    KIO::DeleteJob* deleteJob = KIO::del(url, KIO::HideProgressInfo);
    VERIFY(deleteJob->thread() == this->thread());

    // Correlate DeleteJob with the UnLinkHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(deleteJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(deleteJob),
                            qobject_cast<BaseJobHelper*>(unLinkHelper));
    qDebug()<<"url"<<url<<endl;
    connect(deleteJob, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));
}

/*********** Release ***********/
void KioFuseApp::releaseJobMainThread(/*const QUrl& url,*/
                                      const uint64_t& fileHandleId,
                                      const bool& jobIsAnnulled,
                                      ReleaseJobHelper* releaseJobHelper)
{
    int error;
    //QMutexLocker locker(&m_cacheMutex);

    if (fhIdtoFileJobData.contains(fileHandleId)){
        FileJobData* fileJobData = fhIdtoFileJobData.value(fileHandleId);
        //qDebug()<<"fileJobData->fileJob->thread()"<<fileJobData->fileJob->thread()<<"this->thread()"<<this->thread()<<endl;
        //VERIFY(fileJobData->fileJob->thread() == this->thread());

        //FIXME Fuse renames temporary files on the server. This is a FUSE bug.
        /*// The urls of the file being accessed and the opened file must match
        VERIFY(url.path(QUrl::RemoveTrailingSlash) == 
                fileJobData->fileJob->url().path(QUrl::RemoveTrailingSlash));*/

        removeJob(fileHandleId, fileJobData, jobIsAnnulled);
        fileJobData = NULL;
        error = 0;
    } else {
        // FIXME There should be an error KIO::ERR_ALREADY_CLOSED
        error = KIO::ERR_DOES_NOT_EXIST;
    }

    VERIFY(QMetaObject::invokeMethod(releaseJobHelper, "jobDone",
                                       Q_ARG(int, error)));
    /*connect(this, SIGNAL(sendJobDone(const int&)),
            releaseJobHelper, SLOT(jobDone(const int&)), Qt::QueuedConnection);
    emit sendJobDone(error);*/
}

/*********** ChTime ***********/
void KioFuseApp::chTimeMainThread(const QUrl& url, const QDateTime& dt,
                                  ChTimeHelper* chTimeHelper)
{
    KIO::SimpleJob* simpleJob = KIO::setModificationTime(url, dt);
    VERIFY(simpleJob->thread() == this->thread());

    // Correlate SimpleJob with the ChTimeHelper that needs it
    VERIFY(m_jobToJobHelper.count(qobject_cast<KJob*>(simpleJob)) == 0);
    m_jobToJobHelper.insertMulti(qobject_cast<KJob*>(simpleJob),
                            qobject_cast<BaseJobHelper*>(chTimeHelper));

    connect(simpleJob, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));
}

void KioFuseApp::setUpTypes()
{
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<KIO::UDSEntryList>("KIO::UDSEntryList");
    qRegisterMetaType<KIO::UDSEntry>("KIO::UDSEntry");
    qRegisterMetaType<KIO::filesize_t>("KIO::filesize_t");
    qRegisterMetaType<off_t>("off_t");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
    qRegisterMetaType<mode_t>("mode_t");
    qRegisterMetaType<ListJobHelper*>("ListJobHelper*");
    qRegisterMetaType<StatJobHelper*>("StatJobHelper*");
    qRegisterMetaType<OpenJobHelper*>("OpenJobHelper*");
    qRegisterMetaType<ReadJobHelper*>("ReadJobHelper*");
    qRegisterMetaType<WriteJobHelper*>("WriteJobHelper*");
    qRegisterMetaType<MkNodHelper*>("MkNodHelper*");
    qRegisterMetaType<SymLinkHelper*>("SymLinkHelper*");
    qRegisterMetaType<ReNameHelper*>("ReNameHelper*");
    qRegisterMetaType<ChModHelper*>("ChModHelper*");
    qRegisterMetaType<ReleaseJobHelper*>("ReleaseJobHelper*");
    qRegisterMetaType<MkDirHelper*>("MkDirHelper*");
    qRegisterMetaType<UnLinkHelper*>("UnLinkHelper*");
    qRegisterMetaType<ChTimeHelper*>("ChTimeHelper*");
    qRegisterMetaType<LockHelper*>("LockHelper*");
    qRegisterMetaType<QMutex*>("QMutex*");
    qRegisterMetaType<KJob*>("KJob*");
    qRegisterMetaType<KIO::FileJob*>("KIO::FileJob*");
}

/*void KioFuseApp::setUpErrorTranslator()
{
    KIOErrToSysError.insert(KIO::ERR_CANNOT_OPEN_FOR_READING, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_OPEN_FOR_WRITING, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_LAUNCH_PROCESS, EPERM);
    KIOErrToSysError.insert(KIO::ERR_INTERNAL, EPROTO);
    KIOErrToSysError.insert(KIO::ERR_MALFORMED_URL, EBADF);
    KIOErrToSysError.insert(KIO::ERR_UNSUPPORTED_PROTOCOL, ENOPROTOOPT);
    KIOErrToSysError.insert(KIO::ERR_NO_SOURCE_PROTOCOL, ENOPROTOOPT);
    KIOErrToSysError.insert(KIO::ERR_UNSUPPORTED_ACTION, ENOTTY);
    KIOErrToSysError.insert(KIO::ERR_IS_DIRECTORY, EISDIR);
    KIOErrToSysError.insert(KIO::ERR_IS_FILE, EEXIST);
    KIOErrToSysError.insert(KIO::ERR_DOES_NOT_EXIST, ENOENT);
    KIOErrToSysError.insert(KIO::ERR_FILE_ALREADY_EXIST, EEXIST);
    KIOErrToSysError.insert(KIO::ERR_DIR_ALREADY_EXIST, EEXIST);
    KIOErrToSysError.insert(KIO::ERR_UNKNOWN_HOST, EHOSTUNREACH);
    KIOErrToSysError.insert(KIO::ERR_ACCESS_DENIED, EACCES);
    KIOErrToSysError.insert(KIO::ERR_WRITE_ACCESS_DENIED, EACCES);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_ENTER_DIRECTORY, ENOENT);
    KIOErrToSysError.insert(KIO::ERR_PROTOCOL_IS_NOT_A_FILESYSTEM, EPROTOTYPE);
    KIOErrToSysError.insert(KIO::ERR_CYCLIC_LINK, ELOOP);
    KIOErrToSysError.insert(KIO::ERR_USER_CANCELED, ECANCELED);
    KIOErrToSysError.insert(KIO::ERR_CYCLIC_COPY, ELOOP);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_CREATE_SOCKET, ENOTCONN);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_CONNECT, ENOTCONN);
    KIOErrToSysError.insert(KIO::ERR_CONNECTION_BROKEN, ENOTCONN);
    KIOErrToSysError.insert(KIO::ERR_NOT_FILTER_PROTOCOL, EPROTOTYPE);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_MOUNT, EIO);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_READ, EIO);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_WRITE, EIO);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_BIND, EPERM);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_LISTEN, EPERM);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_ACCEPT, EPERM);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_LOGIN, ECONNREFUSED);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_STAT, EIO);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_CLOSEDIR, EIO);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_MKDIR, EIO);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_RMDIR, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_RESUME, ECONNABORTED);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_RENAME, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_CHMOD, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_DELETE, EIO);
    KIOErrToSysError.insert(KIO::ERR_OUT_OF_MEMORY, ENOMEM);
    KIOErrToSysError.insert(KIO::ERR_UNKNOWN_PROXY_HOST, EHOSTUNREACH);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_AUTHENTICATE, EACCES);
    KIOErrToSysError.insert(KIO::ERR_ABORTED, ECONNABORTED);
    KIOErrToSysError.insert(KIO::ERR_INTERNAL_SERVER, EPROTO);
    KIOErrToSysError.insert(KIO::ERR_SERVER_TIMEOUT, ETIMEDOUT);
    KIOErrToSysError.insert(KIO::ERR_SERVICE_NOT_AVAILABLE, ENOPROTOOPT);
    KIOErrToSysError.insert(KIO::ERR_UNKNOWN, ENOENT);
    KIOErrToSysError.insert(KIO::ERR_UNKNOWN_INTERRUPT, ENOENT);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_DELETE_ORIGINAL, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_DELETE_PARTIAL, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_RENAME_ORIGINAL, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_RENAME_PARTIAL, EIO);
    KIOErrToSysError.insert(KIO::ERR_NEED_PASSWD, EACCES);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_SYMLINK, EIO);
    KIOErrToSysError.insert(KIO::ERR_NO_CONTENT, ENODATA);
    KIOErrToSysError.insert(KIO::ERR_DISK_FULL, ENOMEM);
    KIOErrToSysError.insert(KIO::ERR_IDENTICAL_FILES, EEXIST);
    KIOErrToSysError.insert(KIO::ERR_SLAVE_DEFINED, EALREADY);
    KIOErrToSysError.insert(KIO::ERR_UPGRADE_REQUIRED, EPROTOTYPE);
    KIOErrToSysError.insert(KIO::ERR_POST_DENIED, EACCES);
    KIOErrToSysError.insert(KIO::ERR_COULD_NOT_SEEK, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_SETTIME, EIO);
    KIOErrToSysError.insert(KIO::ERR_CANNOT_CHOWN, EIO);
}*/

int KioFuseApp::sysErrFromKioErr(const int& kioErr)
{
    /*QMutexLocker locker(&m_errorTransMutex);
    int sysErr = KIOErrToSysError.value(kioErr);
    if (sysErr){
        return sysErr;
    } else {
        return EIO;
    }*/
    switch (kioErr){
        case KIO::ERR_CANNOT_OPEN_FOR_READING      : return EIO;
        case KIO::ERR_CANNOT_OPEN_FOR_WRITING      : return EIO;
        case KIO::ERR_CANNOT_LAUNCH_PROCESS        : return EPERM;
        case KIO::ERR_INTERNAL                     : return EPROTO;
        case KIO::ERR_MALFORMED_URL                : return EBADF;
        case KIO::ERR_UNSUPPORTED_PROTOCOL         : return ENOPROTOOPT;
        case KIO::ERR_NO_SOURCE_PROTOCOL           : return ENOPROTOOPT;
        case KIO::ERR_UNSUPPORTED_ACTION           : return ENOTTY;
        case KIO::ERR_IS_DIRECTORY                 : return EISDIR;
        case KIO::ERR_IS_FILE                      : return EEXIST;
        case KIO::ERR_DOES_NOT_EXIST               : return ENOENT;
        case KIO::ERR_FILE_ALREADY_EXIST           : return EEXIST;
        case KIO::ERR_DIR_ALREADY_EXIST            : return EEXIST;
        case KIO::ERR_UNKNOWN_HOST                 : return EHOSTUNREACH;
        case KIO::ERR_ACCESS_DENIED                : return EACCES;
        case KIO::ERR_WRITE_ACCESS_DENIED          : return EACCES;
        case KIO::ERR_CANNOT_ENTER_DIRECTORY       : return ENOENT;
        case KIO::ERR_PROTOCOL_IS_NOT_A_FILESYSTEM : return EPROTOTYPE;
        case KIO::ERR_CYCLIC_LINK                  : return ELOOP;
        case KIO::ERR_USER_CANCELED                : return ECANCELED;
        case KIO::ERR_CYCLIC_COPY                  : return ELOOP;
        case KIO::ERR_COULD_NOT_CREATE_SOCKET      : return ENOTCONN;
        case KIO::ERR_COULD_NOT_CONNECT            : return ENOTCONN;
        case KIO::ERR_CONNECTION_BROKEN            : return ENOTCONN;
        case KIO::ERR_NOT_FILTER_PROTOCOL          : return EPROTOTYPE;
        case KIO::ERR_COULD_NOT_MOUNT              : return EIO;
        case KIO::ERR_COULD_NOT_READ               : return EIO;
        case KIO::ERR_COULD_NOT_WRITE              : return EIO;
        case KIO::ERR_COULD_NOT_BIND               : return EPERM;
        case KIO::ERR_COULD_NOT_LISTEN             : return EPERM;
        case KIO::ERR_COULD_NOT_ACCEPT             : return EPERM;
        case KIO::ERR_COULD_NOT_LOGIN              : return ECONNREFUSED;
        case KIO::ERR_COULD_NOT_STAT               : return EIO;
        case KIO::ERR_COULD_NOT_CLOSEDIR           : return EIO;
        case KIO::ERR_COULD_NOT_MKDIR              : return EIO;
        case KIO::ERR_COULD_NOT_RMDIR              : return EIO;
        case KIO::ERR_CANNOT_RESUME                : return ECONNABORTED;
        case KIO::ERR_CANNOT_RENAME                : return EIO;
        case KIO::ERR_CANNOT_CHMOD                 : return EIO;
        case KIO::ERR_CANNOT_DELETE                : return EIO;
        case KIO::ERR_OUT_OF_MEMORY                : return ENOMEM;
        case KIO::ERR_UNKNOWN_PROXY_HOST           : return EHOSTUNREACH;
        case KIO::ERR_COULD_NOT_AUTHENTICATE       : return EACCES;
        case KIO::ERR_ABORTED                      : return ECONNABORTED;
        case KIO::ERR_INTERNAL_SERVER              : return EPROTO;
        case KIO::ERR_SERVER_TIMEOUT               : return ETIMEDOUT;
        case KIO::ERR_SERVICE_NOT_AVAILABLE        : return ENOPROTOOPT;
        case KIO::ERR_UNKNOWN                      : return ENOENT;
        case KIO::ERR_UNKNOWN_INTERRUPT            : return ENOENT;
        case KIO::ERR_CANNOT_DELETE_ORIGINAL       : return EIO;
        case KIO::ERR_CANNOT_DELETE_PARTIAL        : return EIO;
        case KIO::ERR_CANNOT_RENAME_ORIGINAL       : return EIO;
        case KIO::ERR_CANNOT_RENAME_PARTIAL        : return EIO;
        case KIO::ERR_NEED_PASSWD                  : return EACCES;
        case KIO::ERR_CANNOT_SYMLINK               : return EIO;
        case KIO::ERR_NO_CONTENT                   : return ENODATA;
        case KIO::ERR_DISK_FULL                    : return ENOMEM;
        case KIO::ERR_IDENTICAL_FILES              : return EEXIST;
        case KIO::ERR_SLAVE_DEFINED                : return EALREADY;
        case KIO::ERR_UPGRADE_REQUIRED             : return EPROTOTYPE;
        case KIO::ERR_POST_DENIED                  : return EACCES;
        case KIO::ERR_COULD_NOT_SEEK               : return EIO;
        case KIO::ERR_CANNOT_SETTIME               : return EIO;
        case KIO::ERR_CANNOT_CHOWN                 : return EIO;
        default                                    : return EIO;
    }
}

void KioFuseApp::addAnnulledFh(const uint64_t& fh)
{
    QMutexLocker locker(&m_terminatedFhListMutex);
    VERIFY(m_terminatedFhList.count(fh) == 0);
    m_terminatedFhList.append(fh);
}

void KioFuseApp::removeAnnulledFh(const uint64_t& fh)
{
    QMutexLocker locker(&m_terminatedFhListMutex);
    int numFhRemoved = m_terminatedFhList.removeAll(fh);
    VERIFY(numFhRemoved == 1);
}

bool KioFuseApp::isAnnulled(const uint64_t& fh)
{
    QMutexLocker locker(&m_terminatedFhListMutex);
    return m_terminatedFhList.contains(fh);
}

void KioFuseApp::quitGracefully(const char* expression, const char* file,
                    const int& line, const char* function)
{
    qDebug()<<endl<<"A fatal error has caused KioFuse to crash. Please send"\
    <<"the following information to http://bugzilla.kde.org:"<<endl\
    <<"File: "<<file<<endl<<"Function: "<<function<<endl\
    <<"Line: "<<line<<endl<<"Expression: "<<expression;
    //    raise(SIGQUIT);
}
