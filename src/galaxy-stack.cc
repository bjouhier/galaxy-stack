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



// BEGIN CODE COPIED FROM api.cc
#define ENTER_V8(isolate)                                          \
  ASSERT((isolate)->IsInitialized());                              \
  i::VMState<i::OTHER> __state__((isolate))

#define ON_BAILOUT(isolate, location, code)                        \
  if (IsDeadCheck(isolate, location) ||                            \
      IsExecutionTerminatingCheck(isolate)) {                      \
    code;                                                          \
    UNREACHABLE();                                                 \
  }

static void DefaultFatalErrorHandler(const char* location,
                                     const char* message) {
  i::Isolate* isolate = i::Isolate::Current();
  if (isolate->IsInitialized()) {
    i::VMState<i::OTHER> state(isolate);
    API_Fatal(location, message);
  } else {
    API_Fatal(location, message);
  }
}

static v8::FatalErrorCallback GetFatalErrorHandler() {
  i::Isolate* isolate = i::Isolate::Current();
  if (isolate->exception_behavior() == NULL) {
    isolate->set_exception_behavior(DefaultFatalErrorHandler);
  }
  return isolate->exception_behavior();
}

static bool ReportV8Dead(const char* location) {
  v8::FatalErrorCallback callback = GetFatalErrorHandler();
  callback(location, "V8 is no longer usable");
  return true;
}

static inline bool IsDeadCheck(i::Isolate* isolate, const char* location) {
  return !isolate->IsInitialized()
      && i::V8::IsDead() ? ReportV8Dead(location) : false;
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
v8::Local<v8::Value> internalGetStackFrame(v8::Handle<v8::Value> handle, int continuation) {
  i::Isolate* isolate = i::Isolate::Current();
  ON_BAILOUT(isolate, "Galaxy_stack::GetStackFrame()", return v8::Local<v8::Value>());
  ENTER_V8(isolate);
  i::Handle<i::JSGeneratorObject> gen = v8::Utils::OpenHandle(*handle);
  i::Handle<i::JSFunction> fun(gen->function(), isolate);
  i::Handle<i::Script> script(i::Script::cast(fun->shared()->script()));
  i::Address pc = gen->function()->code()->instruction_start();
  int script_line_offset = script->line_offset()->value();
  int position = fun->code()->SourcePosition(pc + (continuation >= 0 ? continuation : gen->continuation()));
  int line_number = GetScriptLineNumber(script, position);
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
  return v8::Utils::ToLocal(stack_frame); 
}

v8::Local<v8::Value> internalGetContinuation(v8::Handle<v8::Value> handle) {
  i::Isolate* isolate = i::Isolate::Current();
  ON_BAILOUT(isolate, "Galaxy_stack::GetContinuation()", return v8::Local<v8::Value>());
  ENTER_V8(isolate);
  i::Handle<i::JSGeneratorObject> gen = v8::Utils::OpenHandle(*handle);
  return v8::Number::New(gen->continuation()); 
}

void GetStackFrame(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::HandleScope scope;

  int len = args.Length();
  if (!(len == 1 || len == 2)) {
    v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong number of arguments")));
    args.GetReturnValue().Set(scope.Close(v8::Undefined()));
    return;
  }

  if (!args[0]->IsObject()) {
    v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong argument type")));
    args.GetReturnValue().Set(scope.Close(v8::Undefined()));
    return;
  }
  int continuation = -1;
  if (len == 2) {
    if (!args[1]->IsNumber()) {
      v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong argument type")));
      args.GetReturnValue().Set(scope.Close(v8::Undefined()));
      return; 
    }
    continuation = args[1]->NumberValue();
  }
  v8::Local<v8::Value> result = internalGetStackFrame(args[0], continuation);
  args.GetReturnValue().Set(scope.Close(result));
}

void GetContinuation(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::HandleScope scope;

  if (args.Length() != 1) {
    v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong number of arguments")));
    args.GetReturnValue().Set(scope.Close(v8::Undefined()));
    return;
  }

  if (!args[0]->IsObject()) {
    v8::ThrowException(v8::Exception::TypeError(v8::String::New("Wrong argument type")));
    args.GetReturnValue().Set(scope.Close(v8::Undefined()));
    return; 
  }
  v8::Local<v8::Value> result = internalGetContinuation(args[0]);
  args.GetReturnValue().Set(scope.Close(result));

}


void init(v8::Handle<v8::Object> exports) {
  NODE_SET_METHOD(exports, "getStackFrame", GetStackFrame);
  NODE_SET_METHOD(exports, "getContinuation", GetContinuation);
}

NODE_MODULE(galaxy_stack, init)
