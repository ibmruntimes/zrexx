/*
 ** Licensed Materials - Property of IBM
 ** (C) Copyright IBM Corp. 2019. All Rights Reserved.
 ** US Government Users Restricted Rights - Use, duplication or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 **/

if (process.platform != "os390") {
  console.log("This is for zos only")
  process.exit(-1)
}
const {
  exec
} = require('child_process')
var dsn = require("os").userInfo().username + ".DELETEME.REXXTEST"

async function dataset() {
  return new Promise((resolve, reject) => {
    alloccmd = "tso alloc da\\(\\'" + dsn + "\\'\\) dsorg\\(po\\) block\\(100\\) space\\(2,2\\) lrecl\\(80\\) blksize\\(800\\) dir\\(1\\) recfm\\(f,b\\) mod "
    exec(alloccmd, (err, stdout, stderr) => {
      if (err) {
        console.log("FAILED to create", dsn)
        console.log(err)
        return reject("REJECT")
      }
      console.log(`stdout: ${stdout}`)
      console.log(`stderr: ${stderr}`)
      return resolve("RESOLVE")
    })
  })
}
async function rexxfile() {
  return new Promise((resolve, reject) => {
    var fs = require('fs');
    data = '/*rexx*/\n' +
      'Parse source src\n' +
      'Say "-----------------------------------------------"\n' +
      'say "Me:" src\n' +
      'parse arg n rest\n' +
      ' say "Arguments received:" n rest\n' +
      'If datatype(n,"W") <> 1 then n = 1\n' +
      'Do i=1 TO n\n' +
      '  Say "Hello World the" i". time ..."\n' +
      'End\n' +
      'return n*10;\n'
    fs.writeFile("deleteme.rexx", data, (err) => {
      if (err) {
        console.log(err);
        return reject("REJECT:", err)
      }
      console.log("Successfully Written to File.");
      return resolve("RESOLVE")
    });
  })
}
async function copymember() {
  return new Promise((resolve, reject) => {
    cpcmd = "/bin/cp -O u deleteme.rexx //\\'" + dsn + "\\(HELLO\\)\\'"
    console.log(cpcmd)
    exec(cpcmd, (err, stdout, stderr) => {
      if (err) {
        console.log("FAILED to create", dsn)
        console.log(err)
        return reject("REJECT")
      }
      console.log(`stdout: ${stdout}`)
      console.log(`stderr: ${stderr}`)
      return resolve("RESOLVE")
    })
  })
}


(async () => {
  try {
    const got_dataset = await dataset()
    const got_rexxfile = await rexxfile()
    const donecopy = await copymember()
  } catch (e) {
    console.log(e)
  }
})();

const zrexx = require("../build/Release/zrexx.node");
const expect = require('chai').expect;
const should = require('chai').should;
const assert = require('chai').assert;

describe("zrexx Validation", function() {
  it("check zrexx sync call ", function(done) {
    setTimeout(function() {
      let rc = zrexx.execute(2, dsn, "HELLO", "3", "Synchronous", "misc")
      expect(rc === 30).to.be.true
      done()
    }, 5000);
  });
  it("check zrexx async call ", function(done) {
    setTimeout(function() {
      zrexx.execute_async(2, dsn, "HELLO", "12", "Asynchronous", "misc", function(err, rc) {
        console.log("async call back");
        console.log("error", err);
        console.log("Rexx call RC", rc);
        expect(rc === 120).to.be.true
        done()
      });
    }, 7000);
  });
})
