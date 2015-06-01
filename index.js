"use strict";
var os = require('os'),
	fs = require('fs');

var arch = os.platform() + '-' + os.arch();
var v8 = 'v8-' + /[0-9]+\.[0-9]+/.exec(process.versions.v8)[0];
var filename = __dirname + '/bin/' + arch + '-' + v8 + '/galaxy_stack.node';

if (fs.existsSync(filename)) module.exports = require(filename);
else throw new Error("crypto add-on missing for " + arch + '-' + v8);