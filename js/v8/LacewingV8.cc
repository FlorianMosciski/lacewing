
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "LacewingV8.h"
using namespace v8;
    
#define BeginExportGlobal() int _arg_index = 0;

#define Get_Argument() args[_arg_index ++]
#define Get_Pointer() External::Unwrap(Get_Argument()->ToObject()->GetInternalField(0))

#define Read_Reference(T, N) T &N = *(T *) Get_Pointer();
#define Read_Int(N) int N = Get_Argument()->ToInt32()->Int32Value();
#define Read_Int64(N) lw_i64 N = Get_Argument()->ToInteger()->IntegerValue();
#define Read_Bool(N) bool N = Get_Argument()->ToInt32()->BooleanValue();
#define Read_String(N) String::AsciiValue _##N(Get_Argument()->ToString()); \
                        const char * N = *_##N;
#define Read_Function(N) Handle<Function> N = Handle<Function> (Function::Cast(*Get_Argument()));

#define Return() return Handle<Value>();
#define Return_Bool(x) return Boolean::New(x);
#define Return_String(x) return String::New(x);
#define Return_Int(x) return Int32::New(x);
#define Return_Int64(x) return Integer::New(x);
#define Return_Ref(x) return MakeRef (&x);
#define Return_New(x, c) return MakeRef (x, c);

#define CALLBACK_INIT()
#define CALLBACK_TYPE Persistent <Function>
#define CALLBACK_ARG_TYPE Local <Value>
#define CALLBACK_ARG_STRING(x) String::New(x)
#define CALLBACK_ARG_REF(x) MakeRefLocal (&x)
#define CALLBACK_DO(callback, argc, argv) callback->Call(callback, argc, argv);
#define CALLBACK_INFO(c) new CallbackInfo (Persistent <Function>::New(c))

struct CallbackInfo
{
	CallbackInfo (CALLBACK_TYPE _Callback) : Callback(_Callback)
	{
	}
	
	CALLBACK_TYPE Callback;
};

Persistent <FunctionTemplate> RefTemplate;

Handle <Value> RefConstructor(const Arguments &)
{
    return Handle <Value> ();
}

Handle <Object> MakeRef (void * ptr, WeakReferenceCallback delete_callback)
{
    HandleScope Scope;
    Persistent <Object> Ref = Persistent <Object>::New (RefTemplate->GetFunction()->NewInstance());
   
    Ref->SetInternalField(0, External::New(ptr));
    Ref.MakeWeak(0, delete_callback);
    
    return Ref;
}

Handle <Object> MakeRef (void * ptr)
{
    HandleScope Scope;

    Handle <Object> Ref = RefTemplate->GetFunction()->NewInstance();
    Ref->SetInternalField(0, External::New(ptr));
   
    return Scope.Close(Ref);
}

Local <Value> MakeRefLocal (void * ptr)
{
    return Local<Value>::New(MakeRef(ptr));
}

#define ExportBodies
#define Export(x) Handle<Value> x (const Arguments &args)
#define Deleter(x) void x##Deleter (Persistent<Value> obj, void * ptr) { \
                        delete ((Lacewing::x *) ptr); }

    #include "../exports/eventpump.inc"
    #include "../exports/global.inc"
    #include "../exports/webserver.inc"
    #include "../exports/address.inc"
    #include "../exports/error.inc"
    #include "../exports/filter.inc"

#undef Deleter
#undef ExportBodies
#undef Export

#include "../js/liblacewing.js.inc"

void Lacewing::V8::Export(Handle<Object> Target, Lacewing::Pump * Pump)
{
    HandleScope Scope;

    RefTemplate = Persistent <FunctionTemplate>::New (FunctionTemplate::New(RefConstructor));
    Handle <ObjectTemplate> RefInstTemplate = RefTemplate->InstanceTemplate();
    RefInstTemplate->SetInternalFieldCount(1);
    
    Handle <Object> Exports = Object::New();
    
    #define Export(x) Handle <FunctionTemplate> x##_template = FunctionTemplate::New(x); \
                Exports->Set(String::New(#x), x##_template->GetFunction());
    #define Deleter(x)

        if(!Pump)
        {
            #include "../exports/eventpump.inc"
        }
        else
        {   Exports->Set(String::New("lwjs_global_eventpump"), MakeRef(Pump));
        }

        #include "../exports/global.inc"
        #include "../exports/webserver.inc"
        #include "../exports/address.inc"
        #include "../exports/error.inc"
        #include "../exports/filter.inc"
    
    #undef Deleter
    #undef Export
        
    Local<Function> function = Function::Cast(*Script::Compile(
            String::New((const char *) LacewingJS, (int) sizeof(LacewingJS)),
            String::New("liblacewing.js"))->Run());

    Handle <Value> Params [] = { Exports, Target };
    function->Call(function, 2, Params);
}

void Lacewing::V8::Export(Handle<Context> Context, Lacewing::Pump * Pump)
{
    HandleScope Scope;
    
    Handle <Object> Target = Object::New();
    Export (Target, Pump);
    
    Context->Global()->Set(String::New("Lacewing"), Scope.Close(Target));
}

