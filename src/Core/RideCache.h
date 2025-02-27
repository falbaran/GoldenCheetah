/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_RideCache_h
#define _GC_RideCache_h 1

#include "GoldenCheetah.h"
#include "MainWindow.h"
#include "RideFile.h"
#include "RideItem.h"
#include "PDModel.h"

#include <QVector>
#include <QThread>

#include <QFuture>
#include <QFutureWatcher>
# include <QtConcurrent>

class Context;
class LTMPlot;
class RideCacheRefreshThread;
class Specification;
class AthleteBest;
class RideCacheModel;
class Estimator;
class Banister;

class RideCache : public QObject
{
    Q_OBJECT

    public:

        RideCache(Context *context);
        ~RideCache();

        // table models
        RideCacheModel *model() { return model_; }

        // query the cache
        int count() const { return rides_.count(); }
        RideItem *getRide(QString filename);
        RideItem *getRide(QDateTime dateTime);
	    QList<QDateTime> getAllDates();
        QStringList getAllFilenames();

        // get an aggregate applying the passed spec
        QString getAggregate(QString name, Specification spec, bool useMetricUnits, bool nofmt=false);

        // get top n bests
        QList<AthleteBest> getBests(QString symbol, int n, Specification specification, bool useMetricUnits=true);

        // metadata
        QHash<QString,int> getRankedValues(QString name); // metadata
        QStringList getDistinctValues(QString name); // metadata

        // Count of activities matching specification
        void getRideTypeCounts(Specification specification, int& nActivities,
                               int& nRides, int& nRuns, int& nSwims, QString& sport);
        // Check if metric is relevant for some  activity matching specification
        enum SportRestriction { AnySport, OnlyRides, OnlyRuns, OnlySwims, OnlyXtrains };
        bool isMetricRelevantForRides(Specification specification,
                                      const RideMetric* metric,
                                      SportRestriction sport=AnySport);

        // is running ?
        bool isRunning() { return refreshThreads.count() != 0; }

        // how is update going?
        QMutex updateMutex;
        int updates; // for watching progress
        int nextRefresh(); // returns -1 when all done
        void threadCompleted(RideCacheRefreshThread*);

        // the ride list
	    QVector<RideItem*>&rides() { return rides_; } 

        // add/remove a ride to the list
        void addRide(QString name, bool dosignal, bool select, bool useTempActivities, bool planned);
        void removeCurrentRide();

        // export metrics in CSV format
        void writeAsCSV(QString filename);

        // the background refresher !
        void refresh();
        double progress() { return progress_; }

    public slots:

        // restore / dump cache to disk (json)
        void load();
        void postLoad();
        void save(bool opendata=false, QString filename="");

        // find entry quickly
        int find(RideItem *);

        // user updated options/preferences
        void configChanged(qint32);

        // background refresh progress update
        void progressing(int);

        // cancel background processing because about to exit
        void cancel();

        // item telling us it changed
        void itemChanged();

        // clear deleted objects
        void garbageCollect();

        // first run to initialise estimates
        void initEstimates();

    signals:

        void modelProgress(int, int); // let others know when we're refreshing the model estimates
        void loadComplete(); // when loading the cache completes...

        // us telling the world the item changed
        void itemChanged(RideItem*);

    protected:

        friend class ::Athlete;
        friend class ::MainWindow; // save dialog
        friend class ::LTMPlot; // get weekly performances
        friend class ::Banister; // get weekly performances
        friend class ::Leaf; // get weekly performances
        friend class ::RideItem; // adds to deletelist in destructor
        friend class ::NavigationModel; // checks deletelist during redo/undo
        friend class ::RideCacheRefreshThread;

        Context *context;
        QDir directory, plannedDirectory;

        // rides and reverse are the main lists
        // delete_ is a list of items to garbage collect (delete later)
        // deletelist is a list of items that no longer exist (deleted)
        QVector<RideItem*> rides_, reverse_, delete_, deletelist;
        RideCacheModel *model_;
        bool exiting;
	    double progress_; // percent

        QVector<RideCacheRefreshThread*> refreshThreads;

        Estimator *estimator;
        bool first; // updated when estimates are marked stale
};

class AthleteBest
{
    public:
    double nvalue;
    QString value; // formatted value
    QDate date;

    // for std::sort
    bool operator< (AthleteBest right) const { return (nvalue < right.nvalue); }
};

class RideCacheRefreshThread : public QThread
{
    public:
        RideCacheRefreshThread(RideCache *cache) : cache(cache) {}

    protected:

        // refresh metrics
        virtual void run() override;

    private:
        RideCache *cache;
};

#endif // _GC_RideCache_h
