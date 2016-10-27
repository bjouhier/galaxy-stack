# Stack frame API for harmony generators in node.js

__Warning: Project has been abandoned__

Too difficult to maintain as it needs access to V8's internals. I managed to make it work with node 0.1x and io.js 2.x but did not maintain it any further.

## Intro

This package provides an API to obtain stack frame information from generators.

It is an optional companion package to the [galaxy](https://github.com/bjouhier/galaxy) package.

## API

`var helper = require('galaxy-stack')`

* `cont = helper.getContinuation(gen)`  
  returns the continuation offset where execution will be resumed.  
  `gen` is a suspended generator object. 
* `frame = helper.getStackFrame(gen [, cont])`  
  returns stack frame information for a suspended generator object.  
  `gen` is the suspended generator object.  
  `cont` is an optional continuation offset which was obtained by a prior call to `getContinuation`.
