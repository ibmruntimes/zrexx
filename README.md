# zrexx
Call ZOS REXX scripts reside in PDS from node-js with IRXEXEC

## Installation

<!--
This is a [Node.js](https://nodejs.org/en/) module available through the
[npm registry](https://www.npmjs.com/).
-->

Before installing, [download and install Node.js](https://developer.ibm.com/node/sdk/ztp/).
Node.js 8.16 for z/OS or higher is required.

## Simple to use

### Install

```bash
npm install zrexx
```

### Use

There are two versions of rexx calls

#### zrexx.execute is the Synchronous version of the call
##### Arguments:
1. file descriptor to write REXX program output to (e.g. 2 to stderr)
2. fully qualified PDS name
3. member name of PDS that has the REXX script
4. first argument to the rexx program
5. second argument to the rexx program etc.  (note that to this addon does not create the __argv. stem and all the arguments are space separated, thus arguments with embedded space are not distinguishable with arguments separated by spaces.
6. third etc
7. ...
#### zrexx.execute_async is Asynchronous
##### Arguments:
1. file descriptor to write REXX program output to (e.g. 2 to stderr)
2. fully qualified PDS name
3. member name of PDS that has the REXX script
4. first argument to the rexx program
5. second argument to the rexx program etc. (note that to this addon does not create the __argv. stem and all the arguments are space separated, thus arguments with embedded space are not distinguishable with arguments separated by spaces.
6. third etc.
7. ...
##### [last argument] is the call-back function for asynchronous call.

### Example:
```js
const zrexx = require('../');

id=require("os").userInfo().username
dsn=id+".REXX.TESTPDS"


console.log(zrexx.execute(2,dsn,"HELLO","3","second","third"));

zrexx.execute_async(2,dsn,"HELLO","12","second","third","fourth","fifth",function(err,rc) {
    console.log("async call back");
    console.log("error",err);
    console.log("Rexx call RC",rc);
    } );

```

### Test

```bash
bash examples/setup_sample.sh to create a sample REXX script in a PDS

node examples/sample.js
```

