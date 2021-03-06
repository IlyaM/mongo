// snapshots.h

/**
*    Copyright (C) 2008 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "../../stdafx.h"
#include "../jsobj.h"
#include "top.h"
#include "../../util/background.h"

/**
   handles snapshotting performance metrics and other such things
 */
namespace mongo {

    class SnapshotThread;
    
    /**
     * stores a point in time snapshot
     * i.e. all counters at a given time
     */
    class SnapshotData {
        SnapshotData();

        unsigned long long _created;
        Top::CollectionData _globalUsage;
        unsigned long long _totalWriteLockedTime; // micros of total time locked
        Top::UsageMap _usage;

        friend class SnapshotThread;
        friend class SnapshotDelta;
    };
    
    /**
     * contains performance information for a time period
     */
    class SnapshotDelta {
    public:
        SnapshotDelta( SnapshotData * older , SnapshotData * newer );
        
        unsigned long long start() const {
            return _older->_created;
        }

        unsigned long long elapsed() const {
            return _elapsed;
        }
        
        unsigned long long timeInWriteLock() const {
            return _newer->_totalWriteLockedTime - _older->_totalWriteLockedTime;
        }
        double percentWriteLocked() const {
            double e = (double) elapsed();
            double w = (double) timeInWriteLock();
            return w/e;
        }

        Top::CollectionData globalUsageDiff();
        Top::UsageMap collectionUsageDiff();

    private:
        SnapshotData * _older;
        SnapshotData * _newer;

        unsigned long long _elapsed;
    };

    class Snapshots {
    public:
        Snapshots();
        
        void add( SnapshotData * s );
        
        int numDeltas() const { return _stored-1; }

        SnapshotData* getPrev( int numBack = 0 );
        auto_ptr<SnapshotDelta> computeDelta( int numBack = 0 );
        
        
        void outputLockInfoHTML( stringstream& ss );
    private:
        boost::mutex _lock;
        int _n;
        SnapshotData** _snapshots;
        int _loc;
        int _stored;
    };

    class SnapshotThread : public BackgroundJob {
    public:
        void run();
    };
    
    extern Snapshots statsSnapshots;
    extern SnapshotThread snapshotThread;


}
