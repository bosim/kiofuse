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
 
#include "fusethread.h"

#include <signal.h>
//#include "kiofuseapp.h"

#include <QString>

#include <QDebug>

FuseThread::FuseThread(QObject *parent, struct fuse *fuseHandle,
                       struct fuse_chan *fuseChannel, QUrl mountPoint)
    : QThread(parent),
      m_fuseHandle(fuseHandle),
      m_fuseChannel(fuseChannel),
      m_mountPoint(mountPoint)
      //m_alreadyUnmounted(false)
{
    qDebug()<<"QThread::currentThread()"<<QThread::currentThread()<<endl;
}

void FuseThread::run()
{
    qDebug()<<"QThread::currentThread()"<<QThread::currentThread()<<endl;

    // Give FUSE the control. It will call functions in ops as they are requested by users of the FS.
    // Since fuse_loop_mt() is used instead of fuse_loop(), every call to the ops will be made in a new thread
    fuse_loop_mt(m_fuseHandle);
    //fuse_loop(m_fuseHandle);
    qDebug() << "FUSE loop has stopped!" << endl;
    // FUSE has quit its event loop

    // Takes us to exitHandler()
    raise(SIGQUIT);
    /*if (!m_alreadyUnmounted){
        fuse_unmount(m_mountPoint.path().toLatin1(), m_fuseChannel);

        // Tell the main thread to exit
        QMetaObject::invokeMethod(kioFuseApp, "quit");
    }*/
}

void FuseThread::unmount()
{
    //m_alreadyUnmounted = true;

    // This will only cause fuse_loop_mt() to return in FuseThread::run()
    // if the mounpoint is not currently in use.
    fuse_unmount(m_mountPoint.path().toLatin1(), m_fuseChannel);
}

FuseThread::~FuseThread()
{
    m_fuseHandle = NULL;
    m_fuseChannel = NULL;
}

