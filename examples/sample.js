const zrexx = require('zrexx');

id=require("os").userInfo().username
dsn=id+".REXX.TESTPDS"


console.log("Start zrexx.execute");
console.log(zrexx.execute(2,dsn,"HELLO","3","Synchronous","misc"));
console.log("Return from zrexx.execute");

console.log("Start zrexx.execute_async");
zrexx.execute_async(2,dsn,"HELLO","12","Asynchronous","misc",function(err,rc) {
    console.log("async call back");
    console.log("error",err);
    console.log("Rexx call RC",rc);
    } );
console.log("Return from zrexx.execute_async");
