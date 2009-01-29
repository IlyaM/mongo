// Test resync command

var baseName = "jstests_repl2test";

// spec small oplog to make slave get out of sync
m = startMongod( "--port", "27018", "--dbpath", "/data/db/" + baseName + "-master", "--master", "--oplogSize", "1" );
s = startMongod( "--port", "27019", "--dbpath", "/data/db/" + baseName + "-slave", "--slave", "--source", "127.0.0.1:27018" );

am = m.getDB( baseName ).a
as = s.getDB( baseName ).a

am.save( { _id: new ObjectId() } );
assert.soon( function() { return as.find().count() == 1; } );
assert.eq( 0, s.getDB( "admin" ).runCommand( { "resync" : 1 } ).ok );
stopMongod( 27019 );

big = new Array( 2000 ).toString();
for( i = 0; i < 1000; ++i )
    am.save( { _id: new ObjectId(), i: i, b: big } );

s = startMongodNoReset( "--port", "27019", "--dbpath", "/data/db/" + baseName + "-slave", "--slave", "--source", "127.0.0.1:27018" );
assert.soon( function() { return 1 == s.getDB( "admin" ).runCommand( { "resync" : 1 } ).ok; } );
print( "did resync" );

assert.soon( function() { return s.getDBNames().indexOf( baseName ) != -1; } );
as = s.getDB( baseName ).a
assert.soon( function() { print( as.find().count() ); return 1001 == as.find().count(); } );
assert.eq( 1, as.find( { i: 0 } ).count() );
assert.eq( 1, as.find( { i: 999 } ).count() );

assert.eq( 0, s.getDB( "admin" ).runCommand( { "resync" : 1 } ).ok );