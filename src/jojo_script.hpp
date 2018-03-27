//
// Created by benja on 3/16/2018.
//

#pragma once


#include <memory>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <libplatform/libplatform.h>
#include <v8/v8.h>

#include "debug_trap.h"

// Cause a debug trap if we try to unwrap a maybe val
#define V8_ASSERT_LOCAL(maybe)   \
    if (maybe.IsEmpty()) { \
        psnip_trap();      \
    }
#define V8_ASSERT(maybe)   \
    if (maybe.IsNothing()) { \
        psnip_trap();      \
    }

namespace Scripting {

using namespace v8;

// Read a file into a null-terminated vector of bytes
std::vector<char> readFileNullTerm(const char *moduleName) {
    std::vector<char> contents;
    std::ifstream f(moduleName, std::ios::ate | std::ios::binary);

    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    contents.resize(static_cast<size_t>(size) + 1, 0);
    f.read(contents.data(), size);
    return contents;
}

// Callback necessary for module instantiation
MaybeLocal<Module> resolve(Local<Context> context,
    Local<String> specifier,
    Local<Module> referrer) {

    // TODO: Implement this function for resolving modules
    std::cout << "Resolving: " << *specifier;
    return MaybeLocal<Module>();
}

class Engine {
public:
    Engine(const char *data_dir) {
        // V8 Initialization code
        V8::InitializeICUDefaultLocation(data_dir);
        V8::InitializeExternalStartupData(data_dir);
        m_platform = platform::NewDefaultPlatform();
        V8::InitializePlatform(m_platform.get());
        V8::Initialize();

        // Create a new isolate and only use this one
        Isolate::CreateParams create_params;
        m_arrayBufferAllocator = ArrayBuffer::Allocator::NewDefaultAllocator();
        create_params.array_buffer_allocator = m_arrayBufferAllocator;
        m_isolate = Isolate::New(create_params);
    }

    ~Engine() {
        m_isolate->Dispose();
        delete m_arrayBufferAllocator;
        V8::Dispose();
        V8::ShutdownPlatform();
    }

    // TODO: Load/instantiate module once and evaluate later on
    void runModule(const char *moduleName) {
        const auto &srcData = readFileNullTerm(moduleName);
        auto isolate = m_isolate;

        Isolate::Scope isolateScope(isolate);
        HandleScope handleScope(isolate);
        auto context = Context::New(isolate);
        Context::Scope context_scope(context);

        auto srcStringM = String::NewFromUtf8(isolate, srcData.data(), NewStringType::kNormal);
        V8_ASSERT_LOCAL(srcStringM);
        auto srcString = srcStringM.ToLocalChecked();

        auto srcNameM = String::NewFromUtf8(isolate, moduleName, NewStringType::kNormal);
        V8_ASSERT_LOCAL(srcNameM);
        auto srcName = srcNameM.ToLocalChecked();
        ScriptOrigin srcOrigin(srcName,
            Local<Integer>(),
            Local<Integer>(),
            Local<Boolean>(),
            Local<Integer>(),
            Local<Value>(),
            Local<Boolean>(),
            Local<Boolean>(),
            Boolean::New(isolate, true));

        ScriptCompiler::Source src(srcString, srcOrigin);
        auto moduleM = ScriptCompiler::CompileModule(isolate, &src);
        V8_ASSERT_LOCAL(moduleM);
        auto module = moduleM.ToLocalChecked();

        auto resultM = module->InstantiateModule(context, &resolve);
        V8_ASSERT(resultM);
        auto retvalM = module->Evaluate(context);
        V8_ASSERT_LOCAL(retvalM);
        auto retval = retvalM.ToLocalChecked();

        // TODO: Error handling?
        // auto proto_name = String::NewFromUtf8(isolate, "xyz", NewStringType::kNormal).ToLocalChecked();
        // Local<Value> proto_val;
        // std::cout << context->Global()->Get(context, proto_name).ToLocal(&proto_val);
        // std::cout << proto_val->IsUndefined();
        auto proto = Local<Function>::Cast(retval);

        // Create object
        auto obj = proto->NewInstance(context).ToLocalChecked();

        // Get function
        auto fn_name = String::NewFromUtf8(isolate, "updateLogic", NewStringType::kNormal).ToLocalChecked();
        auto fn = Local<Function>::Cast(obj->Get(context, fn_name).ToLocalChecked());
        auto fnRet = fn->Call(context, obj, 0, nullptr).ToLocalChecked();

        String::Utf8Value fnRetStr(isolate, fnRet);
        std::cout << *fnRetStr;
    }

private:
    std::unique_ptr<Platform>  m_platform;
    ArrayBuffer::Allocator    *m_arrayBufferAllocator;
    Isolate                   *m_isolate;
};

}
