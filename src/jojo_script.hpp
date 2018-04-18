//
// Created by benja on 3/16/2018.
//

#pragma once

#include <cstdlib>
#include <memory>
#include <fstream>
#include <iostream>
#include <string>
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
std::vector<char> readScript(const char *moduleName) {
    std::vector<char> contents;
    std::ifstream f(moduleName, std::ios::ate | std::ios::binary);

    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    contents.resize(static_cast<size_t>(size) + 1, 0);
    f.read(contents.data(), size);
    return contents;
}

// Callback necessary for module instantiation
MaybeLocal<Module> resolve(Local<Context>, Local<String>, Local<Module>) {
    std::cerr << "Module resolving not implemented" << std::endl;
    std::exit(1);
    return MaybeLocal<Module>();
}

class GameObject {
public:
    ~GameObject() {
        m_updateLogic.Reset();
        m_obj.Reset();
        m_context.Reset();
    }

    void updateLogic() {
        auto isolate = m_isolate;
        HandleScope handleScope(isolate);
        auto context = v8::Local<v8::Context>::New(isolate, m_context);
        Context::Scope context_scope(context);
        auto obj = v8::Local<v8::Object>::New(isolate, m_obj);
        auto fn = v8::Local<v8::Function>::New(isolate, m_updateLogic);

        auto fnRet = fn->Call(context, obj, 0, nullptr).ToLocalChecked();
        String::Utf8Value fnRetStr(isolate, fnRet);
        std::cout << *fnRetStr;
    }

    void hook(Isolate *isolate, const Local<Context> &context, const Local<Function> &constructor) {
        m_isolate = isolate;
        m_context.Reset(isolate, context);
        auto obj = constructor->NewInstance(context).ToLocalChecked();
        m_obj.Reset(isolate, obj);

        // Get function
        auto fn_name = String::NewFromUtf8(isolate, "updateLogic", NewStringType::kNormal).ToLocalChecked();
        auto updateLogic = Local<Function>::Cast(obj->Get(context, fn_name).ToLocalChecked());
        m_updateLogic.Reset(isolate, updateLogic);
    }

private:
    Isolate         *m_isolate;
    Global<Context>  m_context;
    Global<Object>   m_obj;
    Global<Function> m_updateLogic;
};

class Engine {
public:
    Engine() {
        // V8 Initialization code
        V8::InitializeICUDefaultLocation("");
        V8::InitializeExternalStartupData("");
        m_platform = platform::NewDefaultPlatform();
        V8::InitializePlatform(m_platform.get());
        V8::Initialize();

        // Create a new isolate and only use this one
        Isolate::CreateParams create_params;
        m_arrayBufferAllocator = ArrayBuffer::Allocator::NewDefaultAllocator();
        create_params.array_buffer_allocator = m_arrayBufferAllocator;
        m_isolate = Isolate::New(create_params);
        m_isolateScope = new Isolate::Scope(m_isolate);
    }

    ~Engine() {
        delete m_isolateScope;
        m_isolate->Dispose();
        delete m_arrayBufferAllocator;
        V8::Dispose();
        V8::ShutdownPlatform();
    }

    void hookScript(GameObject &obj, const char *scriptName) {
        auto isolate = m_isolate;
        HandleScope handleScope(isolate);
        auto context = Context::New(isolate);
        Context::Scope context_scope(context);

        Local<String> srcString;
        {
            auto srcData = readScript(scriptName);
            srcString = String::NewFromUtf8(isolate, srcData.data(), NewStringType::kNormal)
                .ToLocalChecked();
        }

        auto srcName = String::NewFromUtf8(isolate, scriptName, NewStringType::kNormal)
            .ToLocalChecked();
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

        auto constructor = Local<Function>::Cast(retval);
        obj.hook(isolate, context, constructor);
    }

private:
    std::unique_ptr<Platform>  m_platform;
    ArrayBuffer::Allocator    *m_arrayBufferAllocator;
    Isolate                   *m_isolate;
    Isolate::Scope            *m_isolateScope;
};

}
