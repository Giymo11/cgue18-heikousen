//
// Created by benja on 3/16/2018.
//

#pragma once


#include <memory>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <v8/libplatform/libplatform.h>
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
v8::MaybeLocal<v8::Module> resolve(v8::Local<v8::Context> context,
    v8::Local<v8::String> specifier,
    v8::Local<v8::Module> referrer) {

    // TODO: what to do here?
    std::cout << "Resolving: " << *specifier;
    return v8::MaybeLocal<v8::Module>();
}

class Engine {
public:
    Engine(const char *data_dir) {
        // V8 Initialization code
        v8::V8::InitializeICUDefaultLocation(data_dir);
        v8::V8::InitializeExternalStartupData(data_dir);
        m_platform = v8::platform::NewDefaultPlatform();
        v8::V8::InitializePlatform(m_platform.get());
        v8::V8::Initialize();

        // Create a new isolate and only use this one
        v8::Isolate::CreateParams create_params;
        m_arrayBufferAllocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        create_params.array_buffer_allocator = m_arrayBufferAllocator;
        m_isolate = v8::Isolate::New(create_params);
    }

    ~Engine() {
        m_isolate->Dispose();
        delete m_arrayBufferAllocator;
        v8::V8::Dispose();
        v8::V8::ShutdownPlatform();
    }

    void runModule(const char *moduleName) {
        const auto &srcData = readFileNullTerm(moduleName);
        auto isolate = m_isolate;

        v8::Isolate::Scope isolateScope(isolate);
        v8::HandleScope handleScope(isolate);
        auto context = v8::Context::New(isolate);
        v8::Context::Scope context_scope(context);

        auto srcStringM = v8::String::NewFromUtf8(isolate, srcData.data(), v8::NewStringType::kNormal);
        V8_ASSERT_LOCAL(srcStringM);
        auto srcString = srcStringM.ToLocalChecked();

        auto srcNameM = v8::String::NewFromUtf8(isolate, moduleName, v8::NewStringType::kNormal);
        V8_ASSERT_LOCAL(srcNameM);
        auto srcName = srcNameM.ToLocalChecked();
        v8::ScriptOrigin srcOrigin(srcName,
            v8::Local<v8::Integer>(),
            v8::Local<v8::Integer>(),
            v8::Local<v8::Boolean>(),
            v8::Local<v8::Integer>(),
            v8::Local<v8::Value>(),
            v8::Local<v8::Boolean>(),
            v8::Local<v8::Boolean>(),
            v8::Boolean::New(isolate, true));

        v8::ScriptCompiler::Source src(srcString, srcOrigin);
        auto moduleM = v8::ScriptCompiler::CompileModule(isolate, &src);
        V8_ASSERT_LOCAL(moduleM);
        auto module = moduleM.ToLocalChecked();

        auto resultM = module->InstantiateModule(context, &resolve);
        V8_ASSERT(resultM);
        auto retvalM = module->Evaluate(context);
        V8_ASSERT_LOCAL(retvalM);
        auto retval = retvalM.ToLocalChecked();

        v8::String::Utf8Value utf8(isolate, retval);
        std::cout << *utf8;
    }

private:
    std::unique_ptr<v8::Platform>  m_platform;
    v8::ArrayBuffer::Allocator    *m_arrayBufferAllocator;
    v8::Isolate                   *m_isolate;
};

}


void hello_v8(const char *data_dir) {
    // Initialize V8.
    v8::V8::InitializeICUDefaultLocation(data_dir);
    v8::V8::InitializeExternalStartupData(data_dir);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    // Create a new Isolate and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate *isolate = v8::Isolate::New(create_params);
    {
        v8::Isolate::Scope isolate_scope(isolate);

        // Create a stack-allocated handle scope.
        v8::HandleScope handle_scope(isolate);

        // Create a new context.
        v8::Local<v8::Context> context = v8::Context::New(isolate);

        // Enter the context for compiling and running the hello world script.
        v8::Context::Scope context_scope(context);

        // Create a string containing the JavaScript source code.
        v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, "3 + 2 + 1 + ' ' + 'Hello' + ', World!'",
                                                               v8::NewStringType::kNormal)
                .ToLocalChecked();

        // Compile the source code.
        v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();

        // Run the script to get the result.
        v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

        // Convert the result to an UTF8 string and print it.
        v8::String::Utf8Value utf8(isolate, result);
        std::cout << *utf8;
    }

    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
}
