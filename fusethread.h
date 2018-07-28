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

#ifndef FUSE_THREAD_H
#define FUSE_THREAD_H

#define FUSE_USE_VERSION 26
extern "C" {
#include <fuse.h>
}

#include <QThread>
#include <QUrl>

class FuseThread : public QThread
{
    Q_OBJECT

    public:
        FuseThread(QObject *parent, struct fuse *fuseHandle,
                   struct fuse_chan *fuseChannel, QUrl mountPoint);
        void unmount();
        ~FuseThread();

    protected:
        void run();

    private:
        struct fuse *m_fuseHandle;
        struct fuse_chan *m_fuseChannel;
        QUrl m_mountPoint;
        //bool m_alreadyUnmounted;
};

#endif /* FUSE_THREAD_H */
