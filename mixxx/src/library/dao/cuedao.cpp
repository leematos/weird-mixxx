// cuedao.cpp
// Created 10/26/2009 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>
#include <QtSql>
#include <QVariant>

#include "library/dao/cuedao.h"
#include "library/dao/cue.h"
#include "trackinfoobject.h"

CueDAO::CueDAO(QSqlDatabase& database)
        : m_database(database) {
}

CueDAO::~CueDAO() {

}

void CueDAO::initialize() {
    qDebug() << "CueDAO::initialize" << QThread::currentThread() << m_database.connectionName();
}

int CueDAO::cueCount() {
    qDebug() << "CueDAO::cueCount" << QThread::currentThread() << m_database.connectionName();
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM " CUE_TABLE);
    if (query.exec()) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    } else {
        qDebug() << query.lastError();
    }
    //query.finish();
    return 0;
}

int CueDAO::numCuesForTrack(int trackId) {
    qDebug() << "CueDAO::numCuesForTrack" << QThread::currentThread() << m_database.connectionName();
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) FROM " CUE_TABLE " WHERE track_id = :id");
    query.bindValue(":id", trackId);
    if (query.exec()) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    } else {
        qDebug() << query.lastError();
    }
    return 0;
}

Cue* CueDAO::cueFromRow(QSqlQuery& query) const {
    QSqlRecord record = query.record();
    int id = record.value(record.indexOf("id")).toInt();
    int trackId = record.value(record.indexOf("track_id")).toInt();
    int type = record.value(record.indexOf("type")).toInt();
    int position = record.value(record.indexOf("position")).toInt();
    int length = record.value(record.indexOf("length")).toInt();
    int hotcue = record.value(record.indexOf("hotcue")).toInt();
    QString label = record.value(record.indexOf("label")).toString();
    Cue* cue = new Cue(id, trackId, (Cue::CueType)type,
                       position, length, hotcue, label);
    m_cues[id] = cue;
    return cue;
}

Cue* CueDAO::getCue(int cueId) {
    qDebug() << "CueDAO::getCue" << QThread::currentThread() << m_database.connectionName();
    if (m_cues.contains(cueId)) {
        return m_cues[cueId];
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM " CUE_TABLE " WHERE id = :id");
    query.bindValue(":id", cueId);
    if (query.exec()) {
        if (query.next()) {
            return cueFromRow(query);
        }
    } else {
        qDebug() << query.lastError();
    }
    return NULL;
}

QList<Cue*> CueDAO::getCuesForTrack(int trackId) const {
    //qDebug() << "CueDAO::getCuesForTrack" << QThread::currentThread() << m_database.connectionName();
    QList<Cue*> cues;
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM " CUE_TABLE " WHERE track_id = :id");
    query.bindValue(":id", trackId);
    if (query.exec()) {
        while (query.next()) {
            Cue* cue = NULL;
            int cueId = query.value(query.record().indexOf("id")).toInt();
            if (m_cues.contains(cueId)) {
                cue = m_cues[cueId];
            }
            if (cue == NULL) {
                cue = cueFromRow(query);
            }
            if (cue != NULL) {
                cues.push_back(cue);
            }
        }
    } else {
        qDebug() << query.lastError();
    }
    return cues;
}

bool CueDAO::deleteCuesForTrack(int trackId) {
    qDebug() << "CueDAO::deleteCuesForTrack" << QThread::currentThread() << m_database.connectionName();
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM " CUE_TABLE " WHERE track_id = :track_id");
    query.bindValue(":track_id", trackId);
    if (query.exec()) {
        return true;
    } else {
        qDebug() << query.lastError();
    }
    return false;
}

bool CueDAO::saveCue(Cue* cue) {
    qDebug() << "CueDAO::saveCue" << QThread::currentThread() << m_database.connectionName();
    Q_ASSERT(cue);
    if (cue->getId() == -1) {
        //Start the transaction
        Q_ASSERT(m_database.transaction());

        // New cue
        QSqlQuery query(m_database);
        query.prepare("INSERT INTO " CUE_TABLE " (track_id, type, position, length, hotcue, label) VALUES (:track_id, :type, :position, :length, :hotcue, :label)");
        query.bindValue(":track_id", cue->getTrackId());
        query.bindValue(":type", cue->getType());
        query.bindValue(":position", cue->getPosition());
        query.bindValue(":length", cue->getLength());
        query.bindValue(":hotcue", cue->getHotCue());
        query.bindValue(":label", cue->getLabel());

        if (query.exec()) {
            int id = query.lastInsertId().toInt();
            cue->setId(id);
            cue->setDirty(false);
            Q_ASSERT(m_database.commit());
            return true;
        }
        qDebug() << query.executedQuery() << query.lastError();
        m_database.rollback();
    } else {
        // Update cue
        QSqlQuery query(m_database);
        query.prepare("UPDATE " CUE_TABLE " SET "
                        "track_id = :track_id,"
                        "type = :type,"
                        "position = :position,"
                        "length = :length,"
                        "hotcue = :hotcue,"
                        "label = :label"
                        " WHERE id = :id");
        query.bindValue(":id", cue->getId());
        query.bindValue(":track_id", cue->getTrackId());
        query.bindValue(":type", cue->getType());
        query.bindValue(":position", cue->getPosition());
        query.bindValue(":length", cue->getLength());
        query.bindValue(":hotcue", cue->getHotCue());
        query.bindValue(":label", cue->getLabel());

        if (query.exec()) {
            cue->setDirty(false);
            return true;
        } else {
            qDebug() << query.executedQuery() << query.lastError();
        }
    }
    return false;
}

bool CueDAO::deleteCue(Cue* cue) {
    qDebug() << "CueDAO::deleteCue" << QThread::currentThread() << m_database.connectionName();
    if (cue->getId() != -1) {
        QSqlQuery query(m_database);
        query.prepare("DELETE FROM " CUE_TABLE " WHERE id = :id");
        query.bindValue(":id", cue->getId());
        if (query.exec()) {
            return true;
        } else {
            qDebug() << query.lastError();
        }
    } else {
        return true;
    }
    return false;
}

void CueDAO::saveTrackCues(int trackId, TrackInfoObject* pTrack) {
    //qDebug() << "CueDAO::saveTrackCues" << QThread::currentThread() << m_database.connectionName();
    // TODO(XXX) transaction, but people who are already in a transaction call
    // this.
    const QList<Cue*>& cueList = pTrack->getCuePoints();
    const QList<Cue*>& oldCueList = getCuesForTrack(trackId);

    // qDebug() << "CueDAO::saveTrackCues old size:" << oldCueList.size()
    //          << "new size:" << cueList.size();

    QListIterator<Cue*> oldCues(oldCueList);
    QSet<int> oldIds;

    // Build set of old cue ids
    while(oldCues.hasNext()) {
        Cue* cue = oldCues.next();
        oldIds.insert(cue->getId());
    }

    // For each id still in the TIO, save or delete it.
    QListIterator<Cue*> cueIt(cueList);
    while (cueIt.hasNext()) {
        Cue* cue = cueIt.next();
        if (cue->getId() == -1) {
            qDebug() << "Saving new cue";
            // New cue
            cue->setTrackId(trackId);
            saveCue(cue);
        } else {
            oldIds.remove(cue->getId());
            qDebug() << "Updating cue" << cue->getId();
            // Update cue
            saveCue(cue);
        }
    }

    cueIt = QListIterator<Cue*>(oldCueList);
    while (cueIt.hasNext()) {
        Cue* cue = cueIt.next();
        // If the cue's id is still in the oldIds set, then it was not a member
        // of the new set of Cues, so the cue should be deleted.
        if (oldIds.contains(cue->getId())) {
            qDebug() << "Deleting cue" << cue->getId();
            deleteCue(cue);
        }
    }
}