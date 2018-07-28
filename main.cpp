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
#include "kiofuseapp.h"
#include "fusethread.h"

#include <signal.h>

// Needed for QDir.exists() when checking that mountPoint is a directory
#include <QDir>

#include <KAboutData>
#include <QCommandLineParser>
#include <QDebug>
#include <KLocalizedString>

// This pointer to the fuse thread is used by main() and exitHandler()
FuseThread* fuseThread = NULL;


#if 0
static const KAboutData aboutData("kiofuse",
                        NULL,
                        ki18n("KioFuse").toString(),
                        "0.1",
                        ki18n("Expose KIO filesystems to all POSIX-compliant applications").toString,
//                        KAboutData::License_GPL_V3, 
			KAboutLicense::LGPL,
                        ki18n("(c) 2007-2015 The KioFuse Authors"),
                        QString(),
                        "https://techbase.kde.org/Projects/KioFuse",
                        "submit@bugs.kde.org");
#endif

static void exitHandler(int)
{
    qDebug() << "Exit handler called!";
    if (fuseThread != NULL)
    {
        qDebug()<<"exit";
        fuseThread->unmount();
        //kdDebug()<<"Quitting kioFuseApp";
        exit(0);
        //QMetaObject::invokeMethod(kioFuseApp, "aboutToQuit");
        //QMetaObject::invokeMethod(kioFuseApp, "quit");
    }
}


static void set_signal_handlers()
{
    struct sigaction sa;

    sa.sa_handler = exitHandler;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) == -1
        || sigaction(SIGINT, &sa, NULL) == -1
        || sigaction(SIGQUIT, &sa, NULL) == -1
        || sigaction(SIGTERM, &sa, NULL) == -1)
    {
        qDebug()<<"Cannot set exit signal handlers.";
        exit(1);
    }

    sa.sa_handler = SIG_IGN;

    if (sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        qDebug()<<"Cannot set ignored signals.";
        exit(1);
    }
}


// Initializes mountPoint and baseUrl as specified on the command line
// Returns false if needed argument is not found
bool prepareArguments(QCommandLineParser const& args, QUrl &mountPoint, QUrl &baseUrl)
{
    if (args.isSet("mountpoint")){
        qDebug() << args.value("mountpoint") << endl;
        if (QDir(mountPoint.path()).exists()){
            mountPoint = QUrl(args.value("mountpoint"));
        }
        else{
            qDebug() <<"The specified mountpoint is not valid"<<endl;
            return false;
        }
    }
    else{
        qDebug() <<"Please specify the mountpoint"<<endl;
        return false;
    }

    if (args.isSet("URL")){
        baseUrl = QUrl(args.value("URL"));
    }
    else{
        qDebug() <<"Please specify the URL of the remote resource"<<endl;
        return false;
    }

    return true;
}


int main (int argc, char *argv[])
{
    // Holds persistent info (ie. the FS cache)
    kioFuseApp = new KioFuseApp(argc, argv);
    qDebug()<<"kioFuseApp->thread()"<<kioFuseApp->thread()<<endl;

    // Enable QMetaObject::invokeMethod to work with unusual types like off_t
    kioFuseApp->setUpTypes();

    // KDE initialization
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption("mountpoint", ki18n("Where to place the remote files within the root hierarchy").toString(), "mountpoint"));
    parser.addOption(QCommandLineOption("URL", ki18n("The URL of the remote resource").toString(), "URL"));

    QUrl mountPoint;  // Local path where files will appear
    QUrl baseUrl;  // Remote location of the resource
    qDebug() << kioFuseApp->arguments().size() << endl;
    parser.process(kioFuseApp->arguments());

    // Initialize mountPoint and baseUrl as specified on the commandline
    if (!prepareArguments(parser, mountPoint, baseUrl)){
        // Quit program if a needed argument is not provided by the user
        exit(1);
    }

    kioFuseApp->setBaseUrl(baseUrl);
    kioFuseApp->setMountPoint(mountPoint);
    
    // FUSE variables
    struct fuse_operations ops;
    struct fuse_args fuseArguments = FUSE_ARGS_INIT(0, NULL);
    struct fuse_chan *fuseChannel = NULL;
    struct fuse *fuseHandle = NULL;

    // Tell FUSE where the local mountpoint is
    qDebug() << "mount point" << mountPoint.path().toLatin1() << endl;
    fuseChannel = fuse_mount(mountPoint.path().toLatin1(), &fuseArguments);
    if (fuseChannel == NULL){
        qDebug()<<"fuse_mount() failed"<<endl;
        exit(1);
    }

    // Connect the FS operations used by FUSE to their respective KioFuse implementations
    memset(&ops, 0, sizeof(ops));
    ops.getattr = kioFuseGetAttr;
    ops.readlink = kioFuseReadLink;
    ops.mkdir = kioFuseMkDir;
    ops.unlink = kioFuseUnLink;
    ops.rmdir = kioFuseRmDir;
    ops.mknod = kioFuseMkNod;
    ops.symlink = kioFuseSymLink;
    ops.rename = kioFuseReName;
    ops.chmod = kioFuseChMod;
    ops.open = kioFuseOpen;
    ops.read = kioFuseRead;
    ops.write = kioFuseWrite;
    ops.readdir = kioFuseReadDir;
    ops.release = kioFuseRelease;
    ops.truncate = kioFuseTruncate;
    //ops.access = kioFuseAccess;
// TODO #ifdef HAVE_UTIMENSAT
    ops.utimens = kioFuseUTimeNS;
// TODO #endif // HAVE_UTIMENSAT
    ops.link = kioFuseLink;
    ops.chown = kioFuseChOwn;
    ops.statfs = kioFuseStatFs;
    ops.fsync = kioFuseFSync;
// TODO #ifdef HAVE_POSIX_FALLOCATE
    ops.fallocate = kioFuseFAllocate;
// TODO #endif // HAVE_POSIX_FALLOCATE
// TODO #ifdef HAVE_SETXATTR
    ops.setxattr = kioFuseSetXAttr,
    ops.getxattr = kioFuseGetXAttr,
    ops.listxattr = kioFuseListXAttr,
    ops.removexattr = kioFuseRemoveXAttr,
// TODO #endif // HAVE_SETXATTR

    // Tell FUSE about the KioFuse implementations of FS operations
    fuseHandle = fuse_new(fuseChannel, &fuseArguments, &ops, sizeof(ops), NULL);
    if (fuseHandle == NULL){
        qDebug()<<"fuse_new() failed"<<endl;
        exit(1);
    }

    // Translate between KIO::Error and errno.h
    //kioFuseApp->setUpErrorTranslator();

    // Start FUSE's event loop in a separate thread
    fuseThread = new FuseThread(NULL, fuseHandle, fuseChannel, mountPoint);
    fuseThread->start();

    set_signal_handlers();

    // An event loop needs to run in the main thread so that
    // we can connect to slots in the main thread using Qt::QueuedConnection
    while(true)
        qDebug() << "exec exit" << kioFuseApp->exec();

    // kioFuseApp has quit its event loop.
    // Execution will never reach here because exitHandler calls exit(0),
    // but we try to wrap up things nicely nonetheless.

    // Use terminate() rather than quit() because the fuse_loop_mt() that is
    // running in the thread may not yet have returned, so we have to force it.
    // Also, calling quit() and then deleting the thread causes crashes.
    fuseThread->terminate();

    delete fuseThread;
    fuseThread = NULL;

    delete kioFuseApp;
    kioFuseApp = NULL;

    return 0;
}
