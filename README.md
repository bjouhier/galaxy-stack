Stack frame API for harmony generators in node.js

## Intro

This package provides an API to obtain stack frame information from generators.

It is an optional companion package to the [galaxy](../galaxy) package.

## API

`var helper = require('galaxy-stack')`

* `cont = helper.getContinuation(gen)`  
  returns the continuation offset where execution will be resumed.  
  `gen` is a suspended generator object. 
* `frame = helper.getStackFrame(gen [, cont])`  
  returns stack frame information for a suspended generator object.  
  `gen` is the suspended generator object.  
  `cont` is an optional continuation offset which was obtained by a prior call to `getContinuation`.
