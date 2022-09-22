/*
 * Licensed Materials - Property of IBM
 * (C) Copyright IBM Corp. 2022. All Rights Reserved.
 * US Government Users Restricted Rights - Use, duplication or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */
#include "zrexx.h"

// (1) the Async Error class
class ErrorAsyncWorker : public Napi::AsyncWorker {
public:
  ErrorAsyncWorker(const Napi::Function &callback, Napi::Error error)
      : Napi::AsyncWorker(callback), error(error) {}

protected:
  void Execute() override {}
  void OnOK() override {
    Napi::Env env = Env();

    Callback().MakeCallback(Receiver().Value(),
                            {error.Value(), env.Undefined()});
  }

  void OnError(const Napi::Error &e) override {
    Napi::Env env = Env();

    Callback().MakeCallback(Receiver().Value(), {e.Value(), env.Undefined()});
  }

private:
  Napi::Error error;
};

// (2) the actual ZrexxAsyncWork with the base Napi::AsyncWorker
class ZrexxAsyncWorker : public Napi::AsyncWorker {

public:
  // (3) the actual execution is on a different thread, so all the arguments
  // must be copied to persistent storage, they cannot be on the caller's
  // stackframe

  ZrexxAsyncWorker(const Napi::Function &callback, int _fd, const char *_dsname,
                   const char *_member, int _argc, char **_argv)
      : Napi::AsyncWorker(callback), fd(_fd), argc(_argc), argv(_argv) {
    strncpy(dsname, _dsname, 55);
    strncpy(member, _member, 9);
  }
  // (4) free storage malloc'd
  ~ZrexxAsyncWorker() {
    for (int i = 0; i < argc; ++i) {
      free(argv[i]);
    }
    free(argv);
  }

protected:
  // (5) must impliment methods, do the work
  void Execute() override {
    rc = execute(fd, dsname, member, argc, argv);
    if (-1 == rc) {
      SetError("REXX exit -1");
    }
  }

  // (6) must implement method
  void OnOK() override {
    Napi::Env env = Env();
    Callback().MakeCallback(Receiver().Value(),
                            {env.Null(), Napi::Number::New(env, rc)});
  }

  // (7) must implement method
  void OnError(const Napi::Error &e) override {
    Napi::Env env = Env();
    Callback().MakeCallback(Receiver().Value(), {e.Value(), env.Undefined()});
  }

private:
  // (8) instance data
  int rc;
  int fd;
  char dsname[55];
  char member[9];
  int argc;
  char **argv;
};

// (8) the async interface entry
void ZrexxAsyncCallback(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 4) {
    Napi::Error::New(env, "rexx interface needs [output_file descriptor] "
                          "[dataset name] [member name] arguments to rexx "
                          "program [call back function]")
        .ThrowAsJavaScriptException();
    return;
  }

  int argc = info.Length() - 3 - 1;
  char **argv = 0;
  int last_arg = argc + 3;

  if (!info[last_arg].IsFunction()) {
    Napi::TypeError::New(env, "Last argument is not a call back function")
        .ThrowAsJavaScriptException();
    return;
  }
  Napi::Function cb = info[last_arg].As<Napi::Function>();

  if (!info[0].IsNumber()) {
    (new ErrorAsyncWorker(
         cb, Napi::TypeError::New(env, "File descriptor should be a number")))
        ->Queue();
    return;
  }
  int fd = info[0].As<Napi::Number>();

  char dsname[55];
  char member[9];
  strncpy(dsname, static_cast<std::string>(info[1].As<Napi::String>()).c_str(), sizeof(dsname));
  strncpy(member, static_cast<std::string>(info[2].As<Napi::String>()).c_str(), sizeof(member));

  if (argc > 0) {
    int i;
    argv = (char **)malloc(8 * (argc + 1));
    for (i = 0; i < argc; ++i) {
      argv[i] = strdup(
          static_cast<std::string>(info[3 + i].As<Napi::String>()).c_str());
    }
    argv[i] = 0;
  }

  (new ZrexxAsyncWorker(cb, fd, dsname, member, argc, argv))->Queue();
  return;
}

// (9) the synchronous interface entry
Napi::Number ZrexxWrapped(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 3) {
    Napi::Error::New(env, "rexx interface needs [output_file descriptor] "
                          "[dataset name] [member name] arguments to rexx "
                          "program")
        .ThrowAsJavaScriptException();
    return Napi::Number::New(env, -1);
  }
  if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "file descriptor should be a number")
        .ThrowAsJavaScriptException();
    return Napi::Number::New(env, -1);
  }
  int fd = info[0].As<Napi::Number>();

  char dsname[55];
  char member[9];
  strncpy(dsname, static_cast<std::string>(info[1].As<Napi::String>()).c_str(), sizeof(dsname));
  strncpy(member, static_cast<std::string>(info[2].As<Napi::String>()).c_str(), sizeof(member));

  int argc = info.Length() - 3;
  char **argv = 0;

  if (argc > 0) {
    int i;
    argv = (char **)alloca(8 * (argc + 1));
    for (i = 0; i < argc; ++i) {
      std::string str = static_cast<std::string>(info[3 + i].As<Napi::String>());
      const char *t = str.c_str();
      // this pointer value does not change, we have to allocate storage for it.
      int len = strlen(t);
      argv[i] = (char *)alloca(len + 1);
      memcpy(argv[i], t, len + 1);
    }
    argv[i] = 0;
  }

  return Napi::Number::New(env, execute(fd, dsname, member, argc, argv));
}

// (10) maps the function name for the Javascript interface to the C++ ones
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("execute", Napi::Function::New(env, ZrexxWrapped));
  exports.Set("execute_async", Napi::Function::New(env, ZrexxAsyncCallback));
  return exports;
}

// (11) Initialization and register to node
NODE_API_MODULE(zrexx, Init)
