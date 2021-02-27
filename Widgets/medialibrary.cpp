/* This file is path of the Caligo multimedia player
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "medialibrary.h"

MediaLibrary::MediaLibrary(Playlist *pls, QWidget *parent) : QWidget(parent)
{
    setStyleSheet("QPushButton {border: 1px solid black;}");

    playlist = pls;
    lastClicked = nullptr;
    scanPath = "/home/alex/tmp/";

    mainLayout = new QVBoxLayout;

    QVBoxLayout *l = new QVBoxLayout;

    scanButton = new QPushButton("");
    scanButton->setIcon(QIcon(":/img/scan"));
    scanButton->setToolTip(tr("Scan media library"));
    connect(scanButton, &QPushButton::clicked, this, &MediaLibrary::scanLibrary);

    QHBoxLayout *controls = new QHBoxLayout;
    controls->addStretch(1);
    controls->addWidget(scanButton, 0);

    l->addLayout(controls, 0);

    progress = new QProgressBar;
    progress->setRange(0, 100);
    progress->setValue(0);

    status = new QLabel("");

    statusWidget = new QWidget;
    QVBoxLayout *statl = new QVBoxLayout;
    statl->addWidget(status, 0);
    statl->addWidget(progress, 0);
    statusWidget->setLayout(statl);
    statusWidget->setVisible(false);

    l->addWidget(statusWidget, 0);

    QScrollArea *sa = new QScrollArea;
    QWidget *w = new QWidget;
    QVBoxLayout *sal = new QVBoxLayout;
    sal->addLayout(mainLayout, 0);
    sal->addStretch(1);
    sal->setSpacing(0);
    sal->setContentsMargins(0, 0, 0, 0);
    w->setLayout(sal);
    sa->setWidget(w);
    sa->setWidgetResizable(true);

    l->addWidget(sa, 1);
    setLayout(l);

    scaner = new LibraryMetaScaner(&list, progress);
    connect(scaner, &LibraryMetaScaner::done, this, &MediaLibrary::scanerDone);

    rewriteLibrary = false;
    libraryFilePath = qApp->applicationDirPath() + QDir::separator() + "library" + qApp->applicationName();

    load();
}

void MediaLibrary::done()
{
    statusWidget->setVisible(false);
    status->setText("");
}

void MediaLibrary::setProgress(int v)
{
    progress->setValue(v);
}

void MediaLibrary::setStatus(Status s)
{
    if (s == Done) {
        status->setText("Done");
        progress->setValue(100);
        QTimer::singleShot(3000, this, &MediaLibrary::done);
        return;
    }

    statusWidget->setVisible(true);
    if (s == Paths) {
        status->setText(tr("Scanning media files..."));
    }
    else if (s == Meta) {
        status->setText(tr("Parsing metadata..."));
    }
}

void MediaLibrary::keyPressEvent(QKeyEvent *e)
{
    // Key Del
    // Remove selected element from library
    if (e->key() == Qt::Key::Key_Delete) {
        if (lastClicked == nullptr)
            return;

        lastClicked = nullptr;

        if (selection.count() == list.count()) {
            clear();
            return;
        }

        foreach (LibraryElement* e, selection) {
            list.removeAll(e);
            mainLayout->removeWidget(e);
            e->deleteLater();
        }
        selection.clear();
        rewriteLibrary = true;
    }
    // Key Return
    // Add selected to playlist
    else if (e->key() == Qt::Key::Key_Return) {
        if (lastClicked == nullptr)
            return;
        if (selection.isEmpty())
            return;

        foreach (LibraryElement *e, selection) {
            playlist->add(e->getPath(), e->getName());
        }
    }
}

QString MediaLibrary::getLibraryPath() const
{
    return scanPath;
}

void MediaLibrary::setLibraryPath(QString path)
{
    scanPath = path;
}

void MediaLibrary::add(QString &path)
{
    LibraryElement *e = new LibraryElement(path);

    list << e;
    mainLayout->addWidget(e, 0);

    connect(e, &LibraryElement::clicked, this, &MediaLibrary::elementClicked);
    connect(e, &LibraryElement::doubleClicked, this, &MediaLibrary::elementDouble);
    connect(e, &LibraryElement::ctrlClicked, this, &MediaLibrary::elementCtrl);
    connect(e, &LibraryElement::shiftClicked, this, &MediaLibrary::elementShift);
    connect(e, &LibraryElement::focus, this, &MediaLibrary::elementFocus);
}

void MediaLibrary::remove(LibraryElement *e)
{
    list.removeAll(e);
    mainLayout->removeWidget(e);
    e->~LibraryElement();

    rewriteLibrary = true;
}

void MediaLibrary::clear()
{
    selection.clear();
    lastClicked = nullptr;

    foreach (LibraryElement *e, list) {
        mainLayout->removeWidget(e);
        list.removeAll(e);
        e->~LibraryElement();
    }
}

void MediaLibrary::scanDir(QDir d)
{
    QApplication::processEvents();

    QStringList fl = d.entryList(QDir::Files);
    foreach (QString file, fl) {
        files << d.absoluteFilePath(file);
    }

    QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QString dir, dirs) {
        scanDir(QDir(d.absoluteFilePath(dir)));
    }
}

void MediaLibrary::save()
{
    if (not rewriteLibrary)
        return;

    QFile file(libraryFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QStringList li;
        foreach (LibraryElement *e, list) {
            li << e->getString();
        }

        QTextStream stream(&file);
        stream << "# Caligo media library list :: Do not edit this file by yourself!\n";
        foreach (QString line, li) {
            stream << line + "\n";
        }

        file.close();
    }
}

void MediaLibrary::load()
{
    QFile file(libraryFilePath);
    if (not file.exists())
        return;

    QStringList l;
    QString line;
    if (file.open(QIODevice::ReadOnly)) {
        while (not file.atEnd()) {
            line = file.readLine();
            if (line.startsWith("#") or line.isEmpty()) {
                continue;
            }
            l << line;
        }

        file.close();
    }

    foreach (QString str, l) {
        QStringList tmpList = str.split(" ## ");
        if (QFileInfo::exists(tmpList[0])) {
            add(tmpList[0]);
            if (tmpList.count() > 1) {
                tmpList[1].remove("\n");
                list.back()->setText(tmpList[1]);
            }
        }
    }
}

void MediaLibrary::elementClicked()
{
    selection.clear();
    lastClicked = (LibraryElement*) sender();
    selection << lastClicked;
    foreach (LibraryElement* e, list) {
        e->setSelected(false);
    }
    lastClicked->setSelected(true);
}

void MediaLibrary::elementDouble()
{
    LibraryElement *e = (LibraryElement*) sender();
    playlist->add(e->getPath(), e->getName());

    lastClicked = e;
}

void MediaLibrary::elementShift()
{
    if (list.count() < 2)
        return;

    LibraryElement *from = lastClicked;
    if (from == nullptr) {
        from = lastClicked;
        if (from == nullptr) {
            return;
        }
    }

    lastClicked = (LibraryElement*) sender();

    int iFrom = list.indexOf(from);
    int iTo = list.indexOf(lastClicked);

    if (iFrom == iTo)
        return;

    // XOR swap
    if (iFrom > iTo) {
        iFrom ^= iTo;
        iTo ^= iFrom;
        iFrom ^= iTo;
    }

    for (int i = iFrom; i <= iTo; ++i) {
        if (not selection.contains(list[i])) {
            selection << list[i];
            list[i]->setSelected(true);
        }
    }
}

void MediaLibrary::elementCtrl()
{
    lastClicked = (LibraryElement*) sender();
    if (selection.contains(lastClicked)) {
        lastClicked->setSelected(false);
        selection.removeAll(lastClicked);
        return;
    }
    else {
        selection << lastClicked;
        lastClicked->setSelected(true);
    }
}

void MediaLibrary::elementFocus()
{
    this->setFocus();
}

void MediaLibrary::scanLibrary()
{
    if (scanPath.isEmpty()) {
        setScanPath();
    }
    if (scanPath.isEmpty()) {
        return;
    }

    rewriteLibrary = true;
    setStatus(Paths);

    files.clear();
    clear();

    scanDir(QDir(scanPath));

    if (not files.isEmpty()) {
        QString c = "";
        foreach (QString s, files) {
            if (s.endsWith(".m3u")) {
                files.removeAll(s);
                continue;
            }
            c = QMimeDatabase().mimeTypeForFile(s).name();
            if (not c.contains("audio/") and not c.contains("video/")) {
                files.removeAll(s);
            }
            else {
                add(s);
            }
        }
        files.clear();
    }

    setStatus(Meta);

    scaner->scan();
}

void MediaLibrary::setScanPath()
{
    QString str = QFileDialog::getExistingDirectory(0, tr("Select directory") + " # " + qApp->applicationName(), QDir::home().absolutePath());
    if (str.isEmpty())
        return;
    scanPath = str;
}

void MediaLibrary::scanerDone()
{
    setStatus(Done);
    save();
}
