//
// Copyright (c) 2013 Bruno Jouhier <bruno.jouhier@sage.com>
// MIT License
//
// Most of the code borrowed from v8's src/isolate.cc, hence the following copyright notice
// Copyright 2012 the V8 project authors. All rights reserved.

#define ENABLE_DEBUGGER_SUPPORT
#include "api.h"
#include "objects.h"
#include "vm-state-inl.h"
#include <node.h>
#include <v8.h>

using namespace v8;

// BEGIN CODE COPIED FROM api.cc
#define ENTER_V8(isolate)                                          \
  ASSERT((isolate)->IsInitialized());                              \
  i::VMState<i::OTHER> __state__((isolate))

#define ON_BAILOUT(isolate, location, code)                        \
  if (IsExecutionTerminatingCheck(isolate)) {                      \
    code;                                                          \
    UNREACHABLE();                                                 \
  }

static inline bool IsExecutionTerminatingCheck(i::Isolate* isolate) {
  if (!isolate->IsInitialized()) return false;
  if (isolate->has_scheduled_exception()) {
    return isolate->scheduled_exception() ==
        isolate->heap()->termination_exception();
  }
  return false;
}

// END CODE COPIED FROM api.cc

// Got this the API boilerplate from v8::Object::GetPrototype() 
// and the details from Isolate::CaptureCurrentStackTrace
Local<Value> internalGetStackFrame(Handle<Value> handle, int continuation) {
  i::Isolate* isolate = i::Isolate::Current();
  ON_BAILOUT(isolate, "Galaxy_stack::GetStackFrame()", return Local<v8::Value>());
  ENTER_V8(isolate);
  i::Handle<i::JSGeneratorObject> gen = Utils::OpenHandle(*handle);
  i::Handle<i::JSFunction> fun(gen->function(), isolate);
  i::Handle<i::Script> script(i::Script::cast(fun->shared()->script()));
  i::Address pc = gen->function()->code()->instruction_start();
  int script_line_offset = script->line_offset()->value();
  int position = fun->code()->SourcePosition(pc + (continuation >= 0 ? continuation : gen->continuation()));
  int line_number = i::Script::GetLineNumber(script, position);
  // line_number is already shifted by the script_line_offset.
  int relative_line_number = line_number - script_line_offset;

  i::Handle<i::FixedArray> line_ends(i::FixedArray::cast(script->line_ends()));
  int start = (relative_line_number == 0) ? 0 : i::Smi::cast(line_ends->get(relative_line_number - 1))->value() + 1;
  int column_offset = position - start;
  if (relative_line_number == 0) {
    // For the case where the code is on the same line as the script
    // tag.
    column_offset += script->column_offset()->value();
  }
  i::Handle<i::Object> script_name(script->name(), isolate);
  i::Handle<i::Object> fun_name(fun->shared()->name(), isolate);
  if (!fun_name->BooleanValue()) {
    fun_name = i::Handle<i::Object>(fun->shared()->inferred_name(), isolate);
  }

  i::Handle<i::JSObject> stack_frame = isolate->factory()->NewJSObject(isolate->object_function());
  i::Handle<i::String> column_key = isolate->factory()->InternalizeOneByteString(STATIC_ASCII_VECTOR("column"));
  i::Handle<i::String> line_key = isolate->factory()->InternalizeOneByteString(STATIC_ASCII_VECTOR("lineNumber"));
  i::Handle<i::String> script_key = isolate->factory()->InternalizeOneByteString(STATIC_ASCII_VECTOR("scriptName"));  
  i::Handle<i::String> function_key = isolate->factory()->InternalizeOneByteString(STATIC_ASCII_VECTOR("functionName"));
  i::JSObject::SetLocalPropertyIgnoreAttributes(stack_frame, script_key, script_name, NONE);
  i::JSObject::SetLocalPropertyIgnoreAttributes(stack_frame, line_key, i::Handle<i::Smi>(i::Smi::FromInt(line_number + 1), isolate), NONE); 
  i::JSObject::SetLocalPropertyIgnoreAttributes(stack_frame, column_key, i::Handle<i::Smi>(i::Smi::FromInt(column_offset + 1), isolate), NONE);
  i::JSObject::SetLocalPropertyIgnoreAttributes(stack_frame, function_key, fun_name, NONE);
  return Utils::ToLocal(stack_frame); 
}

Local<Value> internalGetContinuation(Handle<Value> handle) {
  i::Isolate* isolate = i::Isolate::Current();
  ON_BAILOUT(isolate, "Galaxy_stack::GetContinuation()", return Local<v8::Value>());
  ENTER_V8(isolate);
  i::Handle<i::JSGeneratorObject> gen = Utils::OpenHandle(*handle);
  return Number::New(Isolate::GetCurrent(), gen->continuation()); 
}

void GetStackFrame(const v8::FunctionCallbackInfo<Value>& args) {
  HandleScope scope(v8::Isolate::GetCurrent());

  int len = args.Length();
  if (!(len == 1 || len == 2)) {
    Isolate::GetCurrent()->ThrowException(Exception::TypeError(String::NewFromUtf8(Isolate::GetCurrent(), "Wrong number of arguments")));
    return;
  }

  if (!args[0]->IsObject()) {
    Isolate::GetCurrent()->ThrowException(Exception::TypeError(String::NewFromUtf8(Isolate::GetCurrent(), "Wrong argument type")));
    return;
  }
  int continuation = -1;
  if (len == 2) {
    if (!args[1]->IsNumber()) {
      Isolate::GetCurrent()->ThrowException(Exception::TypeError(String::NewFromUtf8(Isolate::GetCurrent(), "Wrong argument type")));
      return;  
    }
    continuation = args[1]->NumberValue();
  }
  Local<Value> result = internalGetStackFrame(args[0], continuation);
  args.GetReturnValue().Set(result);
}

void GetContinuation(const v8::FunctionCallbackInfo<Value>& args) {
  HandleScope scope(v8::Isolate::GetCurrent());

  if (args.Length() != 1) {
    Isolate::GetCurrent()->ThrowException(Exception::TypeError(String::NewFromUtf8(Isolate::GetCurrent(), "Wrong number of arguments")));
    return;
  }

  if (!args[0]->IsObject()) {
    Isolate::GetCurrent()->ThrowException(Exception::TypeError(String::NewFromUtf8(Isolate::GetCurrent(), "Wrong argument type")));
    return;
  }
  Local<Value> result = internalGetContinuation(args[0]);
  args.GetReturnValue().Set(result);
}


void init(Handle<Object> exports) {
  // note: first arg was symbol before!
  exports->Set(String::NewFromUtf8(Isolate::GetCurrent(), "getStackFrame"), FunctionTemplate::New(Isolate::GetCurrent(), GetStackFrame)->GetFunction());
  exports->Set(String::NewFromUtf8(Isolate::GetCurrent(), "getContinuation"), FunctionTemplate::New(Isolate::GetCurrent(), GetContinuation)->GetFunction());
}

NODE_MODULE(galaxy_stack, init)