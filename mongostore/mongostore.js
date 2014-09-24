if (process.argv.length < 4) {
    console.log('usage: node jstore.js db collection');
    process.exit(1);
}

var mongo_client = require('mongodb').MongoClient;
var uri = 'mongodb://127.0.0.1:27017/' + process.argv[2];

console.log(uri);

mongo_client.connect(uri, function(err, db) {
    if (err)
        throw err;

    var collection = db.collection(process.argv[3]);

    var reader = require('readline').createInterface({
        input: process.stdin,
        output: process.stdout
    });

    reader.setPrompt('');
    reader.prompt();

    reader.on('line', function (line) {
        var result;

        try {
            result = JSON.parse(line);
            result['date'] = new Date();

            collection.insert(result, function(err, result) {
                if (err) console.warn(err.message);
            });
        } catch (e) {
        }
    });

    process.stdin.on('end', function () {
        //do something
    });
});
