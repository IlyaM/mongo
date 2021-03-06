// top.cpp

#include "stdafx.h"
#include "top.h"
#include "../../util/message.h"
#include "../commands.h"

namespace mongo {
    
    Top::UsageData::UsageData( UsageData& older , UsageData& newer )
        : time(newer.time-older.time) , 
          count(newer.count-older.count) 
    {
        
    }

    Top::CollectionData::CollectionData( CollectionData& older , CollectionData& newer )
        : total( older.total , newer.total ) , 
          readLock( older.readLock , newer.readLock ) ,
          writeLock( older.writeLock , newer.writeLock ) ,
          queries( older.queries , newer.queries ) ,
          getmore( older.getmore , newer.getmore ) ,
          insert( older.insert , newer.insert ) ,
          update( older.update , newer.update ) ,
          remove( older.remove , newer.remove )
    {
        
    }


    void Top::record( const string& ns , int op , int lockType , long long micros ){
        boostlock lk(_lock);

        CollectionData& coll = _usage[ns];
        _record( coll , op , lockType , micros );
        _record( _global , op , lockType , micros );
    }
    
    void Top::_record( CollectionData& c , int op , int lockType , long long micros ){
        c.total.inc( micros );
        
        if ( lockType > 0 )
            c.writeLock.inc( micros );
        else if ( lockType < 0 )
            c.readLock.inc( micros );
        
        switch ( op ){
        case 0:
            // use 0 for unknown, non-specific
            break;
        case dbUpdate:
            c.update.inc( micros );
            break;
        case dbInsert:
            c.insert.inc( micros );
            break;
        case dbQuery:
            c.queries.inc( micros );
            break;
        case dbGetMore:
            c.getmore.inc( micros );
            break;
        case dbDelete:
            c.remove.inc( micros );
            break;
        case opReply: 
        case dbMsg:
        case dbKillCursors:
            log() << "unexpected op in Top::record: " << op << endl;
            break;
        default:
            log() << "unknown op in Top::record: " << op << endl;
        }

    }

    Top::UsageMap Top::cloneMap(){
        boostlock lk(_lock);
        UsageMap x = _usage;
        return x;
    }

    void Top::append( BSONObjBuilder& b ){
        boostlock lk( _lock );
        append( b , _usage );
    }

    void Top::append( BSONObjBuilder& b , const char * name , const UsageData& map ){
        BSONObjBuilder bb( b.subobjStart( name ) );
        bb.appendIntOrLL( "time" , map.time );
        bb.appendIntOrLL( "count" , map.count );
        bb.done();
    }

    void Top::append( BSONObjBuilder& b , const UsageMap& map ){
        for ( UsageMap::const_iterator i=map.begin(); i!=map.end(); i++ ){
            BSONObjBuilder bb( b.subobjStart( i->first.c_str() ) );
            
            const CollectionData& coll = i->second;
            
            append( b , "total" , coll.total );
            
            append( b , "readLock" , coll.readLock );
            append( b , "writeLock" , coll.writeLock );

            append( b , "queries" , coll.queries );
            append( b , "getmore" , coll.getmore );
            append( b , "insert" , coll.insert );
            append( b , "update" , coll.update );
            append( b , "remove" , coll.remove );

            bb.done();
        }
    }

    class TopCmd : public Command {
    public:
        TopCmd() : Command( "top" ){}

        virtual bool slaveOk(){ return true; }
        virtual bool readOnly(){ return true; }
        virtual bool adminOnly(){ return true; }
        virtual void help( stringstream& help ) const { help << "usage by collection"; }

        virtual bool run(const char *ns, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl){
            {
                BSONObjBuilder b( result.subobjStart( "totals" ) );
                Top::global.append( b );
                b.done();
            }
            return true;
        }
        
    } topCmd;

    Top Top::global;
    
    TopOld::T TopOld::_snapshotStart = TopOld::currentTime();
    TopOld::D TopOld::_snapshotDuration;
    TopOld::UsageMap TopOld::_totalUsage;
    TopOld::UsageMap TopOld::_snapshotA;
    TopOld::UsageMap TopOld::_snapshotB;
    TopOld::UsageMap &TopOld::_snapshot = TopOld::_snapshotA;
    TopOld::UsageMap &TopOld::_nextSnapshot = TopOld::_snapshotB;
    boost::mutex TopOld::topMutex;


}
