#ifndef TRACKEXPORTWORKER_H
#define TRACKEXPORTWORKER_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QThread>
#include <future>

#include "trackinfoobject.h"

// A QThread class for copying a list of files to a single destination directory.
// Currently does not preserve subdirectory relationships.  This class performs
// all copies in a blocking style within its own thread.
class TrackExportWorker : public QThread {
    Q_OBJECT
  public:
    enum class OverwriteMode {
        ASK,
        OVERWRITE_ALL,
        SKIP_ALL,
    };

    // Class used to answer the question about what the user would prefer to
    // do in case a file of the same name exists at the destination
    enum class OverwriteAnswer {
        SKIP,
        SKIP_ALL,
        OVERWRITE,
        OVERWRITE_ALL,
        CANCEL = -1,
    };

    // Constructor does not validate the destination directory.  Calling classes
    // should do that.
    TrackExportWorker(QString destDir, QList<TrackPointer> tracks)
            : m_destDir(destDir), m_tracks(tracks) { }
    virtual ~TrackExportWorker() { };

    // exports ALL the tracks.  Thread joins on success or failure.
    void run() override;

    // Calling classes can call errorMessage after a failure for a user-friendly
    // message about what happened.
    QString errorMessage() const {
        return m_errorMessage;
    }

    // Cancels the export after the current copy operation.
    //May be called from another thread.
    void stop();

  signals:
    // Signals and slots necessarily make a copy of the items being passed,
    // so we have to use a bare pointer instead of a smart one. And QT's QFuture
    // is not quite what we want here, so we use the STL's future class.
    // Note that fully qualifying the Answer class name is required for the
    // signal to connect.
    void askOverwriteMode(
            QString filename,
            std::promise<TrackExportWorker::OverwriteAnswer>* promise);
    void progress(QString filename, int progress, int count);
    void canceled();

  private:
    void exportTrack(TrackPointer track);

    // Emit a signal requesting overwrite mode, and block until we get an
    // answer.  Updates m_overwriteMode appropriately.
    OverwriteAnswer makeOverwriteRequest(QString filename);

    QAtomicInt m_bStop = false;
    QString m_errorMessage;

    OverwriteMode m_overwriteMode = OverwriteMode::ASK;
    const QString m_destDir;
    const QList<TrackPointer> m_tracks;
};

#endif  // TRACKEXPORTWORKER_H