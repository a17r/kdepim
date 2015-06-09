/*
 * Copyright 2010 Thomas McGuire <mcguire@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "inserttextfilejob.h"

#include <QTextEdit>

#include <KCharsets>
#include "messagecomposer_debug.h"
#include <KIO/Job>

#include <QTextCodec>

using namespace MessageComposer;

InsertTextFileJob::InsertTextFileJob(QTextEdit *editor, const QUrl &url)
    : KJob(editor), mEditor(editor), mUrl(url)
{
}

InsertTextFileJob::~InsertTextFileJob()
{
}

void InsertTextFileJob::slotFileData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job);
    mFileData += data;
}

void InsertTextFileJob::slotGetJobFinished(KJob *job)
{
    if (job->error()) {
        qCWarning(MESSAGECOMPOSER_LOG) << job->errorString();
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    if (mEditor) {
        if (!mEncoding.isEmpty()) {
            const QTextCodec *fileCodec = KCharsets::charsets()->codecForName(mEncoding);
            if (fileCodec) {
                mEditor->textCursor().insertText(fileCodec->toUnicode(mFileData.data()));
            } else {
                mEditor->textCursor().insertText(QString::fromLocal8Bit(mFileData.data()));
            }
        }
    }

    emitResult();
}

void InsertTextFileJob::setEncoding(const QString &encoding)
{
    mEncoding = encoding;
}

void InsertTextFileJob::start()
{
    KIO::TransferJob *job = KIO::get(mUrl);
    connect(job, &KIO::TransferJob::result, this, &InsertTextFileJob::slotGetJobFinished);
    connect(job, &KIO::TransferJob::data, this, &InsertTextFileJob::slotFileData);
    job->start();
}

