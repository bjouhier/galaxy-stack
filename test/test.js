"use strict";
var gs = require('galaxy-stack');

function* testGenerator() {
	yield 1;
	yield 2;
}

var passed = 0;

function equal(x, y, message) {
	if (x !== y) throw new Error(message + " test FAILED: expected " + y + ", got " + x);
	console.log("ok: " + message + ': ' + y);
	passed++;
}

var gen = testGenerator();
var val = gen.next().value;
equal(val, 1, 'next val');

var frame = gs.getStackFrame(gen);
equal(typeof frame, 'object', 'typeof frame');
equal(frame.scriptName, __filename, 'scriptName');
equal(frame.functionName, 'testGenerator', 'functionName');
equal(frame.lineNumber, 5, 'lineNumber');
equal(frame.column, 2, 'column');
var savedContinuation = gs.getContinuation(gen);

val = gen.next().value;
frame = gs.getStackFrame(gen);
equal(val, 2, 'next val');
equal(frame.lineNumber, 6, 'lineNumber');

frame = gs.getStackFrame(gen, savedContinuation);
equal(frame.lineNumber, 5, 'lineNumber');
equal(passed, 9, 'test count');

console.log(passed + " TESTS PASSED!")