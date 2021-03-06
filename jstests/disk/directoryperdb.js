var baseDir = "jstests_disk_directoryper";
var baseName = "directoryperdb"
port = allocatePorts( 1 )[ 0 ];
dbpath = "/data/db/" + baseDir + "/";

var m = startMongod( "--directoryperdb", "--port", port, "--dbpath", dbpath, "--nohttpinterface", "--bind_ip", "127.0.0.1" );
db = m.getDB( baseName );
db[ baseName ].save( {} );
assert.eq( 1, db[ baseName ].count() );

checkDir = function( dir ) {
    files = listFiles( dir );
    found = false;
    for( f in files ) {
        if ( new RegExp( baseName ).test( files[ f ].name ) ) {
            found = true;
            assert( files[ f ].isDirectory, "file not directory" );
        }
    }
    assert( found, "no directory" );

    files = listFiles( dir + baseName );
    for( f in files ) {
        assert( new RegExp( baseName + "/" + baseName + "." ).test( files[ f ].name ) );
    }
}
checkDir( dbpath );

// file iterator
assert( m.getDBs().totalSize > 0, "bad size calc" );

// repair
db.runCommand( {repairDatabase:1, backupOriginalFiles:true} );
checkDir( dbpath );
files = listFiles( dbpath );
for( f in files ) {
    if ( new RegExp( "^" + dbpath + "backup_" ).test( files[ f ].name ) ) {
        backupDir = files[ f ].name + "/";
    }
}
checkDir( backupDir );
assert.eq( 1, db[ baseName ].count() );

// tool test
stopMongod( port );

externalPath = "/data/db/" + baseDir + "_external/";

runMongoProgram( "mongodump", "--dbpath", dbpath, "--directoryperdb", "--out", externalPath );
resetDbpath( dbpath );
runMongoProgram( "mongorestore", "--dbpath", dbpath, "--directoryperdb", "--dir", externalPath );
m = startMongoProgram( "mongod", "--directoryperdb", "--port", port, "--dbpath", dbpath, "--nohttpinterface", "--bind_ip", "127.0.0.1" );
db = m.getDB( baseName );
checkDir( dbpath );
assert.eq( 1, db[ baseName ].count() );
assert( m.getDBs().totalSize > 0, "bad size calc" );

// drop db test
db.dropDatabase();
files = listFiles( dbpath );
files.forEach( function( f ) { assert( !new RegExp( baseName ).test( f.name ), "drop database - dir not cleared" ); } );
